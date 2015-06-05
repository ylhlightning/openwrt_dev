#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-nodogsplash
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-nodogsplash
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+nodogsplash
  TITLE:=support for nodogsplash
  PKGARCH:=all
endef

define Package/ext-nodogsplash/description
  Helper scripts to enable nodogsplash
endef


define Build/Compile
endef

define Package/ext-nodogsplash/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,ext-nodogsplash))
