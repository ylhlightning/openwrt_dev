#!/bin/sh

SLOT=$1
CURRMODEM=$2

ATCMD="AT+CMGD=$SLOT"

echo "$ATCMD" > /tmp/atcmd$CURRMODEM".at"