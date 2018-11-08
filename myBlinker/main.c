#include <stdbool.h>		// C standard needed for bool
#include <stdint.h>			// C standard for uint8_t, uint16_t, uint32_t etc
#include <stdlib.h>			// Needed for rand
#include "rpi-SmartStart.h"
#include "emb-stdio.h"



static const char Spin[4] = { '|', '/', '-', '\\' };


static bool lit = false;
void c_irq_handler(void)
{
	if (lit) lit = false; else lit = true;							// Flip lit flag
	set_Activity_LED(lit);											// Turn LED on/off as per new flag
	ClearTimerIrq();												// Clear the timer interrupt
}

int main (void) {
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen

	uint32_t Buffer[5] = { 0 };
	mailbox_tag_message(&Buffer[0], 5, MAILBOX_TAG_GET_CLOCK_RATE,
		8, 8, 4, 0);												// Get GPU clock (it varies between 200-450Mhz)
	printf("Timer clock prescale speed = %u\r\n", (unsigned int)Buffer[4]);

	TimerIrqSetup(500000, c_irq_handler);							// Give me flashing LED and timer 
	EnableInterrupts();												// Start interrupts rolling

	int i = 0;
	while (1) {
		printf("Deadloop %c\r", Spin[i]);
		timer_wait(50000);
		i++;
		i %= 4;
	}

	return(0);
}
