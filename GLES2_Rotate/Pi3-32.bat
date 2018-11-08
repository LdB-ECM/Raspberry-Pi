@REM COMPILER COMMAND LINE
@echo off
set "bindir=g:\pi\gcc_pi_6_3\bin\"
set "cpuflags=-Wall -O3 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv8-a -mtune=cortex-a53 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize"
set "asmflags=-nostdlib -nostartfiles -ffreestanding -fno-asynchronous-unwind-tables -fomit-frame-pointer -Wa,-a>list.txt"
set "linkerflags=-Wl,-gc-sections -Wl,--build-id=none -Wl,-Bdynamic -Wl,-Map,kernel.map"
set "outflags=-o kernel.elf"
set "libflags=-lc -lm -lgcc -lnosys"
@echo on
%bindir%arm-none-eabi-gcc %cpuflags% %asmflags% %linkerflags% -Wl,-T,rpi32.ld main.c SmartStart32.S rpi-SmartStart.c emb-stdio.c rpi-GLES.c %outflags% %libflags% 
@echo off
if %errorlevel% EQU 1 (goto build_fail)

@REM LINKER COMMAND LINE
@echo on
%bindir%arm-none-eabi-objcopy kernel.elf -O binary DiskImg\kernel8-32.img
@echo off
if %errorlevel% EQU 1 (goto build_fail) 
echo BUILD COMPLETED NORMALLY
pause
exit /b 0

:build_fail
echo ********** BUILD FAILURE **********
Pause
exit /b 1