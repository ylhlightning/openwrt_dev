#!/bin/sh

local CURRMODEM

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"
local TIMEOUT

log() {
	logger -t "Reconnect MBIM" "$@"
}

handle_timeout(){
	local wget_pid="$1"
	local count=0
	ps | grep -v grep | grep $wget_pid
	res="$?"
	while [ "$res" = 0 -a $count -lt "$((TIMEOUT))" ]; do
		sleep 1
		count=$((count+1))
		ps | grep -v grep | grep $wget_pid
		res="$?"
	done

	if [ "$res" = 0 ]; then
		log "Killing process on timeout"
		kill "$wget_pid" 2> /dev/null
		ps | grep -v grep | grep $wget_pid
		res="$?"
		if [ "$res" = 0 ]; then
			log "Killing process on timeout"
			kill -9 $wget_pid 2> /dev/null	
		fi
	fi
}

CURRMODEM=$1
uci set modem.modem$CURRMODEM.connected=0
uci commit modem

WDMNX=$(uci get modem.modem$CURRMODEM.wdm)
WWANX=$(uci get modem.modem$CURRMODEM.wwan)

NAPN=$(uci get modem.modem$CURRMODEM.apn)
NUSER=$(uci get modem.modem$CURRMODEM.user)
NPASS=$(uci get modem.modem$CURRMODEM.pass)
NAUTH=$(uci get modem.modem$CURRMODEM.auth)
PINCODE=$(uci get modem.modem$CURRMODEM.pin)

killall -9 getsignal$CURRMODEM
rm -f $ROOTER_LINK/getsignal$CURRMODEM
ifdown wan$CURRMODEM
MAN=$(uci get modem.modem$CURRMODEM.manuf)
MOD=$(uci get modem.modem$CURRMODEM.model)
$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Reconnecting"

$ROOTER/log/logger "Retry to Connect Modem #$CURRMODEM ($MAN $MOD)"

#TIMEOUT=10
#$ROOTER/mbim/mbim_connect.lua stop wwan$WWANX cdc-wdm$WDMNX $CURRMODEM &
#handle_timeout "$!"

COUNTER=1
while [ $COUNTER -lt 6 ]; do
	TIMEOUT=70
	$ROOTER/mbim/connectmbim.sh cdc-wdm$WDMNX $CURRMODEM $NAUTH $NAPN $NUSER $NPASS $PINC & 

#	$ROOTER/mbim/mbim_connect.lua start wwan$WWANX cdc-wdm$WDMNX $CURRMODEM $NAUTH $NAPN $NUSER $NPASS $PINC & 
	handle_timeout "$!"
	if [ -f /tmp/mbimgood ]; then
		rm -f /tmp/mbimgood
		break
	fi
#	TIMEOUT=10
#	$ROOTER/mbim/mbim_connect.lua stop wwan$WWANX cdc-wdm$WDMNX $CURRMODEM &
#	handle_timeout "$!"
	let COUNTER=COUNTER+1 
done
if [ $COUNTER -lt 6 ]; then
	ifup wan$CURRMODEM
	source /tmp/mbimcustom$CURRMODEM
	source /tmp/mbimqos$CURRMODEM
	source /tmp/mbimmcc$CURRMODEM
	source /tmp/mbimsig$CURRMODEM
	source /tmp/mbimmode$CURRMODEM
	uci set modem.modem$CURRMODEM.custom=$CUSTOM
	uci set modem.modem$CURRMODEM.provider=$PROV
	uci set modem.modem$CURRMODEM.down=$DOWN" kbps Down | "
	uci set modem.modem$CURRMODEM.up=$UP" kbps Up"
	uci set modem.modem$CURRMODEM.mcc=$MCC
	uci set modem.modem$CURRMODEM.mnc=" "$MNC
	uci set modem.modem$CURRMODEM.sig=$CSQ
	uci set modem.modem$CURRMODEM.mode=$MODE
	uci set modem.modem$CURRMODEM.connected=1
	uci commit modem
	rm -f /tmp/mbimcustom$CURRMODEM
	rm -f /tmp/mbimqos$CURRMODEM
	rm -f /tmp/mbimmcc$CURRMODEM
	rm -f /tmp/mbimsig$CURRMODEM
	rm -f /tmp/mbimmode$CURRMODEM

	ln -s $ROOTER/mbim/mbimsignal.lua $ROOTER_LINK/getsignal$CURRMODEM

	$ROOTER/log/logger "Reconnected Modem #$CURRMODEM"
	$ROOTER_LINK/getsignal$CURRMODEM $CURRMODEM &
else
	$ROOTER/log/logger "Retry to Connect Failed for Modem #$CURRMODEM"
	log "Connection Failed for Modem $CURRMODEM"
	$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Connect"
fi


