#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "Lock Provider" "$@"
}

CURRMODEM=$1
CPORT=/dev/ttyUSB$(uci get modem.modem$CURRMODEM.commport)

LOCK=$(uci get modem.modeminfo$CURRMODEM.lock)
if [ -z $LOCK ]; then
	exit 0
fi

MCC=$(uci get modem.modeminfo$CURRMODEM.mcc)
if [ -z $MCC ]; then
	exit 0
fi
LMCC=`expr length $MCC`
if [ $LMCC -ne 3 ]; then
	exit 0
fi
MNC=$(uci get modem.modeminfo$CURRMODEM.mnc)
if [ -z $MNC ]; then
	exit 0
fi
LMNC=`expr length $MNC`
if [ $LMNC -eq 1 ]; then
	MNC=0$MNC
fi

export MCCMNC=$MCC$MNC

OX=$(gcom -d $CPORT -s $ROOTER/gcom/lock-prov.gcom 2>/dev/null)
ERROR="ERROR"
if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
then
	log "Error While Locking to Provider"
else
	log "Locked to Provider $MCC $MNC"
fi

