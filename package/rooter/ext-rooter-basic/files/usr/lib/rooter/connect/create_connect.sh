#!/bin/sh

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

local CURRMODEM CPORT TIMEOUT

log() {
	logger -t "Create Connection" "$@"
}

handle_timeout(){
	local wget_pid="$1"
	local count=0
	TIMEOUT=70
	res=1
	if [ -d /proc/${wget_pid} ]; then
		res=0
	fi
	while [ "$res" = 0 -a $count -lt "$((TIMEOUT))" ]; do
		sleep 1
		count=$((count+1))
		res=1
		if [ -d /proc/${wget_pid} ]; then
			res=0
		fi
	done

	if [ "$res" = 0 ]; then
		log "Killing process on timeout"
		kill "$wget_pid" 2> /dev/null
		res=1
		if [ -d /proc/${wget_pid} ]; then
			res=0
		fi
		if [ "$res" = 0 ]; then
			log "Killing process on timeout"
			kill -9 $wget_pid 2> /dev/null	
		fi
	fi
}

set_dns() {
	local DNS1=$(uci get modem.modeminfo$CURRMODEM.dns1)
	local DNS2=$(uci get modem.modeminfo$CURRMODEM.dns2)
	if [ -z $DNS1 ]; then
		if [ -z $DNS2 ]; then
			return
		else
			uci set network.wan$CURRMODEM.peerdns=0  
			uci set network.wan$CURRMODEM.dns=$DNS2
		fi
	else
		uci set network.wan$CURRMODEM.peerdns=0
		if [ -z $DNS2 ]; then
			uci set network.wan$CURRMODEM.dns="$DNS1"
		else
			uci set network.wan$CURRMODEM.dns="$DNS2 $DNS1"
		fi
	fi
}

save_variables() {
	echo 'MODSTART="'"$MODSTART"'"' > /tmp/variable.file
	echo 'WWAN="'"$WWAN"'"' >> /tmp/variable.file
	echo 'USBN="'"$USBN"'"' >> /tmp/variable.file
	echo 'ETHN="'"$ETHN"'"' >> /tmp/variable.file
	echo 'WDMN="'"$WDMN"'"' >> /tmp/variable.file
	echo 'BASEPORT="'"$BASEPORT"'"' >> /tmp/variable.file
}

local NAPN NUSER NPASS NAUTH PINCODE

get_connect() {
	NAPN=$(uci get modem.modeminfo$CURRMODEM.apn)
	NUSER=$(uci get modem.modeminfo$CURRMODEM.user)
	NPASS=$(uci get modem.modeminfo$CURRMODEM.passw)
	NAUTH=$(uci get modem.modeminfo$CURRMODEM.auth)
	PINC=$(uci get modem.modeminfo$CURRMODEM.pincode)
#
# QMI and MBIM can't handle nil
#
	case $PROT in
	"2"|"3" )
		if [ -z $NUSER ]; then
			NUSER="NIL"
		fi
		if [ -z $NPASS ]; then
			NPASS="NIL"
		fi
		;;
	esac

	uci set modem.modem$CURRMODEM.apn=$NAPN
	uci set modem.modem$CURRMODEM.user=$NUSER
	uci set modem.modem$CURRMODEM.passw=$NPASS
	uci set modem.modem$CURRMODEM.auth=$NAUTH
	uci set modem.modem$CURRMODEM.pin=$PINC
	uci commit modem
}

CURRMODEM=$1
source /tmp/variable.file

MAN=$(uci get modem.modem$CURRMODEM.manuf)
MOD=$(uci get modem.modem$CURRMODEM.model)
BASEP=$(uci get modem.modem$CURRMODEM.baseport)
$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Connecting"
PROT=$(uci get modem.modem$CURRMODEM.proto)

local DELAY=$(uci get modem.modem$CURRMODEM.delay)
if [ -z $DELAY ]; then
	DELAY=5
fi

uci delete network.wan$CURRMODEM        
uci set network.wan$CURRMODEM=interface
uci set network.wan$CURRMODEM.proto=dhcp  
uci set network.wan$CURRMODEM.ifname=wwan$WWAN  
uci set network.wan$CURRMODEM._orig_bridge=false
uci set network.wan$CURRMODEM.metric=$CURRMODEM"0"
set_dns
uci commit network

uci set modem.modem$CURRMODEM.wdm=$WDMN
uci set modem.modem$CURRMODEM.wwan=$WWAN
uci set modem.modem$CURRMODEM.interface=wwan$WWAN
uci commit modem

#
# QMI, NCM and MBIM use cdc-wdm
#
case $PROT in
"2"|"3"|"4"|"6"|"7" )
	WDMNX=$WDMN
	WDMN=`expr 1 + $WDMN`
	;;
esac

WWANX=$WWAN
WWAN=`expr 1 + $WWAN`
save_variables
rm -f /tmp/usbwait

get_connect

case $PROT in
#
# Sierra Direct-IP modem comm port
#
"1" )
	log "Start Direct-IP Connection"
	while [ ! -e /dev/ttyUSB$BASEP ]; do
		sleep 1
	done
	sleep $DELAY

	CPORT=`expr 4 + $BASEP`
	if [ -e /dev/ttyUSB$CPORT ]; then
		CPORT=`expr 3 + $BASEP`
	else
		CPORT=`expr 2 + $BASEP`
	fi
	log "Sierra Comm Port : /dev/ttyUSB$CPORT"
	;;
#
# QMI modem comm port
#
"2" )
	log "Start QMI Connection"
	while [ ! -e /dev/cdc-wdm$WDMNX ]; do
		sleep 1
	done
	sleep $DELAY

	idV=$(uci get modem.modem$CURRMODEM.idV)
	idP=$(uci get modem.modem$CURRMODEM.idP)
	$ROOTER/common/modemchk.lua "$idV" "$idP" "0" "1"
	source /tmp/parmpass

	CPORT=`expr $CPORT + $BASEP`

	log "QMI Comm Port : /dev/ttyUSB$CPORT"
	;;
"3" )
	log "Start MBIM Connection"
	while [ ! -e /dev/cdc-wdm$WDMNX ]; do
		sleep 1
	done
	sleep $DELAY
	;;
#
# Huawei NCM
#
"4"|"6"|"7"|"24"|"26"|"27" )
	log "Start NCM Connection"
	case $PROT in
	"4"|"6"|"7" )
		while [ ! -e /dev/cdc-wdm$WDMNX ]; do
			sleep 1
		done
		;;
	"24"|"26"|"27" )
		while [ ! -e /dev/ttyUSB$BASEP ]; do
			sleep 1
		done
		;;
	esac
	sleep $DELAY

	idV=$(uci get modem.modem$CURRMODEM.idV)
	idP=$(uci get modem.modem$CURRMODEM.idP)
	if [ $PROT = "4" -o $PROT = "24" ]; then
		$ROOTER/common/modemchk.lua "$idV" "$idP" "0" "0"
	else
		if [ $PROT = "6" -o $PROT = "26" ]; then
			$ROOTER/common/modemchk.lua "$idV" "$idP" "0" "1"
		else
			$ROOTER/common/modemchk.lua "$idV" "$idP" "0" "2"
		fi
	fi
	source /tmp/parmpass

	CPORT=`expr $CPORT + $BASEP`

	log "NCM Comm Port : /dev/ttyUSB$CPORT"
	;;
esac

uci set modem.modem$CURRMODEM.commport=$CPORT
uci commit modem

export SETAPN=$NAPN
export SETUSER=$NUSER
export SETPASS=$NPASS
export SETAUTH=$NAUTH
export PINCODE=$PINC

case $PROT in
#
# Sierra and QMI use AT port reset
#
"1"|"2"|"4"|"6"|"7"|"24"|"26"|"27" )
#	OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/reset.gcom 2>/dev/null)
#	sleep 3
	$ROOTER/common/lockchk.sh $CURRMODEM
	;;
* )
	log "No port reset done"
	;;
esac

COUNTER=1
while [ $COUNTER -lt 6 ]; do
	case $PROT in
#
# Sierra and NCM uses separate Pincode setting
#
	"1"|"4"|"6"|"7"|"24"|"26"|"27" )
		if [ -n "$PINCODE" ]; then
			gcom -d /dev/ttyUSB$CPORT -s /etc/gcom/setpin.gcom || {
				log "Modem $CURRMODEM Failed to Unlock SIM Pin"
				$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Connect : Pin Locked"
				exit 0
			}
		fi
		;;
	* )
		log "Pincode in script"
		;;
	esac
	$ROOTER/log/logger "Attempting to Connect Modem #$CURRMODEM ($MAN $MOD)"
	log "Attempting to Connect"

	BRK=0
	case $PROT in
#
# Sierra connect script
#
	"1" )
		OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/connect-directip.gcom 2>/dev/null)
		ERROR="ERROR"
		if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
		then
			BRK=1
		fi
		;;
#
# QMI connect script
#
	"2" )
		if [ -e /sbin/uqmi ]; then
			$ROOTER/qmi/connectqmi.sh cdc-wdm$WDMNX $NAUTH $NAPN $NUSER $NPASS $PINCODE
		else
			$ROOTER/qmi/qmi_connect.lua start wwan$WWANX cdc-wdm$WDMNX $CURRMODEM $NAUTH $NAPN $NUSER $NPASS $PINCODE
		fi
		if [ -f /tmp/qmigood ]; then
			rm -f /tmp/qmigood
		else
			BRK=1
			if [ ! -e /sbin/uqmi ]; then
				$ROOTER/qmi/qmi_connect.lua stop wwan$WWANX cdc-wdm$WDMNX $CURRMODEM
			fi
		fi
		;;
#
# NCM connect script
#
	"4"|"6"|"7"|"24"|"26"|"27" )
		if [ -e /etc/kernel310 ]; then
			OX=$(gcom -d /dev/ttyUSB$CPORT -s $ROOTER/gcom/connect-ncm.gcom 2>/dev/null)
		else
			OX=$(gcom -d /dev/cdc-wdm$WDMNX -s $ROOTER/gcom/connect-ncm.gcom 2>/dev/null)
		fi
		log "$OX"
		ERROR="ERROR"
		if `echo ${OX} | grep "${ERROR}" 1>/dev/null 2>&1`
		then
			BRK=1
		fi
		;;
#
# MBIM connect script
#
	"3" )
#
# use exit for test.sh
#exit 0
		TIMEOUT=70
		$ROOTER/mbim/connectmbim.sh cdc-wdm$WDMNX $CURRMODEM $NAUTH $NAPN $NUSER $NPASS $PINC & 

#		$ROOTER/mbim/mbim_connect.lua start wwan$WWANX cdc-wdm$WDMNX $CURRMODEM $NAUTH $NAPN $NUSER $NPASS $PINC & 
		handle_timeout "$!"
		if [ -f /tmp/mbimgood ]; then
			rm -f /tmp/mbimgood
		else
			BRK=1
			TIMEOUT=10
			$ROOTER/mbim/mbim_connect.lua stop wwan$WWANX cdc-wdm$WDMNX $CURRMODEM &
			handle_timeout "$!"
		fi
		;;
	esac

	if [ $BRK = 1 ]; then
		$ROOTER/log/logger "Retry Connection with Modem #$CURRMODEM"
		log "Retry Connection"
  		let COUNTER=COUNTER+1
	else
		$ROOTER/log/logger "Modem #$CURRMODEM Connected"
		log "Connected"
		break
	fi
done
if [ $COUNTER -lt 6 ]; then
	case $PROT in
#
# Sierra, NCM and QMI use modemsignal.sh and reconnect.sh
#
	"1"|"2"|"4"|"6"|"7"|"24"|"26"|"27" )
		ln -s $ROOTER/signal/modemsignal.sh $ROOTER_LINK/getsignal$CURRMODEM
		ln -s $ROOTER/connect/reconnect.sh $ROOTER_LINK/reconnect$CURRMODEM
		$ROOTER/sms/check_sms.sh $CURRMODEM
		;;
	"3" )
		source /tmp/mbimcustom$CURRMODEM
		source /tmp/mbimqos$CURRMODEM
		source /tmp/mbimmcc$CURRMODEM
		source /tmp/mbimsig$CURRMODEM
		source /tmp/mbimmode$CURRMODEM
		uci set modem.modem$CURRMODEM.custom=$CUSTOM
		uci set modem.modem$CURRMODEM.provider=$PROV
		uci set modem.modem$CURRMODEM.down=$DOWN" kbps Down | "
		uci set modem.modem$CURRMODEM.up=$UP" kbps Up"
		uci set modem.modem$CURRMODEM.mcc=$MCC
		uci set modem.modem$CURRMODEM.mnc=" "$MNC
		uci set modem.modem$CURRMODEM.sig=$CSQ
		uci set modem.modem$CURRMODEM.mode=$MODE
		uci set modem.modem$CURRMODEM.sms=0
		uci commit modem
		rm -f /tmp/mbimcustom$CURRMODEM
		rm -f /tmp/mbimqos$CURRMODEM
		rm -f /tmp/mbimmcc$CURRMODEM
		rm -f /tmp/mbimsig$CURRMODEM
		rm -f /tmp/mbimmode$CURRMODEM

		#ln -s $ROOTER/mbim/mbimsignal.lua $ROOTER_LINK/getsignal$CURRMODEM
		ln -s $ROOTER/mbim/mbimdata.sh $ROOTER_LINK/getsignal$CURRMODEM
		ln -s $ROOTER/mbim/reconn_mbim.sh $ROOTER_LINK/reconnect$CURRMODEM
		;;
	esac

	$ROOTER_LINK/getsignal$CURRMODEM $CURRMODEM $PROT &
	ifup wan$CURRMODEM
	sleep 20
	uci set modem.modem$CURRMODEM.connected=1
	uci commit modem
else
	$ROOTER/log/logger "Connection Failed for Modem #$CURRMODEM"
	log "Connection Failed for Modem $CURRMODEM"
	$ROOTER/signal/status.sh $CURRMODEM "$MAN $MOD" "Failed to Connect"
fi




