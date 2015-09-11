#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-mjpg-streamer
PKG_VERSION:=1.000
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-mjpg-streamer
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+mjpg-streamer +kmod-video-uvc
  TITLE:=support for mjpg-streamer
  PKGARCH:=all
endef

define Package/ext-mjpg-streamer/description
  Helper scripts to enable mjpg-streamer
endef


define Build/Compile
endef

define Package/ext-mjpg-streamer/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,ext-mjpg-streamer))
