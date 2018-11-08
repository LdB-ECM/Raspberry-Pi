# Core3 Interrupt (Pi2/3 AARCH32 or Pi3 AARCH64)
Yes it's the boring old interrupt timer and blinking LED but this time the interrupts being processed by Core3. This wont work on a Pi1 because it has only 1 core.

The technical background to this is we use the details on datasheet QA7_rev3.4.pdf to route the timer irq to core3.
My SmartStart loader picks up each core and runs them thru most of the same startup as core0 so they share the vector table (yes 1 vector table for all 4 processors).
>
We then assign the interrupt vector (like you would on core0) but then command core3 to jump to a small code that simply enables it's interrupts and deadloops. As the interrupt is re-routed core3 it spends all it's time hard deadlooping and processing the interrupts. Meanwhile core0 draws a deadloop display of it's own on the screen.
>
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test.

To compile edit the makefile so the compiler path matches your compiler:
>
For Pi1: 
>     YOU CAN COMPILE BUT CAN NOT RUN AS ONLY HAS 1 CORE
>     your compiled code however will work on a Pi2,Pi3 thanks to SmartStart
For Pi2:
>     Make Pi2
For Pi3 in 32 Bit:
>     Make Pi3
For Pi3 in 64 Bit:
>     Make Pi3-64
     
To clean for full compile:     
>     Make Clean
     
