include $(TOPDIR)/rules.mk

PKG_NAME:=rqmi
PKG_VERSION:=2014-12-03
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://nbd.name/uqmi.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=86bcdb8cca652676a78b2df8b5e3fb27a40c60a4
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_MAINTAINER:=Matti Laakso <malaakso@elisanet.fi>
# PKG_MIRROR_MD5SUM:=
# CMAKE_INSTALL:=1

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/rqmi
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libubox +libblobmsg-json +kmod-usb-net +kmod-usb-net-qmi-wwan
  TITLE:=Control utility for mobile broadband modems
endef

define Package/rqmi/description
  uqmi is a command line tool for controlling mobile broadband modems using
  the QMI-protocol.
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include -ffunction-sections -fdata-sections

TARGET_LDFLAGS += -Wl,--gc-sections

CMAKE_OPTIONS += \
	-DDEBUG=1

define Package/rqmi/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uqmi $(1)/sbin/
endef

$(eval $(call BuildPackage,rqmi))
