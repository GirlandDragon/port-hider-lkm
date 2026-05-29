port_hider-objs := core.o kallsyms.o helpers.o fdmap.o connect.o bind.o proc.o vfs.o
obj-m := port_hider.o

IRIS := $(dir $(lastword $(MAKEFILE_LIST)))../Iris

PORT_HIDER_VER := $(shell sed -n 's/^version=//p' $(IRIS) 2>/dev/null)
PORT_HIDER_AUTHOR := $(shell sed -n 's/^author=//p' $(IRIS) 2>/dev/null)

ifeq ($(PORT_HIDER_VER),)
$(error cannot read version from ../Iris)
endif
ifeq ($(PORT_HIDER_AUTHOR),)
$(error cannot read author from ../Iris)
endif

ccflags-y += -DPORT_HIDER_VER=\"$(PORT_HIDER_VER)\"
ccflags-y += -DPORT_HIDER_AUTHOR=\"$(PORT_HIDER_AUTHOR)\"
ccflags-y += -Wno-declaration-after-statement
ccflags-y += -Wno-unused-variable
ccflags-y += -Wno-unused-function
ccflags-y += -Wno-unused-result
ccflags-y += -Wno-int-conversion
ccflags-y += -Wno-builtin-macro-redefined -U__FILE__ -D__FILE__='""'

all:
	make -C $(KDIR) M=$(CURDIR) modules

clean:
	make -C $(KDIR) M=$(CURDIR) clean

install:
	make -C $(KDIR) M=$(CURDIR) modules_install

.PHONY: all clean install
