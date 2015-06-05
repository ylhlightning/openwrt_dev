#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-transmission
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-transmission
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+transmission-daemon +transmission-web
  TITLE:=support for transmission
  PKGARCH:=all
endef

define Package/ext-transmission/description
  Helper scripts to enable transmission
endef


define Build/Compile
endef

define Package/ext-transmission/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-transmission))
