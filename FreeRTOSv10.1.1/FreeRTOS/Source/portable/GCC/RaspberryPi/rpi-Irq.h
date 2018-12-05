#ifndef _RPI_IRQ_H_
#define _RPI_IRQ_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

#include <stdbool.h>		// C standard unit needed for bool and true/false
#include <stdint.h>			// C standard unit needed for uint8_t, uint32_t, etc

/*--------------------------------------------------------------------------}
{		  DEFINITION OF AN IRQ FUNCTION POINTER USED BY THE SYSTEM			}
{--------------------------------------------------------------------------*/
typedef void (*FN_INTERRUPT_HANDLER) (uint8_t coreNum, void *pParam);

/*--------------------------------------------------------------------------}
{	   DEFINITION OF A CLEAR IRQ FUNCTION POINTER USED BY THE SYSTEM		}
{--------------------------------------------------------------------------*/
typedef void (*FN_INTERRUPT_CLEAR) (void);

/* Register a function handler and optional interrupt clear function with optionl parameter to a core */
bool irqRegisterHandler(uint8_t coreNum, FN_INTERRUPT_HANDLER pfnHandler, FN_INTERRUPT_CLEAR pfnClear, void *pParam);

/* Entry point for all core interrupt handlers, each core must direct it's Irq vector to here */
void irqHandler(void);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif