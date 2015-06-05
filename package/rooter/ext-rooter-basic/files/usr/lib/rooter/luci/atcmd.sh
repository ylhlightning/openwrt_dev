#!/bin/sh

ATCMD=$1

CURRMODEM=$(uci get modem.general.modemnum)

rm -f /tmp/result$CURRMODEM.at

M2=$(echo "$ATCMD" | sed -e "s#~#\"#g")

echo "$M2" > /tmp/atcmd$CURRMODEM.at