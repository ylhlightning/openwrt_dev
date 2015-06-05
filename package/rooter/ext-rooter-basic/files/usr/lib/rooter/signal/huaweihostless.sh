#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "hostless " "$@"
}

local PROD LUCKNUM TIMEOUT
local PROV NETWORK SIGNAL CSQ

TMPFILE="/tmp/XXXXXX"

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

make_status() {
	echo "$IP" > /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$CSQ" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$MODEM" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$NETWORK" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo " " >> /tmp/status$CURRMODEM.file
	echo " " >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$CONN" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
}

get_signal() {
	TIMEOUT=3
	wget http://$IP/api/monitoring/status -O $TMPFILE > /dev/null 2>&1 &
	handle_timeout "$!"
	local in_CurrentNetworkType="`cat $TMPFILE | grep \"<CurrentNetworkType>\" | cut -d \">\" -f 2 | cut -d \"<\" -f 1`"
	if [ -z $in_CurrentNetworkType ]; then
		NETWORK="-"
	else
		[ "$in_CurrentNetworkType" = "19" ] && NETWORK="LTE" # LTE
		[ "$in_CurrentNetworkType" = "9" ] && NETWORK="HSPA+" # HSPA+
		[ "$in_CurrentNetworkType" = "7" ] && NETWORK="HSPA" # HSPA
		[ "$in_CurrentNetworkType" = "6" ] && NETWORK="HSUPA" # HSUPA
		[ "$in_CurrentNetworkType" = "5" ] && NETWORK="HSDPA" # HSDPA
		[ "$in_CurrentNetworkType" = "4" ] && NETWORK="WCDMA" # WCDMA
		[ "$in_CurrentNetworkType" = "3" ] && NETWORK="EDGE" # EDGE
		[ "$in_CurrentNetworkType" = "2" ] && NETWORK="GPRS" # GPRS
		[ "$in_CurrentNetworkType" = "1" ] && NETWORK="GSM" # GSM
	fi
	SIGNAL=""
	local in_SignalStrength="`cat $TMPFILE | grep \"<SignalStrength>\" | cut -d \">\" -f 2 | cut -d \"<\" -f 1`"
	[ "$in_SignalStrength" != "" ] && SIGNAL="$in_SignalStrength"

	if [ "$SIGNAL" = "" ]; then
		in_SignalStrength="`cat $TMPFILE | grep \"<SignalIcon>\" | cut -d \">\" -f 2 | cut -d \"<\" -f 1`"
		[ "$in_SignalStrength" != "" ] && SIGNAL="$((in_SignalStrength*20))"
	fi
	[ -z $SIGNAL ] && SIGNAL=0
}

CURRMODEM=$1
PROTO=$2
CONN="Modem #"$CURRMODEM

MANUF=$(uci get modem.modem$CURRMODEM.manuf)
MODEL=$(uci get modem.modem$CURRMODEM.model)
MODEM=$MANUF" "$MODEL
IP=$(uci get modem.modem$CURRMODEM.ip)

while [ 1 = 1 ]; do
	get_signal
	CSQ="$SIGNAL"
	make_status
	sleep 1
done

