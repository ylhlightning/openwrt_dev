include $(TOPDIR)/rules.mk

PKG_NAME:=rmbim
PKG_VERSION:=2014-12-11
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/ROOterDairyman/rmbim.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=93b7bd14e969b2c7fd4137ab1f946508537cb106
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MAINTAINER:=dairyman

PKG_LICENSE:=GPLv2
PKG_LICENSE_FILES:=

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/rmbim
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libubox +kmod-usb-net +kmod-usb-net-cdc-mbim
  TITLE:=Control utility for mobile broadband modems
endef

define Package/rmbim/description
  rmbim is a command line tool for controlling mobile broadband modems using
  the MBIM-protocol.
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include -ffunction-sections -fdata-sections

TARGET_LDFLAGS += -Wl,--gc-sections

define Package/rmbim/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/umbim $(1)/sbin/
endef

$(eval $(call BuildPackage,rmbim))
