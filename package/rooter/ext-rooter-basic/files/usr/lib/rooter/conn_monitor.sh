#!/bin/sh

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

log() {
	logger -t "Connection Monitor" "$@"
}

source /tmp/modcnt
MODCNT=$MODCNTX

local ELAPSE=0
local ALIVE=0
local WAIT=60
STARTIME=$(date +%s)

power_toggle() {
	CNT=$1
	if [ -f "/tmp/gpiopin" ]; then
		$ROOTER/pwrtoggle.sh 3
	else
		$ROOTER_LINK/reconnect$CNT $CNT &
	fi
}

ping_modem() {
	local PINGWAIT=$(uci get modem.ping.pingwait)
	local PINGNUM=$(uci get modem.ping.pingnum)
	local PINGSERV1=$(uci get modem.ping.pingserv1)
	local PINGSERV2=$(uci get modem.ping.pingserv2)

	COUNTER=1
	while [ $COUNTER -le $MODCNT ]; do
		EMPTY=$(uci get modem.modem$COUNTER.empty)
		if [ $EMPTY -eq 0 ]; then
			ACTIVE=$(uci get modem.modem$COUNTER.active)
			if [ $ACTIVE -eq 1 ]; then
				CONN=$(uci get modem.modem$COUNTER.connected)
				if [ $CONN -eq 1 ]; then
					CONDWN=0
					INTER=$(uci get modem.modem$COUNTER.interface)
					if ! ping -q -I $INTER -c $PINGNUM -W $PINGWAIT $PINGSERV1 > /dev/null; then
						if [ x"$PINGSERV2" != x ]; then
							if ping -q -I $INTER -c $PINGNUM -W $PINGWAIT $PINGSERV2 > /dev/null; then
								log "Modem $COUNTER Connection is Alive"
								CONDWN=1
							fi
						fi
					else
						log "Modem $COUNTER Connection is Alive"
						CONDWN=1
					fi
					if [ $CONDWN -eq 0 ]; then
						case $ALIVE in
						"1" )
							log "Modem $COUNTER Connection is Down"
							;;
						"2" )
							log "Modem $COUNTER Connection is Down"
							reboot -f
							;;
						"3" )
							log "Modem $COUNTER Connection is Down"
							if [ -f $ROOTER_LINK/reconnect$COUNTER ]; then
								$ROOTER_LINK/reconnect$COUNTER $COUNTER &
							fi
							;;
						"4" )
							log "Modem $COUNTER Connection is Down"
							power_toggle $COUNTER
							;;
						esac
					fi
				fi
			fi
		fi
		let COUNTER=COUNTER+1 
	done
}

while [ 1 = 1 ]; do
	COUNTER=1
	while [ $COUNTER -le $MODCNT ]; do
		EMPTY=$(uci get modem.modem$COUNTER.empty)
		if [ $EMPTY -eq 0 ]; then
			ACTIVE=$(uci get modem.modem$COUNTER.active)
			if [ $ACTIVE -eq 1 ]; then
				CONN=$(uci get modem.modem$COUNTER.connected)
				if [ $CONN -eq 1 ]; then
					if [ -f "/tmp/connstat$COUNTER" ]; then
						log "Modem $COUNTER is down from Modem"
						if [ -f $ROOTER_LINK/reconnect$COUNTER ]; then
							$ROOTER_LINK/reconnect$COUNTER $COUNTER &
						fi
						rm -f /tmp/connstat$COUNTER
					fi
				fi
			fi
		fi
		let COUNTER=COUNTER+1 
	done
	sleep 5

	PTIME=$(uci get modem.ping.pingtime)
	if [ -z $PTIME ]; then
		WAIT=60
	else
		WAIT=$(($PTIME * 60))
	fi
	CURRTIME=$(date +%s)
	let ELAPSE=CURRTIME-STARTIME
	if [ $ELAPSE -ge $WAIT ]; then
		STARTIME=$CURRTIME
		ALIVE=$(uci get modem.ping.alive)
		if [ ! -z $ALIVE ]; then
			if [ $ALIVE != "0" ]; then
				ping_modem
			fi
		fi
	fi
done