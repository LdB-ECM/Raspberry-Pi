@REM COMPILER COMMAND LINE
@echo off
set "bindir=g:\pi\gcc_linaro_7_1\bin\"
set "cpuflags=-Wall -O3 -march=armv8-a+simd -mtune=cortex-a53 -mstrict-align -fno-tree-loop-vectorize -fno-tree-slp-vectorize"
set "asmflags=-nostdlib -nostartfiles -ffreestanding -fno-asynchronous-unwind-tables -fomit-frame-pointer -Wa,-a>list.txt"
set "linkerflags=-Wl,-gc-sections -Wl,--build-id=none -Wl,-Bdynamic -Wl,-Map,kernel.map"
set "outflags=-o kernel.elf"
set "libflags=-lc -lm -lgcc"
@echo on
%bindir%aarch64-elf-gcc.exe %cpuflags% %asmflags% %linkerflags% -Wl,-T,rpi64.ld main.c SmartStart64.S rpi-SmartStart.c emb-stdio.c %outflags% %libflags% 
@echo off
if %errorlevel% EQU 1 (goto build_fail)

@REM LINKER COMMAND LINE
@echo on
%bindir%aarch64-elf-objcopy kernel.elf -O binary DiskImg\kernel8.img
@echo off
if %errorlevel% EQU 1 (goto build_fail) 
echo BUILD COMPLETED NORMALLY
pause
exit /b 0

:build_fail
echo ********** BUILD FAILURE **********
Pause
exit /b 1