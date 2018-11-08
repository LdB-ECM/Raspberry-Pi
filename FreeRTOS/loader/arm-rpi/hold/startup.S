//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//"																			"			
//"      Filename: smartstart32.s											"
//"      Copyright(c): Leon de Boer(LdB) 2017, 2018							"
//"      Version: 2.11														"
//"																			"		
//"***************[ THIS CODE IS FREEWARE UNDER CC Attribution]*************"
//"																			"
//"    This sourcecode is released for the purpose to promote programming   "
//"  on the Raspberry Pi. You may redistribute it and/or modify with the    "
//"  following disclaimer and condition.									"
//"																			"
//"      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO	"
//"   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.			"
//"   Redistributions of source code must retain the copyright notices to	"
//"   maintain the author credit (attribution) .							"
//"																			"
//"*************************************************************************"
//"                                                                         "
//"      This code expands on my earlier SmartStart bootstub assembler for  "
//"  various Pi models. It directly supports multicore operation in C/C++.  "
//"  To do that it provides stack space to each core and provides modified  "
//"  bootloader spinlock that protects from trashing of registers r0-r3     "
//"  As any C/C++ compiler will trash those registers, to use C/C++ in the  "
//"  multicore programming this protection must be done.					"
//"      This is a matching paired AARCH32 stub for the 32bit linker file   "
//"  and carries the same arrangement as AARCH64 pair to ease porting.	    "
//"																			"
//"+++++++++++++++++++++++[ REVISIONS ]+++++++++++++++++++++++++++++++++++++"
//"  1.01 Initial release .. Pi autodetection main aim                      "
//"  1.02 Many functions moved out C to aide 32/64 bit compatability        "
//"  2.01 Futher reductions to bare minmum assembeler code                  "
//"  2.02 Multicore functionality added                                     "
//"  2.03 Timer Irq support added  											"
//"  2.04 David Welch routines GET32/PUT32 compability added                "
//"  2.05 Irq stub made weak linked so it can be easily overridden          "
//"  2.06 Added SmartStartVer											    "
//"  2.07 Fiq stub made weak linked so it can be easily overridden          "
//"  2.08 Added setIrqFuncAddress & setFiqFuncAddress                       "
//"  2.09 Added Hard/Soft float compiler support							"
//"  2.10 Context_switch support added										"
//"  2.11 MiniUart, PL011 Uart and console uart support added				"
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"========================================================================="
;@#				 	 ARM CPU ID CONSTANT DEFINITIONS
;@"========================================================================="
.equ ARM6_CPU_ID, 0x410FB767;							;@ CPU id a BCM2835 reports
.equ ARM7_CPU_ID, 0x410FC073;							;@ CPU id a BCM2836 reports
.equ ARM8_CPU_ID, 0x410FD034;							;@ CPU id a BCM2837 reports

;@"========================================================================="
@#			    ARM CPU MODE CONSTANT DEFINITIONS IN CPSR REGISTER
;@"========================================================================="
.equ ARM_MODE_USR, 0x10;								;@ CPU in USR mode .. Normal User mode
.equ ARM_MODE_FIQ, 0x11;								;@ CPU in FIQ mode .. FIQ Processing
.equ ARM_MODE_IRQ, 0x12;								;@ CPU in IRQ mode .. IRQ Processing
.equ ARM_MODE_SVC, 0x13;								;@ CPU in SVC mode .. Service mode
.equ ARM_MODE_HYP, 0x1A;								;@ CPU in HYP mode .. Hypervisor mode  (ARM7/ARM8 only)
.equ ARM_MODE_UND, 0x1B;								;@ CPU in UND mode .. Undefined Instructions mode
.equ ARM_MODE_SYS, 0x1F;								;@ CPU in SYS mode .. Priviledged Operating System Tasks mode

;@"========================================================================="
@#			          MASK CONSTANTS FOR CPSR REGISTER
;@"========================================================================="
.equ ARM_MODE_MASK, 0x1F;								;@ Mask to clear all but CPU mode bits from cpsr register
.equ ARM_I_BIT,		0x80;								;@ IRQs disabled when set to 1
.equ ARM_F_BIT,		0x40;								;@ FIQs disabled when set to 1


.extern	system_init
.extern __bss_start
.extern __bss_end
.extern vFreeRTOS_ISR
.extern vFreeRTOS_ISR_KLUDGE
.extern vPortYieldProcessor
.extern DisableInterrupts
.extern main
	.section .init
	.globl _start
;; 
_start:
;@"========================================================================="
@#        Grab cpu mode and start address and hold in a high register.
;@"========================================================================="
	mov r12, pc											// Hold boot address in high register R12
	mrs r0, CPSR										// Fetch the cpsr register
	orr r0, r0, #(ARM_I_BIT | ARM_F_BIT)				// Disable Irq/Fiq
	and r11, r0, #ARM_MODE_MASK							// Clear all but CPU mode bits in register r11
;@"========================================================================="
@#      If the cpu is in HYP_MODE(EL2) we will bring it SVC_MODE (EL1).
;@"========================================================================="                        
 	cmp r11, #ARM_MODE_HYP								// Check we are in HYP_MODE											
	bne .NotInHypMode									// Branch if not equal meaning was not in HYP_MODE  
	bic r0, r0, #ARM_MODE_MASK							// Clear the CPU mode bits in register r0							
	orr r0, r0, #ARM_MODE_SVC							// ARM_MODE_SVC bits onto register	
    msr spsr_cxsf,r0									// Hold value in spsr_cxsf
    add lr,pc,#4										// Calculate address of .NotInHypMode label

	/*# I borrowed this trick from Ultibo because ARM6 code running on an ARM7/8 needs this opcode. */
	/*# The ARM6 compiler does not know these instructions as it is not on an ARM6 cpu but we need  */
    /*# the opcode so our ARM6 code can drop an ARM7/ARM8 cpu out of HYP mode. An ARM6 cpu will go  */
	/*# around the code becuase of the initial test for HYP mode. This allow us to write Pi ARM6    */
	/*# code that can run on a Pi with ARM7/ARM8 CPU. Native ARM7/8 compilers already understand    */
	/*# the OPCODE but do not mind that it is entered thise way they just execute the opcode. 	    */        
	.long 0xE12EF30E									// "msr ELR_hyp, lr" Set the address to ELR_hyp
	.long 0xE160006E									// "eret" Elevated return which will exit at .NotInHypMode in SVC_MODE
.NotInHypMode:


;@"========================================================================="
@#      Now store initial CPU boot mode and address we might need later.
;@"========================================================================="
	ldr r1, =RPi_BootAddr								;@ Address to hold Boot address
	sub r12, #8											;@ Subtract op-code offset
	str r12, [r1]										;@ Save the boot address (r12) we started at
	ldr r1, =RPi_CPUBootMode							;@ Memory address to save this CPU boot mode
	str r11, [r1]										;@ Save the boot mode (r11) we started in
;@"========================================================================="
@#           Initial CPU core ready so set CoreReady count to 1.
;@"========================================================================="
	mov r0, #1											;@ Core0 is will be ready
	ldr r1, =RPi_CoresReady								;@ Memory address to save Cores ready
	str r0, [r1]										;@ Set RPi_CoresReady to 1
;@"========================================================================="
@#     Fetch and hold CPU changed mode. If we changed modes this value
@#     will now reflect a change from the original held RPi_CPUBootMode.
;@"========================================================================="
	mrs r2, CPSR
	and r2, r2, #0x1F									;@ Clear all but CPU mode bits in register r2
	ldr r1, =RPi_CPUCurrentMode
	str r2, [r1]										;@ Hold the changed CPU mode
;@"========================================================================="
@#          Read the Arm Main CPUID register => sets RPi_CpuId
;@"========================================================================="
	ldr r1, =RPi_CpuId									;@ Address to hold CPU id
	mrc p15, 0, r0, c0, c0, 0							;@ Read Main ID Register
	str r0, [r1]										;@ Save CPU Id for interface 
;@"========================================================================="
@#             Store the compiler mode in RPi_CompileMode
;@"========================================================================="
	eor r0, r0, r0;										;@ Zero register
.if (__ARM_ARCH == 6)									// Compiling for ARM6
	mov r0, #0x06										;@ Compiled for ARM6 CPU
.endif
.if (__ARM_ARCH == 7)									// Compiling for ARM7
	mov r0, #0x07										;@ Compiled for ARM7 CPU
.endif
.if (__ARM_ARCH == 8)									// Compiling for ARM8
	mov r0, #0x08										;@ Compiled for ARM8 CPU
.endif
	orr r0, r0, #(4 << 5)								;@ Code is setup to support 4 cores	
.if (__ARM_FP == 12)
	orr r0, #0x80000000									;@ Set the hard float flag
.endif	
	ldr r1, =RPi_CompileMode
	str r0, [r1]										;@ Store the compiler mode  
//"========================================================================="
//#        Try Auto-Detect Raspberry PI USB vendor ID at Pi1 position
//#	    LOGIC: All models of Pi have USB as Ethernet/wifi sit off it
//"========================================================================="
	ldr r2, =0x20000000									;@ Default Pi address as 0x20000000
	ldr r3, =0x40000000									;@ Default Pi ARM to GPU alias address as 0x40000000
	ldr r1, =0x4F54280A									;@ This is the Vendor ID a PI1 has
	ldr r0, =0x20980040									;@ USB vendor ID address on a Pi1
	ldr r0, [r0]										;@ Fetch value at vendor ID
	cmp r1, r0											;@ Check if we have the correct vendor ID
	beq .Is_a_Pi1
	ldr r2, =0x3F000000									;@ Change Pi address to Pi2/3 at 0x3F000000
	ldr r3, =0xC0000000									;@ Change Pi ARM to GPU alias address as 0xC0000000
.Is_a_Pi1:
//"========================================================================="
//#		 	   Sets RPi_IO_Base_Addr & RPi_ARM_TO_GPU_Alias
//"========================================================================="
	ldr r1, =RPi_IO_Base_Addr							;@ Address to RPi_IO_Base_Addr
	str r2, [r1]										;@ Save RPi_IO_Base_Addr for interface
	ldr r1, =RPi_ARM_TO_GPU_Alias						;@ Address to RPi_ARM_TO_GPU_Alias
	str r3, [r1]										;@ Save RPi_ARM_TO_GPU_Alias for interface

;@"========================================================================="
@#    ISR table needs position 0x0000 so interrupts can be used, copy it 
;@"========================================================================="
	ldr r0, = _isr_Table								;@ Address of isr_Table
	mov     r1, #0x0000									;@ Destination 0x0000
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

	b reset
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here


;@"*************************************************************************"
@#                      ISR TABLE FOR SMARTSTART			
;@"*************************************************************************"
_isr_Table:
	ldr	pc,reset_handler		;@ 	Processor Reset handler 		-- we will have to force this on the raspi!
	ldr pc,undefined_handler	;@ 	Undefined instruction handler 	-- processors that don't have thumb can emulate thumb!
    ldr pc,swi_handler			;@ 	Software interrupt / TRAP (SVC) -- system SVC handler for switching to kernel mode.
    ldr pc,prefetch_handler		;@ 	Prefetch/abort handler.
    ldr pc,data_handler			;@ 	Data abort handler/
    ldr pc,unused_handler		;@ 	-- Historical from 26-bit addressing ARMs -- was invalid address handler.
    ldr pc,irq_handler			;@ 	IRQ handler
    ldr pc,fiq_handler			;@ 	Fast interrupt handler.

	;@ Here we create an exception address table! This means that reset/hang/irq can be absolute addresses
reset_handler:      .word _start
undefined_handler:  .word undefined_instruction
swi_handler:        .word vPortYieldProcessor
prefetch_handler:   .word prefetch_abort
data_handler:       .word data_abort
unused_handler:     .word unused
irq_handler:        .word vFreeRTOS_ISR
fiq_handler:        .word fiq

reset:
	;@	Set up the various STACK pointers for different CPU modes
    ;@ (PSR_IRQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
    mov r0,#0xD2
    msr cpsr_c,r0
    mov sp,#0x8000

    ;@ (PSR_FIQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
    mov r0,#0xD1
    msr cpsr_c,r0
    mov sp,#0x4000

    ;@ (PSR_SVC_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
    mov r0,#0xD3
    msr cpsr_c,r0
	mov sp,#0x8000000

	ldr r0, =__bss_start
	ldr r1, =__bss_end

	mov r2, #0

zero_loop:
	cmp 	r0,r1
	it		lt
	strlt	r2,[r0], #4
	blt		zero_loop

;@	bl 		DisableInterrupts
   bl  irqBlock
	
	
	;@ 	mov	sp,#0x1000000
	b main									;@ We're ready?? Lets start main execution!

.section .text

undefined_instruction:
	b undefined_instruction

prefetch_abort:
	b prefetch_abort

data_abort:
	b data_abort

unused:
	b unused

fiq:
	b fiq
	
hang:
	b hang

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

.globl dummy
dummy:
    bx lr

;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#		VC4 GPU ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	   
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"========================================================================="
@#		ARMaddrToGPUaddr -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: uint32_t ARMaddrToGPUaddr (void* ARMaddress);
@#		Entry: R0 will have ARMAddress value
;@"========================================================================="
.section .text.ARMaddrToGPUaddr, "ax", %progbits
.balign	4
.globl ARMaddrToGPUaddr;		
.type ARMaddrToGPUaddr, %function
ARMaddrToGPUaddr:
	ldr r1, =RPi_ARM_TO_GPU_Alias						;@ Fetch address of bus alias value
    ldr r1,[r1]											;@ Fetch bus alias	
	orr r0, r0, r1										;@ Create bus address
	bx   lr												;@ Return
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here
.size	ARMaddrToGPUaddr, .-ARMaddrToGPUaddr

;@"========================================================================="
@#		GPUaddrToARMaddr -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: uint32_t GPUaddrToARMaddr (uint32_t BUSaddress);
@#		Entry: R0 will have GPUAddress value
;@"========================================================================="
.section .text.GPUaddrToARMaddr, "ax", %progbits
.balign	4
.globl GPUaddrToARMaddr;		
.type GPUaddrToARMaddr, %function
GPUaddrToARMaddr:
	ldr r1, =RPi_ARM_TO_GPU_Alias						;@ Fetch address of bus alias value
    ldr r1,[r1]											;@ Fetch bus alias	
	bic r0, r0, r1										;@ Create arm address
	bx   lr												;@ Return
.balign	4
.ltorg													;@ Tell assembler ltorg data for this code can go here
.size	GPUaddrToARMaddr, .-GPUaddrToARMaddr

;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#			IRQ AND FIQ HELPER ROUTINES PROVIDE BY RPi-SmartStart API		    
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

;@"========================================================================="
@#		setIrqFuncAddress -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: uintptr_t setIrqFuncAddress (void (*ARMaddress)(void));
@#		Entry: R0 will have ARMAddress value
@#		Return: R0 will return with the previous Irq function address
;@"========================================================================="
.section .text.setIrqFuncAddress, "ax", %progbits
.balign	4
.globl setIrqFuncAddress;
.type setIrqFuncAddress, %function
setIrqFuncAddress:
    cpsid i												// Disable irq interrupts as we are clearly changing call
	ldr r1, =RPi_IrqFuncAddr							// Load address of function to call on interrupt 
	ldr r2, [r1]										// Load current irq call address
	str r0, [r1]										// Store the new function pointer address we were given
	mov r0, r2											// return the old call function
	bx  lr												// Return
.balign	4
.ltorg													// Tell assembler ltorg data for this code can go here
.size	setIrqFuncAddress, .-setIrqFuncAddress

;@"========================================================================="
@#		setFiqFuncAddress -- Composite Pi1, Pi2 & Pi3 code
@#		C Function: uintptr_t setFiqFuncAddress (void (*ARMaddress)(void));
@#		Entry: R0 will have ARMAddress value
@#		Return: R0 will return with the previous Fiq function address
;@"========================================================================="
.section .text.setFiqFuncAddress, "ax", %progbits
.balign	4
.globl setFiqFuncAddress;
.type setFiqFuncAddress, %function
setFiqFuncAddress:
    cpsid i												// Disable irq interrupts as we are clearly changing call
	ldr r1, =RPi_FiqFuncAddr							// Load address of function to call on interrupt 
	ldr r2, [r1]										// Load current fiq call address
	str r0, [r1]										// Store the new function pointer address we were given
	mov r0, r2											// return the old call function
	bx  lr												// Return
.balign	4
.ltorg													// Tell assembler ltorg data for this code can go here
.size	setFiqFuncAddress, .-setFiqFuncAddress

;@"*************************************************************************"
@#          INTERNAL DATA FOR SMARTSTART NOT EXPOSED TO INTERFACE			
;@"*************************************************************************"
.section ".data.notexposed_smartstart32", "aw"
.balign 4
.globl RPi_IrqFuncAddr;
RPi_IrqFuncAddr : .4byte 0;								// Irq Func Address

.globl RPi_FiqFuncAddr;
RPi_FiqFuncAddr : .4byte 0;								// Irq Func Address

;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
@#     	          DATA FOR SMARTSTART32 EXPOSED TO INTERFACE 
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
.section ".data.smartstart32", "aw"
.balign 4

.globl RPi_IO_Base_Addr;								;@ Make sure Pi_IO_Base_Addr label is global
RPi_IO_Base_Addr : .4byte 0;							;@ Peripheral Base addr is 4 byte variable in 32bit mode

.globl RPi_ARM_TO_GPU_Alias;							;@ Make sure RPi_ARM_TO_GPU_Alias label is global
RPi_ARM_TO_GPU_Alias: .4byte 0;							;@ ARM to GPU alias is 4 byte variable in 32bit mode

.globl RPi_BootAddr;									;@ Make sure RPi_BootAddr label is global
RPi_BootAddr : .4byte 0;								;@ CPU boot address is 4 byte variable in 32bit mode

.globl RPi_CoresReady;									;@ Make sure RPi_CoresReady label is global
RPi_CoresReady : .4byte 0;								;@ CPU cores ready for use is 4 byte variable in 32bit mode

.globl RPi_CPUBootMode;									;@ Make sure RPi_CPUBootMode label is global
RPi_CPUBootMode : .4byte 0;								;@ CPU Boot Mode is 4 byte variable in 32bit mode

.globl RPi_CpuId;										;@ Make sure RPi_CpuId label is global
RPi_CpuId : .4byte 0;									;@ CPU Id is 4 byte variable in 32bit mode

.globl RPi_CompileMode;									;@ Make sure RPi_CompileMode label is global
RPi_CompileMode : .4byte 0;								;@ Compile mode is 4 byte variable in 32bit mode

.globl RPi_CPUCurrentMode;								;@ Make sure RPi_CPUCurrentMode label is global
RPi_CPUCurrentMode : .4byte 0;							;@ CPU current Mode is 4 byte variable in 32bit mode

.globl RPi_SmartStartVer;								;@ Make sure RPi_SmartStartVer label is global
RPi_SmartStartVer : .4byte 0x00021010;					;@ SmartStart version is 4 byte variable in 32bit mode
