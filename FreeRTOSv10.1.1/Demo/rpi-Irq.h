#ifndef _RPI_IRQ_H_
#define _RPI_IRQ_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

#include <stdbool.h>		// C standard unit needed for bool and true/false
#include <stdint.h>			// C standard unit needed for uint8_t, uint32_t, etc

#define BCM2837_INTC_TOTAL_IRQ	(64 + 8 + 12)	// PI3 has most Irq of any Pi system being 84

#define PERIPHERAL_TIMER_IRQ	64
#define LOCAL_CORE_TIMER_IRQ	83

/*--------------------------------------------------------------------------}
{		  DEFINITION OF AN IRQ FUNCTION POINTER USED BY THE SYSTEM			}
{--------------------------------------------------------------------------*/
typedef void (*FN_INTERRUPT_HANDLER) (uint8_t coreNum, uint8_t irq, void *pParam);

/*--------------------------------------------------------------------------}
{	   DEFINITION OF A CLEAR IRQ FUNCTION POINTER USED BY THE SYSTEM		}
{--------------------------------------------------------------------------*/
typedef void (*FN_INTERRUPT_CLEAR) (void);

/* Register a function handler and optional interrupt clear function with optionl parameter to a core */
bool irqRegisterHandler(uint8_t coreNum, uint8_t irq, FN_INTERRUPT_HANDLER pfnHandler, FN_INTERRUPT_CLEAR pfnClear, void *pParam);

/* Enables an IRQ handler on a specific core, this is only the handler not the physical hardware */
bool irqEnableHandler (uint8_t coreNum, uint8_t irq);

/* Disbles an IRQ handler on a specific core, this is only the handler not the physical hardware */
bool irqDisableHandler (uint8_t coreNum, uint8_t irq);

/* Enables the physical hardware IRQ must be called from the core itself .. hence no core number */
bool coreEnableIrq (uint8_t irq);

/* Disables the physical hardware IRQ must be called from the core itself .. hence no core number */
bool coreDisableIrq(uint8_t irq);

/* Entry point for all core interrupt handlers, each core must direct it's Irq vector to here */
void irqHandler(void);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif