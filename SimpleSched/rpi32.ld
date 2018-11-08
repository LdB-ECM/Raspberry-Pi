OUTPUT_ARCH(arm)
ENTRY(_start)
   /* Specify the memory areas */
   MEMORY
   {
         RAM (xrw)      : ORIGIN = 0x8000 , LENGTH = 0x8000000 /*128MB which is half memory on smallest memory PI, assumes GPU has other half */
    }

SECTIONS
{
	/*
	* First and formost we need the .init section, containing the code to 
        	* be run first. We allow room for the ATAGs and stack and conform to 
       	* the bootloader's expectation by putting this code at 0x8000.
	*/
	.init 0x8000 : {
		KEEP(*(.init))
	} > RAM
	
	/* 
	* Next we put the rest of the code.
	*/
	.text : {
		. = ALIGN(4);
		 __text_start__ = .;     	/* Label in case we want address of text section start */
		*(.text .text.*)
		 __text_end__ = .;      	/* Label in case we want address of text section end */
	} > RAM
	
	/* 
	* Next we put the data.
	*/
	.data : {
		. = ALIGN(4);
		 __data_start__ = .;     	/* Label in case we want address of data section start */
		*(.data .data.*)
		 __data_end__ = .;     	/* Label in case we want address of data section end */
	} > RAM

	/* 
	* Next we put the align 16 data.
	*/
	.data1 : {
		. = ALIGN(16);
		 __data1_start__ = .;     	/* Label in case we want address of data section start */
		*(.data1 .data1.*)
		 __data1_end__ = .;     	/* Label in case we want address of data section end */
	} > RAM

	/* 
	* Next we put the bss data .. C/C++ compilers produce this.
	*/
	.bss :
    	{
		. = ALIGN(4);
		 __bss_start__ = .;     	/* Label in case we want address of BSS section start */
		*(.bss .bss.*)
 		*(COMMON)
		 __bss_end__ = .;     	/* Label in case we want address of BSS section end */
   	} > RAM

	/* 
	* Next we put the rodata .. C/C++ compilers store preset constants here.
	*/
	.rodata :
    	{
       		. = ALIGN(4);
        		__rodata_start__ = .;	/* Label in case we want address of rodata section start */
        		*(.rodata .rodata.*)
        		__rodata_end__ = .;	/* Label in case we want address of rodata section start */
   	} > RAM

	/* 
	* Next we put stack for Core0 
	*/
	.stack0 :
    	{
        		. = ALIGN(8); 		/* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        		__stack_start__core0 = .;	/* Label in case we want address of stack core 0 section start */
        		. = . + 512;   		/* HYP stack size core 0 */
    		__HYP_stack_core0 = .;
        		. = . + 1024;     		/* IRQ stack size core 0 */
        		__IRQ_stack_core0 = .;
        		. = . + 1024;     		/* FIQ stack size core 0 */
        		__FIQ_stack_core0 = .;
        		. = . + 32768;    		/* SVC stack size core 0 */
        		__SVC_stack_core0 = .;
        		__stack_end__core0 = .;  	/* Label in case we want address of stack core 0 section end */
   	 } > RAM

	/* 
	* Next we put stack for Core1 
	*/
	.stack1 :
    	{
        		. = ALIGN(8); 		/* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        		__stack_start__core1 = .;	/* Label in case we want address of stack core 1 section start */
        		. = . + 512;   		/* HYP stack size core 1 */
    		__HYP_stack_core1 = .;
        		. = . + 1024;     		/* IRQ stack size core 1 */
        		__IRQ_stack_core1 = .;
        		. = . + 1024;     		/* FIQ stack size core 1 */
        		__FIQ_stack_core1 = .;
        		. = . + 4096;    		/* SVC stack size core 1 */
        		__SVC_stack_core1 = .;
        		__stack_end__core1 = .;  	/* Label in case we want address of stack core 1 section end */
   	 } > RAM

	/* 
	* Next we put stack for Core2 
	*/
	.stack2 :
    	{
        		. = ALIGN(8); 		/* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        		__stack_start__core2 = .;	/* Label in case we want address of stack core 2 section start */
        		. = . + 512;   		/* HYP stack size core 2 */
    		__HYP_stack_core2 = .;
        		. = . + 1024;     		/* IRQ stack size core 2 */
        		__IRQ_stack_core2 = .;
        		. = . + 1024;     		/* FIQ stack size core 2 */
        		__FIQ_stack_core2 = .;
        		. = . + 4096;    		/* SVC stack size core 2 */
        		__SVC_stack_core2 = .;
        		__stack_end__core2 = .;  	/* Label in case we want address of stack core 2 section end */
   	 } > RAM

	/* 
	* Next we put stack for Core3
	*/
	.stack3 :
    	{
        		. = ALIGN(8); 		/* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        		__stack_start__core3 = .;	/* Label in case we want address of stack core 3 section start */
        		. = . + 512;   		/* HYP stack size core 3 */
    		__HYP_stack_core3 = .;
        		. = . + 1024;     		/* IRQ stack size core 3 */
        		__IRQ_stack_core3 = .;
        		. = . + 1024;     		/* FIQ stack size core 3 */
        		__FIQ_stack_core3 = .;
        		. = . + 4096;    		/* SVC stack size core 3 */
        		__SVC_stack_core3 = .;
        		__stack_end__core3 = .;  	/* Label in case we want address of stack core 3 section end */
   	 } > RAM

	.heap :
    	{
     		. = ALIGN(4);
     		__heap_start__ = .;	/* Label in case we want address of heap section start */
    		 _end = .; PROVIDE (end = .); /* Any memory from here is free to use so this is end of code and start of heap */
   	 } > RAM

	/*
	* Finally comes everything else. A fun trick here is to put all other 
	* sections into this section, which will be discarded by default.
	*/
	/DISCARD/ : {
		*(*)
	}
    
}
