#include <stdbool.h>		// C standard unit needed for bool and true/false
#include <stdint.h>			// C standard unit needed for uint8_t, uint32_t, etc
#include "rpi-SmartStart.h"	// SmartStart unit needed for getCoreID
#include "rpi-Irq.h"		// This units header

/*--------------------------------------------------------------------------}
{		DEFINITION OF AN INTERRUPT VECTOR STRUCTURE USED BY THE SYSTEM		}
{--------------------------------------------------------------------------*/
typedef struct {
	FN_INTERRUPT_HANDLER pfnHandler;			// Function that handles this IRQn
	FN_INTERRUPT_CLEAR pfnClear;				// Function that handles the clear of this IRQn
	void* pParam;								// A special parameter that the use can pass to the IRQn handler.
} INTERRUPT_VECTOR;

/*--------------------------------------------------------------------------}
{	4 INTERNAL IRQ CORE BLOCK STRUCTURES TO GIVE UP TO 4 CORE SUPPORT		}
{--------------------------------------------------------------------------*/
static INTERRUPT_VECTOR coreICB[4] = { 0 };		// Auto zero the block

bool irqRegisterHandler (uint8_t coreNum, FN_INTERRUPT_HANDLER pfnHandler, FN_INTERRUPT_CLEAR pfnClear, void *pParam)
{
	if (coreNum < RPi_CoresReady)									// Core number and irq number are valid 
	{
		coreICB[coreNum].pfnHandler = pfnHandler;					// Hold the interrupt function handler to this irq
		coreICB[coreNum].pfnClear = pfnClear;						// Hold the interrupt clear function handler to this irq
		coreICB[coreNum].pParam = pParam;							// Hold the interrupt function parameter to this irq
		return true;												// Irq successfully registered
	}
	return false;													// Irq registration failed
}


void irqHandler (void)
{
	unsigned int coreId = getCoreID();								// Fetch core id
	// Call interrupt handler, if enabled and function valid
	if (coreICB[coreId].pfnHandler)
		coreICB[coreId].pfnHandler(coreId, coreICB[coreId].pParam);
	// Call interrupt clear handler, if enabled and function valid
	if (coreICB[coreId].pfnClear) coreICB[coreId].pfnClear();
}