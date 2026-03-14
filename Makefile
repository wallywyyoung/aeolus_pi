BR_DIR = buildroot
BR_EXT = $(PWD)/buildroot-ext
DEFCONFIG = raspberrypi4_64_defconfig
DEBUG_FRAGMENT = $(PWD)/buildroot-ext/debug.config

.PHONY: all setup software buildroot clean

all: setup software buildroot

setup:
	./init.sh

software:
	@echo "Building aeolus_pi..."
	cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
	cmake --build build

buildroot:
	@echo "Building Buildroot OS..."
	$(MAKE) -C $(BR_DIR) BR2_EXTERNAL=$(BR_EXT) $(DEFCONFIG)
ifdef DEBUG
	@echo "Applying debug config fragment..."
	$(BR_DIR)/support/kconfig/merge_config.sh -m $(BR_DIR)/.config $(DEBUG_FRAGMENT)
	$(MAKE) -C $(BR_DIR) olddefconfig
endif
	$(MAKE) -C $(BR_DIR)

clean:
	rm -rf build
	$(MAKE) -C $(BR_DIR) clean
