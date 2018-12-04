#include <stdbool.h>		// C standard unit needed for bool and true/false
#include <stdint.h>			// C standard unit needed for uint8_t, uint32_t, etc
#include "rpi-smartstart.h"
#include "emb-stdio.h"
#include "rpi-Irq.h"		// This units header

/* These now move with the value of RPi_IO_Base_Addr which is auto-detected by SmartStartxx.S based on model */
/* If you do not want to use SmartStart then #define the value of 0x20000000 for Pi1, 0x3F000000 for all other models */
/* Without smartstartxx.S you also need to #define RPi_CoresReady with 1 or 4 matching how many cores the model has */
#define BCM2835_INTC_IRQ_BASIC		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB200))
#define BCM2835_IRQ_PENDING1		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB204))
#define BCM2835_IRQ_PENDING2		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB208))
#define BCM2835_IRQ_FIQ_CTRL		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB20C))
#define BCM2835_IRQ_ENABLE1			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB210))
#define BCM2835_IRQ_ENABLE2			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB214))
#define BCM2835_IRQ_ENABLE_BASIC	((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB218))
#define BCM2835_IRQ_DISABLE1		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB21C))
#define BCM2835_IRQ_DISABLE2		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB220))
#define BCM2835_IRQ_DISABLE_BASIC	((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xB240))


#define QA7_CORE_IRQ_PENDING		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(0x40000060)) // Array of 4 .. one for each core
#define QA7_CORE_FIQ_PENDING		((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(0x40000070)) // Array of 4 .. one for each core


/*--------------------------------------------------------------------------}
{		DEFINITION OF AN INTERRUPT VECTOR STRUCTURE USED BY THE SYSTEM		}
{--------------------------------------------------------------------------*/
typedef struct {
	FN_INTERRUPT_HANDLER pfnHandler;			// Function that handles this IRQn
	FN_INTERRUPT_CLEAR pfnClear;				// Function that handles the clear of this IRQn
	void* pParam;								// A special parameter that the use can pass to the IRQn handler.
} INTERRUPT_VECTOR;

/*--------------------------------------------------------------------------}
{				DEFINITION OF AN IRQ CONTROL BLOCK STRUCTURE				}
{--------------------------------------------------------------------------*/
/*
 * Irq control block (ICB) is allocated to each core under implementation.
 * A single core pi system will have just one, but on a multicore Pi each
 * core may have its own vector table so each core will need an ICB. Our
 * IRQ system is thus designed to work with one or many cores.
 */
typedef struct CoreIrqControlBlock
{
	uint32_t enabled[3];											// A table of bits for each of the 73 vectors aka 0...72 bits
	INTERRUPT_VECTOR vectorTable[BCM2837_INTC_TOTAL_IRQ];			// A table with function pointers for each Irq Vector
} CoreICB;

/*--------------------------------------------------------------------------}
{	4 INTERNAL IRQ CORE BLOCK STRUCTURES TO GIVE UP TO 4 CORE SUPPORT		}
{--------------------------------------------------------------------------*/
static CoreICB coreICB[4] = { 0 };									// Auto zero the block


static void handleRange (uint8_t coreNum, uint32_t pending, uint8_t base)
{
	/* Temp comment so I can watch entry to handle range in case I have error */
	//printf("Handle range %u pending: %u\n", (unsigned int)base, (unsigned int)pending); 
	while (pending)
	{
		// Get index of first set bit:
		uint32_t bit = 31 - __builtin_clz(pending);

		// Map to IRQ number:
		unsigned int irq = base + bit;
		// Call interrupt handler, if enabled and function valid
		if (coreICB[coreNum].vectorTable[irq].pfnHandler)
			coreICB[coreNum].vectorTable[irq].pfnHandler(coreNum, irq, coreICB[coreNum].vectorTable[irq].pParam);
		// Call interrupt clear handler, if enabled and function valid
		if (coreICB[coreNum].vectorTable[irq].pfnClear)
			coreICB[coreNum].vectorTable[irq].pfnClear();
		// Clear bit in bitfield:
		pending &= ~(1 << bit);
	}
}

bool irqRegisterHandler (uint8_t coreNum, uint8_t irq, FN_INTERRUPT_HANDLER pfnHandler, FN_INTERRUPT_CLEAR pfnClear, void *pParam)
{
	if ((coreNum < RPi_CoresReady) && (irq < BCM2837_INTC_TOTAL_IRQ))// Core number and irq number are valid 
	{
		/* I am debating whether to turn Irqs off then back on like original did */
		//irqBlock();
		coreICB[coreNum].vectorTable[irq].pfnHandler = pfnHandler;	// Hold the interrupt function handler to this irq
		coreICB[coreNum].vectorTable[irq].pfnClear = pfnClear;		// Hold the interrupt clear function handler to this irq
		coreICB[coreNum].vectorTable[irq].pParam = pParam;			// Hold the interrupt function parameter to this irq
		/* Same as above */
		//irqUnblock();
		return true;												// Irq successfully registered
	}
	return false;													// Irq registration failed
}

bool irqEnableHandler (uint8_t coreNum, uint8_t irq)
{
	if ((coreNum < RPi_CoresReady) && (irq < BCM2837_INTC_TOTAL_IRQ)// Core number and irq number are valid 
	    && coreICB[coreNum].vectorTable[irq].pfnHandler)			// There is a valid handler registered for this irq
	{
		uint32_t mask = 1 << (irq % 32);							// Bit shift mask position
		if (irq <= 31) {											// Range of IRQ 0 to 31
			coreICB[coreNum].enabled[0] |= mask;					// Set the enabled bit mask in the core table
		}
		else if (irq <= 63) {										// Range of IRQ 32 to 63
			coreICB[coreNum].enabled[1] |= mask;					// Set the enabled bit mask for the core table
		}
		else if (irq < BCM2837_INTC_TOTAL_IRQ) {					// Range of IRQ 64 to 88
			coreICB[coreNum].enabled[2] |= mask;					// Set the enabled bit mask for the core table
		}
		return true;												// Irq enable successful
	}
	return false;													// Irq enable failed
}

bool irqDisableHandler (uint8_t coreNum, uint8_t irq)
{
	if ((coreNum < RPi_CoresReady) && (irq < BCM2837_INTC_TOTAL_IRQ))// Core number and irq number are valid 
	{
		uint32_t mask = 1 << (irq % 32);							// Bit shift mask position
		if (irq <= 31) {											// Range of IRQ 0 to 31
			coreICB[coreNum].enabled[0] &= ~mask;					// Clear the enabled bit mask in the core table
		}
		else if (irq <= 63) {										// Range of IRQ 32 to 63
			coreICB[coreNum].enabled[1] &= ~mask;					// Clear the enabled bit mask for the core table
		}
		else if (irq < BCM2837_INTC_TOTAL_IRQ) {					// Range of IRQ 64 to 88
			coreICB[coreNum].enabled[2] &= ~mask;					// Clear the enabled bit mask for the core table
		}
		return true;												// Irq disable successful
	}
	return false;													// Irq disable failed
}

bool coreEnableIrq (uint8_t irq)
{
	unsigned int coreId = getCoreID();								// Fetch core id
	if ((coreId < RPi_CoresReady) && (irq < BCM2837_INTC_TOTAL_IRQ) // Core number and irq number are valid
		&& coreICB[coreId].vectorTable[irq].pfnHandler)				// There is a valid handler registered for this irq
	{
		uint32_t mask = 1 << (irq % 32);							// Bit shift mask position
		if (irq <= 31) {											// Range of IRQ 0 to 31
			*BCM2835_IRQ_ENABLE1 = mask;							// Enable the IRQ on the physical core
			coreICB[coreId].enabled[0] |= mask;						// Set the enabled bit mask for the core table
		}
		else if (irq <= 63) {										// Range of IRQ 32 to 63
			*BCM2835_IRQ_ENABLE2 = mask;							// Enable the IRQ on the physical core
			coreICB[coreId].enabled[1] |= mask;						// Set the enabled bit mask for the core table
		}
		else if (irq < BCM2837_INTC_TOTAL_IRQ) {					// Range of IRQ 64 to 73
			if (irq < 72) (*BCM2835_IRQ_ENABLE_BASIC) = mask;		// Enable the IRQ on the physical core for first 72 irqs
			coreICB[coreId].enabled[2] |= mask;						// Set the enabled bit mask for the core table
		}
		return true;												// Irq disable successful
	}
	return false;													// Irq disable failed
}

bool coreDisableIrq (uint8_t irq)
{
	unsigned int coreId = getCoreID();								// Fetch core id
	if ((coreId < RPi_CoresReady) && (irq < BCM2837_INTC_TOTAL_IRQ))// Core number and irq number are valid
	{
		uint32_t mask = 1 << (irq % 32);							// Bit shift mask position
		if (irq <= 31) {											// Range of IRQ 0 to 31
			*BCM2835_IRQ_DISABLE1 = mask;							// Disable the IRQ on the physical core
			coreICB[coreId].enabled[0] &= ~mask;					// Clear the enabled bit mask for the core table
		}
		else if (irq <= 63) {										// Range of IRQ 32 to 63
			*BCM2835_IRQ_DISABLE2 = mask;							// Disable the IRQ on the physical core
			coreICB[coreId].enabled[1] &= ~mask;					// Clear the enabled bit mask for the core table
		}
		else if (irq < BCM2837_INTC_TOTAL_IRQ) {					// Range of IRQ 64 to 73
			if (irq < 72) (*BCM2835_IRQ_DISABLE_BASIC) = mask;		// Disable the IRQ on the physical core for all but irq73 being special
			coreICB[coreId].enabled[2] &= ~mask;					// Clear the enabled bit mask for the core table
		}
		return true;												// Irq disable successful
	}
	return false;													// Irq disable failed
}

void irqHandler (void)
{
	uint32_t ulMaskedStatus = (*BCM2835_INTC_IRQ_BASIC);			// Read the interrupt basic register
	unsigned int coreId = getCoreID();								// Fetch core id
	// Bit 8 in IRQBasic indicates interrupts in Pending1 (interrupts 31-0):
	if (ulMaskedStatus & (1 << 8))
		handleRange(coreId, (*BCM2835_IRQ_PENDING1) & coreICB[coreId].enabled[0], 0);

	// Bit 9 in IRQBasic indicates interrupts in Pending2 (interrupts 63-32):
	if (ulMaskedStatus & (1 << 9))
		handleRange(coreId, (*BCM2835_IRQ_PENDING2) & coreICB[coreId].enabled[1], 32);

	// Bits 0 to 7 in IRQBasic represent interrupts 64-71
	ulMaskedStatus &= 0xFF;											// Clear all but bottom 8 bits
	// Bits 0 to 11 in Core Irq pending represent interrupts 72-83
	ulMaskedStatus |= ((QA7_CORE_IRQ_PENDING[coreId] &0xAFF) << 8);	// Add the QA7 irq bits to the IRQbasic (bits 8 & 10) ignored
	if (ulMaskedStatus != 0) 
		handleRange(coreId, ulMaskedStatus & coreICB[coreId].enabled[2], 64);
}