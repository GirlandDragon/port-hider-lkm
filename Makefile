LKM_DIR := lkm

.PHONY: all build clean install uninstall help

all: build

build:
	$(MAKE) -C $(LKM_DIR)

clean:
	$(MAKE) -C $(LKM_DIR) clean

install:
	$(MAKE) -C $(LKM_DIR) install

uninstall:
	-su -c "rmmod port_hider"

help:
	@echo "Port Hider LKM Build System"
	@echo ""
	@echo "Using DDK:"
	@echo "  source envsetup.sh    # set KDIR for your target kernel"
	@echo "  make                  # build the module"
	@echo ""
	@echo "Using NDK (manual):"
	@echo "  export KDIR=/path/to/kernel-headers"
	@echo "  export ARCH=arm64"
	@echo "  export CROSS_COMPILE=aarch64-linux-android-"
	@echo "  make"
	@echo ""
	@echo "Loading:"
	@echo "  su -c 'insmod lkm/port_hider.ko ports=8765,8788 whitelist_uids=0,1000,2000'"
	@echo ""
	@echo "Or use the helper script:"
	@echo "  bash scripts/load.sh insmod"
	@echo "  bash scripts/load.sh rmmod"
