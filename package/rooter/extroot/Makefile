#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=extroot
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/extroot
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+kmod-usb-storage +luci \
	    +kmod-fs-ext4 +kmod-fs-vfat +kmod-nls-utf8 \
	    +block-mount +kmod-nls-cp437 +kmod-nls-iso8859-1
  TITLE:=Install ext-root
  PKGARCH:=all
endef

define Package/extroot/description
  Helper scripts to install ext-root
endef


define Build/Compile
endef

define Package/extroot/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,extroot))
