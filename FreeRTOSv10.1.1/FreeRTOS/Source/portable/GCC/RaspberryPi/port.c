/*
 *	Raspberry Pi Porting layer for FreeRTOS.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "rpi-smartstart.h"
#include "rpi-irq.h"

 /* Constants required to setup the task context. */
#define portNO_CRITICAL_NESTING					( 0 )
#define portNO_FLOATING_POINT_CONTEXT			( 0 )

#if __aarch64__ == 1
#define portINITIAL_PSTATE						( 0x345 )

/* Saved as part of the task context.  If ullPortTaskHasFPUContext is non-zero
then floating point context must be saved and restored for the task. */
uint64_t ulTaskHasFPUContext = pdFALSE;

/* Counts the interrupt nesting depth.  A context switch is only performed if if the nesting depth is 0. */
volatile uint64_t ulCriticalNesting = 9999;

#else
#define portINITIAL_SPSR						( ( portSTACK_TYPE ) 0x1f ) /* System mode, ARM mode, interrupts enabled. */
#define portTHUMB_MODE_BIT						( ( portSTACK_TYPE ) 0x20 )
#define portINSTRUCTION_SIZE					( ( portSTACK_TYPE ) 4 )

/* Saved as part of the task context.  If ullPortTaskHasFPUContext is non-zero
then floating point context must be saved and restored for the task. */
uint32_t ulTaskHasFPUContext = pdFALSE;

/* Counts the interrupt nesting depth.  A context switch is only performed if if the nesting depth is 0. */
volatile uint32_t ulCriticalNesting = 9999;
#endif

/*-----------------------------------------------------------*/

/* Setup the timer to generate the tick interrupts. */
static void prvSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/
#if __aarch64__ == 1
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters)
{
	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro. */

	/* First all the general purpose registers. */
	pxTopOfStack--;
	*pxTopOfStack = 0x0101010101010101ULL;	/* R1 */
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)pvParameters; /* R0 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0303030303030303ULL;	/* R3 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0202020202020202ULL;	/* R2 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0505050505050505ULL;	/* R5 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0404040404040404ULL;	/* R4 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0707070707070707ULL;	/* R7 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0606060606060606ULL;	/* R6 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0909090909090909ULL;	/* R9 */
	pxTopOfStack--;
	*pxTopOfStack = 0x0808080808080808ULL;	/* R8 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1111111111111111ULL;	/* R11 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1010101010101010ULL;	/* R10 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1313131313131313ULL;	/* R13 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1212121212121212ULL;	/* R12 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1515151515151515ULL;	/* R15 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1414141414141414ULL;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1717171717171717ULL;	/* R17 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1616161616161616ULL;	/* R16 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1919191919191919ULL;	/* R19 */
	pxTopOfStack--;
	*pxTopOfStack = 0x1818181818181818ULL;	/* R18 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2121212121212121ULL;	/* R21 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2020202020202020ULL;	/* R20 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2323232323232323ULL;	/* R23 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2222222222222222ULL;	/* R22 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2525252525252525ULL;	/* R25 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2424242424242424ULL;	/* R24 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2727272727272727ULL;	/* R27 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2626262626262626ULL;	/* R26 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2929292929292929ULL;	/* R29 */
	pxTopOfStack--;
	*pxTopOfStack = 0x2828282828282828ULL;	/* R28 */
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)0x00;	/* XZR - has no effect, used so there are an even number of registers. */
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)0x00;	/* R30 - procedure call link register. */
	pxTopOfStack--;

	*pxTopOfStack = portINITIAL_PSTATE;
	pxTopOfStack--;

	*pxTopOfStack = (StackType_t)pxCode; /* Exception return address. */
	pxTopOfStack--;

	/* The task will start with a critical nesting count of 0 as interrupts are
	enabled. */
	*pxTopOfStack = portNO_CRITICAL_NESTING;
	pxTopOfStack--;

	/* The task will start without a floating point context.  A task that uses
	the floating point hardware must call vPortTaskUsesFPU() before executing
	any floating point instructions. */
	*pxTopOfStack = portNO_FLOATING_POINT_CONTEXT;

	return pxTopOfStack;

}
#else
portSTACK_TYPE *pxPortInitialiseStack(portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters)
{
	portSTACK_TYPE *pxOriginalTOS;

	pxOriginalTOS = pxTopOfStack;

	/* To ensure asserts in tasks.c don't fail, although in this case the assert
	is not really required. */
	pxTopOfStack--;

	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro. */

	/* First on the stack is the return address - which in this case is the
	start of the task.  The offset is added to make the return address appear
	as it would within an IRQ ISR. */
	*pxTopOfStack = (portSTACK_TYPE)pxCode + portINSTRUCTION_SIZE;
	pxTopOfStack--;

	*pxTopOfStack = (portSTACK_TYPE)0xaaaaaaaa;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)pxOriginalTOS; /* Stack used when task starts goes in R13. */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x12121212;	/* R12 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x11111111;	/* R11 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x10101010;	/* R10 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x09090909;	/* R9 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x08080808;	/* R8 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x07070707;	/* R7 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x06060606;	/* R6 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x05050505;	/* R5 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x00000000;	/* R4  Must be zero see the align 8 issue in portSaveContext */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x03030303;	/* R3 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x02020202;	/* R2 */
	pxTopOfStack--;
	*pxTopOfStack = (portSTACK_TYPE)0x01010101;	/* R1 */
	pxTopOfStack--;

	/* When the task starts it will expect to find the function parameter in
	R0. */
	*pxTopOfStack = (portSTACK_TYPE)pvParameters; /* R0 */
	pxTopOfStack--;

	/* The last thing onto the stack is the status register, which is set for
	system mode, with interrupts enabled. */
	*pxTopOfStack = (portSTACK_TYPE)portINITIAL_SPSR;

	if (((unsigned long)pxCode & 0x01UL) != 0x00)
	{
		/* We want the task to start in thumb mode. */
		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}

	pxTopOfStack--;

	/* Some optimisation levels use the stack differently to others.  This
	means the interrupt flags cannot always be stored on the stack and will
	instead be stored in a variable, which is then saved as part of the
	tasks context. */
	*pxTopOfStack = portNO_CRITICAL_NESTING;

	return pxTopOfStack;
}
#endif
/*-----------------------------------------------------------*/

extern void restore_context (void);
portBASE_TYPE xPortStartScheduler( void )
{
	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	prvSetupTimerInterrupt();

	/* Start the first task. */
	restore_context();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the ARM port will require this function as there
	is nothing to return to.  */
}
/*-----------------------------------------------------------*/

/*
 *	This is the TICK interrupt service routine, note. no SAVE/RESTORE_CONTEXT here
 *	as thats done in the bottom-half of the ISR.
 *
 *	See bt_interrupts.c in the RaspberryPi Drivers folder.
 */
void vTickISR (uint8_t coreNum, void *pParam )
{
	(void)coreNum;
	(void)pParam;
	xTaskIncrementTick();

	#if configUSE_PREEMPTION == 1
	vTaskSwitchContext();
	#endif

}

static void prvSetupTimerInterrupt( void )
{
	DisableInterrupts();											// Make sure interrupts are off while we do irq registration
	irqRegisterHandler(0, &vTickISR, &ClearTimerIrq, NULL);			// Register the handler 
	TimerIrqSetup((1000000/configTICK_RATE_HZ));					// Peripheral clock is 1Mhz so divid by frequency we want
	EnableInterrupts();												// Enable interrupts
}
/*-----------------------------------------------------------*/


/* The code generated by the GCC compiler uses the stack in different ways at
different optimisation levels.  The interrupt flags can therefore not always
be saved to the stack.  Instead the critical section nesting level is stored
in a variable, which is then saved as part of the stack context. */
void vPortEnterCritical(void)
{
	/* Disable interrupts as per portDISABLE_INTERRUPTS(); 							*/
#if __aarch64__ == 1
	__asm volatile ("MSR DAIFSET, #3" ::: "memory");
	__asm volatile ("DSB SY");
	__asm volatile ("ISB SY");
#else
	/* Disable interrupts as per portDISABLE_INTERRUPTS(); 							*/
	__asm volatile (
		"STMDB	SP!, {R0}			\n\t"	/* Push R0.								*/
		"MRS	R0, CPSR			\n\t"	/* Get CPSR.							*/
		"ORR	R0, R0, #0xC0		\n\t"	/* Disable IRQ, FIQ.					*/
		"MSR	CPSR, R0			\n\t"	/* Write back modified value.			*/
		"LDMIA	SP!, {R0}");				/* Pop R0.								*/
#endif
/* Now interrupts are disabled ulCriticalNesting can be accessed
directly.  Increment ulCriticalNesting to keep a count of how many times
portENTER_CRITICAL() has been called. */
	ulCriticalNesting++;
}

void vPortExitCritical(void)
{
	if (ulCriticalNesting > portNO_CRITICAL_NESTING)
	{
		/* Decrement the nesting count as we are leaving a critical section. */
		ulCriticalNesting--;

		/* If the nesting level has reached zero then interrupts should be
		re-enabled. */
		if (ulCriticalNesting == portNO_CRITICAL_NESTING)
		{
		#if __aarch64__ == 1
			__asm volatile ("MSR DAIFCLR, #3" ::: "memory");
			__asm volatile ("DSB SY");
			__asm volatile ("ISB SY");
		#else
			/* Enable interrupts as per portEXIT_CRITICAL().					*/
			__asm volatile (
				"STMDB	SP!, {R0}		\n\t"	/* Push R0.						*/
				"MRS	R0, CPSR		\n\t"	/* Get CPSR.					*/
				"BIC	R0, R0, #0xC0	\n\t"	/* Enable IRQ, FIQ.				*/
				"MSR	CPSR, R0		\n\t"	/* Write back modified value.	*/
				"LDMIA	SP!, {R0}");			/* Pop R0.						*/
		#endif
		}
	}
}

