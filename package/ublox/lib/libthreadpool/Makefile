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
# $Workfile :: Makefile (libthreadpool)                    
# $Modtime  ::                                            
# $Revision ::                                           
# $Author   :: Linhu Ying                                  
# $Log      :: 01.00 initial UTT - U_M39G_MV-120          
###########################################################

THIS_COMPONENT_NAME = libthreadpool

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

PKG_NAME:=$(THIS_COMPONENT_NAME)
PKG_RELEASE:=1
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

#source files
COMPONENT_SOURCEFILES += src/thpool.c

# Include directories
COMPONENT_INCLUDEDIRS += inc

# Definitions 
COMPONENT_DEFINITIONS += \


# Library dependency
COMPONENT_LIBRARIES :=-lrt -lpthread

# Package configuration
define  Package/$(PKG_NAME)
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=$(PKG_NAME)
	MAINTAINER:=Linhu Ying
endef

define Package/$(PKG_NAME)/description
  thread pool library.
endef

########################################################
########           component processing ################
########           DO NOT MODIFY FROM HERE ########################
########################################################

COMPONENT_OBJ :=  $(patsubst %.c,%.o,$(COMPONENT_SOURCEFILES)) 

#process include directories
TARGET_CFLAGS += $(sort $(strip $(addprefix -I,$(COMPONENT_INCLUDEDIRS)  ) ) )
#process compiling switches / definitions
TARGET_CFLAGS += $(sort $(strip $(addprefix -D,$(COMPONENT_DEFINITIONS)  ) ) )
#process library dependency.
TARGET_LDFLAGS += $(PROJECT_LIBRARIES)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(TARGET_CC) $(TARGET_CFLAGS) -c $(COMPONENT_SOURCEFILES) -o $(COMPONENT_OBJ) $(TARGET_LDFLAGS)
endef

define Build/Compile
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
