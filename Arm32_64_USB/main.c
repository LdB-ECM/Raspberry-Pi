#include <stdbool.h>		// C standard needed for bool
#include <stdint.h>			// C standard for uint8_t, uint16_t, uint32_t etc
#include "rpi-smartstart.h"
#include "emb-stdio.h"
#include "rpi-usb.h"

static bool lit = false;
void c_irq_handler (void) {
	if (lit) lit = false; else lit = true;							// Flip lit flag
	set_Activity_LED(lit);											// Turn LED on/off as per new flag
	ClearTimerIrq();												// Clear the timer interrupt
}

/***************************************************************************}
{		 		    PUBLIC STRUCTURE DEFINITIONS				            }
****************************************************************************/
int main (void) {
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 0, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed

	TimerIrqSetup(500000, c_irq_handler);
	EnableInterrupts();

	/* Initialize USB system we will want keyboard and mouse */
	UsbInitialise();

	/* Display the USB tree */
	printf("\n");
	UsbShowTree(UsbGetRootHub(), 1, '+');
	printf("\n");

	/* Detect the first keyboard on USB bus */
	uint8_t firstKbd = 0;
	for (int i = 1; i <= MaximumDevices; i++) {
		if (IsKeyboard(i)) {
			firstKbd = i;
			break;
		}
	}
	if (firstKbd) printf("Keyboard detected\r\n");

	/* Hold the screen position */
	uint32_t x, y;
	WhereXY(&x, &y);
	while (1) {	
		if (firstKbd) {
			RESULT status;
			uint8_t buf[8];
			status = HIDReadReport(firstKbd, 0, (uint16_t)USB_HID_REPORT_TYPE_INPUT << 8 | 0, &buf[0], 8);
			if (status == OK)
			{
				GotoXY(x, y);
				printf("HID KBD REPORT: Byte1: 0x%02x Byte2: 0x%02x, Byte3: 0x%02x, Byte4: 0x%02x\n",
					buf[0], buf[1], buf[2], buf[3]);
				printf("                Byte5: 0x%02x Byte6: 0x%02x, Byte7: 0x%02x, Byte8: 0x%02x\n",
					buf[4], buf[5], buf[6], buf[7]);
			}
			else printf("Status error: %08x\n", status);
		}
		timer_wait(100000);
	}

	return(0);
}

