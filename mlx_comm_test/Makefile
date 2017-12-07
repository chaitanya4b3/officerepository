#
# Copyright (C) 2010-2013 wangwenjian@zhilutec.com
#
# zhilu Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mlx_comm_test
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/mlx_comm_test

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib
TARGET_LDFLAGS += -lgen -luci -lzl-serial -lmlx_comm

define Package/mlx_comm_test
	SECTION:=Zhilu_Properties
	CATEGORY:=Zhilu_Properties
	DEPENDS:= +libgen +libuci +libzl-serial +libmlx_comm
	TITLE:=mlx_comm_test
  SUBMENU:=Tests  
endef

define Package/mlx_comm_test/description
	A MLX project communication interface test
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

define Package/mlx_comm_test/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mlx_comm_test $(1)/sbin/
endef

$(eval $(call BuildPackage,mlx_comm_test))
