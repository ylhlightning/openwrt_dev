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
# $Workfile :: Makefile (lib320u)                    
# $Modtime  ::                                            
# $Revision ::                                           
# $Author   :: Linhu Ying                                  
# $Log      :: 01.00 initial UTT - U_M39G_MV-120          
###########################################################

THIS_COMPONENT_NAME = lib320u

# -----------------------------------------------------------------------------
# Global makerules
# -----------------------------------------------------------------------------
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

# -----------------------------------------------------------------------------
# UCOM specific makeoptions
# -----------------------------------------------------------------------------

#init of local variables
COMPONENT_SOURCEFILES =
COMPONENT_INCLUDEDIRS =
COMPONENT_DEFINITIONS =
COMPONENT_CFLAGS =
COMPONENT_LDFLAGS =

PKG_NAME:=$(THIS_COMPONENT_NAME)
PKG_RELEASE:=1
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

#source files
COMPONENT_SOURCEFILES += src/lib320u.c

# Include directories
COMPONENT_INCLUDEDIRS += inc

# Definitions 
COMPONENT_DEFINITIONS +=


# Library dependency
COMPONENT_LIBRARIES :=-lrt

# Package configuration
define  Package/$(PKG_NAME)
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=$(PKG_NAME)
	MAINTAINER:=Linhu Ying
endef

define Package/$(PKG_NAME)/description
 This package contains sierra wireless Aircard 320u API
endef

########################################################
########           component processing ################
########           DO NOT MODIFY FROM HERE ########################
########################################################

COMPONENT_OBJ :=  $(patsubst %.c,%.o,$(COMPONENT_SOURCEFILES)) 

#process include directories
COMPONENT_CFLAGS += $(sort $(strip $(addprefix -I,$(COMPONENT_INCLUDEDIRS)  ) ) )
#process compiling switches / definitions
COMPONENT_CFLAGS += $(sort $(strip $(addprefix -D,$(COMPONENT_DEFINITIONS)  ) ) )

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r ./* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(COMPONENT_CFLAGS) -c $(COMPONENT_SOURCEFILES) -o $(COMPONENT_OBJ) $(COMPONENT_LDFLAGS)
	$(TARGET_AR) cru $(PKG_BUILD_DIR)/$(PKG_NAME).a $(COMPONENT_OBJ)
	$(TARGET_RANLIB) $(PKG_BUILD_DIR)/$(PKG_NAME).a
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/$(PKG_NAME).a $(1)/usr/lib
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/$(PKG_NAME).a $(1)/usr/lib
endef

$(eval $(call BuildPackage,$(PKG_NAME)))



