#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "basedata" "$@"
}

local O
CURRMODEM=$1
COMMPORT=$2

get_base() {
	OX=$(gcom -d $COMMPORT -s $ROOTER/gcom/baseinfo.gcom 2>/dev/null)
	$ROOTER/log/at-logger "$COMMPORT $OX"
	M6=$(echo "$OX" | sed -e "s/      / /g")
	M5=$(echo "$M6" | sed -e "s/     / /g")
	M4=$(echo "$M5" | sed -e "s/    / /g")
	M3=$(echo "$M4" | sed -e "s/   / /g")
	M2=$(echo "$M3" | sed -e "s/  / /g")
	M2=$(echo "$M2" | sed -e "s/TAC:/   /;s/Tx Power:/   /;s/SINR/   /;s!SYSINFOEX:!SYSINFOEX: !;s!CNTI:!CNTI: !")
	M1=$(echo "$M2" | sed -e "s!Car0 Tot Ec/Io!+ECIO!;s!Car1 Tot Ec/Io!+ECIO1!;s!Car0 RSCP!+RSCP!;s!+CGMM: !!")
	M2=$(echo "$M1" | sed -e "s!Car1 RSCP!+RSCP1!;s!CSNR:!CSNR: !;s!RSSI (dBm):!RSSI4: !;s!AirCard !AirCard!;s!USB !USB!")
	M3=$(echo "$M2" | sed -e "s!RSRP (dBm):!RSRP4: !;s!RSRQ (dB):!RSRQ4: !;s!LTERSRP:!LTERSRP: !;s!Model:!+MODEL:!;s!SYSCFGEX:!SYSCFGEX: !")
	M=$(echo "$M3" | sed -e "s!RX level Carrier 0 (dBm):!RSSI3: !;s!RX level Carrier 1 (dBm):!RSSI13: !;s!SYSCFG:!SYSCFG: !;s!  ! !g")
	O=$M
}

get_base

COPS="-"
COPS_MCC="-"
COPS_MNC="-"

COPSX=$(echo "$O" | awk -F[\"] '/^\+COPS: 1,0/ {print $2}')
if [ "x$COPSX" != "x" ]; then
	COPS=$COPSX
fi

COPSX=$(echo "$O" | awk -F[\"] '/^\+COPS: 1,2/ {print $2}')
if [ "x$COPSX" != "x" ]; then
	COPS_MCC=${COPSX:0:3}
	COPS_MNC=${COPSX:3:3}
	if [ "$COPS" = "-" ]; then
		COPS=$(awk -F[\;] '/'$COPS'/ {print $2}' $ROOTER/signal/mccmnc.data)
		[ "x$COPS" = "x" ] && COPS="-"
	fi
fi

if [ "$COPS" = "-" ]; then
	COPS=$(echo "$O" | awk -F[\"] '/^\+COPS: 0,0/ {print $2}')
	if [ "x$COPS" = "x" ]; then
		COPS="-"
		COPS_MCC="-"
		COPS_MNC="-"
	fi
fi
COPS_MNC=" "$COPS_MNC

DOWN=$(echo "$O" | awk -F[,] '/\+CGEQNEG:/ {printf "%s", $4}')
if [ "x$DOWN" != "x" ]; then
	UP=$(echo "$O" | awk -F[,] '/\+CGEQNEG:/ {printf "%s", $3}')
	DOWN=$DOWN" kbps Down | "
	UP=$UP" kbps Up"
else
	DOWN="-"
	UP="-"
fi

MANUF=$(echo "$O" | awk -F[:] '/Manufacturer:/ { print $2}')
if [ "x$MANUF" = "x" ]; then
	MANUF=$(uci get modem.modem$CURRMODEM.manuf)
fi

MODEL=$(echo "$O" | awk -F[,\ ] '/^\+MODEL:/ {print $2}')
if [ "x$MODEL" != "x" ]; then
	MODEL=$(echo "$MODEL" | sed -e 's/"//g')
	if [ $MODEL = 0 ]; then
		MODEL = "mf820"
	fi
else
	MODEL=$(uci get modem.modem$CURRMODEM.model)
fi
MODEM=$MANUF" "$MODEL

echo 'COPS="'"$COPS"'"' > /tmp/base$CURRMODEM.file
echo 'COPS_MCC="'"$COPS_MCC"'"' >> /tmp/base$CURRMODEM.file
echo 'COPS_MNC="'"$COPS_MNC"'"' >> /tmp/base$CURRMODEM.file
echo 'MODEM="'"$MODEM"'"' >> /tmp/base$CURRMODEM.file
echo 'DOWN="'"$DOWN"'"' >> /tmp/base$CURRMODEM.file
echo 'UP="'"$UP"'"' >> /tmp/base$CURRMODEM.file

