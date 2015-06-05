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
#                                                         #
#  $Archive ::                                            # 
#                                                         #
# $Workfile :: Makefile (ublxatcmdtest)                   # 
# $Modtime  ::                                            # 
# $Revision ::                                            # 
# $Author   :: Linhu Ying                                 # 
# $Log      ::                               # 
###########################################################

THIS_PROJECT_NAME =ublxatcmdtest

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

PKG_NAME:=$(THIS_PROJECT_NAME )
PKG_RELEASE:=1
PKG_USE_MIPS16:=0

# Source files
PROJECT_SOURCEFILES += src/ublx_at_cmd_test.c

# Include directories
PROJECT_INCLUDEDIRS += $(TOPDIR)/package/ublox/lib/lib320u/inc

# Library dependency
PROJECT_LIBRARIES :=-l320u

# CFLAGS 
PROJECT_DEFINITIONS += \

# Package configuration
define Package/$(THIS_PROJECT_NAME)/Default
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=$(THIS_PROJECT_NAME)
  MAINTAINER:=lyin
endef

# Package description
define Package/$(THIS_PROJECT_NAME)/description
Application to test AT command of Sierra Wireless Aircard 320u
endef

# Package dependency
define Package/$(THIS_PROJECT_NAME)
$(call Package/$(THIS_PROJECT_NAME)/Default)
  TITLE += (library dependency)
  DEPENDS:=+lib320u +librt
endef

########################################################
########      project  processing      #################
########    DO NOT MODIFY FROM HERE    #################
########################################################

#process include directories
TARGET_CFLAGS += $(sort $(strip $(addprefix -I,$(PROJECT_INCLUDEDIRS)  ) ) )
#process compiling switches / definitions
TARGET_CFLAGS += $(sort $(strip $(addprefix -D,$(PROJECT_DEFINITIONS)  ) ) )
#process library dependency.
TARGET_LDFLAGS += $(PROJECT_LIBRARIES)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/$(THIS_PROJECT_NAME) $(PROJECT_SOURCEFILES) -lrt $(TARGET_LDFLAGS)
endef

define Package/$(THIS_PROJECT_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(THIS_PROJECT_NAME) $(1)/usr/sbin/$(THIS_PROJECT_NAME)
endef

$(eval $(call BuildPackage,$(THIS_PROJECT_NAME)))

