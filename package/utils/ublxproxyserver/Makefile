#
# Copyright (C) 2010-2015 Jo-Philipp Wich <jow@openwrt.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ublxproxyserver

PKG_RELEASE:=1

PKG_USE_MIPS16:=0


include $(INCLUDE_DIR)/package.mk

define Package/ublxproxyserver
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=ublxproxyserver
  MAINTAINER:=linhu
endef

define Package/ublxproxyserver/description
  Ublxserver which serves to proxy the applicant message to the handler process.
endef

TARGET_CFLAGS += -Iinc/

SOURCES = \
         src/UblxClient.c \
         src/UblxServer.c \
         src/Error.c \
         src/Main.c \
         src/PollReactor.c \
         src/TcpServer.c

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/ublxproxyserver $(SOURCES)
endef

define Package/ublxproxyserver/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ublxproxyserver $(1)/usr/sbin/ublxproxyserver
endef

$(eval $(call BuildPackage,ublxproxyserver))

