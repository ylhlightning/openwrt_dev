########################################################### 
# 
# Copyright (C) u-blox Italia 
# 
# u-blox Italia 
# Via Stazione di Prosecco 15 
# 34010 Sgonico - TRIESTE, ITALY 
# 
# All rights reserved. 
#
# This source file is the sole property of 
# u-blox Italia. Reproduction or utilization of 
# this source in whole or part is forbidden 
# without the written consent of u-blox Italia. 
# 
########################################################### 
# 
# Project: M39G_MV 
# Purpose: Building system 
# 
########################################################### 
#                                                                                          
#  $Archive ::                                        
#                                                              
# $Workfile :: Makefile (ublxatcmdtest)                   
# $Modtime  ::                                           
# $Revision ::                                           
# $Author   :: Linhu Ying                                
# $Log      ::                               
###########################################################

THIS_PROJECT_NAME =ublx_ubus_client

# -----------------------------------------------------------------------------
# Global makerules
# -----------------------------------------------------------------------------

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

# -----------------------------------------------------------------------------
# Project specific makeoptions
# -----------------------------------------------------------------------------

# Init of local variables
PROJECT_SOURCEFILES =
PROJECT_INCLUDEDIRS =
PROJECT_DEFINITIONS =
PROJECT_LIBRARIES =
PROJECT_CFLAGS =
PROJECT_LDFLAGS =

PKG_NAME:=$(THIS_PROJECT_NAME)
PKG_RELEASE:=1
PKG_USE_MIPS16:=0

# Source files
PROJECT_SOURCEFILES += src/client.c

# Include directories
PROJECT_INCLUDEDIRS +=

# Library dependency
PROJECT_LIBRARIES :=-lrt -lubox -lubus -lblobmsg_json

# CFLAGS 
PROJECT_DEFINITIONS +=\

# Package configuration
define Package/$(PKG_NAME)/Default
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=$(PKG_NAME)
  MAINTAINER:=lyin
endef

# Package description
define Package/$(PKG_NAME)/description
ubus test client.
endef

# Package dependency
define Package/$(PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE += (library dependency)
  DEPENDS:=+librt +libubox +libubus +libblobmsg-json
endef

########################################################
########      project  processing      #################
########    DO NOT MODIFY FROM HERE    #################
########################################################

#process include directories
PROJECT_CFLAGS += $(sort $(strip $(addprefix -I,$(PROJECT_INCLUDEDIRS)  ) ) )
#process compiling switches / definitions
PROJECT_CFLAGS += $(sort $(strip $(addprefix -D,$(PROJECT_DEFINITIONS)  ) ) )
#process library dependency.
PROJECT_LDFLAGS += $(PROJECT_LIBRARIES)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(PROJECT_CFLAGS) -o $(PKG_BUILD_DIR)/$(PKG_NAME) $(PROJECT_SOURCEFILES) $(PROJECT_LDFLAGS)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
