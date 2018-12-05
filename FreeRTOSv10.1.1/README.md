# FreeRTOS 10.1.1 (Pi 2,3 32Bit and Pi3 64 bit now working)
![](https://github.com/LdB-ECM/Docs_and_Images/blob/master/Images/FreeRTOS.jpg?raw=true)
>
I have been messing around with Task Switchers (Single/Multicore) in 32Bit or 64Bit mode and have decided to put some up. This is the easiest one to understand and start with being a simple FreeRTOS 10.1.1 port. Yes it all boots from the standard SmartStart system as usual so it autodetects models etc.  So on this example we have the RTOS simply running on one core doing the boring time slicing and moving some display bars around. 
>
I simplified the original IRQ handler which handles the context switch. I provided FIQ functions on the SmartStart interface so if you do want to use peripherals with interrupts you can use the FIQ. Originally I was going to have the peripherals and the context switch on the IRQ system but it leads to horrible nesting which slows things down. So I decided instead to have the peripherals on the FIQ because more registers are banked (auto saved), which makes the handler faster and the priority is higher so peripheral interrupts take precedence over context switches. I found everything runs a lot smoother in that configuration.  
>
Technically this should probably should be called FreeRTOS 10.1.2 because it has a couple of minor changes I added. I improved a couple of task display string functions (which were ugly in task.c) and added a CPU load percentage as a standard funtion
#### unsigned int xLoadPercentCPU(void);
>
Sorry it's a pretty boring example just moving the bars backwards forward, I will try and make some more advanced samples.
>
### > As usual you can copy prebuilt files in "DiskImg" directory on formatted SD card to test <

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
     


