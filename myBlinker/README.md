# MYBLINKER (Pi1,2,3 32Bit .. Pi3 AARCH64)
Yes it's the boring old interrupt timer and blinking LED this time in either 32Bit or 64Bit mode. The technical background to 64Bit mode is the Pi3 is given to us in EL2 state from the Pi Foundation loader code. The timer interrupt is routed to EL1 where the interrupt service is established. It is obviously the first steps in how to route interrupt signals to services on the Pi3 in 64bit mode. 
>
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test.

To compile edit the makefile so the compiler path matches your compiler:
>
For Pi1: 
>     Make Pi1
For Pi2:
>     Make Pi2
For Pi3 in 32 Bit:
>     Make Pi3
For Pi3 in 64 Bit:
>     Make Pi3-64
     
To clean for full compile:     
>     Make Clean
     

