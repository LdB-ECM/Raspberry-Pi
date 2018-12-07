# If cross compiling from windows use native GNU-Make 4.2.1
# https://sourceforge.net/projects/ezwinports/files/
# download "make-4.2.1-without-guile-w32-bin.zip" and set it on the enviroment path
# There is no need to install cygwin or any of that sort of rubbish

# This little section enables us to make compatible windows and linux make files
ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q
	SLASH = \\
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
endif 

# You will need to change the first line (ARMGNU) of these to match your compiler directories
ifeq ($(MAKECMDGOALS),Pi3-64)
ifeq ($(OS), Windows_NT)
ARMGNU := D:/gcc_linaro_7_1/bin/aarch64-elf
else
ARMGNU := aarch64-elf
endif
SMARTSTART := SmartStart64.S
SPECIAL_FLAGS := -mcpu=cortexa53 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize
LINKER_FILE := rpi64.ld
else
ifeq ($(OS), Windows_NT)
ARMGNU := D:/gcc_pi_7_2/bin/arm-none-eabi
else
ARMGNU := arm-none-eabi
endif
SMARTSTART := SmartStart32.S
SPECIAL_FLAGS := -mno-unaligned-access
LINKER_FILE := rpi32.ld
endif

INCLUDEPATH ?= FreeRTOS

# Temp directory for object files to go into
TOP_DIR := .
BUILD_DIR ?= ${TOP_DIR}/build

# Set the C compilation flags common to all platforms.
CFLAGS  :=

# Optimize for speed.
CFLAGS += -O3

# Enable most useful compiler warnings.
CFLAGS  += -Wall

# Require full prototypes for all functions.
CFLAGS  += -Wstrict-prototypes

# Do not allow gcc to replace function calls with calls to gcc builtins, except
# when explicitly requested through a __builtin prefix.  This ensures that gcc
# does not attempt to replace any of our code with its own.
CFLAGS  += -fno-builtin

# Assume that the memory locations pointed to by any two pointers can alias,
# even if the types of the variables pointed to are not compatible as defined in
# the C standard.  Enabling this option is fairly common, since most programmers
# don't fully understand aliasing in C, and this forces the "expected" behavior.
CFLAGS  += -fno-strict-aliasing

# Do not allow multiple definitions of uninitialized global variables.
CFLAGS  += -fno-common 

# Place each function in a separate section so that the linker can apply garbage
# collection to remove unused functions (the --gc-sections linker flag).
CFLAGS  += -ffunction-sections

# Do not generate position-independent code.  (This flag may be unneeded, since
# generally you have to specify -fPIC to *get* the compiler to generate
# position-independent code).
CFLAGS  += -fno-pic

# Treat signed overflow as fully defined as per two's complement arithmetic,
# even though the C standard specifies that signed overflow is undefined
# behavior.  Many programmers are not aware of this, so we force the expected
# behavior.
CFLAGS  += -fwrapv

# ADD any special AARCH compile mode flags
CFLAGS  += $(SPECIAL_FLAGS)

Pi3-64: CFLAGS += -ffreestanding -nostartfiles -std=c11 -mcpu=cortex-a53
Pi3-64: IMGFILE := kernel8.img

Pi3: CFLAGS += -ffreestanding -nostartfiles -std=c11 -mcpu=cortex-a53
Pi3: IMGFILE := kernel8-32.img

Pi2: CFLAGS += -ffreestanding -nostartfiles -std=c11 -mcpu=cortex-a7 
Pi2: IMGFILE := kernel7.img

Pi1: CFLAGS += -ffreestanding -nostartfiles -std=c11 -mcpu=arm1176jzf-s
Pi1: IMGFILE := kernel.img

# Set linker flags common to all platforms.  platformVars can add additional
# flags if needed.  Do not use the "-Wl," prefix either here on in platformVars.
LDFLAGS := -Wl,-T $(LINKER_FILE) -Wl,--gc-sections -Wl,--build-id=none

# Set default external libraries.  Embedded Xinu is, of course, stand-alone and
# ordinarily does not need to be linked to any external libraries; however,
# platformVars can add -lgcc to this if needed by the platform.
LDLIBS  := -lc -lm -lgcc

# platform name
PLATFORM := RaspberryPi

# The loader directory should have a directory to match the platform string
# In that directory is a Makerules file that specifies what .C and .S files to include
# They will appear in the C_FILES and S_FILES lists respectively
# (The loader should always be first entry, ensuring the image starts with it if you want multiple directories.)
SYSCOMPS := $(TOP_DIR)/loader/$(PLATFORM)
INCLUDEPATH1 ?=  $(TOP_DIR)/loader/$(PLATFORM)
INCLUDEPATH2 ?=  $(TOP_DIR)/FreeRTOS/Source/include
INCLUDEPATH3 ?=  $(TOP_DIR)/FreeRTOS/Source/portable/GCC/$(PLATFORM)

INCLUDE = -I$(INCLUDEPATH1) -I$(INCLUDEPATH2) -I$(INCLUDEPATH3) -I$(TOP_DIR)/Demo

# Directory which has our demo files to compile
DEMOCOMPS := $(TOP_DIR)/Demo

# Directory that has the FreeRTOS source
RTOSCOMPS := $(TOP_DIR)/FreeRTOS/Source

# List of all components to include  ... Loader + FreeRTOS + Demo
COMPS := $(SYSCOMPS) $(RTOSCOMPS) $(DEMOCOMPS)

# Include component files, each should add its part to the compile source
# This builds two lists C_FILES and S_FILES from iteration thru the makerules files
COMP_SRC :=
include $(COMPS:%=%/Makerules)

CFILES := $(strip $(filter-out %.S, $(COMP_SRC)))
SFILES := $(strip $(filter-out %.c, $(COMP_SRC)))
SOFILES := $(patsubst %.S,$(BUILD_DIR)/%.o, $(notdir $(SFILES)))
COFILES := $(patsubst %.c,$(BUILD_DIR)/%.o, $(notdir $(CFILES)))

gcc : freertos.elf

# Control silent mode  .... we want silent in clean
.SILENT: clean
clean :
	$(RM) build$(SLASH)*.o
	$(RM) build$(SLASH)*.d
	$(RM) *.elf
	$(RM) *.list
	$(RM) *.map
.PHONY: clean

# How we compile assembler files
$(SOFILES): $(SFILES)
	$(ARMGNU)-gcc -MMD -MP -g -c $(CFLAGS) $(INCLUDE) $(filter %/$(patsubst %.o,%.S,$(notdir $@)), $(SFILES)) -c -o $@ -lc -lm -lgcc

# How we compile C files
 $(COFILES): $(CFILES)
	$(ARMGNU)-gcc -MMD -MP -g -c $(CFLAGS) $(INCLUDE) $(filter %/$(patsubst %.o,%.c,$(notdir $@)), $(CFILES)) -c -o $@ -lc -lm -lgcc

$(SFILES): ;
$(CFILES): ;

Pi3-64: freertos.elf
.PHONY: Pi3-64

Pi3: freertos.elf
.PHONY: Pi3

Pi2: freertos.elf
.PHONY: Pi2

Pi1: freertos.elf
.PHONY: Pi1

freertos.elf : $(SOFILES) $(COFILES)
	$(ARMGNU)-gcc $(CFLAGS) -o $@ $(LDFLAGS) $^ $(LDLIBS)
	$(ARMGNU)-objdump -d freertos.elf > freertos.list
	$(ARMGNU)-objcopy freertos.elf -O binary DiskImg$(SLASH)$(IMGFILE)
	$(ARMGNU)-nm -n freertos.elf > freertos.map
