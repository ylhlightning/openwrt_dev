#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-aria2
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-aria2
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+aria2
  TITLE:=support for aria2
  PKGARCH:=all
endef

define Package/ext-aria2/description
  Helper scripts to enable aria2
endef


define Build/Compile
endef

define Package/ext-aria2/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-aria2))
