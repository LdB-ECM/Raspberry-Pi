OUTPUT_ARCH(aarch64)
ENTRY(_start)
   /* Specify the memory areas */
   MEMORY
   {
         RAM (xrw)      : ORIGIN = 0x80000 , LENGTH = 0x8000000 /*128MB which is half memory on smallest memory PI, assumes GPU has other half */
    }

SECTIONS
{
	/*
	* First and formost we need the .init section, containing the code to 
    * be run first. We must conform to the bootloader's expectation by 
	* putting this code at 0x80000.
	*/
	.init 0x80000 : {
		. = ALIGN(4);
		KEEP(*(.init))
	} > RAM

	/* 
	* Next we put the rest of the executable code.
	*/
	.text : {
		. = ALIGN(4);
		__text_start__ = .;     	/* Label in case we want address of text section start */
		KEEP(*(.text.boot)) *(.text .text.*)
		__text_end__ = .;      	/* Label in case we want address of text section end */
	} > RAM

	/* 
	* Next we put the rodata .. C/C++ compilers store preset read_only constants here.
	*/
	.rodata : {
       	. = ALIGN(4);
        __rodata_start__ = .;		/* Label in case we want address of rodata section start */
        *(.rodata .rodata.*)
        __rodata_end__ = .;			/* Label in case we want address of rodata section start */
   	} > RAM

	/* 
	* Next we put the data.
	*/
	.data : {
		. = ALIGN(4);
		 PROVIDE(_data = .);
		__data_start__ = .;     	/* Label in case we want address of data section start */
		*(.data .data.*)
		__data_end__ = .;     		/* Label in case we want address of data section end */
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
	.bss : {
		. = ALIGN(4);
		__bss_start__ = .;     	/* Label in case we want address of BSS section start */
		*(.bss .bss.*)
 		*(COMMON)
		__bss_end__ = .;     		/* Label in case we want address of BSS section end */
   	} > RAM

	.stack_core0 : {
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core0__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core0 = .;
        . = . + 16384;				/* EL1 stack size */
        __EL1_stack_core0 = .;
        . = . + 4096;				/* EL2 stack size (start-up) */
        __EL2_stack_core0 = .;
        __stack_end_core0__ = .;    /* Label in case we want address of stack core0 ends */
    } > RAM

	.stack_core1 : {
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core1__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core1 = .;
        . = . + 512;				/* EL1 stack size */
        __EL1_stack_core1 = .;
        . = . + 4096;				/* EL2 stack size (start-up) */
        __EL2_stack_core1 = .;
        __stack_end_core1__ = .;    /* Label in case we want address of stack core1 ends */
    } > RAM

	.stack_core2 : {
		. = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core2__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core2 = .;
        . = . + 512;				/* EL1 stack size */
        __EL1_stack_core2 = .;
        . = . + 4096;				/* EL2 stack size (start-up) */
		__EL2_stack_core2 = .;
        __stack_end_core2__ = .;	/* Label in case we want address of stack core2 ends */
    } > RAM

	.stack_core3 : {
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core3__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core3 = .;
        . = . + 512;				/* EL1 stack size */
        __EL1_stack_core3 = .;
        . = . + 4096;				/* EL2 stack size (start-up) */
       	__EL2_stack_core3 = .;
        __stack_end_core3__ = .;	/* Label in case we want address of stack core2 ends */
    } > RAM

	.heap : {
		. = ALIGN(4);
     	__heap_start__ = .;			/* Label in case we want address of heap section start */
    	 _end = .; 
		 PROVIDE (end = .);			/* Any memory from here is free to use so this is end of code and start of heap */
   	 } > RAM

	/*
	* Finally comes everything else. A fun trick here is to put all other 
	* sections into this section, which will be discarded by default.
	*/
	/DISCARD/ : {
		*(*)
	}
}
