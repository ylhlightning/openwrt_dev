#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "QMI Connect" "$@"
}

device=/dev/$1
auth=$2
NAPN=$3
username=$4
password=$5
pincode=$6

case $auth in
	"0" )
		auth="none"
	;;
	"1" )
		auth="pap"
	;;
	"2" )
		auth="chap"
	;;
	*)
		auth="none"
	;;
esac
if [ $username = NIL ]; then
	username=
fi
if [ $password = NIL ]; then
	password=
fi

while uqmi -s -d "$device" --get-pin-status | grep '"UIM uninitialized"' > /dev/null; do
		sleep 1;
done

[ -n "$pincode" ] && {
	uqmi -s -d "$device" --verify-pin1 "$pincode" || {
		log "Unable to verify PIN"
		exit 1
	}
}

uqmi -s -d "$device" --stop-network 0xffffffff --autoconnect > /dev/null

uqmi -s -d "$device" --set-data-format 802.3
uqmi -s -d "$device" --wda-set-data-format 802.3

log "Waiting for network registration"
while uqmi -s -d "$device" --get-serving-system | grep '"searching"' > /dev/null; do
	sleep 5;
done

log "Starting network $apn"
cid=`uqmi -s -d "$device" --get-client-id wds`
[ $? -ne 0 ] && {
	log "Unable to obtain client ID"
	exit 1
}

ST=$(uqmi -s -d "$device" --set-client-id wds,"$cid" --start-network "$NAPN" ${auth:+--auth-type $auth} \
	${username:+--username $username} ${password:+--password $password} --autoconnect)
log "Connection returned : $ST"

CONN=$(uqmi -s -d "$device" --get-data-status)
log "status is $CONN"
T=$(echo $CONN | grep "disconnected")
if [ -z $T ]; then
	echo "1" > /tmp/qmigood
else
	uqmi -s -d "$device" --stop-network 0xffffffff --autoconnect > /dev/null
fi

