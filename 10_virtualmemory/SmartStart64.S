;/*"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
;@"																			"			
;@"      Filename: smartstart64.s											"
;@"      Copyright(c): Leon de Boer(LdB) 2017,2018							"
;@"      Version: 2.04														"
;@"																			"		
;@"***************[ THIS CODE IS FREEWARE UNDER CC Attribution]*************"
;@"																            "
;@"     This sourcecode is released for the purpose to promote programming  "
;@"  on the Raspberry Pi. You may redistribute it and/or modify with the    "
;@"  following disclaimer and condition.                                    "
;@"																            "
;@"      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO    "
;@"   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.          "
;@"   Redistributions of source code must retain the copyright notices to   "
;@"   maintain the author credit (attribution) .							"
;@"																			"
;@"*************************************************************************"
;@"                                                                         "
;@"       This code expands on my earlier SmartStart bootstub assembler for "
;@" the Pi3. It directly supports multicore operation in C/C++. To do that  "
;@" it provides stack space to each core and provides a modified bootloader " 
;@" spinlock that protects against registers X0-X7 trashed. As any C/C++    "
;@" 64 bit compiler will trash those registers, to use C/C++ in multicore   "
;@" programming this protection must be done.								"
;@"       This is a matching paired AARCH64 stub for the 64bit linker file  "
;@" and carries the samme arrangement as AARCH32 to ease porting.		    "
;@"																            "
;@"+++++++++++++++++++++++[ REVISIONS ]+++++++++++++++++++++++++++++++++++++"
;@"  1.01 Initial release .. Pi autodetection main aim                      "
;@"  1.02 Many functions moved out C to aide 32/64 bit compatability        "
;@"  2.01 Futher reductions to bare minmum assembeler code                  "
;@"  2.02 Multicore functionality added                                     "
;@"  2.03 Timer Irq support added  											"
;@"  2.04 Semaphore code added												"			
;@"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"*/

.section ".init", "ax", %progbits
.balign	4
.globl _start
_start:	
	adr x12, _start							// Hold boot address in high register R12
	ldr x1, =RPi_BootAddr					// Address of RPi_BootAddr
	str w12, [x1]							// Store the boot address

	ldr	x0, =0x3F000000						// No need to detect address in 64bit mode it has to be 0x3F000000 on Pi3
	ldr x1, =RPi_IO_Base_Addr				// Address of RPi_IO_Base_Addr
	str w0, [x1]							// Store the IO base address

	mov x0, #1								// Multicore support starts as 1 core
	ldr x1, =RPi_CoresReady					// Address of RPi_CoresReady
	str w0, [x1]							// Store the CoresReady count as 1

	mov x0, #0x98							// Compiled for ARM8 CPU in AARCH64 and supports 4 cores
	ldr x1, =RPi_CompileMode				// Address of RPi_CompileMode
	str w0, [x1]							// Store the compiler mode  

	mrs	x0, midr_el1						// Read CPU Id register
	ldr x1, =RPi_CpuId						// Address of RPi_CpuId
	str w0, [x1]							// Store the CPU id 

//"================================================================"
//  Initialize Generic Timers for Core0
//"================================================================"
	mrs	x0, cnthctl_el2
	orr	x0, x0, #0x3						/* Enable EL1 access to timers */
	msr	cnthctl_el2, x0
	msr	cntvoff_el2, xzr
	
	/* Not sure if I want this yet */
	mrs	x0, cntkctl_el1
	orr	x0, x0, #0x3						/* Enable EL0 access to timers */
	msr	cntkctl_el1, x0

//"================================================================"
//  Setup stack pointers for each core and each CPU operation mode
//"================================================================"
multicore_start:
    ldr x2, = __EL2_stack_core0				// Address of EL2_stack_core0 stack pointer value
    ldr x3, = __EL1_stack_core0				// Address of EL1_stack_core0 stack pointer value
    ldr x4, = __EL0_stack_core0				// Address of EL0_stack_core0 stack pointer value
	mrs x6, mpidr_el1						// Read core id on ARM8
	ands x6, x6, #0x3						// Make cpu id bitmask
	beq set_stacks							// Ready to set core 0 stacks
    ldr x2, = __EL2_stack_core1				// Address of EL2_stack_core1 stack pointer value
    ldr x3, = __EL1_stack_core1				// Address of EL1_stack_core1 stack pointer value
    ldr x4, = __EL0_stack_core1				// Address of EL0_stack_core1 stack pointer value
	cmp x6, #1								// Check cpu id for core 1
	beq set_stacks							// Ready to set core 1 stacks
    ldr x2, = __EL2_stack_core2				// Address of EL2_stack_core2 stack pointer value
    ldr x3, = __EL1_stack_core2				// Address of EL1_stack_core2 stack pointer value
    ldr x4, = __EL0_stack_core2				// Address of EL0_stack_core2 stack pointer value
	cmp x6, #2								// Check cpu id for core 2
	beq set_stacks							// Ready to set core 1 stacks
    ldr x2, = __EL2_stack_core3				// Address of EL2_stack_core3 stack pointer value
    ldr x3, = __EL1_stack_core3				// Address of EL1_stack_core3 stack pointer value
    ldr x4, = __EL0_stack_core3				// Address of EL0_stack_core3 stack pointer value
set_stacks:
	mov	sp, x2								/* EL2 stack set */
	msr	sp_el1, x3							/* EL1 stack set */
	msr	sp_el0, x4							/* EL0 stack set */

//"================================================================"
//  Initilize MPID/MPIDR registers for all Cores
//"================================================================"
	mrs	x0, midr_el1
	mrs	x1, mpidr_el1
	msr	vpidr_el2, x0
	msr	vmpidr_el2, x1

//"================================================================"
//  Disable coprocessor traps for all Cores
//"================================================================"
	mov	x0, #0x33ff
	msr	cptr_el2, x0						// Disable coprocessor traps to EL2
	msr	hstr_el2, xzr						// Disable coprocessor traps to EL2
	mov	x0, #3 << 20
	msr	cpacr_el1, x0						// Enable FP/SIMD at EL1

//"================================================================"
//  Initialize HCR_EL2 so EL1 is 64 bits for all Cores
//"================================================================"
	mov	x0, #(1 << 31)						// 64bit EL1
	msr	hcr_el2, x0

//"================================================================"
//  Initialize SCTLR_EL1 for all Cores
//"================================================================"
    /*  RES1 bits (29,28,23,22,20,11) to 1
	 *  RES0 bits (31,30,27,21,17,13,10,6) +
	 *  UCI,EE,EOE,WXN,nTWE,nTWI,UCT,DZE,I,UMA,SED,ITD,
	 *  CP15BEN,SA0,SA,C,A,M to 0 */
	mov	x0, #0x0800
	movk	x0, #0x30d0, lsl #16
	orr    x0, x0, #(0x1 << 2)            // The C bit on (data cache). 
	orr    x0, x0, #(0x1 << 12)           // The I bit on (instruction cache)
	msr	sctlr_el1, x0

//"================================================================"
//  Return to the EL1_SP1 mode from EL2 for all Cores
//"================================================================"
	mov	x0, #0x3c5							// EL1_SP1 | D | A | I | F
	msr	spsr_el2, x0						// Set spsr_el2 with settings
	adr	x0, exit_el1						// Address to exit EL2
	msr	elr_el2, x0							// Set elevated return register
	eret									// Call elevated return
exit_el1:
	mrs x6, mpidr_el1						// Read core id on ARM8
	and x6, x6, #0x3						// Make cpu id bitmask
	cbz x6, cpu0_exit_multicore_park		// Core0 continues on

//"================================================================"
//      Now park Core 1,2,3 into secondary spinloop on BCM2837
//"================================================================"
	ldr x1, =RPi_CoresReady					// Address of CoreReady count
	ldr w0, [x1]							// Load current core count 
	add w0, w0, #1							// Add one as core about to go ready
	str w0, [x1]							// Update CoreReady count
	b  StartSecondarySpin					// Jump to setup secondary spin
cpu0_exit_multicore_park:

//"================================================================"
//  Set vector table for EL1 for Core0 (All others cores parked)
//"================================================================"
    ldr x0, =VectorTable					// Vector table address			
    msr vbar_el1,x0							// Set to EL1 vbar

//"================================================================"
//  About to go to into C kernel clear BSS (Core0 only)
//"================================================================"
	ldr x3, =__bss_end__
	ldr x0, =__bss_start__
	cmp	x0, x3
	bcs	.bss_cleared
.bss_zero_loop:
	str	wzr, [x0], 4
	cmp	x3, x0
	bhi	.bss_zero_loop
.bss_cleared:

//"================================================================"
//			Core0 will bring Core 1,2,3 to secondary spin 
//"================================================================"
#define spin_cpu1 0xe0
	mov x1, #spin_cpu1						// Spin core1 jump address
	ldr x2, =multicore_start				// Function we are going to call
	str	x2, [x1]							// Store the function address to core1
	sev										// Wake core1 up
 	ldr x3, =RPi_CoresReady					// Set CoresReady count address
.WaitCore1ACK:
	ldr	w1, [x3]							// Read CoresReady count
	cmp	w1, #2								// Wait for setting of second core ready
	bne	.WaitCore1ACK						// Core1 not ready so read again
#define spin_cpu2 0xe8
	mov x1, #spin_cpu2						// Spin core2 jump address
	ldr x2, =multicore_start				// Function we are going to call
	str	x2, [x1]							// Store the function address to core2
	sev										// Wake core2 up
 	ldr x3, =RPi_CoresReady					// Set CoresReady count address
.WaitCore2ACK:
	ldr	w1, [x3]							// Read CoresReady count
	cmp	w1, #3								// Wait for setting of third core ready
	bne	.WaitCore2ACK						// Core2 not ready so read again
#define spin_cpu3 0xf0
	mov x1, #spin_cpu3						// Spin core3 jump address
	ldr x2, =multicore_start				// Function we are going to call
	str	x2, [x1]							// Store the function address to core3
	sev										// Wake core3 up
 	ldr x3, =RPi_CoresReady					// Set CoresReady count address
.WaitCore3ACK:
	ldr	w1, [x3]							// Read CoresReady count
	cmp	w1, #4								// Wait for setting of third core ready
	bne	.WaitCore3ACK						// Core3 not ready so read again

//"================================================================"
// Finally that all done Core0 jumps to the C compiler entry point
//"================================================================"
	b main									// Jump out to C kernel 

/*================================================================"
      Just safety incase C code ever returns back
"================================================================*/
hang:
	b hang

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{    Modified bootloader Spin loop but tolerant on registers X0-X7 for C    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
.balign	4
StartSecondarySpin:
	wfe										// This is all I really want processor to sleep
	mov	x2, 204
	movk x2, 0x4000, lsl 16					// Load mailbox0 read address  = 0x400000CC
	mrs x0, MPIDR_EL1						// Fetch core Id
	ubfiz x0, x0, 4, 4						// Multiply core ID x 16
	ldr	w1, [x0, x2]						// Read the mailbox	
	cbz w1, StartSecondarySpin				// If zero spin again
	str	w1, [x0, x2]						// Clear the read address
	uxtw x1, w1								// Expand 32bit addr to 64bit addr
	dmb sy
	blr x1									// Call the function set
	dmb sy
	b StartSecondarySpin					// Loop back to spinlock
.balign	4
.ltorg										/* Tell assembler ltorg data for code above can go here */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			  GLOBAL INTERRUPT CONTROL PROVIDE BY SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//"========================================================================="
//	EnableInterrupts -- AARCH64 Pi3 code
//	C Function: "void EnableInterrupts (void);"
//	Entry: nothing
//	Return: nothing
//"========================================================================="
.section .text.EnableInterrupts, "ax", %progbits
.balign	4
.globl EnableInterrupts;
EnableInterrupts:
	msr daifclr,#2
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	EnableInterrupts, .-EnableInterrupts

//"========================================================================="
//	DisableInterrupts -- AARCH64 Pi3 code
//	C Function: "void DisableInterrupts (void);"
//	Entry: nothing
//	Return: nothing
//"========================================================================="
.section .text.DisableInterrupts, "ax", %progbits
.balign	4
.globl DisableInterrupts;
.type DisableInterrupts, %function
DisableInterrupts:
	msr daifset,#2
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	DisableInterrupts, .-DisableInterrupts


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{				IRQ HELPER ROUTINES PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//"========================================================================="
//	setTimerIrqAddress -- AARCH64 Pi3 code
//	C Function: "TimerIrqHandler setTimerIrqAddress ( TimerIrqHandler* ARMaddress);"
//	Entry: X0 has address for new Irq call
//	Return: X0 has address of lats old Irq call
//"========================================================================="
.section .text.setTimerIrqAddress, "ax", %progbits
.balign	4
.globl setTimerIrqAddress;
.type setTimerIrqAddress, %function
setTimerIrqAddress:
	msr daifset,#2							// Disable irq interrupts as we are clearly changing call
	ldr x1, =RPi_TimerIrqAddr				// Load address of function to call on interrupt 
	ldr x2, [x1]							// Load current irq call address
	str x0, [x1]							// Store the new function pointer address we were given
	mov x0, x2								// return the old call function
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	setTimerIrqAddress, .-setTimerIrqAddress

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			VC4 ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//"================================================================"
//	ARMaddrToGPUaddr -- AARCH64 Pi3 code
//	C Function: "uint32_t ARMaddrToGPUaddr (void* ARMaddress);"
//	Entry: X0 will have ARMAddress value
//	Return: X0 will have the converted GPU address
//"================================================================"
.section .text.ARMaddrToGPUaddr, "ax", %progbits
.balign	4
.globl ARMaddrToGPUaddr;		
.type ARMaddrToGPUaddr, %function
ARMaddrToGPUaddr:
	mov x1, #0xC0000000						// ARM to VC conversion value
	orr x0, x0, x1							// Create bus address
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	ARMaddrToGPUaddr, .-ARMaddrToGPUaddr

//"================================================================"
// GPUaddrToARMaddr -- AARCH64 Pi3 code
// C Function: "uint32_t GPUaddrToARMaddr (uint32_t BUSaddress);"
// Entry: X0 will have GPUAddress value
//	Return: X0 will have the converted ARM address
//"================================================================"
.section .text.GPUaddrToARMaddr, "ax", %progbits
.balign	4
.globl GPUaddrToARMaddr;		
.type GPUaddrToARMaddr, %function
GPUaddrToARMaddr:
	mov x1, #0xC0000000						// ARM to VC conversion value	
	bic x0, x0, x1							// Create arm address
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	GPUaddrToARMaddr, .-GPUaddrToARMaddr

//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//	  RPi-SmartStart Compatability for David Welch CALLS he always uses	    }
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

//"========================================================================="
//	PUT32 -- AARCH64 Pi3 code
//	C Function: "void PUT32 (uint32_t addr, uint32_t value);"
//	Entry: X0 contains address   X1 contains value
//	Return: nothing
//"========================================================================="
.section .text.PUT32, "ax", %progbits
.balign	4
.globl PUT32
.type PUT32, %function
PUT32:
	str w1,[x0]
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	PUT32, .-PUT32

 //"========================================================================="
//	GET32 -- AARCH64 Pi3 code
//	C Function: "uint32_t GET32 (uint32_t addr);"
//	Entry: X0 contains address
//	Return: X) conatins value read
//"========================================================================="
 .section .text.GET32, "ax", %progbits
.balign	4
.globl GET32
.type GET32, %function
GET32:
	mov x1, #0
	mov w1, w0
    ldr w0,[x1]
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	GET32, .-GET32

//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//		 SmartStart API TO GET CORE ID & EXECUTE ROUTINE AT ADDRESS 		    
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

//"========================================================================="
//	GetCoreID -- AARCH64 Pi3 code
//	C Function: "uint8_t GetCoreID (void);"
//	Entry: nothing
//	Return: X0 with the read Core ID
//"========================================================================="
 .section .text.GetCoreID, "ax", %progbits
.balign	4
.globl GetCoreID
.type GetCoreID, %function
GetCoreID:
	mrs x0, MPIDR_EL1						// Fetch core Id
	and x0, x0, #0x3						// Create 2 bit mask of core Id
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	GetCoreID, .-GetCoreID

//"========================================================================="
//	CoreExcute -- AARCH64 Pi3 code
//	C Function: "bool CoreExecute (uint8_t core, void (*func)(void));"
//	Entry: X0 will have core id  X1 will have address of function to run
//	Return: nothing
//"========================================================================="
.section .text.CoreExecute, "ax", %progbits
.balign	4
.globl CoreExecute;
.type CoreExecute, %function
CoreExecute:
    dmb sy
	adrp x2, RPi_CoresReady
	ldr	w2, [x2, #:lo12:RPi_CoresReady]
	and	w0, w0, 255
	cmp	w0, w2
	bcs CoreExecuteFail
	ubfiz x0, x0, 2, 8						// multiply core number x16
	mov	x2, 140
	movk x2, 0x4000, lsl 16					// Load address of mailbox0 write addr = 0x4000008C
	str	w1, [x2, x0, lsl 2]					// Save caller address
	sev
	dmb sy
 	mov	w0, 1
	ret
CoreExecuteFail:
	dmb sy
	mov x0, #0
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	CoreExecute, .-CoreExecute

//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//				SEMAPHORE ROUTINES PROVIDE BY RPi-SmartStart API		    
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

//"========================================================================="
//	semaphore_inc -- AARCH64 Pi3 code
//	C Function: "void semaphore_inc (uint32_t* sem);"
//	Entry: X0 will have semaphore address value
//	Return: nothing
//"========================================================================="
.section .text.semaphore_inc, "ax", %progbits
.balign	4
.globl semaphore_inc;
.type semaphore_inc, %function
semaphore_inc:
      mov   w2, 1
	.p2align 2
.lockloop:
    ldaxr   w1, [x0]
    stxr    w3, w2, [x0]
    cbnz    w3, .lockloop
    cbnz    w1, .lockloop
	dmb ish
    ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	semaphore_inc, .-semaphore_inc

//"========================================================================="
//	semaphore_dec -- Composite Pi1, Pi2 & Pi3 code
//	C Function: "void semaphore_dec (uint32_t* sem);"
//	Entry: X0 will have semaphore address value
//	Return: nothing
//"========================================================================="
.section .text.semaphore_dec, "ax", %progbits
.balign	4
.globl semaphore_dec;
.type semaphore_dec, %function
semaphore_dec:
	stlrb	wzr, [x0]
	dmb ish
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	semaphore_dec, .-semaphore_dec


/* Re-entrant interrupt handler stub */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch10s05.html */
.globl irq_handler
irq_handler:
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	str	x0, [sp, #-16]!
	
	mrs	x1, SPSR_EL1
	mrs	x2, ELR_EL1
	stp	x1, x2, [sp, #-16]!

	
	//  if (IRQ->IRQBasicPending.Timer_IRQ_pending) {				// Check irq on timer is triggered
	//	  ARMTIMER->Clear = 1;										// Write any value to register to clear irq ... PAGE 198
	//	  IRQ->IRQPending1 &= ~0x1;									// Clear timer pending irq bit 0
	//  }
	ldr x0, =RPi_IO_Base_Addr
	ldr	w0, [x0]						// Fetch Pi IO base address
	mov	w3, 45568						// W3 = 0xB200
	add	w1, w0, w3						// W1 = Pi IO base addres + 0xB200
	ldr	w2, [x1]						// W2 = IRQ->IRQBasicPending
	tbz	x2, 0, .TimerIrqNotPending	    // If timer IRQ pending not yet exit		
	mov	w2, 46080						// W2 = 0xB400
	add	w0, w0, w2						// Add Pi Base Addr + 0xB400
	str	w2, [x0, 12]					// Store W2 to IRQPending1 clearing it				
	ldr	w0, [x1, 4]						// W0 = IRQPending1
	and	w0, w0, -2						// Clear timer pending irq bit 0
	str	w0, [x1, 4]						// Write IRQPending1 clearing
.TimerIrqNotPending:

	msr daifclr,#2						// Enable irq interrupts
	ldr x0, =RPi_TimerIrqAddr			// Address to TimerIrqAddr
	ldr x0, [x0]						// Load TimerIrqAddr value
	cbz x0, no_irqset					// If zero no irq set 
	blr x0								// Call Irqhandler that has been set
	msr daifset,#2						// Disable irq interrupts
no_irqset:


	ldp	x1, x2, [sp], #16
	msr	ELR_EL1, x2
	msr	SPSR_EL1, x1

	ldr	x0, [sp], #16
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp	x29, x30, [sp], #16
	eret

/* macro to align handlers every 0x80 bytes */
.macro	vector handler
.balign 0x80
b	\handler
.endm

.balign 0x800
.globl	VectorTable
VectorTable:
	/* from current EL with sp_el0 */
	vector	_start			/* Synchronous */
	vector  hang			/* IRQ */
	vector	hang			/* FIQ */
	vector	hang			/* SErrorStub */

	/* from current EL with sp_elx, x != 0 */
	vector	hang			/* Synchronous */
	vector	irq_handler		/* IRQ */
	vector	hang			/* FIQ */
	vector	hang			/* SErrorStub */

	/* from lower EL, target EL minus 1 is AArch64 */
	vector	hang			/* Synchronous */
	vector  hang			/* IRQ */
	vector	hang			/* FIQ */
	vector	hang			/* SErrorStub */

	/* from lower EL, target EL minus 1 is AArch32 */
	vector	hang			/* Synchronous */
	vector  hang			/* IRQ */
	vector	hang			/* FIQ */
	vector	hang			/* SErrorStub */

/****************************************************************
       	   DATA FOR SMARTSTART64  EXPOSED TO INTERFACE 
****************************************************************/
.section ".data.smartstart64", "aw"
.balign 4

.globl RPi_IO_Base_Addr;				// Make sure Pi_IO_Base_Addr label is global
RPi_IO_Base_Addr : .4byte 0;			// Peripheral Base addr is 4 byte variable in 64bit mode

.globl RPi_BootAddr;					// Make sure RPi_BootAddr label is global
RPi_BootAddr : .4byte 0;				// CPU boot address is 4 byte variable in 64bit mode

.globl RPi_CoresReady;					// Make sure RPi_CoresReady label is global
RPi_CoresReady : .4byte 0;				// CPU cores ready for use is 4 byte variable in 32bit mode

.globl RPi_CPUBootMode;					// Make sure RPi_CPUBootMode label is global
RPi_CPUBootMode : .4byte 0;				// CPU Boot Mode is 4 byte variable in 64bit mode

.globl RPi_CpuId;						// Make sure RPi_CpuId label is global
RPi_CpuId : .4byte 0;					// CPU Id is 4 byte variable in 64bit mode

.globl RPi_CompileMode;					// Make sure RPi_CompileMode label is global
RPi_CompileMode : .4byte 0;				// Compile mode is 4 byte variable in 64bit mode

/****************************************************************
       	   DATA FOR SMARTSTART64  NOT EXPOSED TO INTERFACE 
****************************************************************/
RPi_TimerIrqAddr : .8byte 0;			// Timer Irq Address