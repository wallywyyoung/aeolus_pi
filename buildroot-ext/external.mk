AEOLUS_PI_VERSION = 1.0
AEOLUS_PI_SITE = $(BR2_EXTERNAL_AEOLUS_PATH)/..
AEOLUS_PI_SITE_METHOD = local
AEOLUS_PI_CONF_OPTS = \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(HOST_DIR)/share/buildroot/toolchainfile.cmake \
    -DVCPKG_TARGET_TRIPLET=arm64-linux \
    -GNinja

$(eval $(cmake-package))
