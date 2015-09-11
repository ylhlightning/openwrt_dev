#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "Huawei Data" "$@"
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

process_huawei() {
	CSNR=$(echo "$O" | awk -F[,\ ] '/^\^CSNR:/ {print $2}')
	if [ "x$CSNR" != "x" ]; then
		RSCP=$CSNR
		CSNR=$(echo "$O" | awk -F[,\ ] '/^\^CSNR:/ {print $3}')
		if [ "x$CSNR" != "x" ]; then
			ECIO=$CSNR
		else
			ECIO=`expr $RSCP - $CSQX`
		fi
	else
		EC=$(echo "$O" | awk -F[,\ ] '/^\+CSQ:/ {print $4}')
		if [ "x$EC" != "x" ]; then
			ECIO=$EC
			EX=$(printf %.0f $ECIO)
			RSCP=`expr $CSQX + $EX`
		fi
	fi

	LTERSRP=$(echo "$O" | awk -F[,\ ] '/^\^LTERSRP:/ {print $2}')
	if [ "x$LTERSRP" != "x" ]; then
		RSCP=$LTERSRP" (RSRP)"
		LTERSRP=$(echo "$O" | awk -F[,\ ] '/^\^LTERSRP:/ {print $3}')
		if [ "x$LTERSRP" != "x" ]; then
			ECIO=$LTERSRP" (RSRQ)"
		else
			ECIO=`expr $RSCP - $CSQX`
		fi
	fi

	NETMODE="0"
	SYSCFG=$(echo "$O" | awk -F[,\"] '/^\^SYSCFGEX:/ {print $2}')
	if [ "x$SYSCFG" != "x" ]; then
		MODTYPE="3"
		case $SYSCFG in
		"00" )
			NETMODE="1"
			;;
		"01" )
			NETMODE="3"
			;;
		"03" )
			NETMODE="7"
			;;
		* )
			ACQ=${SYSCFG:0:2}
			case $ACQ in
			"01" )
				NETMODE="2"
				;;
			"02" )
				NETMODE="4"
				;;
			"03" )
				NETMODE="6"
				;;
			esac
			;;
		esac
	else
		SYSCFG=$(echo "$O" | awk -F[,\ ] '/^\^SYSCFG:/ {print $2}')
		if [ "x$SYSCFG" != "x" ]; then
			MODTYPE="4"
			case $SYSCFG in
			"7" )
				NETMODE="1"
				;;
			"13" )
				NETMODE="3"
				;;
			"14" )
				NETMODE="5"
				;;
			* )
				SYSCFG=$(echo "$O" | awk -F[,\ ] '/^\^SYSCFG:/ {print $3}')
				case $SYSCFG in
				"0" )
					NETMODE="1"
					;;
				"1" )
					NETMODE="2"
					;;
				"2" )
					NETMODE="4"
					;;
				esac
				;;
			esac
		fi
	fi


	MODE="-"
	TECH=$(echo "$O" | awk -F[,] '/^\^SYSINFOEX:/ {print $9}' | sed 's/"//g')
	if [ "x$TECH" != "x" ]; then
		MODE="$TECH"
	fi

	if [ "x$MODE" = "x-" ]; then
		TECH=$(echo "$O" | awk -F[,\ ] '/^\^SYSINFO:/ {print $7}')
		if [ "x$TECH" != "x" ]; then
			case $TECH in
				17*) MODE="HSPA+ (64QAM)";;
				18*) MODE="HSPA+ (MIMO)";;
				1*) MODE="GSM";;
				2*) MODE="GPRS";;
				3*) MODE="EDGE";;
				4*) MODE="UMTS";;
				5*) MODE="HSDPA";;
				6*) MODE="HSUPA";;
				7*) MODE="HSPA";;
				9*) MODE="HSPA+";;
				 *) MODE=$TECH;;
			esac
		fi
	fi

	CMODE=$(uci get modem.modem$CURRMODEM.cmode)
	if [ $CMODE = 0 ]; then
		NETMODE="10"
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

OY=$(gcom -d $COMMPORT -s $ROOTER/gcom/huaweiinfo.gcom 2>/dev/null)
$ROOTER/log/at-logger "$COMMPORT $OY"
fix_data
process_csq
process_huawei

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
