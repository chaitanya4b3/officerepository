#
# Copyright (C) 2010-2013 donghaiming@zhilutec.com
#
# zhilu Property Software.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk


PKG_NAME:=mlx_fingerprint_test
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

#PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
#PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/mlx_fingerprint_test
  SECTION:=Zhilu_Properties
  CATEGORY:=Zhilu_Properties
  TITLE:=mlx_fingerprint_test -- mlx_fingerprint_test app for testing FPR module
  DEPENDS:= +libpthread +libstdcpp +librt +libgen +libuci +libzl-serial +libmlx_comm
  #+libsqlite3  
  SUBMENU:=Tests  
endef

define Package/mlx_fingerprint_test/description
	mlx_fingerprint_test for chipsailling
endef




define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./CSAlgDll.h $(STAGING_DIR)/usr/include/
	$(CP) ./libCSAlg.a $(STAGING_DIR)/usr/lib
endef

TARGET_CFLAGS+= $(FPIC)
TARGET_CFLAGS+=-I$(LINUX_DIR)/include/
TARGET_CFLAGS+=-I$(STAGING_DIR)/usr/include
TARGET_LDFLAGS += -L$(STAGING_DIR)/usr/lib 
TARGET_LDFLAGS += -lpthread -lrt -lCSAlg -lstdc++ -lgen -luci -lzl-serial -lmlx_comm

define Build/Compile
	$(MAKE)  $(TARGET_CONFIGURE_OPTS) CFLAGS_D=$(ARTCH) CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)" -C $(PKG_BUILD_DIR)
endef


define Package/mlx_fingerprint_test/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mlx_fingerprint_test $(1)/sbin/
	$(INSTALL_BIN) ./libCSAlg.a $(1)/usr/lib
endef


$(eval $(call BuildPackage,mlx_fingerprint_test))
