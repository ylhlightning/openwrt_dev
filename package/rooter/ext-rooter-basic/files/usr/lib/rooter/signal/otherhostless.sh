#!/bin/sh

ROOTER=/usr/lib/rooter

log() {
	logger -t "hostless " "$@"
}

CURRMODEM=$1
PROTO=$2
CONN="Modem #"$CURRMODEM

MANUF=$(uci get modem.modem$CURRMODEM.manuf)
MODEL=$(uci get modem.modem$CURRMODEM.model)
MODEM=$MANUF" "$MODEL
IP=$(uci get modem.modem$CURRMODEM.ip)

echo "$IP" > /tmp/status$CURRMODEM.file
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
