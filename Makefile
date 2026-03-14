BR_DIR = buildroot
BR_EXT = $(PWD)/buildroot-ext
DEFCONFIG = $(BR_EXT)/raspberrypi4_64_defconfig
DEBUG_FRAGMENT = $(PWD)/buildroot-ext/debug/debug.config

.PHONY: all setup software config buildroot clean

all: setup software buildroot

setup:
	./init.sh

software:
	@echo "Building aeolus_pi..."
	cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
	cmake --build build

config:
	$(MAKE) -C $(BR_DIR) BR2_EXTERNAL=$(BR_EXT) BR2_DEFCONFIG=$(DEFCONFIG) defconfig
	@if [ -n "$(DEBUG)" ]; then \
		echo "Applying debug config fragment..."; \
		KCONFIG_CONFIG=$(BR_DIR)/.config $(BR_DIR)/support/kconfig/merge_config.sh $(BR_DIR)/.config $(DEBUG_FRAGMENT); \
		$(MAKE) -C $(BR_DIR) olddefconfig; \
	fi

buildroot: config
	@echo "Building Buildroot OS..."
	@if [ -n "$(DEBUG)" ]; then \
		mkdir -p $(BR_EXT)/overlay/etc/init.d; \
		cp $(BR_EXT)/debug/S01bootlog $(BR_EXT)/overlay/etc/init.d/S01bootlog; \
		chmod +x $(BR_EXT)/overlay/etc/init.d/S01bootlog; \
	fi
	$(MAKE) -C $(BR_DIR)

clean:
	rm -rf build
	rm -f $(BR_EXT)/overlay/etc/init.d/S01bootlog
	$(MAKE) -C $(BR_DIR) clean