#include <stdbool.h>			// Neede for bool
#include <stdint.h>				// Needed for uint32_t, uint16_t etc
#include <string.h>				// Needed for memcpy
#include "emb-stdio.h"			// Needed for printf
#include "rpi-smartstart.h"		// Needed for smart start API 
#include "rpi-GLES.h"




/*static uint32_t shader2[12] = { // Fill Color Shader
		0x009E7000, 0x100009E7,   // nop; nop; nop
		0xFFFFFFFF,	0xE0020BA7,	  // ldi tlbc, RGBA White
		0x009E7000, 0x500009E7,   // nop; nop; sbdone
		0x009E7000, 0x300009E7,   // nop; nop; thrend
		0x009E7000, 0x100009E7,   // nop; nop; nop
		0x009E7000, 0x100009E7,   // nop; nop; nop
};*/



int main (void) {
	Init_EmbStdio(Embedded_Console_WriteChar);						// Initialize embedded stdio
	PiConsole_Init(0, 0, 32, printf);								// Auto resolution console, message to screen
	displaySmartStart(printf);										// Display smart start details
	ARM_setmaxspeed(printf);										// ARM CPU to max speed no message to screen

	Desktop_Start();

	while (1){
		Move_Windows();
		timer_wait(10000);	// 10000 usec delay
    }

	// Release resources
	//V3D_mem_unlock(handle);
	//V3D_mem_free(handle);
	return(0);
}
