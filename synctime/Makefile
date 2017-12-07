#
# Copyright (C) 2010-2013 wangwenjian@zhilutec.com
#
# zhilu Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=synctime
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/synctime

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib
TARGET_LDFLAGS += -lgen -luci -lmlx_rtc -lmlx_comm -lzl-serial

define Package/synctime
	SECTION:=Zhilu_Properties
	CATEGORY:=Zhilu_Properties
	DEPENDS:= +libgen +libuci +libmlx_rtc +libmlx_comm +libzl-serial
	TITLE:=synctime
  SUBMENU:=Tests  
endef

define Package/synctime/description
	A rtc get set test
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)" all
endef

define Package/synctime/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/synctime $(1)/sbin/
endef

$(eval $(call BuildPackage,synctime))
