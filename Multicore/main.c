#include <stdbool.h>		// C standard needed for bool
#include <stdint.h>			// C standard for uint8_t, uint16_t, uint32_t etc
#include <stdlib.h>			// Needed for rand
#include "rpi-smartstart.h"
#include "emb-stdio.h"

static int idx = 0;
static int core = 0;
void somerand(void)
{
    idx++;
	printf("idx = %d call by core:%d\n", idx, core);
}

void marco(void)
{
     printf("Core 1 says marco\n");
}

void polo(void)
{
     printf("Core 2 says polo\n");
}

void caught(void)
{
     printf("Core 3 says caught\n");
}


static bool lit = false;
void c_irq_handler (void) {
	if (lit) lit = false; else lit = true;							// Flip lit flag
	set_Activity_LED(lit);											// Turn LED on/off as per new flag
	if (lit) {
		core = (rand() % 3) + 1;
		CoreExecute(core, somerand);
	}
}


CORECALLFUNC test;

int main (void) {
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen

	volatile int i = 0;
	while (i < 100000) i++;   // My printf is not re-entrant so small delay
	CoreExecute(1, marco);
	i = 0;
	while (i < 100000) i++;   // My printf is not re-entrant so small delay
	CoreExecute(2, polo);
	i = 0;
	while (i < 100000) i++;   // My printf is not re-entrant so small delay
	CoreExecute(3, caught);
	i = 0;
	while (i < 100000) i++;   // My printf is not re-entrant so small delay

	TimerIrqSetup(500000, c_irq_handler);							// Give me flashing LED and timer based hyperthread launch 
	EnableInterrupts();												// Start interrupts rolling



	
	while (1) {

	}

	return(0);
}