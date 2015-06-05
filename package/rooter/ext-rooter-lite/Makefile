#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-rooter-lite
PKG_VERSION:=4.500
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-rooter-lite
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+luci +ext-rooter-basic
  TITLE:=ROOter support for usbmodems
  PKGARCH:=all
endef

define Package/ext-rooter-lite/description
  Helper scripts to enable ROOter to manage usb modem interfaces
endef


define Build/Compile
endef


$(eval $(call BuildPackage,ext-rooter-lite))
