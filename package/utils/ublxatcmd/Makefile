#
# Copyright (C) 2010-2015 Jo-Philipp Wich <jow@openwrt.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ublxatcmd

PKG_RELEASE:=1

PKG_USE_MIPS16:=0


include $(INCLUDE_DIR)/package.mk

define Package/ublxatcmd/Default
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=ublxatcmd
  MAINTAINER:=linhu
endef

define Package/ublxatcmd/description
 Px5g is a tiny standalone X.509 certificate generator.
 It suitable to create key files and certificates in DER
 and PEM format for use with stunnel, uhttpd and others.
endef

define Package/ublxatcmd
$(call Package/ublxatcmd/Default)
  TITLE += (library dependency)
  DEPENDS:=+librt
endef

TARGET_LDFLAGS += -lrt

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/ublxatcmd ublxatcmd.c 
endef

define Package/ublxatcmd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ublxatcmd $(1)/usr/sbin/ublxatcmd
endef

$(eval $(call BuildPackage,ublxatcmd))
