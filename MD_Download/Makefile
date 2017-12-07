#
# Copyright (C) 2010-2013 wangwenjian@zhilutec.com
#
# zhilu Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=MD_Download
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/MD_Download

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib
TARGET_LDFLAGS += -lpthread -lzl_share_api -luci -lmlx_comm -lgen -lzl-serial -lgen -luci -lsqlite3

define Package/MD_Download
	SECTION:=Zhilu_Properties
	CATEGORY:=Zhilu_Properties
	DEPENDS:= +libpthread +libzl_share_api +libuci +libmlx_comm +libzl-serial +libgen +libuci +libsqlite3
	TITLE:=MD_Download
  SUBMENU:=Tests  
endef

define Package/MD_Download/description
	A Md download
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

define Package/MD_Download/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/MD_Download $(1)/sbin/
endef

$(eval $(call BuildPackage,MD_Download))
