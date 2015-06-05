#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-rtorrent
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-rtorrent
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+rtorrent-rpc +screen
  TITLE:=support for rtorrent
  PKGARCH:=all
endef

define Package/ext-mjpg-rtorrent/description
  Helper scripts to enable rtorrent
endef


define Build/Compile
endef

define Package/ext-rtorrent/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,ext-rtorrent))
