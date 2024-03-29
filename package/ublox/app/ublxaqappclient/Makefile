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
# $Workfile :: Makefile (ublxaqappclient)
# $Modtime  ::                                           
# $Revision ::                                           
# $Author   :: Linhu Ying                                
# $Log      ::                               
###########################################################

THIS_PROJECT_NAME =ublxaqappclient

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
PROJECT_SOURCEFILES += \
                       src/main.c \
                       src/ublx_client.c \
                       src/ublx_client_handler.c \
                       src/ublx_server.c \
                       src/error.c \
                       src/poll_reactor.c \
                       src/tcp_server.c

# Include directories
PROJECT_INCLUDEDIRS += $(TOPDIR)/package/ublox/lib/lib320u/inc\
                       $(TOPDIR)/package/ublox/lib/libthreadpool/inc\
                       inc

# Library dependency
PROJECT_LIBRARIES :=-lubus -lubox -lblobmsg_json -lpthread -lrt

PROJECT_LIBRARIES +=-L$(BUILD_DIR)/lib320u -l320u

PROJECT_LIBRARIES +=-L$(BUILD_DIR)/libthreadpool -lthreadpool

# CFLAGS 
#PROJECT_DEFINITIONS +=\
PROJECT_DEFINITIONS +=UBLX_USE_THREAD_POOL

# Package configuration
define Package/$(PKG_NAME)/Default
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=$(PKG_NAME)
  MAINTAINER:=lyin
endef

# Package description
define Package/$(PKG_NAME)/description
ublx ubus client application
endef

# Package dependency
define Package/$(PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE += (library dependency)
  DEPENDS:=+lib320u +libthreadpool +librt +libpthread +libubus +libubox +libblobmsg-json
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

