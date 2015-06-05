#!/bin/sh

MODEMTYPE=$1
NETMODE=$2

log() {
	logger -t "ModeChange" "$@"
}

CURRMODEM=$(uci get modem.general.modemnum)
uci set modem.modem$CURRMODEM.cmode="0"
uci commit modem

#ZTE
if [ $MODEMTYPE -eq 1 ]; then
	case $NETMODE in
		1*)
			ATC="AT+ZSNT=0,0,0" ;;
		2*)
			ATC="AT+ZSNT=0,0,1" ;;
		3*)
			ATC="AT+ZSNT=1,0,0" ;;
		4*)
			ATC="AT+ZSNT=0,0,2" ;;
		5*)
			ATC="AT+ZSNT=2,0,0" ;;
		6*)
			ATC="AT+ZSNT=0,0,6" ;;
		7*)
			ATC="AT+ZSNT=6,0,0" ;;
	esac
	echo "$ATC" > /tmp/modecmd$CURRMODEM.at
	echo "AT+ZBANDI=0" > /tmp/bandcmd$CURRMODEM.at
fi

#SIERRA
if [ $MODEMTYPE -eq 2 ]; then
	case $NETMODE in
		1*)
			ATC="AT!SELRAT=0" ;;
		2*)
			ATC="AT!SELRAT=4" ;;
		3*)
			ATC="AT!SELRAT=2" ;;
		4*)
			ATC="AT!SELRAT=3" ;;
		5*)
			ATC="AT!SELRAT=1" ;;
		6*)
			ATC="AT!SELRAT=6" ;;
		7*)
			ATC="AT!SELRAT=7" ;;
	esac
	echo "$ATC" > /tmp/modecmd$CURRMODEM.at
	echo "AT!BAND=0" > /tmp/bandcmd$CURRMODEM.at
fi

#Huawei
if [ $MODEMTYPE -eq 3 ]; then
	case $NETMODE in
		1*)
			ATC="AT^SYSCFGEX=\"00\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
		2*)
			ATC="AT^SYSCFGEX=\"0102\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
		3*)
			ATC="AT^SYSCFGEX=\"01\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
		4*)
			ATC="AT^SYSCFGEX=\"0201\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
		5*)
			ATC="AT^SYSCFGEX=\"02\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
		6*)
			ATC="AT^SYSCFGEX=\"0302\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
		7*)
			ATC="AT^SYSCFGEX=\"03\",3FFFFFFF,2,4,7FFFFFFFFFFFFFFF,," ;;
	esac
	echo "$ATC" > /tmp/modecmd$CURRMODEM.at
fi

#Huawei
if [ $MODEMTYPE -eq 4 ]; then
	case $NETMODE in
		1*)
			ATC="AT^SYSCFG=2,0,03FFFFFFF,2,4" ;;
		2*)
			ATC="AT^SYSCFG=2,1,03FFFFFFF,2,4" ;;
		3*)
			ATC="AT^SYSCFG=13,1,03FFFFFFF,2,4" ;;
		4*)
			ATC="AT^SYSCFG=2,2,03FFFFFFF,2,4" ;;
		5*)
			ATC="AT^SYSCFG=14,2,03FFFFFFF,2,4" ;;
	esac
	echo "$ATC" > /tmp/modecmd$CURRMODEM.at
fi

