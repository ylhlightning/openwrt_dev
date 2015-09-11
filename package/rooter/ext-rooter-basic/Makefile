#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-rooter-basic
PKG_VERSION:=4.500
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-rooter-basic
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+kmod-usb-net +kmod-usb-net-cdc-ether +kmod-usb-net-huawei-cdc-ncm \
	    +kmod-usb-net-cdc-mbim +kmod-usb-net-qmi-wwan +kmod-usb-net-rndis \
	    +kmod-usb-net-sierrawireless +kmod-usb-ohci +kmod-usb-serial \
	    +kmod-usb-serial-option +kmod-usb-serial-sierrawireless \
	    +kmod-usb-uhci +kmod-usb2 +luci-proto-3g +luci \
	    +usb-modeswitch +wireless-tools +wpad-mini +rmbim
  TITLE:=ROOter support for usbmodems
  PKGARCH:=all
endef

define Package/ext-rooter-basic/description
  Helper scripts to enable ROOter to manage usb modem interfaces
endef


define Build/Compile
endef

define Package/ext-rooter-basic/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-rooter-basic))
