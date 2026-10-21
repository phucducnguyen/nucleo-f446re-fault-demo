# Bench test firmware, NUCLEO-F446RE. Bare metal, no vendor library.
TOOLCHAIN  ?= arm-none-eabi-
FW_VERSION ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo dev)

CC      = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE    = $(TOOLCHAIN)size

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS)) build/version.o

# `make SIM=1` (QEMU only): loopback reports OK without touching pins. Only the command line counts,
# never an environment variable, so the bench's plain `make` can not pick it up by accident.
SIM = 0
ifeq ($(origin SIM),command line)
ifeq ($(SIM),1)
SIMFLAG = -DSIM
endif
endif
CFLAGS  = $(SIMFLAG) -ffile-prefix-map=$(CURDIR)=. -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -g -std=c11 -Wall -Wextra \
          -ffreestanding -ffunction-sections -fdata-sections -fno-common \
          -fno-tree-loop-distribute-patterns -Isrc
LDFLAGS = -T link.ld -nostartfiles -nostdlib -Wl,--gc-sections -Wl,-Map=build/fw.map

all: build/fw.elf build/fw.bin

build/fw.elf: $(OBJS) link.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

build/fw.bin: build/fw.elf
	$(OBJCOPY) -O binary $< $@

build/%.o: src/%.c $(wildcard src/*.h) Makefile build/buildcfg | build
	$(CC) $(CFLAGS) -c $< -o $@

# Rewritten only when FW_VERSION changes, so a new commit rebuilds one object.
build/version.c: FORCE | build
	@printf 'const char fw_version[] = "%s";\n' '$(FW_VERSION)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@
build/version.o: build/version.c
	$(CC) $(CFLAGS) -c $< -o $@

# Records SIM so switching it rebuilds everything (a SIM object must never reach the board).
build/buildcfg: FORCE | build
	@echo 'SIM=$(if $(SIMFLAG),1,0)' > $@.tmp; cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

build:
	mkdir -p build

clean:
	rm -rf build

FORCE:
.PHONY: all clean FORCE
