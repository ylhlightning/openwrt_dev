#!/bin/sh

local CURRMODEM

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"
local TIMEOUT

log() {
	logger -t "Disconnect Modem" "$@"
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

CURRMODEM=$(uci get modem.general.miscnum)
uci set modem.modem$CURRMODEM.connected=0
uci commit modem

killall -9 getsignal$CURRMODEM
rm -f $ROOTER_LINK/getsignal$CURRMODEM
ifdown wan$CURRMODEM

MAN=$(uci get modem.modem$CURRMODEM.manuf)
MOD=$(uci get modem.modem$CURRMODEM.model)
$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Disconnected"

PROT=$(uci get modem.modem$CURRMODEM.proto)
CPORT=$(uci get modem.modem$CURRMODEM.commport)

case $PROT in
"3" )
	WDMNX=$(uci get modem.modem$CURRMODEM.wdm)
	WWANX=$(uci get modem.modem$CURRMODEM.wwan)
	TIMEOUT=10
	$ROOTER/mbim/mbim_connect.lua stop wwan$WWANX cdc-wdm$WDMNX $CURRMODEM &
	handle_timeout "$!"
	;;
* )
	OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/reset.gcom 2>/dev/null)
	;;
esac

$ROOTER/log/logger "Modem #$CURRMODEM was Manually Disconnected"
