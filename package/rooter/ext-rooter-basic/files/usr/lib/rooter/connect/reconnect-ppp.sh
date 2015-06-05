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

killall -9 getsignal$CURRMODEM
rm -f $ROOTER_LINK/getsignal$CURRMODEM
ifdown wan$CURRMODEM
MAN=$(uci get modem.modem$CURRMODEM.manuf)
MOD=$(uci get modem.modem$CURRMODEM.model)
$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Reconnecting"
PROT=$(uci get modem.modem$CURRMODEM.proto)

CPORT=$(uci get modem.modem$CURRMODEM.commport)

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
	ifup wan$CURRMODEM
else
	log "Reset Failed for Modem $CURRMODEM"
	$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Reset"
fi




