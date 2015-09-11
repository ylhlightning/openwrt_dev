#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-vsftpd
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-vsftpd
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+vsftpd
  TITLE:=Install ext-vsftpd
  PKGARCH:=all
endef

define Package/ext-vsftpd/description
  Helper scripts to install ext-vsftpd
endef


define Build/Compile
endef

define Package/ext-vsftpd/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-vsftpd))
