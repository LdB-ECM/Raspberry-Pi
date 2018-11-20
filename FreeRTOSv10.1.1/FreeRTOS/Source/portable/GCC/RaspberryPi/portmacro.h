#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------
 * Port specific definitions.  
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	unsigned portLONG
#define portBASE_TYPE	portLONG

/* added */
typedef portSTACK_TYPE StackType_t;
typedef portLONG BaseType_t;
typedef unsigned portLONG UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef unsigned portSHORT  TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef unsigned portLONG TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif
/* stop */

/*
#if( configUSE_16_BIT_TICKS == 1 )
	typedef unsigned portSHORT portTickType;
	#define portMAX_DELAY ( portTickType ) 0xffff
#else
	typedef unsigned portLONG portTickType;
	#define portMAX_DELAY ( portTickType ) 0xffffffff
#endif
*/
/*-----------------------------------------------------------*/	

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( portTickType ) 10000 / configTICK_RATE_HZ )		
#define portBYTE_ALIGNMENT			8
#define portNOP()						__asm volatile ( "NOP" );
/*-----------------------------------------------------------*/	


/* Scheduler utilities. */

/*
 * portSAVE_CONTEXT, portRESTORE_CONTEXT, portENTER_SWITCHING_ISR
 * and portEXIT_SWITCHING_ISR can only be called from ARM mode, but
 * are included here for efficiency.  An attempt to call one from
 * THUMB mode code will result in a compile time error.
 */

#define portRESTORE_CONTEXT()															\
{																						\
	/* Set the LR to the task stack.												*/	\
	__asm volatile (																	\
	/* Restore the original stack alignment (see note about 8-byte alignment below).*/	\
	"add sp, sp, r4															\n\t"		\
	/* Put the address of the current TCB into R1.									*/	\
	"LDR		R0, =pxCurrentTCB											\n\t"		\
	/* Load the 32-bit value stored at the address in R1.							*/	\
	/* First item in the TCB is the top of the stack for the current TCB.			*/	\
	"LDR		R0, [R0]													\n\t"		\
																						\
	/* Move the value into the Link Register!										*/	\
	"LDR		LR, [R0]													\n\t"		\
																						\
	/* The critical nesting depth is the first item on the stack. */					\
	/* Load it into the ulCriticalNesting variable. */									\
	"LDR		R0, =ulCriticalNesting										\n\t"		\
	"LDMFD		LR!, {R1}													\n\t"		\
	"STR		R1, [R0]													\n\t"		\
																						\
	/* Get the SPSR from the stack. */													\
	"LDMFD		LR!, {R0}													\n\t"		\
	"MSR		SPSR_cxsf, R0												\n\t"		\
																						\
	/* Restore all system mode registers for the task. */								\
	"LDMFD	LR, {R0-R14}^													\n\t"		\
	"NOP																	\n\t"		\
																						\
	/* Restore the return address. */													\
	"LDR		LR, [LR, #+60]												\n\t"		\
																						\
	/* And return - correcting the offset in the LR to obtain the */					\
	/* correct address. */																\
	"SUBS		PC, LR, #4													\n\t"		\
	"NOP																	\n\t"		\
	"NOP																	\n\t"		\
	);																					\
}
/*-----------------------------------------------------------*/

#define portSAVE_CONTEXT()													\
{																			\
	/* Push R0 as we are going to use the register. */						\
	__asm volatile (														\
	"STMDB	SP!, {R0}												\n\t"	\
																			\
	/* Set R0 to point to the task stack pointer. */						\
	"STMDB	SP,{SP}^	\n\t"	/* ^ means get the user mode SP value. */	\
	"SUB	SP, SP, #4												\n\t"	\
	"LDMIA	SP!,{R0}												\n\t"	\
																			\
	/* Push the return address onto the stack. */							\
	"STMDB	R0!, {LR}												\n\t"	\
																			\
	/* Now we have saved LR we can use it instead of R0. */					\
	"MOV	LR, R0													\n\t"	\
																			\
	/* Pop R0 so we can save it onto the system mode stack. */				\
	"LDMIA	SP!, {R0}												\n\t"	\
																			\
	/* Push all the system mode registers onto the task stack. */			\
	"STMDB	LR,{R0-LR}^												\n\t"	\
	"NOP															\n\t"	\
	"SUB	LR, LR, #60												\n\t"	\
																			\
	/* Push the SPSR onto the task stack. */								\
	"MRS	R0, SPSR												\n\t"	\
	"STMDB	LR!, {R0}												\n\t"	\
																			\
	"LDR	R0, =ulCriticalNesting									\n\t"	\
	"LDR	R0, [R0]												\n\t"	\
	"STMDB	LR!, {R0}												\n\t"	\
																			\
	/* Store the new top of stack for the task. */							\
	"LDR	R0, =pxCurrentTCB										\n\t"	\
	"LDR	R0, [R0]												\n\t"	\
	"STR	LR, [R0]												\n\t"	\
	/* According to the document "Procedure Call Standard for the ARM   */	\
	/* Architecture", the stack pointer is 4-byte aligned at all times, */	\
	/* but it must be 8-byte aligned when calling an externally visible */	\
	/* function. This is important because this code is reached from an */	\
	/* IRQ and therefore the stack currently may only be 4-byte aligned.*/  \
	/* If this is so, the stack must be padded to an 8-byte boundary	*/	\
	/* before calling any external C code								*/	\
	"AND R4, SP, #4													\n\t"	\
	"SUB SP, SP, R4													\n\t"	\
	);																		\
}

extern void vTaskSwitchContext( void );
#define portYIELD_FROM_ISR()		vTaskSwitchContext()
#define portYIELD()					__asm volatile ( "SWI 0" )
/*-----------------------------------------------------------*/


/* Critical section management. */

/*
 * The interrupt management utilities can only be called from ARM mode.	 When
 * THUMB_INTERWORK is defined the utilities are defined as functions in 
 * portISR.c to ensure a switch to ARM mode.  When THUMB_INTERWORK is not 
 * defined then the utilities are defined as macros here - as per other ports.
 */

#ifdef THUMB_INTERWORK

	extern void vPortDisableInterruptsFromThumb( void ) __attribute__ ((naked));
	extern void vPortEnableInterruptsFromThumb( void ) __attribute__ ((naked));

	#define portDISABLE_INTERRUPTS()	vPortDisableInterruptsFromThumb()
	#define portENABLE_INTERRUPTS()	vPortEnableInterruptsFromThumb()
	
#else

	#define portDISABLE_INTERRUPTS()														\
		__asm volatile (																		\
			"STMDB	SP!, {R0}		\n\t"		/* Push R0.							*/	\
			"MRS		R0, CPSR			\n\t"		/* Get CPSR.						*/	\
			"ORR		R0, R0, #0xC0	\n\t"		/* Disable IRQ, FIQ.				*/	\
			"MSR		CPSR, R0			\n\t"		/* Write back modified value.	*/	\
			"LDMIA	SP!, {R0}			 " )	/* Pop R0.							*/
			
	#define portENABLE_INTERRUPTS()														\
		__asm volatile (																		\
			"STMDB	SP!, {R0}		\n\t"		/* Push R0.							*/	\
			"MRS		R0, CPSR			\n\t"		/* Get CPSR.						*/	\
			"BIC		R0, R0, #0xC0	\n\t"		/* Enable IRQ, FIQ.				*/	\
			"MSR		CPSR, R0			\n\t"		/* Write back modified value.	*/	\
			"LDMIA	SP!, {R0}			 " )	/* Pop R0.							*/

#endif /* THUMB_INTERWORK */

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portENTER_CRITICAL()		vPortEnterCritical();
#define portEXIT_CRITICAL()		vPortExitCritical();
/*-----------------------------------------------------------*/

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

	/* Check the configuration. */
	#if( configMAX_PRIORITIES > 32 )
		#error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 difference priorities as tasks that share a priority will time slice.
	#endif

	/* Store/clear the ready priorities in a bit map. */
	#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
	#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

	/*-----------------------------------------------------------*/

	#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - _clz( ( uxReadyPriorities ) ) )

#endif /* taskRECORD_READY_PRIORITY */

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
