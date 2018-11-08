ifeq ($(OS), Windows_NT)
	#WINDOWS LIKES THESE DEFINITIONS
	RM=-del /Q
else
	#LINUX LIKES THESE DEFINITIONS
	RM=-rm -f
endif 

# The toolchain to use.
ARMGNU ?= D:\gcc_pi_7_2\bin\arm-none-eabi

Pi3: AFLAGS = -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv8-a
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

# The directory SmartStart files are in
#SMARTSTARTDIR = $(SOURCE)SmartStart/

# The name of the linker script to use.
LINKERFILE = rpi32.ld

# The SmartStart bootloader file.
#SMARTSTARTC := $(wildcard $(SMARTSTARTDIR)*.c)
#SMARTSTART = $(SMARTSTARTDIR)SmartStart32.s $(SMARTSTARTC)
SMARTSTART = SmartStart32.S

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.
C_FILES := $(filter-out main.c, $(wildcard *.c))

# Rule to make everything.
Pi3 Pi2 Pi1: $(LIST)
	
# Rule to remake everything. Does not include clean.
all: Pi3 Pi2 Pi1

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
	$(ARMGNU)-gcc $(CFLAGS) $(LDFLAGS) -S $^ -o $(patsubst %.o,%.s,$(@))

kernel.elf : main.c
	$(ARMGNU)-gcc $(CFLAGS) $(LDFLAGS) $(LINKERFLAGS) -Wl,-T,rpi32.ld $(wildcard $(OBJDIR)/*.o) main.c $(SMARTSTART) -o $@ $(LIBFLAGS)

# Rule to clean Pi1 files.
Pi1clean : clean
	$(RM) DiskImg$\$(IMGFILE)

# Rule to clean Pi2 files.
Pi2clean : clean
	$(RM) DiskImg$\$(IMGFILE)

# Rule to clean Pi3 files.
Pi3clean : clean
	$(RM) DiskImg$\$(IMGFILE)

clean:
	$(RM) $(MAP)
	$(RM) kernel.elf
	$(RM) $(LIST)
	$(RM) $(OBJDIR)\*.o
	$(RM) $(OBJDIR)\*.s

Pi3Rebuild: clean Pi3
Pi2Rebuild: clean Pi2
Pi1Rebuild: clean Pi1
