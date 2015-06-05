#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "Other Data" "$@"
}

local O
local OY
CURRMODEM=$1
COMMPORT=$2

fix_data() {
	OX=$OY
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

process_csq() {
	CSQ=$(echo "$O" | awk -F[,\ ] '/^\+CSQ:/ {print $2}')
	[ "x$CSQ" = "x" ] && CSQ=-1
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
}

CSQ="-"
CSQ_PER="-"
CSQ_RSSI="-"
ECIO="-"
RSCP="-"
ECIO1=" "
RSCP1=" "
MODE="-"
MODETYPE="-"
NETMODE="-"

OY=$(gcom -d $COMMPORT -s $ROOTER/gcom/otherinfo.gcom 2>/dev/null)
$ROOTER/log/at-logger "$COMMPORT $OY"
fix_data
process_csq

echo 'CSQ="'"$CSQ"'"' > /tmp/signal$CURRMODEM.file
echo 'CSQ_PER="'"$CSQ_PER"'"' >> /tmp/signal$CURRMODEM.file
echo 'CSQ_RSSI="'"$CSQ_RSSI"'"' >> /tmp/signal$CURRMODEM.file
echo 'ECIO="'"$ECIO"'"' >> /tmp/signal$CURRMODEM.file
echo 'RSCP="'"$RSCP"'"' >> /tmp/signal$CURRMODEM.file
echo 'ECIO1="'"$ECIO1"'"' >> /tmp/signal$CURRMODEM.file
echo 'RSCP1="'"$RSCP1"'"' >> /tmp/signal$CURRMODEM.file
echo 'MODE="'"$MODE"'"' >> /tmp/signal$CURRMODEM.file
echo 'MODTYPE="'"$MODTYPE"'"' >> /tmp/signal$CURRMODEM.file
echo 'NETMODE="'"$NETMODE"'"' >> /tmp/signal$CURRMODEM.file

CONNECT=$(uci get modem.modem$CURRMODEM.connected)
if [ $CONNECT -eq 0 ]; then
	exit 0
fi

WWANX=$(uci get modem.modem$CURRMODEM.interface)
OPER=$(cat /sys/class/net/$WWANX/operstate 2>/dev/null)

if [ ! $OPER ]; then
	exit 0
fi
if echo $OPER | grep -q "unknown"; then
	exit 0
fi

if echo $OPER | grep -q "down"; then
	echo "1" > "/tmp/connstat"$CURRMODEM
fi
