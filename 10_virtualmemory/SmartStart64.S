//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//"																			"			
//"      Filename: smartstart64.s											"
//"      Copyright(c): Leon de Boer(LdB) 2017, 2018, 2019					"
//"      Version: 2.14														"
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
//" the Pi3. It directly supports multicore operation in C/C++. To do that  "
//" it provides stack space to each core and provides modified bootloader   " 
//" spinlock that protects against registers X0-X7 trashed. As any C/C++    "
//" 64 bit compiler will trash those registers, to use C/C++ in multicore   "
//" programming this protection must be done.								"
//"       This is a matching paired AARCH64 stub for the 64bit linker file  "
//" and carries the same arrangement as AARCH32 pairs to ease porting.	    "
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
//"  2.12 New FIQ, DAIF flag support added									"
//"  2.13 Graphics routines relocated to there own unit						"
//"  2.14 Multicore task switcher support Added								"		
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

.section ".init", "ax", %progbits
.balign	4
.globl _start
_start:	
//"================================================================"
//  This simply hold startup data for SmartStart its trivial 
//"================================================================"
	adr x12, _start							// Hold boot address in high register R12
	ldr x1, =RPi_BootAddr					// Address of RPi_BootAddr
	str w12, [x1]							// Store the boot address

	ldr	x0, =0x3F000000						// No need to detect address in 64bit mode it has to be 0x3F000000 on Pi3
	ldr x1, =RPi_IO_Base_Addr				// Address of RPi_IO_Base_Addr
	str w0, [x1]							// Store the IO base address

	ldr	x0, =0xC0000000						// No need to detect address in 64bit mode it has to be 0xC0000000 on Pi3
	ldr x1, =RPi_ARM_TO_GPU_Alias			// Address of RPi_IO_Base_Addr
	str w0, [x1]							// Store the IO base address

	mov x0, #1								// Multicore support starts as 1 core
	ldr x1, =RPi_CoresReady					// Address of RPi_CoresReady
	str w0, [x1]							// Store the CoresReady count as 1

	mov x0, #0x98							// Compiled for ARM8 CPU in AARCH64 and supports 4 cores
.if (__ARM_FP == 14)
	orr x0, x0, #0x80000000				    // Set the hard float flag
.endif	
	ldr x1, =RPi_CompileMode				// Address of RPi_CompileMode
	str w0, [x1]							// Store the compiler mode  

	mrs	x0, midr_el1						// Read CPU Id register
	ldr x1, =RPi_CpuId						// Address of RPi_CpuId
	str w0, [x1]							// Store the CPU id 

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
//  Initialize Generic Timers for Core0
//"================================================================"
	mrs	x0, cnthctl_el2
	orr	x0, x0, #0x3						/* Enable EL1 access to timers */
	msr	cnthctl_el2, x0
	msr	cntvoff_el2, xzr
	
//"================================================================"
//  Disable coprocessor traps for all Cores
//"================================================================"
.if (__ARM_FP == 14)		//# If hard float compiling selected
	mov	x0, #0x33ff
	msr	cptr_el2, x0						// Disable coprocessor traps to EL2
	msr	hstr_el2, xzr						// Disable coprocessor traps to EL2
	mov	x0, #3 << 20
	msr	cpacr_el1, x0						// Enable FP/SIMD at EL1
.endif						//# __ARM_FP == 14 hard float on for compiling

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

//"================================================================"
//  Set vector table for EL1 for Cores (Yep all use same vectors)
//"================================================================"
    ldr x0, =VectorTable						
    msr vbar_el1,x0

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
.equ spin_cpu1, 0xe0
	mov x1, #spin_cpu1						// Spin core1 jump address
	ldr x2, =multicore_start				// Function we are going to call
	str	x2, [x1]							// Store the function address to core1
	sev										// Wake core1 up
 	ldr x3, =RPi_CoresReady					// Set CoresReady count address
.WaitCore1ACK:
	ldr	w1, [x3]							// Read CoresReady count
	cmp	w1, #2								// Wait for setting of second core ready
	bne	.WaitCore1ACK						// Core1 not ready so read again
.equ spin_cpu2, 0xe8
	mov x1, #spin_cpu2						// Spin core2 jump address
	ldr x2, =multicore_start				// Function we are going to call
	str	x2, [x1]							// Store the function address to core2
	sev										// Wake core2 up
 	ldr x3, =RPi_CoresReady					// Set CoresReady count address
.WaitCore2ACK:
	ldr	w1, [x3]							// Read CoresReady count
	cmp	w1, #3								// Wait for setting of third core ready
	bne	.WaitCore2ACK						// Core2 not ready so read again
.equ spin_cpu3, 0xf0
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
	mov	x5, 204
	movk x5, 0x4000, lsl 16					// Load mailbox0 read address = 0x400000CC
	mrs x6, MPIDR_EL1						// Fetch core Id
	and x6, x6, #0x3						// Create 2 bit mask of core Id
	ubfiz x6, x6, 4, 4						// multiply core number x16
	ldr w4, [x5, x6]						// Read the mailbox
	cbz x4, StartSecondarySpin				// If zero spin 
	str	w4, [x5, x6]						// Clear the read address	
	ldr x30, =StartSecondarySpin			// Set link register to secondary spin address
    blr x4									// Call address in x4 
	b StartSecondarySpin					// Loop back to spinlock
.balign	4
.ltorg										/* Tell assembler ltorg data for code above can go here */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{							Save Context MACRO								}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
.macro portSAVE_CONTEXT
	/* Save the entire context. */
	STP 	X0, X1, [SP, #-0x10]!
	STP 	X2, X3, [SP, #-0x10]!
	STP 	X4, X5, [SP, #-0x10]!
	STP 	X6, X7, [SP, #-0x10]!
	STP 	X8, X9, [SP, #-0x10]!
	STP 	X10, X11, [SP, #-0x10]!
	STP 	X12, X13, [SP, #-0x10]!
	STP 	X14, X15, [SP, #-0x10]!
	STP 	X16, X17, [SP, #-0x10]!
	STP 	X18, X19, [SP, #-0x10]!
	STP 	X20, X21, [SP, #-0x10]!
	STP 	X22, X23, [SP, #-0x10]!
	STP 	X24, X25, [SP, #-0x10]!
	STP 	X26, X27, [SP, #-0x10]!
	STP 	X28, X29, [SP, #-0x10]!
	STP 	X30, XZR, [SP, #-0x10]!

	/* Save the SPSR. */
	MRS		X3, SPSR_EL1
	MRS		X2, ELR_EL1
	STP 	X2, X3, [SP, #-0x10]!

	/* Fetch core Id and multiply x8 as an offset */
	MRS		X1, MPIDR_EL1
	AND		X1, X1, #0x3
	LSL		X1, X1, #3

	/* Fetch current task stack top and add core id offset */
	LDR 	X0, =RPi_coreCB_PTR
	ADD		X0, X0, X1
	LDR 	X1, [X0]
	LDR     X1, [X1]

	/* Save FPU registers if task has FPU use flag set in pxflags */
	LDR		X3, [x1, #8]			// Load pxFlags
	CMP     X3, 0
    BGE     1f
	STP		Q0, Q1, [SP,#-0x20]!
	STP		Q2, Q3, [SP,#-0x20]!
	STP		Q4, Q5, [SP,#-0x20]!
	STP		Q6, Q7, [SP,#-0x20]!
	STP		Q8, Q9, [SP,#-0x20]!
	STP		Q10, Q11, [SP,#-0x20]!
	STP		Q12, Q13, [SP,#-0x20]!
	STP		Q14, Q15, [SP,#-0x20]!
	STP		Q16, Q17, [SP,#-0x20]!
	STP		Q18, Q19, [SP,#-0x20]!
	STP		Q20, Q21, [SP,#-0x20]!
	STP		Q22, Q23, [SP,#-0x20]!
	STP		Q24, Q25, [SP,#-0x20]!
	STP		Q26, Q27, [SP,#-0x20]!
	STP		Q28, Q29, [SP,#-0x20]!
	STP		Q30, Q31, [SP,#-0x20]!
1:

	/* Now save the new SP value as core RPi_CurrentTaskStackTop */
	MOV 	X0, SP   /* Move SP into X0 for saving. */
	STR 	X0, [X1]
.endm

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{							restore Context MACRO							}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

.macro portRESTORE_CONTEXT
	/* Fetch core Id and multiply x8 as an offset */
	MRS		X1, MPIDR_EL1
	AND		X1, X1, #0x3
	LSL		X1, X1, #3

	/* Set the SP to point to the stack of the task being restored. */
	LDR		X0, =RPi_coreCB_PTR
	ADD		X0, X0, X1
	LDR		X1, [X0]
	LDR     X1, [X1]
	LDR		X0, [X1]
	MOV		SP, X0

	/* Restore FPU registers if task has FPU use flag set in pxflags */
	LDR		X3, [X1, #8]			// Load pxFlags
	CMP     X3, 0
    BGE     1f
	LDP		Q30, Q31, [SP], #0x20
	LDP		Q28, Q29, [SP], #0x20
	LDP		Q26, Q27, [SP], #0x20
	LDP		Q24, Q25, [SP], #0x20
	LDP		Q22, Q23, [SP], #0x20
	LDP		Q20, Q21, [SP], #0x20
	LDP		Q18, Q19, [SP], #0x20
	LDP		Q16, Q17, [SP], #0x20
	LDP		Q14, Q15, [SP], #0x20
	LDP		Q12, Q13, [SP], #0x20
	LDP		Q10, Q11, [SP], #0x20
	LDP		Q8, Q9, [SP], #0x20
	LDP		Q6, Q7, [SP], #0x20
	LDP		Q4, Q5, [SP], #0x20
	LDP		Q2, Q3, [SP], #0x20
	LDP		Q0, Q1, [SP], #0x20
1:
	LDP 	X2, X3, [SP], #0x10  /* SPSR and ELR. */

	/* Restore the SPSR. */
	MSR		SPSR_EL1, X3
	/* Restore the ELR. */
	MSR		ELR_EL1, X2

	LDP 	X30, XZR, [SP], #0x10
	LDP 	X28, X29, [SP], #0x10
	LDP 	X26, X27, [SP], #0x10
	LDP 	X24, X25, [SP], #0x10
	LDP 	X22, X23, [SP], #0x10
	LDP 	X20, X21, [SP], #0x10
	LDP 	X18, X19, [SP], #0x10
	LDP 	X16, X17, [SP], #0x10
	LDP 	X14, X15, [SP], #0x10
	LDP 	X12, X13, [SP], #0x10
	LDP 	X10, X11, [SP], #0x10
	LDP 	X8, X9, [SP], #0x10
	LDP 	X6, X7, [SP], #0x10
	LDP 	X4, X5, [SP], #0x10
	LDP 	X2, X3, [SP], #0x10
	LDP 	X0, X1, [SP], #0x10

	ERET
.endm
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{ DEFAULT IRQ HANDLERS STUB ON WEAK REFERENCE PROVIDE BY RPi-SmartStart API	}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
.weak swi_handler_stub
swi_handler_stub:
	
	portSAVE_CONTEXT									// Save current task context

	MOV X1, SP											// Fetch SP
    AND X1, X1, #0xF									// Ensure 16-byte stack alignment
    SUB SP, SP, X1										// adjust stack as necessary
    STP	X1, XZR, [SP, #-16]!							// Store adjustment 

	MRS	X0, MPIDR_EL1									// Fetch core ID							
	AND	X0, X0, #0x3									// Create core id mask
	BL 	xSchedule										// Reschedule .. aka pick new pxCurrentTask

	LDP	X1, XZR,  [SP], #16								// Reload adjustment
    ADD SP, SP, X1										// Un-adjust stack

	portRESTORE_CONTEXT									// Restore new current task context and return

	/* code should never reach this deadloop */
	B		.


.weak irq_handler_stub
irq_handler_stub:
	portSAVE_CONTEXT									// Save current task context

	MOV X1, SP											// Fetch SP
    AND X1, X1, #0xF									// Ensure 16-byte stack alignment
    SUB SP, SP, X1										// adjust stack as necessary
    STP	X1, XZR, [SP, #-16]!							// Store adjustment

	BL xTickISR											// Timer tick ISR

	LDP	X1, XZR,  [SP], #16								// Reload adjustment
    ADD SP, SP, X1										// Un-adjust stack

	portRESTORE_CONTEXT									// Restore new current task context and return

	/* code should never reach this deadloop */
	B .


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{  DEFAULT FIQ HANDLER STUB ON WEAK REFERENCE PROVIDE BY RPi-SmartStart API	}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* Non re-entrant fiq interrupt handler stub */
/* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch10s05.html */
/* We assume interrupt code has no FPU use, if it does user must save FPU registers  */
/* So we will need to save registers x0-x7, x8-x15, x16-x18, x29, x30, 				 */
.weak fiq_handler_stub
fiq_handler_stub:	
	stp	x29, x30, [sp, #-16]!							// Store frame pointer and Link register
	stp	x18, x19,  [sp, #-16]!							// Save register x18, as C will trash it.. 19 just there as a pair
	stp	x16, x17,  [sp, #-16]!							// Save register x16, x17 as C will trash them
	stp	x14, x15,  [sp, #-16]!							// Save register x14, x15 as C will trash them
	stp	x12, x13,  [sp, #-16]!							// Save register x12, x13 as C will trash them
	stp	x10, x11,  [sp, #-16]!							// Save register x10, x11 as C will trash them
	stp	x8, x9,  [sp, #-16]!							// Save register x8, x9 as C will trash them
	stp	x6, x7,  [sp, #-16]!							// Save register x6, x7 as C will trash them
	stp	x4, x5,  [sp, #-16]!							// Save register x4, x5 as C will trash them
	stp	x2, x3,  [sp, #-16]!							// Save register x2, x3 as C will trash them
	stp	x0, x1,  [sp, #-16]!							// Save register x0, x1 as C will trash them

	mov x1, sp
    and x1, x1, #0xF									// Ensure 16-byte stack alignment
    sub sp, sp, x1										// adjust stack as necessary
    stp	x1, xzr, [sp, #-16]!							// Store adjustment

	ldr x0, =RPi_FiqFuncAddr							// Address to FiqFuncAddr
	ldr x0, [x0]										// Load FiqFuncAddr value
	cbz x0, no_fiqset									// If zero no fiq function set so ignore 
	blr x0												// Call Irqhandler that has been set
no_fiqset:

	ldp	x1, xzr,  [sp], #16								// Reload adjustment
    add sp, sp, x1										// Un-adjust stack

	ldp	x0, x1,  [sp], #16								// Restore register x0, x1
	ldp	x2, x3,  [sp], #16								// Restore register x2, x3
	ldp	x4, x5,  [sp], #16								// Restore register x4, x5
	ldp	x6, x7,  [sp], #16								// Restore register x6, x7
	ldp	x8, x9,  [sp], #16								// Restore register x8, x9
	ldp	x10, x11,  [sp], #16							// Restore register x10, x11
	ldp	x12, x13,  [sp], #16							// Restore register x12, x13
	ldp	x14, x15,  [sp], #16							// Restore register x14, x15
	ldp	x16, x17,  [sp], #16							// Restore register x16, x17
	ldp	x18, x19,  [sp], #16							// Restore register x18, x19 
    ldp x29, x30, [sp], #16								// Restore frame pointer and Link register
	eret



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{				  VECTOR TABLE IN USE BY RPi-SmartStart API					}	
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* macro to align handlers every 0x80 bytes */
.macro	vector handler
.balign 0x80
b	\handler
.endm


.balign 0x800
.globl	VectorTable
VectorTable:
	/* from current EL with sp_el0 */
	vector	hang			 /* Synchronous */
	vector  hang			 /* IRQ */
	vector	hang			 /* FIQ */
	vector	hang			 /* SErrorStub */

	/* from current EL with sp_elx, x != 0 */
	vector	swi_handler_stub /* Synchronous */
	vector	irq_handler_stub /* IRQ */
	vector	fiq_handler_stub /* FIQ */
	vector	hang			 /* SErrorStub */

	/* from lower EL, target EL minus 1 is AArch64 */
	vector	hang			 /* Synchronous */
	vector  hang			 /* IRQ */
	vector	hang			 /* FIQ */
	vector	hang			 /* SErrorStub */

	/* from lower EL, target EL minus 1 is AArch32 */
	vector	hang			/* Synchronous */
	vector  hang			/* IRQ */
	vector	hang			/* FIQ */
	vector	hang			/* SErrorStub */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			GLOBAL INTERRUPT CONTROL PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* "PROVIDE C FUNCTION: void EnableInterrupts (void);" */
.section .text.EnableInterrupts, "ax", %progbits
.balign	4
.globl EnableInterrupts
.type EnableInterrupts, %function
EnableInterrupts:
	msr daifclr,#2
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	EnableInterrupts, .-EnableInterrupts

/* "PROVIDE C FUNCTION: void DisableInterrupts (void);" */
.section .text.DisableInterrupts, "ax", %progbits
.balign	4
.globl DisableInterrupts
.type DisableInterrupts, %function
DisableInterrupts:
	msr daifset,#2
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	DisableInterrupts, .-DisableInterrupts

/* "PROVIDE C FUNCTION: void EnableFIQ (void);" */
.section .text.EnableFIQ, "ax", %progbits
.balign	4
.globl EnableFIQ
.type EnableFIQ, %function
EnableFIQ:
	msr daifclr, #1
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	EnableFIQ, .-EnableFIQ

/* "PROVIDE C FUNCTION: void DisableFIQ (void);" */
.section .text.DisableFIQ, "ax", %progbits
.balign	4
.globl DisableFIQ
.type DisableFIQ, %function
DisableFIQ:
	msr daifset, #1
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	DisableFIQ, .-DisableFIQ

/* "PROVIDE C FUNCTION: unsigned long getDAIF (void);" */
.section .text.getDAIF, "ax", %progbits
.balign	4
.globl getDAIF
.type getDAIF, %function
getDAIF:
	mrs x0, daif
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	getDAIF, .-getDAIF

/* "PROVIDE C FUNCTION: void setDAIF (unsigned long flags);" */
.section .text.setDAIF, "ax", %progbits
.balign	4
.globl setDAIF
.type setDAIF, %function
setDAIF:
	msr daif, x0
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	setDAIF, .-setDAIF

//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//				RPi-SmartStart API TO MULTICORE FUNCTIONS
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

/* "PROVIDE C FUNCTION: uint8_t getCoreID (void);" */
.section .text.getCoreID, "ax", %progbits
.balign	4
.globl getCoreID		
.type getCoreID, %function
getCoreID:
	mrs x0, MPIDR_EL1						// Read the CORE id register
	and x0, x0, #3							// Create a valid core ID mask (0..3)
	ret	
.balign	4
.ltorg
.size	getCoreID, .-getCoreID

/* "PROVIDE C FUNCTION: bool CoreExecute (uint8_t core, void (*func)(void));" */
.section .text.CoreExecute, "ax", %progbits
.balign	4
.globl CoreExecute;
.type CoreExecute, %function
CoreExecute:
	adrp x2, RPi_CoresReady
	ldr	w2, [x2, #:lo12:RPi_CoresReady]		// Fetch cores ready count
	and	w0, w0, 255
	cmp	w0, w2								// Check core requested id is less than coresready
	bcs CoreExecuteFail
	ubfiz x0, x0, 4, 4						// multiply core number x16
	mov	x2, 140
	movk x2, 0x4000, lsl 16					// Load address of mailbox0 write addr = 0x4000008C
	str	w1, [x2, x0]						// Save caller func address
	sev										// Wake any sleeping core to read mailbox
 	mov	x0, #1								// Return success
	ret
CoreExecuteFail:
	mov x0, #0								// Return failure
	ret
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	CoreExecute, .-CoreExecute


//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//	  RPi-SmartStart Compatability for David Welch CALLS he always uses	    
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

/* "PROVIDE C FUNCTION: void PUT32 (uint32_t addr, uint32_t value);" */
.section .text.PUT32, "ax", %progbits
.balign	4
.globl PUT32
.type PUT32, %function
PUT32:
	str w1,[x0]								// Half word write of value the address
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	PUT32, .-PUT32

/* "PROVIDE C FUNCTION: uint32_t GET32 (uint32_t addr);" */
 .section .text.GET32, "ax", %progbits
.balign	4
.globl GET32
.type GET32, %function
GET32:
	uxtw x1, w0								// Expand 32bit addr to 64bit addr
    ldr w0,[x1]								// Now read the address
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	GET32, .-GET32


//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//	        EL0 TIMER HELPER ROUTINES PROVIDE BY RPi-SmartStart API	    
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"


/* "PROVIDE C FUNCTION: RegType_t EL0_Timer_Frequency (void);" */
 .section .text.EL0_Timer_Frequency, "ax", %progbits
.balign	4
.globl EL0_Timer_Frequency
.type EL0_Timer_Frequency, %function
EL0_Timer_Frequency:
    mrs x0, CNTFRQ_EL0						// Read the CNTFRQ_EL0 register
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	EL0_Timer_Frequency, .-EL0_Timer_Frequency

/* "PROVIDE C FUNCTION: void EL0_Timer_Set (RegType_t nextCount);" */
 .section .text.EL0_Timer_Set, "ax", %progbits
.balign	4
.globl EL0_Timer_Set
.type EL0_Timer_Set, %function
EL0_Timer_Set:
	mrs x1, CNTPCT_EL0						// Read the EL0 countdown timer
	add x1, x1, x0							// Add nextCount timer count
	msr CNTP_CVAL_EL0, x1					// Update the EL0 countdown timer
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	EL0_Timer_Set, .-EL0_Timer_Set

/* "PROVIDE C FUNCTION: bool EL0_Timer_Irq_Setup (void);" */
 .section .text.EL0_Timer_Irq_Setup, "ax", %progbits
.balign	4
.globl EL0_Timer_Irq_Setup
.type EL0_Timer_Irq_Setup, %function
EL0_Timer_Irq_Setup:
	MOV		X2, #0x40000000					// QA7 start address
	MRS		X0, MPIDR_EL1					// Fetch core ID
	AND		X0, X0, #0x3					// Create core id mask
	LSL		X0, X0, #2						// Multiply x4 for pointer offset	
	ADD		X3, X0, X2						// X3 = QA7 register address 
	MOV		W1, #0x80000000					// X1 = divisor of 0x80000000
	STR		W1, [X3, #8]					// QA7 prescaler = 0x80000000
	LDR		W2, [X3, #64]					// Read Core? timer Interrupt control 
	ORR		W2, W2, #2						// nCNTPNSIRQ IRQ enable on core
	STR		W2, [X3, #64]					// Write Core? timer Interrupt control
	MOV		X0, #1							// Return true
	MSR		CNTP_CTL_EL0, X0				// Enable EL0 timer
	RET									// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	EL0_Timer_Irq_Setup, .-EL0_Timer_Irq_Setup
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			VC4 ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* "PROVIDE C FUNCTION: uint32_t ARMaddrToGPUaddr (void* ARMaddress);" */
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


/* "PROVIDE C FUNCTION: uint32_t GPUaddrToARMaddr (uint32_t BUSaddress);" */
.section .text.GPUaddrToARMaddr, "ax", %progbits
.balign	4
.globl GPUaddrToARMaddr;		
.type GPUaddrToARMaddr, %function
GPUaddrToARMaddr:
	mov x1, ~0xC0000000						// ARM to VC conversion value	
	and x0, x0, x1							// Create arm address
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	GPUaddrToARMaddr, .-GPUaddrToARMaddr

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			FIQ HELPER ROUTINES PROVIDE BY RPi-SmartStart API		        }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* "PROVIDE C FUNCTION: uintptr_t setFiqFuncAddress (void (*ARMaddress)(void));" */
.section .text.setFiqFuncAddress, "ax", %progbits
.balign	4
.globl setFiqFuncAddress;
.type setFiqFuncAddress, %function
setFiqFuncAddress:
	msr daifset,#1							// Disable fiq interrupts as we are clearly changing call
	ldr x1, =RPi_FiqFuncAddr				// Load address of function to call on interrupt 
	ldr x2, [x1]							// Load current fiq call address
	str x0, [x1]							// Store the new function pointer address we were given
	mov x0, x2								// return the old call function
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	setFiqFuncAddress, .-setFiqFuncAddress


.section .text.getSP, "ax", %progbits
.balign	4
.globl getSP;
.type getSP, %function
getSP:
	mov x0, sp
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	.getSP, .-getSP

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{	 	TASK SWITCHER HELPER ROUTINES PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* "PROVIDE C Function: void xStartFirstTask( void );" */
.section .text.xStartFirstTask, "ax", %progbits
.balign	4
.globl xStartFirstTask;
.type xStartFirstTask, %function
xStartFirstTask:
	portRESTORE_CONTEXT						// This will restore current task which will jump start the task
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	.xStartFirstTask, .-xStartFirstTask

/* "PROVIDE C FUNCTION: RegType_t* taskInitialiseStack (RegType_t* pxTopOfStack, TaskFunction_t pxCode, void* pvParameters);" */
.section .text.taskInitialiseStack, "ax", %progbits
.balign	4
.globl taskInitialiseStack;
.type taskInitialiseStack, %function
taskInitialiseStack:
	mov x3, #0x0101010101010101
	str x3, [x0, #-8]!           //  X1
	str x2, [x0, #-8]!           //  X0 = pvParameters
	mov x3, #0x0303030303030303
	str x3, [x0, #-8]!           //  X3
	mov x3, #0x0202020202020202
	str x3, [x0, #-8]!           //  X2
	ldr x3, #=0x0505050505050505
	str x3, [x0, #-8]!           //  X5
	mov x3, #0x0404040404040404
	str x3, [x0, #-8]!           //  X4
	mov x3, #0x0707070707070707
	str x3, [x0, #-8]!           //  X7
	mov x3, #0x0606060606060606
	str x3, [x0, #-8]!           //  X6
	ldr x3, #=0x0909090909090909
	str x3, [x0, #-8]!           //  X9
	mov x3, #0x0808080808080808
	str x3, [x0, #-8]!           //  X8
	mov x3, #0x1111111111111111
	str x3, [x0, #-8]!           //  X11
	mov x3, #0x1010101010101010
	str x3, [x0, #-8]!           //  X10
	ldr x3, #=0x1313131313131313
	str x3, [x0, #-8]!           //  X13
	ldr x3, #=0x1212121212121212
	str x3, [x0, #-8]!           //  X12
	ldr x3, #=0x1515151515151515
	str x3, [x0, #-8]!           //  X15
	ldr x3, #=0x1414141414141414
	str x3, [x0, #-8]!           //  X14
	ldr x3, #=0x1717171717171717
	str x3, [x0, #-8]!           //  X17
	ldr x3, #=0x1616161616161616
	str x3, [x0, #-8]!           //  X16
	ldr x3, #=0x1919191919191919
	str x3, [x0, #-8]!           //  X19
	mov x3, #0x1818181818181818
	str x3, [x0, #-8]!           //  X18
	ldr x3, #=0x2121212121212121
	str x3, [x0, #-8]!           //  X21
	mov x3, #0x2020202020202020
	str x3, [x0, #-8]!           //  X20
	ldr x3, #=0x2323232323232323
	str x3, [x0, #-8]!           //  X23
	mov x3, #0x2222222222222222
	str x3, [x0, #-8]!           //  X22
	ldr x3, #=0x2525252525252525
	str x3, [x0, #-8]!           //  X25
	ldr x3, #=0x2424242424242424
	str x3, [x0, #-8]!           //  X24
	ldr x3, #=0x2727272727272727
	str x3, [x0, #-8]!           //  X27
	ldr x3, #=0x2626262626262626
	str x3, [x0, #-8]!           //  X26
	ldr x3, #=0x2929292929292929
	str x3, [x0, #-8]!           //  X29
	ldr x3, #=0x2828282828282828
	str x3, [x0, #-8]!           //  X28
	mov x3, #0x0000000000000000
	str x3, [x0, #-8]!           //  XZR
	mov x3, #0x0000000000000000
	str x3, [x0, #-8]!           //  X30
	mov x3, # 0x345
	str x3, [x0, #-8]!           // portINITIAL_PSTATE
	str x1, [x0, #-8]!           // pxCode address
	ret										// Return
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	.taskInitialiseStack, .-taskInitialiseStack

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			MMU HELPER ROUTINES PROVIDE BY RPi-SmartStart API			    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
.equ MT_DEVICE_NGNRNE,	0
.equ MT_DEVICE_NGNRE,	1
.equ MT_DEVICE_GRE,		2
.equ MT_NORMAL_NC,		3
.equ MT_NORMAL,		    4
.equ MAIR1VAL, ( (0x00ul << (MT_DEVICE_NGNRNE * 8)) |\
                 (0x04ul << (MT_DEVICE_NGNRE * 8)) |\
				 (0x0cul << (MT_DEVICE_GRE * 8)) |\
                 (0x44ul << (MT_NORMAL_NC * 8)) |\
				 (0xfful << (MT_NORMAL * 8)) )

   // Specify mapping characteristics in translate control register
#define TCREL1VAL  ( (0b00LL << 37) |   /* TBI=0, no tagging */\
					 (0b000LL << 32) |  /* IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit */\
					 (0b10LL << 30)  |  /* TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0 */\
					 (0b11LL << 28)  |  /* SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
					 (0b01LL << 26)  |  /* ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b01LL << 24)  |  /* IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b0LL  << 23)  |  /* EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 = generate fault */\
					 (25LL   << 16)  |  /* T1SZ=25 (512G) ... The region size is 2 POWER (64-T1SZ) bytes */\
					 (0b00LL << 14)  |  /* TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1 */\
					 (0b11LL << 12)  |  /* SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
					 (0b01LL << 10)  |  /* ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b01LL << 8)   |  /* IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
					 (0b0LL  << 7)   |  /* EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 = generate fault */\
					 (25LL   << 0) ) 	/* T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes */
				     
#define SCTLREL1VAL ( (0xC00800) |		/* set mandatory reserved bits */\
					  (1 << 12)  |      /* I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1 */\
					  (1 << 4)   |		/* SA0, tack Alignment Check Enable for EL0 */\
					  (1 << 3)   |		/* SA, Stack Alignment Check Enable */\
					  (1 << 2)   |		/* C, Data cache enable. This is an enable bit for data caches at EL0 and EL1 */\
					  (1 << 1)   |		/* A, Alignment check enable bit */\
					  (1 << 0) )		/* set M, enable MMU */

/* "PROVIDE C Function: void enable_mmu_tables ( RegType_t map1to1, RegType_t virtalmap);" */
.section .text.enable_mmu_tables, "ax", %progbits
.balign	4
.globl enable_mmu_tables;
.type enable_mmu_tables, %function
enable_mmu_tables:
	dsb sy

	/* Set the memattrs values into mair_el1*/
	ldr x2, =MAIR1VAL
    msr mair_el1, x2

	/* Bring both tables online and execute memory barrier */
	msr ttbr0_el1, x0
	msr ttbr1_el1, x1
	isb

	ldr x0, =TCREL1VAL
	msr tcr_el1, x0
	isb

	mrs x0, sctlr_el1
	ldr x1, =SCTLREL1VAL
	orr x0, x0, x1
	msr sctlr_el1, x0

	ret									
.balign	4
.ltorg										// Tell assembler ltorg data for this code can go here
.size	.enable_mmu_tables, .-enable_mmu_tables

//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
//				SEMAPHORE ROUTINES PROVIDE BY RPi-SmartStart API		    
//"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

//"========================================================================="
//	semaphore_take -- AARCH64 Pi3 code
//	C Function: "void semaphore_take (uint32_t* sem);"
//	Entry: X0 will have semaphore address value
//	Return: nothing
//"========================================================================="
.section .text.semaphore_take, "ax", %progbits
.balign	4
.globl semaphore_take;
.type semaphore_take, %function
semaphore_take:
     	mov   w2, 1
    	ldaxr   w1, [x0]
    	stxr    w3, w2, [x0]
    	cbnz    w3, semaphore_take
    	cbnz    w1, semaphore_take
	dmb ish
   	 ret
.size	semaphore_take, .-semaphore_take

//"========================================================================="
//	semaphore_give -- AARCH64 Pi3 code
//	C Function: "void semaphore_give (uint32_t* sem);"
//	Entry: X0 will have semaphore address value
//	Return: nothing
//"========================================================================="
.section .text.semaphore_give, "ax", %progbits
.balign	4
.globl semaphore_give;
.type semaphore_give, %function
semaphore_give:
	stlrb	wzr, [x0]
	dmb ish
	ret
.size	semaphore_give, .-semaphore_give

/****************************************************************
       	   DATA FOR SMARTSTART64  NOT EXPOSED TO INTERFACE 
****************************************************************/
.section ".data.notexposed_smartstart64", "aw"
.balign 8
RPi_FiqFuncAddr : .8byte 0;				// Fiq Function Address

/****************************************************************
       	   DATA FOR SMARTSTART64  EXPOSED TO INTERFACE 
****************************************************************/
.section ".data.smartstart64", "aw"
.balign 8

.globl RPi_IO_Base_Addr;				// Make sure Pi_IO_Base_Addr label is global
RPi_IO_Base_Addr : .4byte 0;			// Peripheral Base addr is 4 byte variable in 64bit mode

.globl RPi_ARM_TO_GPU_Alias;			// Make sure RPi_ARM_TO_GPU_Alias label is global
RPi_ARM_TO_GPU_Alias: .4byte 0;			// ARM to GPU alias is 4 byte variable in 32bit mode

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

.globl RPi_SmartStartVer;				// Make sure RPi_SmartStartVer label is global
RPi_SmartStartVer : .4byte 0x00020104;  // SmartStart version is 4 byte variable in 32bit mode

.balign 8
.globl RPi_coreCB_PTR;
RPi_coreCB_PTR: 
.8byte 0x0;								// Core 0 Control Block pointer
.8byte 0x0;								// Core 1 Control Block pointer	
.8byte 0x0;								// Core 2 Control Block pointer
.8byte 0x0;								// Core 3 Control Block pointer