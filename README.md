# Raspberry-Pi-Multicore
Finally started the series on multicore task schedulers and concepts. This code is designed specifically for multicore Raspberry Pi 2 & 3's it will not work on a Pi 1. On Pi3 the choice of AARCH32 or AARCH64 is available.

The first step is xRTOS our start point with a 4 core switcher with simple round robin task schedule. Each core is manually loaded with 2 tasks and will create an idle task when started. The 2 tasks are simple moving the bars on screen at this stage.
>
**Update:** The second step with the MMU turned on has now also been added.
>
Multicore code has been given it's own repository
>
https://github.com/LdB-ECM/Raspberry-Pi-Multicore
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/xRTOS.jpg?raw=true)
>
# FreeRTOS
So I am starting a series on Task Switchers on Single and Multicores. We are going to start out with the simple standard task switcher (FreeRTOS 10.1.1) on a single core. Yeah it's boring but we need to sort out base code stability before we start to fly.
>
https://github.com/LdB-ECM/Raspberry-Pi/tree/master/FreeRTOSv10.1.1
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/FreeRTOS.jpg?raw=true)
>

# *** Cross Compiling Details
Visual Studio 2017 recently added direct make support so I am finally switching to make files so linux users can directly compile. The compiling background has been quickly documented. Most of the directories are now carrying makefiles as I have relented and started doing it.
>
https://github.com/LdB-ECM/Docs_and_Images/tree/master/Documentation/Code_Background.md
>
If you are on Windows and want a windows executable version of GNU make you can get 4.2.1 from
>
https://sourceforge.net/projects/ezwinports/files/
>
# BareMetal Raspberry-Pi (Linux free zone .. AKA windows)

32 Bit Cross Compiler Toolchain I use (Multiple O/S are supported):
>https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
>
32 Bit compile on the Pi itself with GCC 6: 
>https://github.com/LdB-ECM/Docs_and_Images/tree/master/Documentation/GCC6_On_Pi.md
>
64 Bit Cross Compiler Toolchain I use (Multiple O/S are supported):
>https://releases.linaro.org/components/toolchain/binaries/latest-7/aarch64-elf/
>
# OLDER WORK
>
# USB (Pi1,2,3 32Bit .. Pi3 AARCH64)
>https://github.com/LdB-ECM/Raspberry-Pi/tree/master/Arm32_64_USB
>
Complete redux of CSUD (Chadderz's Simple USB Driver) by Alex Chadwick. All the memory allocation is gone and compacted to a single file (usb.c). It provides the Control channel functionality for a USB which enables enumeration. Now merged to latest SmartStart 2.0.3 code to bring the Alpha USB up to my latest bootstub. I have new GLES code which requires access to ethernet which means having the LAN9512/LAN9514 running. This is a small step to merge the existing code onto the newer smartstart bootstub to prepare a release for that. 
If you just want to see it just put the files in the DiskImg directory on a formatted SD card and place in Pi.
>

![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/USB64_alpha.jpg)
>
# MULTICORE (Pi1,2,3 32Bit .. Pi3 AARCH64)
>https://github.com/LdB-ECM/Raspberry-Pi/tree/master/Multicore
>
Please remember the Pi1 is single processor. So while you can build code for a Pi1 it can't be used for hyperthreading unless used on a Pi2 or Pi3. The fact you can run your Pi1 code on a Pi2/3 will only work because the SmartStartxx.s stub sorts all that out, just remember its ARM6 code and slightly slower. The assembler and linker files are paired you use either the 32 bit or 64 bit together.
>
The SmartStartxx.S assembler boot stub was extended to setup cores 1,2,3 for hyperthreading. A new spinlock was created which mimics the bootloaders but is C compiler safe. To do that registers that would be trashed by C routines where restored when the core process is called. In addition to that each core has its own stack the size of which is controlled by the new matching linker file (rpixx.ld).
>
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test 
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/Multicore.jpg?raw=true)
# MYBLINKER (Pi1,2,3 32Bit .. Pi3 AARCH64)
https://github.com/LdB-ECM/Raspberry-Pi/tree/master/myBlinker
>
Yes it's the boring old interrupt timer and blinking LED this time in either 32Bit or 64Bit mode. For 64bit the technical background is the Pi3 is given to us in EL2 state. The timer interrupt is routed to EL1 where the interrupt service is established. It is obviously the first steps in how to route interrupt signals to services on the Pi3 in 64bit mode. 

# ***New GLES (Pi1,2,3 32Bit .. Pi3 AARCH64)
>https://github.com/LdB-ECM/Raspberry-Pi/tree/master/GLES2
>
This will be my ongoing work to try to build a baremetal GLES interface of some kind. Having gone thru the firmware blob via a VCOS shim this time I am going to try going direct onto the VC4. The reason for the new attempt is the GL pipleine details are finally being really exposed by Eric AnHolt in his work on the OpenGL and his current work on the VC5 successor from Broadcom.
>
I have not yet settled on an interface format but more going to try to follow a tutorial series on OpenGL 3.3 and develop a baremetal interface as I go.
>
Tutorial series currently at number 3: http://www.mbsoftworks.sk/index.php?page=tutorials&series=1
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/GL_code2.jpg?raw=true)
>
# ***New ROTATE 3D MODEL - (Pi1, 2, 3 - AARCH32, Pi3 AARCH64)
>https://github.com/LdB-ECM/Raspberry-Pi/tree/master/GLES2_Model
>
Update: 64Bit version solved sscanf was bugged in the newlib, I had to rewrite the function. I am getting so fed up with stdio.h in newlib I might just complete the job and replace all the functions.
>
Update: All memory allocation now removed from render call and render speed now goes well over 800fps so I timed the rotation speed. You will notice rotation is alot smoother.
>
So we continue our GL pipe adventure slowly crawling towards something that ressembles minimal OpenGL.
>
So we work on rotating an actual LightWave OBJ 3D mesh model:
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/biplane.jpg?raw=true)

Streaming video of output:
>
https://streamable.com/t6efv
>
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test 

>
# ***New SD + FAT32 (Pi1,2,3 32Bit .. Pi3 AARCH64)
>https://github.com/LdB-ECM/Raspberry-Pi/tree/master/SD_FAT32
>
To take my accelerated graphics further I found I was in need of the ability to Read Files from the SD Card. I reworked some old code I had done for an article, simplifying it to something that meets the requirements. At this stage the code is strictly READ file only and the API calls mimic the standard file functions from the Windows API.
>
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card (make sure this includes the subdirectory "Bitmaps") and place in Pi to test 
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/SD_FAT32.jpg?raw=true)

