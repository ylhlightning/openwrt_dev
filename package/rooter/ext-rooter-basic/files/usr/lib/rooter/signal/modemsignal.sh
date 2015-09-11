#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "modem signal" "$@"
}

CURRMODEM=$1
PROTO=$2
CONN="Modem #"$CURRMODEM
STARTIME=$(date +%s)
SMSTIME=0
COMMPORT="/dev/ttyUSB"$(uci get modem.modem$CURRMODEM.commport)
local CELL USED MAXED
NUMB=0

make_connect() {
	echo "Changing Port" > /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "-" >> /tmp/status$CURRMODEM.file
	echo "$MODEM" >> /tmp/status$CURRMODEM.file
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

make_signal() {
	echo "$COMMPORT" > /tmp/status$CURRMODEM.file
	echo "$CSQ" >> /tmp/status$CURRMODEM.file
	echo "$CSQ_PER" >> /tmp/status$CURRMODEM.file
	echo "$CSQ_RSSI" >> /tmp/status$CURRMODEM.file
	echo "$MODEM" >> /tmp/status$CURRMODEM.file
	echo "$COPS" >> /tmp/status$CURRMODEM.file
	echo "$MODE" >> /tmp/status$CURRMODEM.file
	echo "$LAC" >> /tmp/status$CURRMODEM.file
	echo "$LAC_NUM" >> /tmp/status$CURRMODEM.file
	echo "$CID" >> /tmp/status$CURRMODEM.file
	echo "$CID_NUM" >> /tmp/status$CURRMODEM.file
	echo "$COPS_MCC" >> /tmp/status$CURRMODEM.file
	echo "$COPS_MNC" >> /tmp/status$CURRMODEM.file
	echo "$RNC" >> /tmp/status$CURRMODEM.file
	echo "$RNC_NUM" >> /tmp/status$CURRMODEM.file
	echo "$DOWN" >> /tmp/status$CURRMODEM.file
	echo "$UP" >> /tmp/status$CURRMODEM.file
	echo "$ECIO" >> /tmp/status$CURRMODEM.file
	echo "$RSCP" >> /tmp/status$CURRMODEM.file
	echo "$ECIO1" >> /tmp/status$CURRMODEM.file
	echo "$RSCP1" >> /tmp/status$CURRMODEM.file
	echo "$NETMODE" >> /tmp/status$CURRMODEM.file
	echo "$CELL" >> /tmp/status$CURRMODEM.file
	echo "$MODTYPE" >> /tmp/status$CURRMODEM.file
	echo "$CONN" >> /tmp/status$CURRMODEM.file
	echo "$CHANNEL" >> /tmp/status$CURRMODEM.file
}

get_basic() {
	$ROOTER/signal/basedata.sh $CURRMODEM $COMMPORT
	source /tmp/base$CURRMODEM.file
	rm -f /tmp/base$CURRMODEM.file
	$ROOTER/signal/celldata.sh $CURRMODEM $COMMPORT
	source /tmp/cell$CURRMODEM.file
	rm -f /tmp/cell$CURRMODEM.file
	$ROOTER/signal/celltype.lua "$MODEM" $CURRMODEM
	source /tmp/celltype$CURRMODEM
	rm -f /tmp/celltype$CURRMODEM
}

SMS=$(uci get modem.modem$CURRMODEM.sms)
if [ $SMS = 1 ]; then
	if [ -e /usr/lib/sms/startsms ]; then
		/usr/lib/sms/startsms $CURRMODEM
	fi
fi

get_basic
while [ 1 = 1 ]; do
	if [ -e /tmp/port$CURRMODEM.file ]; then
		source /tmp/port$CURRMODEM.file
		rm -f /tmp/port$CURRMODEM.file
		COMMPORT="/dev/ttyUSB"$PORT
		uci set modem.modem$CURRMODEM.commport=$PORT
		make_connect
		get_basic
		STARTIME=$(date +%s)
	else
		CURRTIME=$(date +%s)
		let ELAPSE=CURRTIME-STARTIME
		if [ $ELAPSE -ge 60 ]; then
			STARTIME=$CURRTIME
			$ROOTER/signal/celldata.sh $CURRMODEM $COMMPORT
			source /tmp/cell$CURRMODEM.file
			rm -f /tmp/cell$CURRMODEM.file
		fi
		if [ $SMS = 1 ]; then
			if [ -e /usr/lib/sms/processsms ]; then
				/usr/lib/sms/processsms $CURRMODEM
			fi
		fi
		if [ -e /tmp/port$CURRMODEM.file ]; then
			source /tmp/port$CURRMODEM.file
			rm -f /tmp/port$CURRMODEM.file
			COMMPORT="/dev/ttyUSB"$PORT
			uci set modem.modem$CURRMODEM.commport=$PORT
			make_connect
			get_basic
			STARTIME=$(date +%s)
		else
			if [ -e /tmp/atcmd$CURRMODEM.at ]; then
				read line < /tmp/atcmd$CURRMODEM.at
				export ATCMD="$line"
				rm -f /tmp/atcmd$CURRMODEM.at
				rm -f /tmp/result$CURRMODEM.at
				OX=$(gcom -d $COMMPORT -s $ROOTER/gcom/run-at.gcom 2>/dev/null)
				$ROOTER/log/at-logger "$COMMPORT $OX"
				echo "$OX" > /tmp/result$CURRMODEM.at
			fi
			if [ -e /tmp/smscmd$CURRMODEM.at ]; then
				read line < /tmp/smscmd$CURRMODEM.at
				export ATCMD="$line"
				rm -f /tmp/smscmd$CURRMODEM.at
				OX=$(gcom -d $COMMPORT -s /usr/lib/sms/gcom/sendsms-at.gcom 2>/dev/null)
				$ROOTER/log/at-logger "$COMMPORT $OX"
			fi
			if [ -e /tmp/modecmd$CURRMODEM.at ]; then
				read line < /tmp/modecmd$CURRMODEM.at
				export ATMODE="$line"
				rm -f /tmp/modecmd$CURRMODEM.at
			else
				export ATMODE="xx"
			fi
			if [ -e /tmp/bandcmd$CURRMODEM.at ]; then
				read line < /tmp/bandcmd$CURRMODEM.at
				export ATBAND="$line"
				rm -f /tmp/bandcmd$CURRMODEM.at
			else
				export ATBAND="xx"
			fi

			VENDOR=$(uci get modem.modem$CURRMODEM.idV)
			case $VENDOR in
			"1199"|"0f3d" )
				$ROOTER/common/sierradata.sh $CURRMODEM $COMMPORT
				;;
			"19d2" )
				$ROOTER/common/ztedata.sh $CURRMODEM $COMMPORT
				;;
			"12d1" )
				$ROOTER/common/huaweidata.sh $CURRMODEM $COMMPORT
				;;
			* )
				$ROOTER/common/otherdata.sh $CURRMODEM $COMMPORT
				;;
			esac
			CHANNEL="-"
			source /tmp/signal$CURRMODEM.file
			rm -f /tmp/signal$CURRMODEM.file
			make_signal
			uci set modem.modem$CURRMODEM.cmode="1"
			uci commit modem
		fi
		if [ -e /tmp/port$CURRMODEM.file ]; then
			source /tmp/port$CURRMODEM.file
			rm -f /tmp/port$CURRMODEM.file
			COMMPORT="/dev/ttyUSB"$PORT
			uci set modem.modem$CURRMODEM.commport=$PORT
			make_connect
			get_basic
			STARTIME=$(date +%s)
		fi
	fi
done

