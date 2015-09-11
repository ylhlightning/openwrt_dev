#!/bin/sh

CURRMODEM=$1
NUMBER=$2
TEXT=$3

IX="AT+CMGS=\""$NUMBER"\""$TEXT
echo "$IX" > /tmp/smscmd$CURRMODEM".at"

