#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-samba
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-samba
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+luci-app-samba +ntfs-3g +fdisk +kmod-usb-storage +luci-app-hd-idle \
	    +kmod-fs-ext4 +kmod-fs-vfat +kmod-nls-cp437 +kmod-nls-iso8859-1 +kmod-nls-utf8 \
	    +block-mount
  TITLE:=Install ext-samba
  PKGARCH:=all
endef

define Package/ext-samba/description
  Helper scripts to install ext-samba
endef


define Build/Compile
endef

define Package/ext-samba/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-samba))
