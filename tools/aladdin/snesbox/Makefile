# snesbox: the original ROM run headless on the pinned core, driven by a script.
TOP := ../../..
CORE_SRC := $(TOP)/tools/toolchains/src/bsnes/bsnes
OUT := $(TOP)/build/aladdin/box/snesbox
INC := $(TOP)/src/aladdin/include

$(OUT): snesbox.c check.c check.h gsunone.c gsucall.h spccall.c spccall.h $(INC)/machine.h $(INC)/spc.h
	@mkdir -p $(dir $(OUT))
	cc -O2 -Wall -I$(CORE_SRC)/target-libretro -I$(INC) -Wl,-export_dynamic \
		-o $(OUT) snesbox.c check.c gsunone.c spccall.c -ldl
