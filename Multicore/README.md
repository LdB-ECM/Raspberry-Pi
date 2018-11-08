# MULTICORE Pi 1,2,3 AARCH 32, NEW*** Pi3 AARCH64 
>
Yes the multicore sample was extended to 64 BIT code.
>
Please remember the Pi1 is single processor. So while you can build code for a Pi1 it can't be used for hyperthreading unless used on a Pi2 or Pi3. The assembler and linker files are paired you use either the 32 bit or 64 bit together.
>
The SmartStartxx.S assembler boot stub was extended to setup cores 1,2,3 for hyperthreading. A new spinlock was created which mimics the bootloaders but is C compiler safe. To do that registers that would be trashed by C routines where restored when the core process is called. In addition to that each core has its own stack the size of which is controlled by the new matching linker file (rpixx.ld).
>
The demo uses printf to screen which is very dangerous because printf is not re-entrant so I took a bit of care to avoid clashes using it. Please don't try and take using printf too far what is expected is you setup proper thread safe C code. The random core hyperthread calls are cause by the interrupt timer.
>
 rpi-SmartStart.h provides the C call
>function bool CoreExecute (uint8_t coreNum, CORECALLFUNC func);
>
Where CORECALLFUNC is defined as
>typedef void (*CORECALLFUNC) (void);
>
So CoreExecute can assign any "void function (void)" C routine to be execute by the given core (1..3).
>
 As per usual you can simply copy the files in the DiskImg directory onto a format SD card and place in Pi to test
>
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/Multicore.jpg)
