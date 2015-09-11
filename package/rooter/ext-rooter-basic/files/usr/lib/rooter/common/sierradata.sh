#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "Sierra Data" "$@"
}

local O
CURRMODEM=$1
COMMPORT=$2

get_sierra() {
	OX=$(gcom -d $COMMPORT -s $ROOTER/gcom/sierrainfo.gcom 2>/dev/null)
	M6=$(echo "$OX" | sed -e "s/      / /g")
	M5=$(echo "$M6" | sed -e "s/     / /g")
	M4=$(echo "$M5" | sed -e "s/    / /g")
	M3=$(echo "$M4" | sed -e "s/   / /g")
	M2=$(echo "$M3" | sed -e "s/  / /g")
	M2=$(echo "$M2" | sed -e "s/TAC:/   /;s/Tx Power:/   /;s/SINR/   /;s!SYSINFOEX:!SYSINFOEX: !;s!CNTI:!CNTI: !")
	M1=$(echo "$M2" | sed -e "s!Car0 Tot Ec/Io!+ECIOx!;s!Car1 Tot Ec/Io!+ECIO1x!;s!Car0 RSCP!+RSCPx!;s!+CGMM: !!")
	M2=$(echo "$M1" | sed -e "s!Car1 RSCP!+RSCP1x!;s!CSNR:!CSNR: !;s!RSSI (dBm):!RSSI4: !;s!AirCard !AirCard!;s!USB !USB!")
	M3=$(echo "$M2" | sed -e "s!RSRP (dBm):!RSRP4: !;s!RSRQ (dB):!RSRQ4: !;s!LTERSRP:!LTERSRP: !;s!Model:!+MODEL:!;s!SYSCFGEX:!SYSCFGEX: !")
	M7=$(echo "$M3" | sed -e "s!RX level Carrier 0 (dBm):!RSSI3: !;s!RX level Carrier 1 (dBm):!RSSI13: !;s!SYSCFG:!SYSCFG: !;s!  ! !g")
	M=$(echo "$M7" | sed -e "s!WCDMA channel:!UMTS:!")
	O=$M
	$ROOTER/log/at-logger "$COMMPORT $O"
}

get_sierra

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

ECIO=$(echo "$O" | awk -F[\ ] '/^\+ECIOx/ {print $2}')
[ "x$ECIO" = "x" ] && ECIO="-"
ECIO1=$(echo "$O" | awk -F[\ ] '/^\+ECIO1x/ {print $2}')
[ "x$ECIO1" = "x" ] && ECIO1=" "
[ "$ECIO1" = "n/a" ] && ECIO1=" "

RSCP=$(echo "$O" | awk -F[\ ] '/^\+RSCPx/ {print $2}')
[ "x$RSCP" = "x" ] && RSCP="-"
RSCP1=$(echo "$O" | awk -F[\ ] '/^\+RSCP1x/ {print $2}')
[ "x$RSCP1" = "x" ] && RSCP1=" "
[ "$RSCP1" = "n/a" ] && RSCP1=" "

RSSI3=$(echo "$O" | awk -F[\ ] '/^\RSSI3/ {print $2}')
if [ "x$RSSI3" != "x" ]; then
	CSQ_RSSI=$RSSI3" dBm"
else
	if [ "$ECIO" != "-" -a "$RSCP" != "-" ]; then
		EX=$(printf %.0f $ECIO)
		CSQ_RSSI=`expr $RSCP - $EX`
		CSQ_RSSI=$CSQ_RSSI" dBm"
	fi
fi

RSSI4=$(echo "$O" | awk -F[\ ] '/^\RSSI4/ {print $2}')
if [ "x$RSSI4" != "x" ]; then
	CSQ_RSSI=$RSSI4" dBm"
	RSRP4=$(echo "$O" | awk -F[\ ] '/^\RSRP4/ {print $2}')
	if [ "x$RSRP4" != "x" ]; then
		RSCP=$RSRP4" (RSRP)"
		RSRQ4=$(echo "$O" | awk -F[\ ] '/^\RSRQ4/ {print $2}')
		if [ "x$RSRQ4" != "x" ]; then
			ECIO=$RSRQ4" (RSRQ)"
		fi
	fi
fi

WCHANNEL=$(echo "$O" | awk -F[\ ] '/^\UMTS:/ {print $2}')
if [ "x$WCHANNEL" = "x" ]; then
	WCHANNEL="-"
fi

CHANNEL=$(echo "$O" | awk -F[\ ] '/^\Channel:/ {print $2}')
if [ "x$CHANNEL" = "x" ]; then
	CHANNEL="-"
fi

if [ "$WCHANNEL" != "-" ]; then
	CHANNEL=$WCHANNEL" ("$CHANNEL")"
fi

MODE="-"
TECH=$(echo "$O" | awk -F[,\ ] '/^\*CNTI:/ {print $3}' | sed 's|/|,|g')
if [ "x$TECH" != "x" ]; then
	MODE="$TECH"
fi

SELRAT=$(echo "$O" | awk -F[,\ ] '/^\!SELRAT:/ {print $2}')
if [ "x$SELRAT" != "x" ]; then
	MODTYPE="2"
	case $SELRAT in
	"00" )
		NETMODE="1"
		;;
	"01" )
		NETMODE="5"
		;;
	"03" )
		NETMODE="4"
		;;
	"02" )
		NETMODE="3"
		;;
	"04" )
		NETMODE="2"
		;;
	"05" )
		NETMODE="6"
		;;
	"06" )
		NETMODE="7"
		;;

	esac
fi

CMODE=$(uci get modem.modem$CURRMODEM.cmode)
if [ $CMODE = 0 ]; then
	NETMODE="10"
fi

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
echo 'CHANNEL="'"$CHANNEL"'"' >> /tmp/signal$CURRMODEM.file

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

