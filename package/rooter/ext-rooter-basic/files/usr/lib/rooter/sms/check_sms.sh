#!/bin/sh

ROOTER=/usr/lib/rooter

CURRMODEM=$1
CPORT=$(uci get modem.modem$CURRMODEM.commport)

OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/smschk.gcom 2>/dev/null)
ERROR="ERROR"
if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
then
	uci set modem.modem$CURRMODEM.sms=0
else
	uci set modem.modem$CURRMODEM.sms=1
fi
uci commit modem