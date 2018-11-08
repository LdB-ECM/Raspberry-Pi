# If cross compiling from windows use native GNU-Make 3.81
# http://gnuwin32.sourceforge.net/packages/make.htm
# There is no need to install cygwin or any of that sort of rubbish

ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q /f
	SLASH = \\
	SWALLOW_OUTPUT = >nul 2>nul
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
	SWALLOW_OUTPUT =
endif 

# The toolchain path and prefix for 32bit compiler .. 64bit compiler will override
ARMGNU = D:/gcc_pi_7_2/bin/arm-none-eabi

# The name of the linker script to use for 32 bit compiler ...  64bit compiler will override
LINKERFILE = rpi32.ld

# The bootloader file defaut is 32bit ... 64bit compiler will override
SMARTSTART = SmartStart32.S

Pi3-64: CFLAGS = -Wall -O3 -march=armv8-a+simd -mtune=cortex-a53 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi3-64: IMGFILE = kernel8.img
Pi3-64: ARMGNU  = g:/pi/gcc_linaro_7_1/bin/aarch64-elf
Pi3-64: LINKERFILE = rpi64.ld
Pi3-64: SMARTSTART = SmartStart64.S

Pi3: CFLAGS = -Wall -O3 -march=armv8-a -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a53 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi3: IMGFILE = kernel8-32.img

Pi2: CFLAGS = -Wall -O3 -mfpu=neon -mfloat-abi=hard -march=armv7ve -mtune=cortex-a7 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi2: IMGFILE = kernel7.img

Pi1: CFLAGS = -Wall -O3 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi1: IMGFILE = kernel.img

LDFLAGS = -nostdlib -nodefaultlibs -nostartfiles -ffreestanding -fno-asynchronous-unwind-tables -fomit-frame-pointer

LIBFLAGS=-lc -lm -lgcc

LINKERFLAGS=-Wl,-gc-sections -Wl,--build-id=none -Wl,-Bdynamic

# The directory in which source files are stored.
SOURCE = ${CURDIR}

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
C_FILES := $(filter-out main.c, $(wildcard *.c))

# Rule to make everything.
Pi3 Pi2 Pi1 Pi3-64: $(LIST)
	
# Rule to remake everything. Does not include clean.
all: Pi3 Pi2 Pi1 Pi3-64

# Rule to make the image & listing files.
$(LIST) : kernel.elf
	$(ARMGNU)-objdump -d kernel.elf > $(LIST)
	$(ARMGNU)-objcopy kernel.elf -O binary DiskImg/$(IMGFILE)
	$(ARMGNU)-nm -n kernel.elf > $(MAP)

OBJDIR := build
target = ${OBJDIR}/$(patsubst %.c,%.o,$(notdir ${1}))
obj.c :=
define obj
  $(call target,${1}) : ${1} | 
  obj$(suffix ${1}) += $(call target,${1})
endef

define SOURCES
  $(foreach src,${1},$(eval $(call obj,${src})))
endef

$(eval $(call SOURCES,${C_FILES}))

main.c : ${obj.c}

 ${obj.c} : % :
	$(ARMGNU)-gcc $(CFLAGS) $(LDFLAGS) -c $^ -o $@
# Uncomment this if you want the assembler output of each object file
#	$(ARMGNU)-gcc $(CFLAGS) $(LDFLAGS) -S $^ -o $(patsubst %.o,%.s,$(@))

kernel.elf : main.c
	$(ARMGNU)-gcc $(CFLAGS) $(LDFLAGS) $(LINKERFLAGS) -Wl,-T,$(LINKERFILE) $(wildcard $(OBJDIR)/*.o) main.c $(SMARTSTART) -o $@ $(LIBFLAGS)

# Control silent mode  .... we want silent in clean
.silent:clean

# cleanup temp files
clean:
	$(RM) $(MAP) $(SWALLOW_OUTPUT)
	$(RM) kernel.elf $(SWALLOW_OUTPUT)
	$(RM) $(LIST) $(SWALLOW_OUTPUT)
	$(RM) $(OBJDIR)$(SLASH)*.o $(SWALLOW_OUTPUT)
	$(RM) $(OBJDIR)$(SLASH)*.s $(SWALLOW_OUTPUT)
	echo CLEAN COMPLETED

