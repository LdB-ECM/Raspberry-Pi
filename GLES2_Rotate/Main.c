#include <stdbool.h>			// Neede for bool
#include <stdint.h>				// Needed for uint32_t, uint16_t etc
#include <string.h>				// Needed for memcpy
#include "emb-stdio.h"			// Needed for printf
#include "rpi-smartstart.h"		// Needed for smart start API 
#include "rpi-GLES.h"



int main (void) {
	InitV3D();														// Start 3D graphics
	ARM_setmaxspeed(NULL);											// ARM CPU to max speed no message to screen
	PiConsole_Init(0, 0, 32, NULL);									// Auto resolution console, no message to screen

	// Render a triangle to screen.
	// In this simple example we are using live screen framebuffer
	// Usually you would do to an off screen buffer or memory
	// I strongly suggest you don't do this when we start animation or you will see screen tearing
	testTriangle(GetConsole_Width(), GetConsole_Height(), 
		ARMaddrToGPUaddr((void*)(uintptr_t)GetConsole_FrameBuffer()),
		printf);

	while (1){
		DoRotate(0.1f);
		testTriangle(GetConsole_Width(), GetConsole_Height(),
			ARMaddrToGPUaddr((void*)(uintptr_t)GetConsole_FrameBuffer()),
			printf);
    }
	return(0);
}
