OUTPUT_ARCH(arm)
ENTRY(_start)
   /* Specify the memory areas */
   MEMORY
   {
         RAM (xrw)      : ORIGIN = 0x8000 , LENGTH = 0x8000000 /*128MB which is half memory on smallest memory PI, assumes GPU has other half */
    }

SECTIONS
{
.text :
    {
        . = ALIGN(4);
        __text_start__ = .;
        _start = .;
        KEEP(*(.text.startup))     /* This is what the startup code block is called in rpi-smartstart.S */
        *(.text .text.* .gnu.linkonce.t.*)
        *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)
        *(.init .init.*)
        . = ALIGN(4);
        __text_end__ = .;
    } > RAM

.bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
   } > RAM

.rodata :
    {
        . = ALIGN(4);
        __rodata_start__ = .;
        *(.rodata .rodata.*)
        *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
        . = ALIGN(4);
        __rodata_end__ = .;
    } > RAM

.data :
    {
        . = ALIGN(4);      /* Normal data memory is align 4 */
        __data_start__ = .;
        *(.data .data.* .gnu.linkonce.d.*)
        *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end__ = .;
   } > RAM

.data1 :
    {
        . = ALIGN(16);      /* data 1 is align 16 data needed for mailbox exchanges */
        __data1_start__ = .;
        *(.data1 .data1.*)
        *(.rel.data1 .rel.data1.*)
        . = ALIGN(16);
        __data1_end__ = .;
   } > RAM

.stack :
    {
        . = ALIGN(8);  /* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        __stack_start__core0 = .;
        . = . + 512;    /* HYP stack size core 0 */
        __HYP_stack_core0 = .;
        . = . + 32768;    /* SVC stack size core 0 */
        __SVC_stack_core0 = .;
        . = . + 4096;     /* IRQ stack size core 0 */
        __IRQ_stack_core0 = .;
        . = . + 4096;     /* FIQ stack size core 0 */
        __FIQ_stack_core0 = .;
        . = ALIGN(8);
        __stack_end__core0 = .;  
    } > RAM

.stack_core1 :
    {
        . = ALIGN(8);  /* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        __stack_start__core1 = .;
        . = . + 512;    /* HYP stack size core 1 */
        __HYP_stack_core1 = .;
        . = . + 4096;    /* SVC stack size core 1 */
        __SVC_stack_core1 = .;
        . = . + 512;     /* IRQ stack size core 1 */
        __IRQ_stack_core1 = .;
        . = . + 512;     /* FIQ stack size core 1 */
        __FIQ_stack_core1 = .;
        . = ALIGN(8);
        __stack_end__core1 = .;  
    } > RAM

.stack_core2 :
    {
        . = ALIGN(8);  /* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        __stack_start__core2 = .;
        . = . + 512;    /* HYP stack size core 2 */
        __HYP_stack_core2 = .;
        . = . + 4096;    /* SVC stack size core 2 */
        __SVC_stack_core2 = .;
        . = . + 512;     /* IRQ stack size core 2 */
        __IRQ_stack_core2 = .;
        . = . + 512;     /* FIQ stack size core 2 */
        __FIQ_stack_core2 = .;
        . = ALIGN(8);
        __stack_end__core2 = .;  
    } > RAM

.stack_core3 :
    {
        . = ALIGN(8);  /* Stack must always be aligned to 8 byte boundary AAPCS32 call standard */
        __stack_start__core3 = .;
        . = . + 512;    /* HYP stack size core 3 */
        __HYP_stack_core3 = .;
        . = . + 4096;    /* SVC stack size core 3 */
        __SVC_stack_core3 = .;
        . = . + 512;     /* IRQ stack size core 3 */
        __IRQ_stack_core3 = .;
        . = . + 512;     /* FIQ stack size  core 3 */
        __FIQ_stack_core3 = .;
        . = ALIGN(8);
        __stack_end__core3 = .;  
    } > RAM

.ARM.attributes 0 : { *(.ARM.attributes) }

.Heap :
    {
     . = ALIGN(4);
     __heap_start__ = .;
     _end = .; PROVIDE (end = .); /* Any memory from here is free to use so this is end of code and start of heap */
    } > RAM

    /* Remove information from the standard libraries */
    /DISCARD/ :
    {
        /* We cant debug */

        /* so no point to hold arm unwind tables */
        *(.ARM.exidx .ARM.exidx.*)
        *(.rel.ARM.exidx .rel.ARM.exidx.* .gnu.linkonce.armexidx.*)

         /* same with debug frames */
        *(.debug_frame .debug_frame.*)
       
        /* We really don't need comments from compiler either  */
        *(.comment *.comment.*) 

        /* We really don't need the symbols and string tables  */
        *(.symtab .symtab.*)
        *(.strtab .strtab.*)
        *(.shstrtab .shstrtab.*)

        /* remove any standard library hang over bits */
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
        *(.note.GNU-stack)
        *(.gnu_debuglink) 
        *(.gnu.lto_*)
    }

    
}
