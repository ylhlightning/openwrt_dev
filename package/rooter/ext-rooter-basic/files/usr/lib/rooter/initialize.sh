#!/bin/sh
. /lib/functions.sh

ROOTER=/usr/lib/rooter
ROOTER_LINK=$ROOTER"/links"

CODENAME="ROOter "
if [ -f "/etc/codename" ]; then
	source /etc/codename
fi

#
# Set the maximum number of modems supported
#
MAX_MODEMS=2
local MODCNT

log() {
	logger -t "ROOter Initialize" "$@"
}

firstboot() {
	uci set system.@system[-1].hostname="ROOter"
	uci set system.@system[-1].cronloglevel="9"
	uci commit system
	uci set wireless.@wifi-device[0].disabled=0; uci commit wireless; 
	uci set wireless.@wifi-iface[0].disabled=0; uci commit wireless; /sbin/wifi up
	log "ROOter First Boot finalized"

	mkdir -p $ROOTER_LINK
}

do_zone() {
	local config=$1
	local name
	local network
	local newnet="wan"

	config_get name $1 name
	config_get network $1 network
	if [ $name = wan ]; then
		COUNTER=1
		while [ $COUNTER -le $MODCNT ]; do
			newnet="$newnet wan$COUNTER"
			let COUNTER=COUNTER+1
		done
		uci_set firewall "$config" network "$newnet"
		uci_commit firewall
		/etc/init.d/firewall restart
	fi
}

if [ -e /tmp/installing ]; then
	exit 0
fi

log " Initializing Rooter"

sed -i -e 's|/etc/savevar|#removed line|g' /etc/rc.local

[ -f "/etc/firstboot" ] || {
	firstboot
}

uci delete modem.Version
uci set modem.Version=version
uci set modem.Version.ver=$CODENAME
uci commit modem

source /etc/openwrt_release
rm -f /etc/openwrt_release
DISTRIB_DESCRIPTION=$(uci get modem.Version.ver)" ( OpenWRT "$DISTRIB_REVISION" )"
echo 'DISTRIB_ID="'"$DISTRIB_ID"'"' >> /etc/openwrt_release
echo 'DISTRIB_RELEASE="'"$DISTRIB_RELEASE"'"' >> /etc/openwrt_release
echo 'DISTRIB_REVISION="'"$DISTRIB_REVISION"'"' >> /etc/openwrt_release
echo 'DISTRIB_CODENAME="'"$DISTRIB_CODENAME"'"' >> /etc/openwrt_release
echo 'DISTRIB_TARGET="'"$DISTRIB_TARGET"'"' >> /etc/openwrt_release
echo 'DISTRIB_DESCRIPTION="'"$DISTRIB_DESCRIPTION"'"' >> /etc/openwrt_release

if `cat /proc/version | grep "3.10" 1>/dev/null 2>&1`
then
	echo "1" > /etc/kernel310
fi

MODSTART=1
WWAN=0
USBN=0
ETHN=1
BASEPORT=0
WDMN=0
if 
	ifconfig eth1
then
	ETHN=2
fi
echo 'MODSTART="'"$MODSTART"'"' > /tmp/variable.file
echo 'WWAN="'"$WWAN"'"' >> /tmp/variable.file
echo 'USBN="'"$USBN"'"' >> /tmp/variable.file
echo 'ETHN="'"$ETHN"'"' >> /tmp/variable.file
echo 'WDMN="'"$WDMN"'"' >> /tmp/variable.file
echo 'BASEPORT="'"$BASEPORT"'"' >> /tmp/variable.file

MODCNT=$MAX_MODEMS
echo 'MODCNTX="'"$MODCNT"'"' > /tmp/modcnt
uci set modem.general.max=$MODCNT
uci set modem.general.modemnum=1
uci set modem.general.smsnum=1
uci set modem.general.miscnum=1

COUNTER=1
while [ $COUNTER -le $MODCNT ]; do
	uci delete modem.modem$COUNTER        
	uci set modem.modem$COUNTER=modem  
	uci set modem.modem$COUNTER.empty=1

	INEX=$(uci get modem.modeminfo$COUNTER)
	if [ -z $INEX ]; then
		uci set modem.modeminfo$COUNTER=minfo$COUNTER
	fi

	rm -f $ROOTER_LINK/getsignal$COUNTER
	rm -f $ROOTER_LINK/reconnect$COUNTER
	rm -f $ROOTER_LINK/create_proto$COUNTER
	uci delete network.wan$COUNTER
	$ROOTER/signal/status.sh $COUNTER "No Modem Present"
	let COUNTER=COUNTER+1 
done
uci commit modem
uci commit network

config_load firewall
config_foreach do_zone zone

$ROOTER/gpiomodel.lua

$ROOTER/conn_monitor.sh &

echo "0" > /etc/firstboot
echo "0" > /tmp/bootend.file


