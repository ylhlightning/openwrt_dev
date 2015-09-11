#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-rooter8
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-rooter8
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+luci-app-qos +luci-app-ddns +luci-proto-relayd +luci-app-wshaper
  TITLE:=support for rooter8
  PKGARCH:=all
endef

define Package/ext-rooter8/description
  Helper scripts to enable rooter8
endef


define Build/Compile
endef

define Package/ext-rooter8/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-rooter8))
