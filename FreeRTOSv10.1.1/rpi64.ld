OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
	/*
	 * First and formost we need the .init section, containing the code to 
     * be run first. We allow room for the ATAGs and stack and conform to 
     * the bootloader's expectation by putting this code at 0x8000.
	 */
	.init 0x80000 : {
		KEEP(*(.init))
	}

	/* 
	 * Next we put the rest of the code.
	 */
	.text : {
		. = ALIGN(4);
		 __text_start__ = .;     	/* Label in case we want address of text section start */
		*(.text .text.*)
		 __text_end__ = .;      	/* Label in case we want address of text section end */
	}

	/* 
	 * Next we put the rodata .. C/C++ compilers store preset constants here.
	 */
	.rodata : {
       	. = ALIGN(4);
        __rodata_start__ = .;		/* Label in case we want address of rodata section start */
        *(.rodata .rodata.*)
        __rodata_end__ = .;			/* Label in case we want address of rodata section start */
   	}

	/* 
	 * Next we put the data.
	 */
	.data : {
		. = ALIGN(4);
		 __data_start__ = .;     	/* Label in case we want address of data section start */
		*(.data .data.*)
		 __data_end__ = .;     		/* Label in case we want address of data section end */
	}

	/* 
	 * Next we put the align 16 data.
	 */
	.data1 : {
		. = ALIGN(16);
		 __data1_start__ = .;     	/* Label in case we want address of data section start */
		*(.data1 .data1.*)
		 __data1_end__ = .;     	/* Label in case we want address of data section end */
	}

	/* 
	 * Next we put the bss data .. C/C++ compilers produce this and needs to be zeroed by startup
	 */
	.bss : {
		. = ALIGN(4);
		 __bss_start__ = .;     	/* Label in case we want address of BSS section start */
		*(.bss .bss.*)
 		*(COMMON)
		 __bss_end__ = .;     		/* Label in case we want address of BSS section end */
   	}

	.stack_core0 : {
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core0__ = .;
        . = . + 16384;				/* EL0 stack size */
        __EL0_stack_core0 = .;
        . = . + 16384;				/* EL1 stack size */
        __EL1_stack_core0 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
        __EL2_stack_core0 = .;
        __stack_end_core0__ = .;    
    }

	.stack_core1 : {
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS64 call standard */
        __stack_start_core1__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core1 = .;
		. = . + 1024;				/* EL1 stack size */
        __EL1_stack_core1 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
        __EL2_stack_core1 = .;
        __stack_end_core1__ = .;    
    }

	.stack_core2 :	{
        . = ALIGN(16);				 /* Stack must always be aligned to 16 byte boundary AAPCS call standard */
        __stack_start_core2__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core2 = .;
        . = . + 1024;				/* EL1 stack size */
        __EL1_stack_core2 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
        __EL2_stack_core2 = .;
        __stack_end_core2__ = .;  
    }

	.stack_core3 :	{
        . = ALIGN(16);				/* Stack must always be aligned to 16 byte boundary AAPCS call standard */
        __stack_start_core3__ = .;
        . = . + 512;				/* EL0 stack size */
        __EL0_stack_core3 = .;
        . = . + 1024;				/* EL1 stack size */
        __EL1_stack_core3 = .;
        . = . + 512;				/* EL2 stack size (start-up) */
       	__EL2_stack_core3 = .;
       	__stack_end_core3__ = .;  
    }

	/**
	 *	Stack starts at the top of the RAM, and moves down!
	 **/
	. = ALIGN(8); 					/* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
	. = . + 32768;
	_estack = .;

	.heap :	{
     	. = ALIGN(4);
     	__heap_start__ = .;			/* Label in case we want address of heap section start */
    	_end = .; PROVIDE (end = .);/* Any memory from here is free to use so this is end of code and start of heap */
	}

	/*
	* Finally comes everything else. A fun trick here is to put all other 
	* sections into this section, which will be discarded by default.
	*/
	/DISCARD/ : {
		*(*)
	}
}
