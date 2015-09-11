. /lib/functions/procd.sh

local uVid uPid uMa uPr uSe
local idV idP
local sVe sMo sRe
local CURRMODEM BASEP
local retresult
local reinsert
local MODCNT=6
local OPENM="\x01\x00\x00\x00\x10\x00\x00\x00\x01\x00\x00\x00\x00\x01\x00\x00"

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

local modeswitch="/usr/bin/usb_modeswitch"

log() {
	logger -t "usb-modeswitch" "$@"
}

sanitize() {
	sed -e 's/[[:space:]]\+$//; s/[[:space:]]\+/_/g' "$@"
}

find_usb_attrs() {
	local usb_dir="/sys$DEVPATH"
	[ -f "$usb_dir/idVendor" ] || usb_dir="${usb_dir%/*}"

	uVid=$(cat "$usb_dir/idVendor")
	uPid=$(cat "$usb_dir/idProduct")
	uMa=$(sanitize "$usb_dir/manufacturer")
	uPr=$(sanitize "$usb_dir/product")
	uSe=$(sanitize "$usb_dir/serial")
}

display_top() {
	log "*****************************************************************"
	log "*"
}

display_bottom() {
	log "*****************************************************************"
}


display() {
	local line1=$1
	log "* $line1"
	log "*"
}

#
# Save Interface variables
#
save_variables() {
	echo 'MODSTART="'"$MODSTART"'"' > /tmp/variable.file
	echo 'WWAN="'"$WWAN"'"' >> /tmp/variable.file
	echo 'USBN="'"$USBN"'"' >> /tmp/variable.file
	echo 'ETHN="'"$ETHN"'"' >> /tmp/variable.file
	echo 'WDMN="'"$WDMN"'"' >> /tmp/variable.file
	echo 'BASEPORT="'"$BASEPORT"'"' >> /tmp/variable.file
}
#
# delay until ROOter Initialization done
#
bootdelay() {
	if [ ! -f /tmp/bootend.file ]; then
		log "Delay for boot up"
		sleep 10
		while [ ! -f /tmp/bootend.file ]; do
			sleep 1
		done
		sleep 10
	fi
}

#
# return modem number based on port number
# 0 is not found
#
find_device() {
	DEVN=$1
	COUNTER=1
	while [ $COUNTER -le $MODCNT ]; do
		EMPTY=$(uci get modem.modem$COUNTER.empty)
		if [ $EMPTY -eq 0 ]; then
			DEVS=$(uci get modem.modem$COUNTER.device)
			if [ $DEVN = $DEVS ]; then
				retresult=$COUNTER
				return
			fi
		fi
		let COUNTER=COUNTER+1 
	done
	retresult=0
}

#
# check if all modems are inactive or empty
# delete all if nothing active
#
check_all_empty() {
	COUNTER=1
	while [ $COUNTER -le $MODCNT ]; do
		EMPTY=$(uci get modem.modem$COUNTER.empty)
		if [ $EMPTY -eq 0 ]; then
			ACTIVE=$(uci get modem.modem$COUNTER.active)
			if [ $ACTIVE -eq 1 ]; then
				return
			fi
		fi
		let COUNTER=COUNTER+1 
	done
	COUNTER=1
	while [ $COUNTER -le $MODCNT ]; do
		uci delete modem.modem$COUNTER        
		uci set modem.modem$COUNTER=modem  
		uci set modem.modem$COUNTER.empty=1
		let COUNTER=COUNTER+1 
	done
	uci set modem.general.modemnum=1
	uci commit modem
	MODSTART=1
	WWAN=0
	USBN=0
	ETHN=1
	WDMN=0
	BASEPORT=0
	if 
		ifconfig eth1
	then
		ETHN=2
	fi
	save_variables
	display_top; display "No Modems present"; display_bottom
}

#
# Add Modem and connect
#
if [ "$ACTION" = add ]; then
	bootdelay
	find_usb_attrs

	if echo $DEVICENAME | grep -q ":" ; then
		exit 0
	fi
	
	if [ -z $uMa ]; then
		log "Ignoring Unnamed Hub"
		exit 0
	fi

	UPR=${uPr}
	CT=`echo $UPR | tr '[A-Z]' '[a-z]'`
	if echo $CT | grep -q "hub" ; then
		log "Ignoring Named Hub"
		exit 0
	fi

	if [ $uVid = 1d6b ]; then
		log "Ignoring Linux Hub"
		exit 0
	fi

	if [ -f /tmp/usbwait ]; then
		log "Delay for previous modem"
		while [ -f /tmp/usbwait ]; do
			sleep 5
		done
	fi
	echo "1" > /tmp/usbwait

	source /tmp/variable.file
	source /tmp/modcnt
	MODCNT=$MODCNTX

	reinsert=0
	find_device $DEVICENAME
	if [ $retresult -gt 0 ]; then
		ACTIVE=$(uci get modem.modem$retresult.active)
		if [ $ACTIVE = 1 ]; then
			rm -f /tmp/usbwait
			exit 0
		else
			IDP=$(uci get modem.modem$retresult.uPid)
			IDV=$(uci get modem.modem$retresult.uVid)
			if [ $uVid = $IDV -a $uPid = $IDP ]; then
				reinsert=1
				CURRMODEM=$retresult
			else
				display_top; display "Reinsert of different Modem not allowed"; display_bottom
				rm -f /tmp/usbwait
				exit 0
			fi
		fi
	fi

	log "Add : $DEVICENAME: Manufacturer=${uMa:-?} Product=${uPr:-?} Serial=${uSe:-?} $uVid $uPid"

	if [ $MODSTART -gt $MODCNT ]; then
		display_top; display "Exceeded Maximun Number of Modems"; display_bottom
		exit 0
	fi

	if [ $reinsert = 0 ]; then
		CURRMODEM=$MODSTART
	fi

	idV=$uVid
	idP=$uPid

	FILEN=$uVid:$uPid
	display_top; display "Start of Modem Detection and Connection Information" 
	display "Product=${uPr:-?} $uVid $uPid"; display_bottom
	cat /sys/kernel/debug/usb/devices > /tmp/prembim
	$ROOTER/mbimfind.lua $uVid $uPid 
	local retval=$?
	rm -f /tmp/prembim
	if [ ! -e /sbin/umbim ]; then
		retval=0
	fi
	if [ $retval -eq 1 ]; then
		display_top; display "Found MBIM Modem at $DEVICENAME"; display_bottom
		echo 2 >/sys/bus/usb/devices/$DEVICENAME/bConfigurationValue
	else
		if grep "$FILEN" /etc/usb-mode.json > /dev/null ; then
			procd_open_service "usbmode"
			procd_open_instance
			procd_set_param command "/sbin/usbmode" -s
			procd_close_instance
			procd_close_service
		else
			display_top; display "This device does not have a switch data file" 
			display "Product=${uPr:-?} $uVid $uPid"; display_bottom
		fi
	fi
	sleep 10
	usb_dir="/sys$DEVPATH"
	idV="$(sanitize "$usb_dir/idVendor")"
	idP="$(sanitize "$usb_dir/idProduct")"
	display_top; display "Switched to : $idV:$idP"; display_bottom

	if [ $idV = 2357 -a $idP = 9000 ]; then
		sleep 10
	fi

	cat /sys/kernel/debug/usb/devices > /tmp/wdrv
	$ROOTER/protofind.lua $idV $idP 
	local retval=$?
	display_top; display "ProtoFind returns : $retval"; display_bottom
	rm -f /tmp/wdrv

	if [ $reinsert = 0 ]; then
		BASEP=$BASEPORT
		if [ -f /tmp/drv ]; then
			source /tmp/drv
			BASEPORT=`expr $PORTN + $BASEPORT`
		fi
	fi
	rm -f /tmp/drv

	FORCE=$(uci get modem.modeminfo$CURRMODEM.ppp)
	if [ -n $FORCE ]; then
		if [ $FORCE = 1 ]; then
			log "Forcing PPP mode"
			if [ $idV = 12d1 ]; then
				retval=10
			else
				retval=11
			fi
			log "Forced Protcol Value : $retval"
		fi
	fi

	if [ $retval -ne 0 ]; then
		log "Found Modem$CURRMODEM"
		if [ $reinsert = 0 ]; then
			uci set modem.modem$CURRMODEM.empty=0
			uci set modem.modem$CURRMODEM.uVid=$uVid
			uci set modem.modem$CURRMODEM.uPid=$uPid
			uci set modem.modem$CURRMODEM.idV=$idV
			uci set modem.modem$CURRMODEM.idP=$idP
			uci set modem.modem$CURRMODEM.device=$DEVICENAME
			uci set modem.modem$CURRMODEM.baseport=$BASEP
			uci set modem.modem$CURRMODEM.maxport=$BASEPORT
			uci set modem.modem$CURRMODEM.proto=$retval
			uci set modem.modem$CURRMODEM.maxcontrol=/sys$DEVPATH/descriptors
			find_usb_attrs
			uci set modem.modem$CURRMODEM.manuf=$uMa
			uci set modem.modem$CURRMODEM.model=$uPr
			uci set modem.modem$CURRMODEM.serial=$uSe
		fi
		uci set modem.modem$CURRMODEM.active=1
		uci set modem.modem$CURRMODEM.connected=0
		uci commit modem
	fi

	if [ $reinsert = 0 -a $retval != 0 ]; then
		MODSTART=`expr $MODSTART + 1`
		save_variables
	fi

#
# Handle specific modem models
#
	case $retval in
	"0" )
		#
		# ubox GPS module
		#
		if [ $idV = 1546 ]; then
			if echo $uPr | grep -q "GPS"; then
				SYMLINK="gps0"
				BASEX=`expr 1 + $BASEP`
				ln -s /dev/ttyUSB$BASEX /dev/${SYMLINK}
				display_top ; display "Hotplug Symlink from /dev/ttyUSB$BASEX to /dev/${SYMLINK} created"
				display_bottom
			fi
		fi
		rm -f /tmp/usbwait
		exit 0
		;;
	"1" )
		log "Connecting a Sierra Modem"
		ln -s $ROOTER/connect/create_connect.sh $ROOTER_LINK/create_proto$CURRMODEM
		$ROOTER_LINK/create_proto$CURRMODEM $CURRMODEM &
		;;
	"2" )
		log "Connecting a QMI Modem"
		ln -s $ROOTER/connect/create_connect.sh $ROOTER_LINK/create_proto$CURRMODEM
		$ROOTER_LINK/create_proto$CURRMODEM $CURRMODEM &
		;;
	"3" )
		log "Connecting a MBIM Modem"
# use this for test.sh
#		ln -s $ROOTER/mbim/test.sh $ROOTER_LINK/create_proto$CURRMODEM
		ln -s $ROOTER/connect/create_connect.sh $ROOTER_LINK/create_proto$CURRMODEM
		$ROOTER_LINK/create_proto$CURRMODEM $CURRMODEM &
		;;
	"6"|"4"|"7"|"24"|"26"|"27" )
		log "Connecting a Huawei NCM Modem"
		ln -s $ROOTER/connect/create_connect.sh $ROOTER_LINK/create_proto$CURRMODEM
		$ROOTER_LINK/create_proto$CURRMODEM $CURRMODEM &
		;;
	"5" )
		log "Connecting a Hostless Modem or Phone"
		ln -s $ROOTER/connect/create_hostless.sh $ROOTER_LINK/create_proto$CURRMODEM
		$ROOTER_LINK/create_proto$CURRMODEM $CURRMODEM &
		;;
	"10"|"11"|"12"|"13"|"14" )
		log "Connecting a PPP Modem"
		ln -s $ROOTER/ppp/create_ppp.sh $ROOTER_LINK/create_proto$CURRMODEM
		$ROOTER_LINK/create_proto$CURRMODEM $CURRMODEM
		;;
	"9" )
		log "PPP HSO Modem"
		rm -f /tmp/usbwait
		;;
	esac

fi

#
# Remove Modem
#
if [ "$ACTION" = remove ]; then
	find_usb_attrs

	if echo $DEVICENAME | grep -q ":" ; then
		exit 0
	fi
	find_device $DEVICENAME
	if [ $retresult -gt 0 ]; then
		IDP=$(uci get modem.modem$retresult.idP)
		IDV=$(uci get modem.modem$retresult.idV)
		if [ $uVid = $IDV ]; then
			exit 0
		else
			uci set modem.modem$retresult.active=0
			uci set modem.modem$retresult.connected=0
			uci commit modem
			SMS=$(uci get modem.modem$CURRMODEM.sms)
			if [ $SMS = 1 ]; then
				if [ -e /usr/lib/sms/stopsms ]; then
					/usr/lib/sms/stopsms $CURRMODEM
				fi
			fi
			ifdown wan$retresult
			uci delete network.wan$retresult
			uci commit network
			killall -9 getsignal$retresult
			rm -f $ROOTER_LINK/getsignal$retresult
			killall -9 reconnect$retresult
			rm -f $ROOTER_LINK/reconnect$retresult
			killall -9 create_proto$retresult
			rm -f $ROOTER_LINK/create_proto$retresult
			$ROOTER/signal/status.sh $retresult "No Modem Present"
			$ROOTER/log/logger "Disconnect (Removed) Modem #$retresult"
			display_top; display "Remove : $DEVICENAME : Modem$retresult"; display_bottom
			check_all_empty
			rm -f /tmp/usbwait
		fi
	fi
fi

