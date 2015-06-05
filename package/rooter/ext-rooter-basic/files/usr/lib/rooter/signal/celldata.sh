#!/bin/sh

ROOTER=/usr/lib/rooter

local O
CURRMODEM=$1
COMMPORT=$2

get_cell() {
	OX=$(gcom -d $COMMPORT -s $ROOTER/gcom/cellinfo.gcom 2>/dev/null)
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

get_cell

CREG="+CREG:"
LAC=$(echo "$O" | awk -F[,] '/\'$CREG'/ {printf "%s", toupper($3)}' | sed 's/[^A-F0-9]//g')
if [ "x$LAC" = "x" ]; then
    CREG="+CGREG:"
    LAC=$(echo "$O" | awk -F[,] '/\'$CREG'/ {printf "%s", toupper($3)}' | sed 's/[^A-F0-9]//g')
fi

if [ "x$LAC" != "x" ]; then
	LAC_NUM=$(printf %d 0x$LAC)
else
	LAC="-"
	LAC_NUM="-"
fi
LAC_NUM="  ("$LAC_NUM")"

CID=$(echo "$O" | awk -F[,] '/\'$CREG'/ {printf "%s", toupper($4)}' | sed 's/[^A-F0-9]//g')
if [ "x$CID" != "x" ]; then
	if [ ${#CID} -le 4 ]; then
		LCID="-"
		LCID_NUM="-"
		RNC="-"
		RNC_NUM="-"
	else
		LCID=$CID
		LCID_NUM=$(printf %d 0x$LCID)
		RNC=$(echo "$LCID" | awk '{print substr($1,1,length($1)-4)}')
		RNC_NUM=$(printf %d 0x$RNC)
		CID=$(echo "$LCID" | awk '{print substr($1,length(substr($1,1,length($1)-4))+1)}')
		RNC_NUM=" ($RNC_NUM)"
	fi

	CID_NUM=$(printf %d 0x$CID)
else
	LCID="-"
	LCID_NUM="-"
	RNC="-"
	RNC_NUM="-"
	CID="-"
	CID_NUM="-"
fi
CID_NUM="  ("$CID_NUM")"

echo 'LAC="'"$LAC"'"' > /tmp/cell$CURRMODEM.file
echo 'LAC_NUM="'"$LAC_NUM"'"' >> /tmp/cell$CURRMODEM.file
echo 'CID="'"$CID"'"' >> /tmp/cell$CURRMODEM.file
echo 'CID_NUM="'"$CID_NUM"'"' >> /tmp/cell$CURRMODEM.file
echo 'RNC="'"$RNC"'"' >> /tmp/cell$CURRMODEM.file
echo 'RNC_NUM="'"$RNC_NUM"'"' >> /tmp/cell$CURRMODEM.file

