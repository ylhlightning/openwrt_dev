#!/bin/sh

CURRMODEM=$1
MSG=$2
MSG1=$3
if [ -z $MSG1 ]; then
	MSG1="-"
fi

echo "-" > /tmp/status$CURRMODEM.file
echo "-" >> /tmp/status$CURRMODEM.file
echo "-" >> /tmp/status$CURRMODEM.file
echo "-" >> /tmp/status$CURRMODEM.file
echo "$MSG" >> /tmp/status$CURRMODEM.file
echo "$MSG1" >> /tmp/status$CURRMODEM.file
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
echo "Modem $CURRMODEM" >> /tmp/status$CURRMODEM.file
echo "-" >> /tmp/status$CURRMODEM.file
