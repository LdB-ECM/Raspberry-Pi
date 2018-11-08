@REM COMPILER COMMAND LINE
@echo off
set "bindir=g:\pi\gcc_linaro_6_3\bin\"
set "cpuflags=-Wall -O3 -march=armv8-a+simd+fp -mtune=cortex-a53 -mabi=lp64 -mstrict-align"
set "asmflags=-nostdlib -nostartfiles -ffreestanding -fno-asynchronous-unwind-tables -fomit-frame-pointer -Wa,-a>list.txt"
set "linkerflags=-Wl,-gc-sections -Wl,--build-id=none -Wl,-Bdynamic -Wl,-Map,kernel.map"
set "outflags=-o kernel.elf"
set "libflags=-lc -lm -lg -lgcc"
@echo on
%bindir%\aarch64-elf-gcc.exe %cpuflags% %asmflags% %linkerflags% -Wl,-T,rpi64.ld main.c  SmartStart64.S rpi-BasicHardware.c  %outflags% %libflags% 
@echo off
if %errorlevel% EQU 1 (goto build_fail)

@REM LINKER COMMAND LINE
@echo on
%bindir%\aarch64-elf-objcopy kernel.elf -O binary kernel8.img
@echo off
if %errorlevel% EQU 1 (goto build_fail) 
echo BUILD COMPLETED NORMALLY
pause
exit /b 0

:build_fail
echo ********** BUILD FAILURE **********
Pause
exit /b 1