#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>				// C standard unit needed for size_t

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
#define portSTACK_TYPE	size_t
#define portBASE_TYPE	portLONG

typedef portSTACK_TYPE StackType_t;
typedef portLONG BaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
#error "The Raspberry Pi port does not support 16 bit timer ticks"
#endif

#if __aarch64__ == 1
typedef uint64_t UBaseType_t;
typedef uint64_t TickType_t;
#define portMAX_DELAY ( ( TickType_t ) 0xffffffffffffffffULL )
#define portPOINTER_SIZE_TYPE  uint64_t
#define portBYTE_ALIGNMENT	16
#else
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
#define portMAX_DELAY ( ( TickType_t ) 0xffffffffUL )
#define portBYTE_ALIGNMENT	8
#endif


/*-----------------------------------------------------------*/	
/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( portTickType ) 1000 / configTICK_RATE_HZ )		
#define portNOP()					__asm volatile ( "NOP" );
/*-----------------------------------------------------------*/	

#if __aarch64__ == 1
#define portYIELD()		__asm volatile ( "SVC 0" ::: "memory" )
#else
#define portYIELD()		__asm volatile ( "SWI 0" )
#endif
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
	#if __aarch64__ == 1
		#define portDISABLE_INTERRUPTS()													\
			__asm volatile ( "MSR DAIFSET, #2" ::: "memory" );								\
			__asm volatile ( "DSB SY" );													\
			__asm volatile ( "ISB SY" );

		#define portENABLE_INTERRUPTS()														\
			__asm volatile ( "MSR DAIFCLR, #2" ::: "memory" );								\
			__asm volatile ( "DSB SY" );													\
			__asm volatile ( "ISB SY" );
	#else
		#define portDISABLE_INTERRUPTS()													\
			__asm volatile (																\
				"STMDB	SP!, {R0}		\n\t"		/* Push R0.							*/	\
				"MRS	R0, CPSR		\n\t"		/* Get CPSR.						*/	\
				"ORR	R0, R0, #0xC0	\n\t"		/* Disable IRQ, FIQ.				*/	\
				"MSR	CPSR, R0		\n\t"		/* Write back modified value.		*/	\
				"LDMIA	SP!, {R0}			 " )	/* Pop R0.							*/
			
		#define portENABLE_INTERRUPTS()														\
			__asm volatile (																\
				"STMDB	SP!, {R0}		\n\t"		/* Push R0.							*/	\
				"MRS	R0, CPSR		\n\t"		/* Get CPSR.						*/	\
				"BIC	R0, R0, #0xC0	\n\t"		/* Enable IRQ, FIQ.					*/	\
				"MSR	CPSR, R0		\n\t"		/* Write back modified value.		*/	\
				"LDMIA	SP!, {R0}			 " )	/* Pop R0.							*/
	#endif

#endif /* THUMB_INTERWORK */

extern void vPortEnterCritical ( void );
extern void vPortExitCritical ( void );

#define portENTER_CRITICAL()	vPortEnterCritical()
#define portEXIT_CRITICAL()		vPortExitCritical()
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
