#!/bin/sh

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

local CURRMODEM

log() {
	logger -t "MBIM Data" "$@"
}

build_status() {
	CSQ=$signal
	if [ $CSQ -ge 0 -a $CSQ -le 31 ]; then
		CSQ_PER=$(($CSQ * 100/31))
		CSQ_RSSI=$((2 * CSQ - 113))
		CSQX=$CSQ_RSSI
		[ $CSQ -eq 0 ] && CSQ_RSSI="<= "$CSQ_RSSI
		[ $CSQ -eq 31 ] && CSQ_RSSI=">= "$CSQ_RSSI
		CSQ_PER=$CSQ_PER"%"
		CSQ_RSSI=$CSQ_RSSI" dBm"
	else
		CSQ="-"
		CSQ_PER="-"
		CSQ_RSSI="-"
	fi
	echo "-" > /tmp/status$CURRMODEM.file
	echo "$CSQ" >> /tmp/status$CURRMODEM.file
	echo "$CSQ_PER" >> /tmp/status$CURRMODEM.file
	echo "$CSQ_RSSI" >> /tmp/status$CURRMODEM.file
	echo "$manuf" >> /tmp/status$CURRMODEM.file
	echo "$provider" >> /tmp/status$CURRMODEM.file
	echo "$cellmode" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$mcc" >> /tmp/status$CURRMODEM.file
	echo "$mnc" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$down" >> /tmp/status$CURRMODEM.file
	echo "$up" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo " " >> /tmp/status$CURRMODEM.file
	echo " " >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$conn" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
}

CURRMODEM=$1

conn="Modem #"$CURRMODEM
custom=$(uci get modem.modem$CURRMODEM.custom)
port=$(uci get modem.modem$CURRMODEM.wdm)
netd=$(uci get modem.modem$CURRMODEM.wwan)
manuf=$(uci get modem.modem$CURRMODEM.manuf)
model=$(uci get modem.modem$CURRMODEM.model)
mcc=$(uci get modem.modem$CURRMODEM.mcc)
mnc=$(uci get modem.modem$CURRMODEM.mnc)
up=$(uci get modem.modem$CURRMODEM.up)
down=$(uci get modem.modem$CURRMODEM.down)
provider=$(uci get modem.modem$CURRMODEM.provider)
cellmode=$(uci get modem.modem$CURRMODEM.mode)
if [ $cellmode = "CUSTOM" ]; then
	cellmode=$custom
fi
signal=$(uci get modem.modem$CURRMODEM.sig)

device="/dev/cdc-wdm"$port
netdev="wwan"$netd
manuf=$manuf" "$model

tid=2
while [ 1 -eq 1 ]; do
	tid=2
	ATTACH=$(umbim -n -t $tid -d $device attach)
	MODE=$(echo "$ATTACH" | awk '/highestavailabledataclass:/ {print $2}')
	if [ -z $MODE ]; then
		MODE=0
	fi
	let "CS=0x1"
	let "CT = $MODE & $CS"
	if [ $CT -eq $CS ]; then
		CLASS="GPRS"
	fi
	let "CS=0x2"
	let "CT = $MODE & $CS"
	if [ $CT -eq $CS ]; then
		CLASS="EDGE"
	fi
	let "CS=0x4"
	let "CT = $MODE & $CS"
	if [ $CT -eq $CS ]; then
		CLASS="UMTS"
	fi
	let "CS=0x8"
	let "CT = $MODE & $CS"
	if [ $CT -eq $CS ]; then
		CLASS="HSDPA"
	fi
	let "CS=0x10"
	let "CT = $MODE & $CS"
	if [ $CT -eq $CS ]; then
		CLASS="HSUPA"
	fi
	let "CS=0x20"
	let "CT = $MODE & $CS"
	if [ $CT -eq $CS ]; then
		CLASS="LTE"
	fi
	CUS=${MODE:0:1}
	if [ $CUS = "8" ]; then
		CLASS=$custom
	fi
	if [ -z $CLASS ]; then
		CLASS="UNKNOWN"
	fi
	cellmode=$CLASS
	tid=$((tid + 1))
	SIGNAL=$(umbim -n -t $tid -d $device signal)
	signal=$(echo "$SIGNAL" | awk '/rssi:/ {print $2}')
	build_status
	sleep 2
done
