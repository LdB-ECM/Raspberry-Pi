###############################################################################
#	makefile
#	by Leon de Boer
#
#	A makefile script for use from command line or VS2017
###############################################################################
OSFLAG := $(OS)
ifeq ($(OSFLAG), Windows_NT)
	#WINDOWS LIKES THESE DEFINITIONS
	RM=-del /Q
	DIRSLASH=\\
else
	#LINUX LIKES THESE DEFINITIONS
	RM=-rm -f
	DIRSLASH=/
endif 

# The toolchain to use. Common name part and path 
ARMGNU ?= g:\pi\gcc_linaro_7_1\bin\aarch64-elf

Pi3: AFLAGS = -march=armv8-a+simd -mtune=cortex-a53
Pi3: CFLAGS = -Wall -O3 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize
Pi3: IMGFILE = kernel8.img

LDFLAGS = -nostdlib -nodefaultlibs -nostartfiles -ffreestanding -fno-asynchronous-unwind-tables -fomit-frame-pointer -specs=nosys.specs

LIBFLAGS=-lc -lm -lgcc

LINKERFLAGS=-Wl,-gc-sections -Wl,--build-id=none -Wl,-Bdynamic

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The name of the linker script to use.
LINKERFILE = rpi64.ld

# Source files.
SRCFILES :=  $(filter-out main.c, $(wildcard *.c))

# Rule to make everything.
Pi3: $(LIST)
	
# Rule to remake everything. Does not include clean.
all: Pi3

# Rule to make the image & listing files.
$(LIST) : kernel.elf
	$(ARMGNU)-objdump -d kernel.elf > $(LIST)
	$(ARMGNU)-objcopy kernel.elf -O binary $(IMGFILE)
	$(ARMGNU)-nm -n kernel.elf > $(MAP)

# Rule to make the elf file.
kernel.elf : $(SRCFILES)
	$(ARMGNU)-gcc $(CFLAGS) $(LDFLAGS) $(LINKERFLAGS) -Wl,-T,$(LINKERFILE) SmartStart64.S main.c $(SRCFILES) -o $@ $(LIBFLAGS)

clean:
	$(RM) $(MAP)
	$(RM) kernel.elf
	$(RM) $(LIST)
	
