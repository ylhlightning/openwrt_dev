
#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=lib320u
PKG_RELEASE:=1
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1


include $(INCLUDE_DIR)/package.mk

define  Package/lib320u
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=lib320u
    MAINTAINER:=linhu
endef

define Package/lib320u/description
 This package contains sierra wireless Aircard 320u API
endef

TARGET_CFLAGS += -Iinc

PROJECT_CODE = src/lib320u.c
OBJ = src/lib320u.o


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(TARGET_CC) $(TARGET_CFLAGS) -c $(PROJECT_CODE) -o $(OBJ) -lrt
endef

define Build/Compile
    $(TARGET_AR) cru $(PKG_BUILD_DIR)/$(PKG_NAME).a $(OBJ)
	$(TARGET_RANLIB) $(PKG_BUILD_DIR)/$(PKG_NAME).a
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/$(PKG_NAME).a $(1)/usr/lib/
endef

define Package/lib320u/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/$(PKG_NAME).a $(1)/usr/lib
endef

$(eval $(call BuildPackage,lib320u))


