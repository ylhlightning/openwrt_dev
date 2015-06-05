#Owned by DairyMan@Whirlpool
#
#Copyright GNU act.
include $(TOPDIR)/rules.mk

PKG_NAME:=ext-openvpn
PKG_VERSION:=4.500
PKG_RELEASE:=1

PKG_MAINTAINER:=Created by DM/makefile by Cobia@whirlpool
include $(INCLUDE_DIR)/package.mk

define Package/ext-openvpn
  SECTION:=utils
  CATEGORY:=Network
  DEPENDS:=+openvpn-easy-rsa +openvpn-openssl
  TITLE:=support for openvpn
  PKGARCH:=all
endef

define Package/ext-openvpn/description
  Helper scripts to enable openvpn
endef


define Build/Compile
endef

define Package/ext-openvpn/install
	$(CP) ./files/* $(1)/


endef

$(eval $(call BuildPackage,ext-openvpn))
