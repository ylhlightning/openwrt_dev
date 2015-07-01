#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=udp_capture
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/udp_capture
  SUBMENU:=Other modules
  TITLE:=UDP capture module
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/udp_capture.ko
  AUTOLOAD:=$(call AutoLoad,10,udp_capture)
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,udp_capture))

