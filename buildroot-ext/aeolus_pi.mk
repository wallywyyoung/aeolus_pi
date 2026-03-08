AEOLUS_APP_VERSION = 1.0
AEOLUS_APP_SITE = $(BR2_EXTERNAL_AEOLUS_PATH)/..
AEOLUS_APP_SITE_METHOD = local
AEOLUS_APP_CONF_OPTS = \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$(HOME)/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(HOST_DIR)/share/buildroot/toolchainfile.cmake \
    -DVCPKG_TARGET_TRIPLET=arm64-linux \
    -GNinja
AEOLUS_APP_CONF_ENV = CMAKE_GENERATOR=Ninja
$(eval $(cmake-package))
