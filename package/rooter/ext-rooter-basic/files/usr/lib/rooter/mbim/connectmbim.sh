#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "MBIM Connect" "$@"
}

device=/dev/$1
CURRMODEM=$2
NAUTH=$3
NAPN=$4
NUSER=$5
NPASS=$6
PINC=$7

case $NAUTH in
	"0" )
		NAUTH=
	;;
	"1" )
		NAUTH="pap"
	;;
	"2" )
		NAUTH="chap"
	;;
esac
if [ $NUSER = NIL ]; then
	NUSER=
fi
if [ $NPASS = NIL ]; then
	NPASS=
fi

tid=2

log "Open modem and get capabilities"
DCAPS=$(umbim -n -d $device caps)
retq=$?
if [ $retq -eq 0 ]; then
	CUSTOM=$(echo "$DCAPS" | awk '/customdataclass:/ {print $2}')
	echo 'CUSTOM="'"$CUSTOM"'"' > /tmp/mbimcustom$CURRMODEM
	tid=$((tid + 1))
	if [ ! -z $PINC ]; then
		log "Sending PIN"
		umbim -n -t $tid -d $device unlock "$pincode"
		retq=$?
		if [ $retq -ne 0 ]; then
			log "Pin unlock failed"
			exit 1
		fi
	fi
	tid=$((tid + 1))
	log "Check PIN state"
	umbim -n -t $tid -d $device pinstate
	retq=$?
	if [ $retq != 0 ]; then
		log "PIN is required"
		exit 1
	else
		log "PIN unlocked"
	fi
	tid=$((tid + 1))
	log "Check Subscriber ready state"
	umbim -n -t $tid -d $device subscriber
	retq=$?
	tid=$((tid + 1))
	log "Check Network Registration"
	REG=$(umbim -n -t $tid -d $device registration)
	retq=$?
	if [ $retq != 0 ]; then
		log "Failed to Register"
		exit 1
	else
		log "Registered to network"
	fi
	MCCMNC=$(echo "$REG" | awk '/provider_id:/ {print $2}')
	PROV=$(echo "$REG" | awk '/provider_name:/ {print $2}')
	MCC=${MCCMNC:0:3}
	MNC=${MCCMNC:3}
	echo 'MCC="'"$MCC"'"' > /tmp/mbimmcc$CURRMODEM
	echo 'MNC="'"$MNC"'"' >> /tmp/mbimmcc$CURRMODEM
	echo 'PROV="'"$PROV"'"' >> /tmp/mbimmcc$CURRMODEM
	tid=$((tid + 1))
	log "Try to Attach to network"
	ATTACH=$(umbim -n -t $tid -d $device attach)
	retq=$?
	if [ $retq != 0 ]; then
		log "Failed to attach to network"
		exit 1
	else
		log "Attached to network"
	fi
	log "Attempt to connect to network"
	COUNTER=1
	BRK=0
	while [ $COUNTER -lt 6 ]; do
		umbim -n -t $tid -d $device connect "$NAPN" "$NAUTH" "$NUSER" "$NPASS"
		retq=$?
		if [ $retq != 0 ]; then
			tid=$((tid + 1))
			sleep 1;
			let COUNTER=COUNTER+1
		else
			log "Connected to network"
			BRK=1
			break
		fi 
	done

	if [ $BRK -eq 0 ]; then
		log "Failed to connect to network"
		exit 1
	fi

	tid=$((tid + 1))
	ATTACH=$(umbim -n -t $tid -d $device attach)
	UP=$(echo "$ATTACH" | awk '/uplinkspeed:/ {print $2}')
	DWN=$(echo "$ATTACH" | awk '/downlinkspeed:/ {print $2}')
	MODE=$(echo "$ATTACH" | awk '/highestavailabledataclass:/ {print $2}')
	echo 'UP="'"$UP"'"' > /tmp/mbimqos$CURRMODEM
	echo 'DOWN="'"$DWN"'"' >> /tmp/mbimqos$CURRMODEM
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
		CLASS="CUSTOM"
	fi
	if [ -z $CLASS ]; then
		CLASS="UNKNOWN"
	fi
	echo 'MODE="'"$CLASS"'"' > /tmp/mbimmode$CURRMODEM
	tid=$((tid + 1))
	SIGNAL=$(umbim -n -t $tid -d $device signal)
	CSQ=$(echo "$SIGNAL" | awk '/rssi:/ {print $2}')
	echo 'CSQ="'"$CSQ"'"' > /tmp/mbimsig$CURRMODEM
	echo "1" > /tmp/mbimgood
else
	log "Failed to open modem"
	exit 1
fi


exit 0
