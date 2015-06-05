#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-wifi
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-wifi
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+kmod-ath9k-htc +kmod-carl9170 +kmod-hostap-pci +kmod-hostap-plx +kmod-iwl3945 \
	 +kmod-iwl4965 +kmod-iwlagn +kmod-libertas-sd +kmod-ibertas-usb +kmod-mac80211-hwsim \
	 +kmod-mwifiex-pcie +kmod-mwl8k +kmod-net-ario +kmod-net-hermes-pci +kmod-net-hermes-plx \
 	 +kmod-net-ipw2100 +kmod-net-ipw2200 +kmod-nel-libipw +kmod-net-prism54 \
	 +kmod-net-zd1201 +kmod-p54-pci +kmod-p54-usb +kmod-rt2400-pci +kmod-rt2500-pci \
	 +kmod-rt2500-usb +kmod-rt2800-pci +kmod-rt2800-usb +kmod-rt61-pci +kmod-rtl8180 \
	 +kmod-rtl8187 +kmod-rtl8192ce +kmod-rtl8192cu +kmod-rtl8192de +kmod-rtl8192se
  TITLE:=support for wifi
  PKGARCH:=all
endef

define Package/ext-wifi/description
  Helper scripts to enable wifi
endef


define Build/Compile
endef


$(eval $(call BuildPackage,ext-wifi))
