# snesbox: the original ROM run headless on the pinned core, driven by a script.
TOP := ../../..
CORE_SRC := $(TOP)/tools/toolchains/src/bsnes/bsnes
OUT := $(TOP)/build/aladdin/box/snesbox
INC := $(TOP)/src/aladdin/include
# The core and the C library call back into the box: export its symbols.
ifeq ($(shell uname),Darwin)
EXPORT := -Wl,-export_dynamic
else
EXPORT := -rdynamic
# glibc declares dladdr and Dl_info only under _GNU_SOURCE.
CFLAGS_OS := -D_GNU_SOURCE
endif

$(OUT): snesbox.c check.c check.h gsunone.c gsucall.h spccall.c spccall.h $(INC)/machine.h $(INC)/spc.h
	@mkdir -p $(dir $(OUT))
	cc -O2 -Wall -I$(CORE_SRC)/target-libretro -I$(INC) $(CFLAGS_OS) $(EXPORT) \
		-o $(OUT) snesbox.c check.c gsunone.c spccall.c -ldl
