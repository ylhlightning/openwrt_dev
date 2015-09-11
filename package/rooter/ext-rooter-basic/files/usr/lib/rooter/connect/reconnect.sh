#!/bin/sh

local CURRMODEM

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

log() {
	logger -t "Reconnect Modem" "$@"
}

CURRMODEM=$1
uci set modem.modem$CURRMODEM.connected=0
uci commit modem

WWANX=$(uci get modem.modem$CURRMODEM.wwan)
WDMNX=$(uci get modem.modem$CURRMODEM.wdm)

NAPN=$(uci get modem.modem$CURRMODEM.apn)
NUSER=$(uci get modem.modem$CURRMODEM.user)
NPASS=$(uci get modem.modem$CURRMODEM.passw)
NAUTH=$(uci get modem.modem$CURRMODEM.auth)
PINCODE=$(uci get modem.modem$CURRMODEM.pin)

killall -9 getsignal$CURRMODEM
rm -f $ROOTER_LINK/getsignal$CURRMODEM
ifdown wan$CURRMODEM
MAN=$(uci get modem.modem$CURRMODEM.manuf)
MOD=$(uci get modem.modem$CURRMODEM.model)
$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Reconnecting"
PROT=$(uci get modem.modem$CURRMODEM.proto)

PROT=$(uci get modem.modem$CURRMODEM.proto)
CPORT=$(uci get modem.modem$CURRMODEM.commport)
export SETAPN=$NAPN
export SETUSER=$NUSER
export SETPASS=$NPASS
export SETAUTH=$NAUTH
export PINCODE=$PINC

COUNTER=1
while [ $COUNTER -lt 6 ]; do
	OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/reset.gcom 2>/dev/null)
	ERROR="ERROR"
	if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
	then
		log "Retry Reset"
		sleep 3
  		let COUNTER=COUNTER+1
	else
		log "Modem Reset"
		sleep 3
		$ROOTER/common/lockchk.sh $CURRMODEM
		break
	fi
done
if [ $COUNTER -lt 6 ]; then
	COUNTER=1
	while [ $COUNTER -lt 6 ]; do
		case $PROT in
		"1" )
			if [ -n "$PINCODE" ]; then
				gcom -d /dev/ttyUSB$CPORT -s /etc/gcom/setpin.gcom || {
					log "Modem $CURRMODEM Failed to Unlock SIM Pin"
					$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Connect : Pin Locked"
					exit 0
				}
			fi
			;;
		esac
		$ROOTER/log/logger "Attempting to Reconnect Modem #$CURRMODEM ($MAN $MOD)"
		log "Attempting to ReConnect"

		BRK=0
		case $PROT in
		"1" )
			OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/connect-directip.gcom 2>/dev/null)
			ERROR="ERROR"
			if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
			then
				BRK=1
			fi
			;;
		"2" )
			if [ -e /sbin/uqmi ]; then
				$ROOTER/qmi/connectqmi.sh cdc-wdm$WDMNX $NAUTH $NAPN $NUSER $NPASS $PINCODE
			else
				$ROOTER/qmi/qmi_connect.lua start wwan$WWANX cdc-wdm$WDMNX $CURRMODEM $NAUTH $NAPN $NUSER $NPASS $PINCODE
			fi
			if [ -f /tmp/qmigood ]; then
				rm -f /tmp/qmigood
			else
				BRK=1
				if [ ! -e /sbin/uqmi ]; then
					$ROOTER/qmi/qmi_connect.lua stop wwan$WWANX cdc-wdm$WDMNX $CURRMODEM
				fi
			fi
			;;
		"4"|"6"|"7"|"24"|"26"|"27" )
			if [ ! -z /etc/kernel310 ]; then
				OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/connect-ncm.gcom 2>/dev/null)
			else
				OX=$(gcom -d /dev/cdc-wdm$WDMNX -s $ROOTER/gcom/connect-ncm.gcom 2>/dev/null)
			fi
			log "$OX"
			ERROR="ERROR"
			if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
			then
				BRK=1
			fi
			;;
		esac

		if [ $BRK = 1 ]; then
			$ROOTER/log/logger "Retry to Connect Modem #$CURRMODEM"
			log "Retry Connection"
  			let COUNTER=COUNTER+1
		else
			$ROOTER/log/logger "Reconnected Modem #$CURRMODEM"
			log "Connected"
			break
		fi
	done
	if [ $COUNTER -lt 6 ]; then
		case $PROT in
		"1"|"2"|"4"|"6"|"7"|"24"|"26"|"27" )
			ln -s $ROOTER/signal/modemsignal.sh $ROOTER_LINK/getsignal$CURRMODEM
			;;
		esac

		$ROOTER_LINK/getsignal$CURRMODEM $CURRMODEM $PROT &
		ifup wan$CURRMODEM
		sleep 20
		uci set modem.modem$CURRMODEM.connected=1
		uci commit modem
	else
		$ROOTER/log/logger "Retry to Connect Failed for Modem #$CURRMODEM"
		log "Connection Failed for Modem $CURRMODEM"
		$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Connect"
	fi
else
	log "Reset Failed for Modem $CURRMODEM"
	$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Reset"
fi




