#!/bin/sh
INCLUDE_ONLY=1

. ../netifd-proto.sh
. ./ppp.sh
init_proto "$@"

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

log() {
	logger -t "Create PPP Connection" "$@"
}

proto_3g_init_config() {
	no_device=1
	available=1
	ppp_generic_init_config
	proto_config_add_string "device"
	proto_config_add_string "apn"
	proto_config_add_string "service"
	proto_config_add_string "pincode"
	proto_config_add_string "dialnumber"
}

proto_3g_setup() {
	local interface="$1"
	local chat

	if [ ! -f /tmp/bootend.file ]; then
		return 0
	fi

	CURRMODEM=${interface:3}
	uci set modem.modem$CURRMODEM.connected=0
	uci commit modem
	killall -9 getsignal$CURRMODEM
	rm -f $ROOTER_LINK/getsignal$CURRMODEM

	json_get_var device device
	json_get_var apn apn
	json_get_var service service
	json_get_var pincode pincode

	[ -e "$device" ] || {
		proto_set_available "$interface" 0
		return 1
	}

	chat="/etc/chatscripts/3g.chat"
	if [ -n "$pincode" ]; then
		PINCODE="$pincode" gcom -d "$device" -s /etc/gcom/setpin.gcom || {
			proto_notify_error "$interface" PIN_FAILED
			proto_block_restart "$interface"
			return 1
		}
	fi

	export SETUSER=$(uci get modem.modeminfo$CURRMODEM.user)
	export SETPASS=$(uci get modem.modeminfo$CURRMODEM.pass)
	export SETAUTH=$(uci get modem.modeminfo$CURRMODEM.auth)
	OX=$(gcom -d $device -s $ROOTER/gcom/connect-ppp.gcom 2>/dev/null)
log "$OX"
	ERROR="ERRORs"
	if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
	then
		log "Connection Failed for Modem $CURRMODEM on Authorization"
		proto_set_available "$interface" 0
		return 1
	fi

	if [ -z "$dialnumber" ]; then
		dialnumber="*99***1#"
	fi

	connect="${apn:+USE_APN=$apn }DIALNUMBER=$dialnumber /usr/sbin/chat -t5 -v -E -f $chat"

	ppp_generic_setup "$interface" \
		noaccomp \
		nopcomp \
		novj \
		nobsdcomp \
		noauth \
		lock \
		crtscts \
		115200 "$device"

	sleep 20
	MAN=$(uci get modem.modem$CURRMODEM.manuf)
	MOD=$(uci get modem.modem$CURRMODEM.model)
	$ROOTER/log/logger "Modem #$CURRMODEM Connected ($MAN $MOD)"
	PROT=$(uci get modem.modem$CURRMODEM.proto)
	ln -s $ROOTER/signal/modemsignal.sh $ROOTER_LINK/getsignal$CURRMODEM
	ln -s $ROOTER/connect/reconnect-ppp.sh $ROOTER_LINK/reconnect$CURRMODEM
	$ROOTER/sms/check_sms.sh $CURRMODEM
	$ROOTER_LINK/getsignal$CURRMODEM $CURRMODEM $PROT &
	uci set modem.modem$CURRMODEM.connected=1
	uci set modem.modem$CURRMODEM.interface="3g-"$interface
	uci commit modem
	rm -f /tmp/usbwait
	return 0
}

proto_3g_teardown() {
	proto_kill_command "$interface"
}

add_protocol 3g
