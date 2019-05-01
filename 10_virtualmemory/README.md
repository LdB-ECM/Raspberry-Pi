MMU on Raspberry Pi 2 and 3 (AARCH32, AARCH64)
========================================
As per usual you can simply copy the files in the DiskImg directory onto a formatted SD card and place in Pi to test
>
If you need help compiling you can look at
>https://github.com/LdB-ECM/Docs_and_Images/blob/master/Documentation/Multicore_Build.md
>
This is a correction and expansion of the tutorial 10 from bzt it maintains the use of 4K granuals.
>https://github.com/bztsrc/raspi3-tutorial/tree/master/10_virtualmemory

What bzt's tutorial did not do was setup the memory for synchronization primitives (LDREX/STREX) it dealt only with the virtualization.

### AARCH64
TRBR0 is mapped directly to a Stage1 and Stage2 table implementation which provide 1:1 translation.

The Stage1 sections are 2M and there are maximum of 512 entries per table so each table entry covers a range of 1GB.
1GB = 0x40000000 which covers most of the Pi3 memory space just leaving out the hardware block at 0x40000000+ as detailed on QA7_rev3.4.pdf. Hence we require two entries which cover 0x0 - 0x40200000.

With that in mind the Stage2 table must be 2 * 512 = 1024 entries in size. The first entry from the Stage1 table will point to Stage2[0] and the second entry will point to Stage2[512]. Most of the back second table will all be unused.

TRBR1 is mapped to a 3 stage table implementation which covers virtual memory.
I have setup a basic single table page at the Stage1, Stage2 and Stage3 levels in which to create virtual memory allocations.

We start by simply mapping the last entry of the Stage1 table to the Stage2 table.
We then map the last entry of the Stage2 table to the first Stage3 table.

There are 512 entries each of 4K in size for the Stage3 table so it covers a range of 2M.
As we have mapped the last page of both the Stage1 and Stage2 tables it will mean all FF's at the top of the virtual address but then it starts at the lower range. Yeah all the table reverse is just to make it easier for me to look at the virtual allocation :-)

So essentially we have 512, 4K virtual pages available for the 2MB of address range from
0xFFFFFFFFFFE00000 to 0xFFFFFFFFFFFFFFFF

So if you call the function
uint64_t virtualmap (uint32_t phys_addr);
It will allocate the first unused Stage3 table entry to the physical Pi address you provide and return the virtual address to you. Note it is currently mapped as device memory you may want to change the function to pass in mapping flags.

In operation the first call will always return 0xFFFFFFFFFFE00000, the second call 0xFFFFFFFFFFE01000, the 3rd 0xFFFFFFFFFFE01000.

Now it's obvious that it is setup to have an unmap function where the stage3 table entry is simply zeroed marking it not in use.

### AARCH32
>
There is a very simple VMSAv7 L1 section table setup with 4096 entries each of 1Mb section. Similar to the 64bit example a single Virtual table is setup at the back end of the covered 4GB space. This will give  1:1 mapping on the lower 2GB and then 2Mb of Virtual 4K pages up at 0xFFE00000.

### DEMO PROGRAM

The provided sample "main.c" uses my smartstart ability to task the cores to a C function (that feature is done by smartstart assembler stub). So once core 0 has setup the MMU table and all cores have engaged it then synchronization semaphores are possible. All 4 cores actually share the same MMU table which is setup to cache all the memory from 0 to Videocore memory. So the 4 cores can run synchronization semaphore on any memory address and we checked that ability.

In the main test Core0 will lock a semaphore and then each core will be sent to try and aquire the semaphore. After ten seconds waiting (displayed on screen) core 0 gives the semaphore back. Then one of the waiting cores can take the semaphore print its message and wait 5 seconds and give the semaphore back. The process repeats until each core has printed it's message and waited 5 seconds. This proves we can synchronize the cores in any critical section using the simple binary semaphore.
http://www.cs.umd.edu/~shankar/412-Notes/10-BinarySemaphores.html
>
This is all just for testing, I don't suggest you really do this for real work.  You would also obviously extend the range of synchronizing primitives (ARM has whitepaper on them) but this at least gives you a start.

Now the big warning .. you now have the L2 cache running ... you will need memory barriers on device access code :-)
