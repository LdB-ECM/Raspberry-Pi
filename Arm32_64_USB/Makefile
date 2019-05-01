# If cross compiling from windows use native GNU-Make 4.2.1
# https://sourceforge.net/projects/ezwinports/files/
# download "make-4.2.1-without-guile-w32-bin.zip" and set it on the enviroment path
# There is no need to install cygwin or any of that sort of rubbish

ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q
	SLASH = \\
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
endif 


Pi3-64: CFLAGS = -Wall -O3 -mcpu=cortex-a53+fp+simd -ffreestanding -nostartfiles -std=c11 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi3-64: ARMGNU = D:/gcc-linaro-7.4.1-2019.02-i686-mingw32_aarch64-elf/bin/aarch64-elf
Pi3-64: LINKERFILE = rpi64.ld
Pi3-64: SMARTSTART = SmartStart64.S
Pi3-64: IMGFILE = kernel8.img

Pi3: CFLAGS = -Wall -O3 -mcpu=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard -ffreestanding -nostartfiles -std=c11 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi3: ARMGNU = D:/gcc_pi_7_2/bin/arm-none-eabi
Pi3: LINKERFILE = rpi32.ld
Pi3: SMARTSTART = SmartStart32.S
Pi3: IMGFILE = kernel8-32.img

Pi2: CFLAGS = -Wall -O3 -mcpu=cortex-a7 -mfpu=neon -mfloat-abi=hard -ffreestanding -nostartfiles -std=c11 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi2: ARMGNU = D:/gcc_pi_7_2/bin/arm-none-eabi
Pi2: LINKERFILE = rpi32.ld
Pi2: SMARTSTART = SmartStart32.S
Pi2: IMGFILE = kernel7.img

Pi1: CFLAGS = -Wall -O3 -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -ffreestanding -nostartfiles -std=c11 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi1: ARMGNU = D:/gcc_pi_7_2/bin/arm-none-eabi
Pi1: LINKERFILE = rpi32.ld
Pi1: SMARTSTART = SmartStart32.S
Pi1: IMGFILE = kernel.img

# The directory in which source files are stored.
SOURCE = ${CURDIR}
BUILD = Build


# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
ASMOBJS = $(patsubst $(SOURCE)/%.S,$(BUILD)/%.o,$(SMARTSTART))
COBJS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))


Pi3-64: kernel.elf
BINARY = $(IMGFILE)
.PHONY: Pi3-64

Pi3: kernel.elf
BINARY = $(IMGFILE)
.PHONY: Pi3

Pi2: kernel.elf
BINARY = $(IMGFILE)
.PHONY: Pi2

Pi1: kernel.elf
BINARY = $(IMGFILE)
.PHONY: Pi1

all: Pi3-64 Pi3 Pi2 Pi1

$(BUILD)/%.o: $(SOURCE)/%.s
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)/%.o: $(SOURCE)/%.S
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)/%.o: $(SOURCE)/%.c
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

kernel.elf: $(ASMOBJS) $(COBJS) 
	$(ARMGNU)-gcc $(CFLAGS) $(ASMOBJS) $(COBJS) -T $(LINKERFILE) -o kernel.elf -lc -lm -lgcc
	$(ARMGNU)-objdump -d kernel.elf > $(LIST)
	$(ARMGNU)-objcopy kernel.elf -O binary DiskImg/$(BINARY)
	$(ARMGNU)-nm -n kernel.elf > $(MAP)

# Control silent mode  .... we want silent in clean
.SILENT: clean

# cleanup temp files
clean:
	$(RM) $(MAP) 
	$(RM) kernel.elf 
	$(RM) $(LIST) 
	$(RM) $(BUILD)$(SLASH)*.o 
	$(RM) $(BUILD)$(SLASH)*.d 
	echo CLEAN COMPLETED
.PHONY: clean

