/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: windows.c													}
{       Copyright(c): Leon de Boer(LdB) 2019								}
{       Version: 1.00														}
{																			}
{***************[ THIS CODE IS FREEWARE UNDER CC Attribution]***************}
{																            }
{     This sourcecode is released for the purpose to promote programming    }
{  on the Raspberry Pi. You may redistribute it and/or modify with the      }
{  following disclaimer and condition.                                      }
{																            }
{      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO      }
{   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.            }
{   Redistributions of source code must retain the copyright notices to     }
{   maintain the author credit (attribution) .								}
{																			}
{***************************************************************************}
{                                                                           }
{      This code provides a matching code to the Win32 API on windows for   }
{  the Raspberry Pi.				                                        }
{																			}
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  1.00 Initial version														}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>			// C standard unit needed for bool and true/false
#include <stdint.h>				// C standard unit needed for uint8_t, uint32_t, etc
#include "rpi-smartstart.h"
#include "Font8x16.h"			// Provides the 8x16 bitmap font for console 
#include "windows.h"			// This units header

/*--------------------------------------------------------------------------}
{					 INTERNAL DEVICE CONTEXT STRUCTURE						}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(4))) tagINTDC {
	/* Position control */
	POINT curPos;													// Current position of graphics pointer
	POINT cursor;													// Current cursor position

	/* Current colour controls in RGBA format */
	RGBA TxtColor;													// Text colour currently set in RGBA format
	RGBA BkColor;													// Background colour currently set in RGBA format
	RGBA BrushColor;												// Brush colour currently set in RGBA format

	/* Current color controls in RGBA565 format */
	RGB565 TxtColor565;												// Text colour currently set in RGBA565 format
	RGB565 BkColor565;												// Background colour currently set in RGBA565 format
	RGB565 BrushColor565;											// Brush colour currently set in RGBA565 format

	struct {
		unsigned BkGndTransparent : 1;								// Background is transparent
		unsigned _reserved : 14;
		unsigned usedDC;											// DC is in use				
	};

	/* Bitmap handle .. if one assigned to DC */
	HBITMAP bmp;													// The bitmap assigned to DC
} INTDC;

/*--------------------------------------------------------------------------}
{					 WIN API CONTROL BLOCK STRUCTURE						}
{--------------------------------------------------------------------------*/
static struct __attribute__((__packed__, aligned(4))) {
	/* Screen data that is collected at initialization */
	uintptr_t fb;													// Frame buffer address
	uint32_t wth;													// Screen width (of frame buffer)
	uint32_t ht;													// Screen height (of frame buffer)
	uint32_t depth;													// Colour depth (of frame buffer)
	uint32_t pitch;													// Pitch (Line to line offset)
	/* Function pointers that are set to graphics primitives depending on colour depth */
	void (*ClearArea) (struct tagINTDC* dc, uint_fast32_t x1, uint_fast32_t y1, uint_fast32_t x2, uint_fast32_t y2);
	void (*VertLine) (struct tagINTDC* dc, uint_fast32_t cy, int_fast8_t dir);
	void (*HorzLine) (struct tagINTDC* dc, uint_fast32_t cx, int_fast8_t dir);
	void (*DiagLine) (struct tagINTDC* dc, uint_fast32_t dx, uint_fast32_t dy, int_fast8_t xdir, int_fast8_t ydir);
	void (*WriteChar) (struct tagINTDC* dc, uint8_t Ch);
	void (*TransparentWriteChar) (struct tagINTDC* dc, uint8_t Ch);
	void (*PutImage) (struct tagINTDC* dc, uint_fast32_t dx, uint_fast32_t dy, uint_fast32_t p2wth, HIMAGE imgSrc, bool BottomUp);
} WINAPI_CB = { 0 };

#define MAX_EXT_DC 10
static unsigned int extDCcount = 0;
static INTDC extDC[MAX_EXT_DC] = { 0 };

/***************************************************************************}
{						  PRIVATE C ROUTINES 			                    }
{***************************************************************************/

/*--------------------------------------------------------------------------}
{					   16 BIT COLOUR GRAPHICS ROUTINES						}
{--------------------------------------------------------------------------*/

/*-[INTERNAL: ClearArea16]--------------------------------------------------}
. 16 Bit colour version of the clear area call which block fills the given
. area (x1,y1) up to but not including (x2,y2) with the current brush colour.
. As an internal function pairs assumed to be correctly ordered and dc valid.
.--------------------------------------------------------------------------*/
static void ClearArea16(INTDC* dc, uint_fast32_t x1, uint_fast32_t y1, uint_fast32_t x2, uint_fast32_t y2) {
	RGB565* __attribute__((__packed__, aligned(2))) video_wr_ptr = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (y1 * WINAPI_CB.pitch * 2) + (x1 * 2));
	for (uint_fast32_t y = 0; y < (y2 - y1); y++) {					// For each y line
		for (uint_fast32_t x = 0; x < (x2 - x1); x++) {				// For each x between x1 and x2
			video_wr_ptr[x] = dc->BrushColor565;					// Write the colour
		}
		video_wr_ptr += WINAPI_CB.pitch;							// Offset to next line
	}
}

/*-[INTERNAL: VertLine16]---------------------------------------------------}
. 16 Bit colour version of the vertical line draw from (x,y), to cy pixels
. in dir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void VertLine16(INTDC* dc, uint_fast32_t cy, int_fast8_t dir) {
	RGB565* __attribute__((aligned(2))) video_wr_ptr = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 2) + (dc->curPos.x * 2));
	for (uint_fast32_t i = 0; i < cy; i++) {						// For each y line
		video_wr_ptr[0] = dc->TxtColor565;							// Write the current text colour
		if (dir == 1) video_wr_ptr += WINAPI_CB.pitch;				// Positive offset to next line
		  else  video_wr_ptr -= WINAPI_CB.pitch;					// Negative offset to next line
	}
}

/*-[INTERNAL: HorzLine16]---------------------------------------------------}
. 16 Bit colour version of the horizontal line draw from (x,y), to cx pixels
. in dir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void HorzLine16(INTDC * dc, uint_fast32_t cx, int_fast8_t dir) {
	RGB565* __attribute__((aligned(2))) video_wr_ptr = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 2) + (dc->curPos.x * 2));
	for (uint_fast32_t i = 0; i < cx; i++) {						// For each x pixel
		video_wr_ptr[0] = dc->TxtColor565;							// Write the current text colour
		video_wr_ptr += dir;										// Positive offset to next pixel
	}
}

/*-[INTERNAL: DiagLine16]---------------------------------------------------}
. 16 Bit colour version of the diagonal line draw from (x,y) to dx pixels
. in xdir, and dy pixels in ydir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void DiagLine16(INTDC * dc, uint_fast32_t dx, uint_fast32_t dy, int_fast8_t xdir, int_fast8_t ydir) {
	RGB565* __attribute__((aligned(2))) video_wr_ptr = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 2) + (dc->curPos.x * 2));
	uint_fast32_t tx = 0;											// Zero test x value
	uint_fast32_t ty = 0;											// Zero test y value
	uint_fast32_t eulerMax = (dy > dx) ? dy : dx;					// Larger of dx and dy value
	for (uint_fast32_t i = 0; i < eulerMax; i++) {					// For euler steps
		video_wr_ptr[0] = dc->TxtColor565;							// Write pixel in current text colour
		tx += dx;													// Increment test x value by dx
		if (tx >= eulerMax) {										// If tx >= eulerMax we step
			tx -= eulerMax;											// Subtract eulerMax
			video_wr_ptr += xdir;									// Move pointer left/right 1 pixel
		}
		ty += dy;													// Increment test y value by dy
		if (ty >= eulerMax) {										// If ty >= eulerMax we step
			ty -= eulerMax;											// Subtract eulerMax
			video_wr_ptr += (ydir * WINAPI_CB.pitch);				// Move pointer up/down 1 line
		}
	}
}

/*-[INTERNAL: WriteChar16]--------------------------------------------------}
. 16 Bit colour version of the text character draw. The given character is
. drawn at the current position in the current text and background colours.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void WriteChar16(INTDC * dc, uint8_t Ch) {
	RGB565* __attribute__((aligned(2))) video_wr_ptr = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 2) + (dc->curPos.x * 2));
	for (uint_fast32_t y = 0; y < 4; y++) {
		uint32_t b = BitFont[(Ch * 4) + y];							// Fetch character bits
		for (uint_fast32_t i = 0; i < 32; i++) {					// For each bit
			RGB565 col = dc->BkColor565;							// Preset background colour
			int xoffs = i % 8;										// X offset
			if ((b & 0x80000000) != 0) col = dc->TxtColor565;		// If bit set take current text colour
			video_wr_ptr[xoffs] = col;								// Write pixel
			b <<= 1;												// Roll font bits left
			if (xoffs == 7) video_wr_ptr += WINAPI_CB.pitch;		// If was bit 7 next line down
		}
	}
}

/*-[INTERNAL: TransparentWriteChar16]---------------------------------------}
. 16 Bit colour version of the text character draw. The given character is
. drawn at the current position in the current text color but with current
. background pixels outside font pixels left untouched.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void TransparentWriteChar16(INTDC * dc, uint8_t Ch) {
	RGB565* __attribute__((aligned(2))) video_wr_ptr = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 2) + (dc->curPos.x * 2));
	for (uint_fast32_t y = 0; y < 4; y++) {
		uint32_t b = BitFont[(Ch * 4) + y];							// Fetch character bits
		for (uint_fast32_t i = 0; i < 32; i++) {					// For each bit
			int xoffs = i % 8;										// X offset
			if ((b & 0x80000000) != 0) 								// If bit set take text colour
				video_wr_ptr[xoffs] = dc->TxtColor565;				// Write pixel in current text colour
			b <<= 1;												// Roll font bits left
			if (xoffs == 7) video_wr_ptr += WINAPI_CB.pitch;		// If was bit 7 next line down
		}
	}
}

/*-[INTERNAL: PutImage16]---------------------------------------------------}
. 16 Bit colour version of the put image draw. The image is transferred from
. the source position and drawn to screen. The source and destination format
. need to be the same and should be ensured by preprocess code.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void PutImage16(INTDC * dc, uint_fast32_t dx, uint_fast32_t dy, uint_fast32_t p2wth, HIMAGE ImageSrc, bool BottomUp) {
	HIMAGE video_wr_ptr;
	video_wr_ptr.ptrRGB565 = (RGB565*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 2) + (dc->curPos.x * 2));
	for (uint_fast32_t y = 0; y < dy; y++) {						// For each line
		for (uint_fast32_t x = 0; x < dx; x++) {					// For each pixel
			video_wr_ptr.ptrRGB565[x] = ImageSrc.ptrRGB565[x];		// Transfer pixel
		}
		if (BottomUp) video_wr_ptr.ptrRGB565 -= WINAPI_CB.pitch;	// Next line up
			else video_wr_ptr.ptrRGB565 += WINAPI_CB.pitch;			// Next line down
		ImageSrc.rawImage += p2wth;									// Adjust image pointer by power 2 width
	}
}

/*--------------------------------------------------------------------------}
{					   24 BIT COLOUR GRAPHICS ROUTINES						}
{--------------------------------------------------------------------------*/

/*-[INTERNAL: ClearArea24]--------------------------------------------------}
. 24 Bit colour version of the clear area call which block fills the given
. area (x1,y1) up to but not including (x2,y2) with the current brush colour.
. As an internal function pairs assumed to be correctly ordered and dc valid.
.--------------------------------------------------------------------------*/
static void ClearArea24(INTDC * dc, uint_fast32_t x1, uint_fast32_t y1, uint_fast32_t x2, uint_fast32_t y2) {
	RGB* __attribute__((aligned(1))) video_wr_ptr = (RGB*)(uintptr_t)(WINAPI_CB.fb + (y1 * WINAPI_CB.pitch * 3) + (x1 * 3));
	for (uint_fast32_t y = 0; y < (y2 - y1); y++) {					// For each y line
		for (uint_fast32_t x = 0; x < (x2 - x1); x++) {				// For each x between x1 and x2
			video_wr_ptr[x] = dc->BrushColor.rgb;					// Write the colour
		}
		video_wr_ptr += WINAPI_CB.pitch;							// Offset to next line
	}
}

/*-[INTERNAL: VertLine24]---------------------------------------------------}
. 24 Bit colour version of the vertical line draw from (x,y), to cy pixels
. in dir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void VertLine24(INTDC * dc, uint_fast32_t cy, int_fast8_t dir) {
	RGB* __attribute__((aligned(1))) video_wr_ptr = (RGB*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 3) + (dc->curPos.x * 3));
	for (uint_fast32_t i = 0; i < cy; i++) {						// For each y line
		video_wr_ptr[0] = dc->TxtColor.rgb;							// Write the colour
		if (dir == 1) video_wr_ptr += WINAPI_CB.pitch;				// Positive offset to next line
		  else  video_wr_ptr -= WINAPI_CB.pitch;					// Negative offset to next line
	}
}

/*-[INTERNAL: HorzLine24]---------------------------------------------------}
. 24 Bit colour version of the horizontal line draw from (x,y), to cx pixels
. in dir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void HorzLine24(INTDC * dc, uint_fast32_t cx, int_fast8_t dir) {
	RGB* __attribute__((aligned(1))) video_wr_ptr = (RGB*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 3) + (dc->curPos.x * 3));
	for (uint_fast32_t i = 0; i < cx; i++) {						// For each x pixel
		video_wr_ptr[0] = dc->TxtColor.rgb;							// Write the colour
		video_wr_ptr += dir;										// Positive offset to next pixel
	}
}

/*-[INTERNAL: DiagLine24]---------------------------------------------------}
. 24 Bit colour version of the diagonal line draw from (x,y) to dx pixels
. in xdir, and dy pixels in ydir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void DiagLine24(INTDC * dc, uint_fast32_t dx, uint_fast32_t dy, int_fast8_t xdir, int_fast8_t ydir) {
	RGB* __attribute__((aligned(1))) video_wr_ptr = (RGB*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 3) + (dc->curPos.x * 3));
	uint_fast32_t tx = 0;
	uint_fast32_t ty = 0;
	uint_fast32_t eulerMax = (dy > dx) ? dy : dx;					// Larger of dx and dy value
	for (uint_fast32_t i = 0; i < eulerMax; i++) {					// For euler steps
		video_wr_ptr[0] = dc->TxtColor.rgb;							// Write pixel
		tx += dx;													// Increment test x value by dx
		if (tx >= eulerMax) {										// If tx >= eulerMax we step
			tx -= eulerMax;											// Subtract eulerMax
			video_wr_ptr += xdir;									// Move pointer left/right 1 pixel
		}
		ty += dy;													// Increment test y value by dy
		if (ty >= eulerMax) {										// If ty >= eulerMax we step
			ty -= eulerMax;											// Subtract eulerMax
			video_wr_ptr += (ydir * WINAPI_CB.pitch);				// Move pointer up/down 1 line
		}
	}
}

/*-[INTERNAL: WriteChar24]--------------------------------------------------}
. 24 Bit colour version of the text character draw. The given character is
. drawn at the current position in the current text and background colours.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void WriteChar24(INTDC * dc, uint8_t Ch) {
	RGB* __attribute__((aligned(1))) video_wr_ptr = (RGB*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 3) + (dc->curPos.x * 3));
	for (uint_fast32_t y = 0; y < 4; y++) {
		uint32_t b = BitFont[(Ch * 4) + y];							// Fetch character bits
		for (uint_fast32_t i = 0; i < 32; i++) {					// For each bit
			RGB col = dc->BkColor.rgb;								// Preset background colour
			int xoffs = i % 8;										// X offset
			if ((b & 0x80000000) != 0) col = dc->TxtColor.rgb;		// If bit set take text colour
			video_wr_ptr[xoffs] = col;								// Write pixel
			b <<= 1;												// Roll font bits left
			if (xoffs == 7) video_wr_ptr += WINAPI_CB.pitch;		// If was bit 7 next line down
		}
	}
}

/*-[INTERNAL: TransparentWriteChar24]---------------------------------------}
. 24 Bit colour version of the text character draw. The given character is
. drawn at the current position in the current text color but with current
. background pixels outside font pixels left untouched.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void TransparentWriteChar24(INTDC * dc, uint8_t Ch) {
	RGB* __attribute__((aligned(1))) video_wr_ptr = (RGB*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 3) + (dc->curPos.x * 3));
	for (uint_fast32_t y = 0; y < 4; y++) {
		uint32_t b = BitFont[(Ch * 4) + y];							// Fetch character bits
		for (uint_fast32_t i = 0; i < 32; i++) {					// For each bit
			int xoffs = i % 8;										// X offset
			if ((b & 0x80000000) != 0)								// If bit set take text colour
				video_wr_ptr[xoffs] = dc->TxtColor.rgb;				// Write pixel
			b <<= 1;												// Roll font bits left
			if (xoffs == 7) video_wr_ptr += WINAPI_CB.pitch;		// If was bit 7 next line down
		}
	}
}

/*-[INTERNAL: PutImage24]---------------------------------------------------}
. 24 Bit colour version of the put image draw. The image is transferred from
. the source position and drawn to screen. The source and destination format
. need to be the same and should be ensured by preprocess code.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void PutImage24(INTDC * dc, uint_fast32_t dx, uint_fast32_t dy, uint_fast32_t p2wth, HIMAGE ImageSrc, bool BottomUp) {
	HIMAGE video_wr_ptr;
	video_wr_ptr.ptrRGB = (RGB*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 3) + (dc->curPos.x * 3));
	for (uint_fast32_t y = 0; y < dy; y++) {						// For each line
		for (uint_fast32_t x = 0; x < dx; x++) {					// For each pixel
			video_wr_ptr.ptrRGB[x] = ImageSrc.ptrRGB[x];			// Transfer pixel
		}
		if (BottomUp) video_wr_ptr.ptrRGB -= WINAPI_CB.pitch;		// Next line up
			else video_wr_ptr.ptrRGB += WINAPI_CB.pitch;			// Next line down
		ImageSrc.rawImage += p2wth;									// Adjust image pointer by power 2 width
	}
}

/*--------------------------------------------------------------------------}
{					   32 BIT COLOUR GRAPHICS ROUTINES						}
{--------------------------------------------------------------------------*/

/*-[INTERNAL: ClearArea32]--------------------------------------------------}
. 32 Bit colour version of the clear area call which block fills the given
. area (x1,y1) up to but not including (x2,y2) with the current brush colour.
. As an internal function pairs assumed to be correctly ordered and dc valid.
.--------------------------------------------------------------------------*/
static void ClearArea32(INTDC * dc, uint_fast32_t x1, uint_fast32_t y1, uint_fast32_t x2, uint_fast32_t y2) {
	RGBA* __attribute__((aligned(4))) video_wr_ptr = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (y1 * WINAPI_CB.pitch * 4) + (y1 * 4));
	for (uint_fast32_t y = 0; y < (y2 - y1); y++) {					// For each y line
		for (uint_fast32_t x = 0; x < (x2 - x1); x++) {				// For each x between x1 and x2
			video_wr_ptr[x] = dc->BrushColor;						// Write the current brush colour
		}
		video_wr_ptr += WINAPI_CB.pitch;							// Next line down
	}
}

/*-[INTERNAL: VertLine32]---------------------------------------------------}
. 24 Bit colour version of the vertical line draw from (x,y), to cy pixels
. in dir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void VertLine32(INTDC * dc, uint_fast32_t cy, int_fast8_t dir) {
	RGBA* __attribute__((aligned(4))) video_wr_ptr = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 4) + (dc->curPos.x * 4));
	for (uint_fast32_t i = 0; i < cy; i++) {						// For each y line
		video_wr_ptr[0] = dc->TxtColor;								// Write the colour
		if (dir == 1) video_wr_ptr += WINAPI_CB.pitch;				// Positive offset to next line
			else  video_wr_ptr -= WINAPI_CB.pitch;					// Negative offset to next line
	}
}

/*-[INTERNAL: HorzLine32]---------------------------------------------------}
. 32 Bit colour version of the horizontal line draw from (x,y), to cx pixels
. in dir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void HorzLine32(INTDC * dc, uint_fast32_t cx, int_fast8_t dir) {
	RGBA* __attribute__((aligned(4))) video_wr_ptr = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 4) + (dc->curPos.x * 4));
	for (uint_fast32_t i = 0; i < cx; i++) {						// For each x pixel
		video_wr_ptr[0] = dc->TxtColor;								// Write the colour
		video_wr_ptr += dir;										// Positive offset to next pixel
	}
}

/*-[INTERNAL: DiagLine32]---------------------------------------------------}
. 32 Bit colour version of the diagonal line draw from (x,y) to dx pixels
. in xdir, and dy pixels in ydir in the current text colour.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void DiagLine32(INTDC * dc, uint_fast32_t dx, uint_fast32_t dy, int_fast8_t xdir, int_fast8_t ydir) {
	RGBA* __attribute__((aligned(4))) video_wr_ptr = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 4) + (dc->curPos.x * 4));
	uint_fast32_t tx = 0;
	uint_fast32_t ty = 0;
	uint_fast32_t eulerMax = (dy > dx) ? dy : dx;					// Larger of dx and dy value
	for (uint_fast32_t i = 0; i < eulerMax; i++) {					// For euler steps
		video_wr_ptr[0] = dc->TxtColor;								// Write pixel
		tx += dx;													// Increment test x value by dx
		if (tx >= eulerMax) {										// If tx >= eulerMax we step
			tx -= eulerMax;											// Subtract eulerMax
			video_wr_ptr += xdir;									// Move pointer left/right 1 pixel
		}
		ty += dy;													// Increment test y value by dy
		if (ty >= eulerMax) {										// If ty >= eulerMax we step
			ty -= eulerMax;											// Subtract eulerMax
			video_wr_ptr += (ydir * WINAPI_CB.pitch);				// Move pointer up/down 1 line
		}
	}
}

/*-[INTERNAL: WriteChar32]--------------------------------------------------}
. 32 Bit colour version of the text character draw. The given character is
. drawn at the current position in the current text and background colours.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void WriteChar32(INTDC * dc, uint8_t Ch) {
	RGBA* __attribute__((__packed__, aligned(4))) video_wr_ptr = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 4) + (dc->curPos.x * 4));
	for (uint_fast32_t y = 0; y < 4; y++) {
		uint32_t b = BitFont[(Ch * 4) + y];							// Fetch character bits
		for (uint_fast32_t i = 0; i < 32; i++) {					// For each bit
			RGBA col = dc->BkColor;									// Preset background colour
			uint_fast8_t xoffs = i % 8;								// X offset
			if ((b & 0x80000000) != 0) col = dc->TxtColor;			// If bit set take text colour
			video_wr_ptr[xoffs] = col;								// Write pixel
			b <<= 1;												// Roll font bits left
			if (xoffs == 7) video_wr_ptr += WINAPI_CB.pitch;		// If was bit 7 next line down
		}
	}
}

/*-[INTERNAL: TransparentWriteChar32]---------------------------------------}
. 32 Bit colour version of the text character draw. The given character is
. drawn at the current position in the current text color but with current
. background pixels outside font pixels left untouched.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void TransparentWriteChar32(INTDC * dc, uint8_t Ch) {
	RGBA* __attribute__((__packed__, aligned(4))) video_wr_ptr = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 4) + (dc->curPos.x * 4));
	for (uint_fast32_t y = 0; y < 4; y++) {
		uint32_t b = BitFont[(Ch * 4) + y];							// Fetch character bits
		for (uint_fast32_t i = 0; i < 32; i++) {					// For each bit
			uint_fast8_t xoffs = i % 8;								// X offset
			if ((b & 0x80000000) != 0)								// If bit set take text colour
				video_wr_ptr[xoffs] = dc->TxtColor;					// Write pixel
			b <<= 1;												// Roll font bits left
			if (xoffs == 7) video_wr_ptr += WINAPI_CB.pitch;		// If was bit 7 next line down
		}
	}
}

/*-[INTERNAL: PutImage32]---------------------------------------------------}
. 32 Bit colour version of the put image draw. The image is transferred from
. the source position and drawn to screen. The source and destination format
. need to be the same and should be ensured by preprocess code.
. As an internal function the dc is assumed to be valid.
.--------------------------------------------------------------------------*/
static void PutImage32(INTDC * dc, uint_fast32_t dx, uint_fast32_t dy, uint_fast32_t p2wth, HIMAGE ImageSrc, bool BottomUp) {
	HIMAGE video_wr_ptr;
	video_wr_ptr.ptrRGBA = (RGBA*)(uintptr_t)(WINAPI_CB.fb + (dc->curPos.y * WINAPI_CB.pitch * 4) + (dc->curPos.x * 4));
	for (uint_fast32_t y = 0; y < dy; y++) {						// For each line
		for (uint_fast32_t x = 0; x < dx; x++) {					// For each pixel
			video_wr_ptr.ptrRGBA[x] = ImageSrc.ptrRGBA[x];			// Transfer pixel
		}
		if (BottomUp) video_wr_ptr.ptrRGBA -= WINAPI_CB.pitch;		// Next line up
			else video_wr_ptr.ptrRGBA += WINAPI_CB.pitch;			// Next line down
		ImageSrc.rawImage += p2wth;									// Adjust image pointer by power 2 width
	}
}


/*==========================================================================}
{		      SMARTSTART GRAPHICS COLOUR CONTROL ROUTINES					}
{==========================================================================*/

/*-[SetBkMode]--------------------------------------------------------------}
. Matches WIN32 API, Sets the background mix mode of the device context.
. The background mix mode is used with text, hatched brushes, and pen styles
. that are not solid lines. Most used to make transparent text.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL SetBkMode (HDC hdc,											// Handle to the DC (0 means use standard console DC)
				int mode)											// The new mode
{
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	if ((mode == OPAQUE) || (mode == TRANSPARENT))
	{
		intDC->BkGndTransparent = (mode == TRANSPARENT) ? 1 : 0;	// Set or clear the transparent background flag
		return TRUE;												// Return success								
	}
	return FALSE;													// Return fail as mode is invalid
}

/*-[SetDCPenColor]----------------------------------------------------------}
. Matches WIN32 API, Sets the current text color to specified color value,
. or to the nearest physical color if the device cannot represent the color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
COLORREF SetDCPenColor (HDC hdc,									// Handle to the DC (0 means use standard console DC)
						COLORREF crColor)							// The new pen color
{
	COLORREF retValue;
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	retValue = intDC->TxtColor.ref;									// We will return current text color
	intDC->TxtColor.ref = crColor;									// Update text color on the RGBA format
	intDC->TxtColor565.R = intDC->TxtColor.rgbRed >> 3;				// Transfer converted red bits onto the RGBA565 format
	intDC->TxtColor565.G = intDC->TxtColor.rgbGreen >> 2;			// Transfer converted green bits onto the RGBA565 format
	intDC->TxtColor565.B = intDC->TxtColor.rgbBlue >> 3;			// Transfer converted blue bits onto the RGBA565 format
	return (retValue);												// Return color reference
}

/*-[SetDCBrushColor]--------------------------------------------------------}
. Matches WIN32 API, Sets the current brush color to specified color value,
. or to the nearest physical color if the device cannot represent the color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
COLORREF SetDCBrushColor (HDC hdc,									// Handle to the DC (0 means use standard console DC)
						  COLORREF crColor)							// The new brush color
{
	COLORREF retValue;
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	retValue = intDC->BrushColor.ref;								// We will return current brush color
	intDC->BrushColor.ref = crColor;								// Update brush colour
	intDC->BrushColor565.R = intDC->BrushColor.rgbRed >> 3;			// Transfer converted red bits onto the RGBA565 format
	intDC->BrushColor565.G = intDC->BrushColor.rgbGreen >> 2;		// Transfer converted green bits onto the RGBA565 format
	intDC->BrushColor565.B = intDC->BrushColor.rgbBlue >> 3;		// Transfer converted blue bits onto the RGBA565 format
	return (retValue);												// Return color reference
}

/*-[SetBkColor]-------------------------------------------------------------}
. Matches WIN32 API, Sets the current background color to specified color,
. or to nearest physical color if the device cannot represent the color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
COLORREF SetBkColor (HDC hdc,										// Handle to the DC (0 means use standard console DC)
					 COLORREF crColor)								// The new background color
{
	COLORREF retValue;
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want xtDC[0] otherwise convert handle
	retValue = intDC->BkColor.ref;									// We will return current background color
	intDC->BkColor.ref = crColor;									// Update background colour
	intDC->BkColor565.R = intDC->BkColor.rgbRed >> 3;				// Transfer converted red bits onto the RGBA565 format
	intDC->BkColor565.G = intDC->BkColor.rgbGreen >> 2;				// Transfer converted green bits onto the RGBA565 format
	intDC->BkColor565.B = intDC->BkColor.rgbBlue >> 3;				// Transfer converted blue bits onto the RGBA565 format
	return (retValue);												// Return color reference
}


/*==========================================================================}
{		       SMARTSTART GRAPHICS PRIMITIVE ROUTINES						}
{==========================================================================*/

/*-[MoveToEx]---------------------------------------------------------------}
. Matches WIN32 API, Updates the current graphics position to the specified
. point and optionally stores the previous position (if valid ptr provided).
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL MoveToEx (HDC hdc,												// Handle to the DC (0 means use standard console DC)
			   int X,												// New x graphics position 
			   int Y,												// New y graphics position
			   POINT * lpPoint)										// Pointer to return old position in (If set as 0 return ignored) 
{
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	if (WINAPI_CB.fb) {
		if (lpPoint) (*lpPoint) = intDC->curPos;					// Return current position
		intDC->curPos.x = X;										// Update x position
		intDC->curPos.y = Y;										// Update y position
		return TRUE;												// Function successfully completed
	}
	return FALSE;													// The DC has no valid framebuffer it is not initialized
}

/*-[LineTo]-----------------------------------------------------------------}
. Matches WIN32 API, Draws a line from the current position up to, but not
. including, specified endpoint coordinate.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL LineTo (HDC hdc,												// Handle to the DC (0 means use standard console DC)
			 int nXEnd,												// End at x graphics position
			 int nYEnd)												// End at y graphics position
{
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	if (WINAPI_CB.fb) {
		int_fast8_t xdir, ydir;
		uint_fast32_t dx, dy;

		if (nXEnd < intDC->curPos.x) {								// End x value given is less than x start position
			dx = intDC->curPos.x - nXEnd;							// Unsigned x distance is (current x - end x) 
			xdir = -1;												// Set the x direction as negative
		}
		else {
			dx = nXEnd - intDC->curPos.x;							// Unsigned x distance is (end x - current x)
			xdir = 1;												// Set the x direction as positive
		}

		if (nYEnd < intDC->curPos.y) {								// End y value given is less than y start position
			dy = intDC->curPos.y - nYEnd;							// Unsigned y distance is (current y - end y)
			ydir = -1;												// Set the y direction as negative
		}
		else {
			dy = nYEnd - intDC->curPos.y;							// Unsigned y distance is (end y - current y)
			ydir = 1;												// Set the y direction as positive
		}
		if (dx == 0) WINAPI_CB.VertLine(intDC, dy, ydir);			// Zero dx means vertical line
			else if (dy == 0) WINAPI_CB.HorzLine(intDC, dx, xdir);	// Zero dy means horizontal line
			else WINAPI_CB.DiagLine(intDC, dx, dy, xdir, ydir);		// Anything else is a diagonal line
		intDC->curPos.x = nXEnd;									// Update x position
		intDC->curPos.y = nYEnd;									// Update y position
		return TRUE;												// Function successfully completed
	}
	return FALSE;													// The DC has no valid framebuffer it is not initialized
}

/*-[Rectangle]--------------------------------------------------------------}
. Matches WIN32 API, The rectangle defined by the coords is outlined using
. the current pen and filled by using the current brush. The right and bottom
. co-ordinates are not included in the drawing.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL Rectangle (HDC hdc,											// Handle to the DC (0 means use standard console DC)
				int nLeftRect,										// Left x value of rectangle
				int nTopRect,										// Top y value of rectangle
				int nRightRect,										// Right x value of rectangle (not drawn)
				int nBottomRect)									// Bottom y value of rectangle (not drawn)
{
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	if ((nRightRect > nLeftRect) && (nBottomRect > nTopRect) && (WINAPI_CB.fb))// Make sure coords are in ascending order and we have a frame buffer
	{
		if (intDC->TxtColor.ref != intDC->BrushColor.ref)			// The text colour and brush colors differ 
		{
			POINT orgPoint = { 0 };
			MoveToEx(hdc, nLeftRect, nTopRect, &orgPoint);			// Move to top left coord (hold original coords)
			if (nRightRect - nLeftRect <= 2) {						// Single or double vertical line
				LineTo(hdc, nLeftRect, nBottomRect - 1);			// Draw left line
				if (nRightRect - nLeftRect == 2) {					// The double line case
					MoveToEx(hdc, nRightRect, nBottomRect - 1, 0);	// Move to right side
					LineTo(hdc, nRightRect, nTopRect);				// Draw the rigth side line
				}
			}
			else if (nBottomRect - nTopRect <= 2) {					// Single or double horizontal line
				LineTo(hdc, nRightRect - 1, nTopRect);				// Draw top line
				if (nBottomRect - nTopRect == 2) {					// The double line case
					MoveToEx(hdc, nLeftRect, nBottomRect - 1, 0);	// Move to left side bottom
					LineTo(hdc, nRightRect - 1, nBottomRect - 1);	// Draw the bottom line
				}
			}
			else {													// Normal case where both direction are greater than 2
				WINAPI_CB.ClearArea(intDC, nLeftRect + 1, nTopRect + 1,
					nRightRect - 1, nBottomRect - 1);				// Call clear area function
				LineTo(hdc, nRightRect - 1, nTopRect);				// Draw top line
				LineTo(hdc, nRightRect - 1, nBottomRect - 1);		// Draw right line
				LineTo(hdc, nLeftRect, nBottomRect - 1);			// Draw bottom line
				LineTo(hdc, nLeftRect, nTopRect);					// Draw left line
			}
			MoveToEx(hdc, orgPoint.x, orgPoint.y, 0);				// Restore the graphics coords
		}
		else {
			WINAPI_CB.ClearArea(intDC, nLeftRect, nTopRect, nRightRect,
				nBottomRect);										// Call clear area function
		}
		return TRUE;												// Return success
	}
	return FALSE;													// Return fail as one or both coords pairs were inverted 
}

/*-[TextOut]----------------------------------------------------------------}
. Matches WIN32 API, Writes a character string at the specified location,
. using the currently selected font, background color, and text color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL TextOut (HDC hdc,												// Handle to the DC (0 means use standard console DC)
			  int nXStart,											// Start text at x graphics position
			  int nYStart,											// Start test at y graphics position
			  LPCSTR lpString,										// Pointer to character string to print
			  int cchString)										// Number of characters to print
{
	INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;			// If hdc is zero then we want extDC[0] otherwise convert handle
	if ((cchString > 0) && (lpString) && (WINAPI_CB.fb)) {			// Check text data valid and we have a frame buffer
		intDC->curPos.x = nXStart;									// Set x graphics position
		intDC->curPos.y = nYStart;									// Set y graphics position
		for (int i = 0; i < cchString; i++) {						// For each character
			if (intDC->BkGndTransparent)
				WINAPI_CB.TransparentWriteChar(intDC, lpString[i]);	// Write the character in transparent mode
			else WINAPI_CB.WriteChar(intDC, lpString[i]);			// Write the character in fore/back colours
			intDC->curPos.x += BitFontWth;							// Move X position
		}
		return TRUE;												// Return success
	}
	return FALSE;													// Return failure as string or count zero or no frame buffer
}

/*==========================================================================}
{				   PI BASIC DISPLAY CONSOLE ROUTINES 						}
{==========================================================================*/

/*-[PiConsole_Init]---------------------------------------------------------}
. Attempts to initialize the Pi graphics screen to the given resolutions.
. The width, Height and Depth can al individually be set to 0 which invokes
. autodetection of that particular settings. So it allows you to set very
. specific settings or just default for the HDMI monitor detected. The print
. handler is invoked to display details about result and can be the screen or
. a uart handler for monitoring. This routine needs to be called before any
. attempt to use any console or graphical routines, although most will just
. return a fail if attempted to use before this is called.
.--------------------------------------------------------------------------*/
bool PiConsole_Init (int Width,										// Screen width request (Use 0 if you wish autodetect width)
					 int Height,									// Screen height request (Use 0 if you wish autodetect height)
					 int Depth,										// Screen colour depth request (Use 0 if you wish autodetect colour depth) 
					 int(*prn_handler) (const char* fmt, ...))		// Print handler to display setting result on if successful
{
	uint32_t buffer[23];
	if ((Width == 0) || (Height == 0)) {							// Has auto width or height been requested
		if (mailbox_tag_message(&buffer[0], 5,
			MAILBOX_TAG_GET_PHYSICAL_WIDTH_HEIGHT,
			8, 0, 0, 0)) {											// Get current width and height of screen
			if (Width == 0) Width = buffer[3];						// Width passed in as zero set set Screenwidth variable
			if (Height == 0) Height = buffer[4];					// Height passed in as zero set set ScreenHeight variable
		} else return false;										// For some reason get screen physical failed
	}
	if (Depth == 0) {												// Has auto colour depth been requested
		if (mailbox_tag_message(&buffer[0], 4,
			MAILBOX_TAG_GET_COLOUR_DEPTH, 4, 4, 0)) {				// Get current colour depth of screen
			Depth = buffer[3];										// Depth passed in as zero set set current screen colour depth
		} else return false;										// For some reason get screen depth failed
	}
	if (!mailbox_tag_message(&buffer[0], 23,
		MAILBOX_TAG_SET_PHYSICAL_WIDTH_HEIGHT, 8, 8, Width, Height,
		MAILBOX_TAG_SET_VIRTUAL_WIDTH_HEIGHT, 8, 8, Width, Height,
		MAILBOX_TAG_SET_COLOUR_DEPTH, 4, 4, Depth,
		MAILBOX_TAG_ALLOCATE_FRAMEBUFFER, 8, 4, 16, 0,
		MAILBOX_TAG_GET_PITCH, 4, 0, 0))							// Attempt to set the requested settings
		return false;												// The requesting settings failed so return the failure
	WINAPI_CB.fb = GPUaddrToARMaddr(buffer[17]);					// Transfer the frame buffer
	WINAPI_CB.pitch = buffer[22];									// Transfer the line pitch
	WINAPI_CB.wth = Width;											// Transfer the screen width
	WINAPI_CB.ht = Height;											// Transfer the screen height
	WINAPI_CB.depth = Depth;										// Transfer the screen depth

	SetDCPenColor(0, 0xFFFFFFFF);									// Default text colour is white
	SetBkColor(0, 0x00000000);										// Default background colour black
	SetDCBrushColor(0, 0xFF00FF00);									// Default brush colour is green????

	switch (Depth) {
	case 32:														/* 32 bit colour screen mode */
		WINAPI_CB.ClearArea = ClearArea32;							// Set console function ptr to 32bit colour version of clear area
		WINAPI_CB.VertLine = VertLine32;							// Set console function ptr to 32bit colour version of vertical line
		WINAPI_CB.HorzLine = HorzLine32;							// Set console function ptr to 32bit colour version of horizontal line
		WINAPI_CB.DiagLine = DiagLine32;							// Set console function ptr to 32bit colour version of diagonal line
		WINAPI_CB.WriteChar = WriteChar32;							// Set console function ptr to 32bit colour version of write character
		WINAPI_CB.TransparentWriteChar = TransparentWriteChar32;	// Set console function ptr to 32bit colour version of transparent write character
		WINAPI_CB.PutImage = PutImage32;							// Set console function ptr to 32bit colour version of put bitmap image
		WINAPI_CB.pitch /= 4;										// 4 bytes per write
		break;
	case 24:														/* 24 bit colour screen mode */
		WINAPI_CB.ClearArea = ClearArea24;							// Set console function ptr to 24bit colour version of clear area
		WINAPI_CB.VertLine = VertLine24;							// Set console function ptr to 24bit colour version of vertical line
		WINAPI_CB.HorzLine = HorzLine24;							// Set console function ptr to 24bit colour version of horizontal line
		WINAPI_CB.DiagLine = DiagLine24;							// Set console function ptr to 24bit colour version of diagonal line
		WINAPI_CB.WriteChar = WriteChar24;							// Set console function ptr to 24bit colour version of write character
		WINAPI_CB.TransparentWriteChar = TransparentWriteChar24;	// Set console function ptr to 24bit colour version of transparent write character
		WINAPI_CB.PutImage = PutImage24;							// Set console function ptr to 24bit colour version of put bitmap image
		WINAPI_CB.pitch /= 3;										// 3 bytes per write
		break;
	case 16:														/* 16 bit colour screen mode */
		WINAPI_CB.ClearArea = ClearArea16;							// Set console function ptr to 16bit colour version of clear area
		WINAPI_CB.VertLine = VertLine16;							// Set console function ptr to 16bit colour version of vertical line
		WINAPI_CB.HorzLine = HorzLine16;							// Set console function ptr to 16bit colour version of horizontal line
		WINAPI_CB.DiagLine = DiagLine16;							// Set console function ptr to 16bit colour version of diagonal line
		WINAPI_CB.WriteChar = WriteChar16;							// Set console function ptr to 16bit colour version of write character
		WINAPI_CB.TransparentWriteChar = TransparentWriteChar16;	// Set console function ptr to 16bit colour version of transparent write character
		WINAPI_CB.PutImage = PutImage16;							// Set console function ptr to 16bit colour version of put bitmap image
		WINAPI_CB.pitch /= 2;										// 2 bytes per write
		break;
	}

	extDC[0].usedDC = 1;
	extDCcount++;

	if (prn_handler) prn_handler("Screen resolution %i x %i Colour Depth: %i Line Pitch: %i\n",
		WINAPI_CB.wth, WINAPI_CB.ht, WINAPI_CB.depth, WINAPI_CB.pitch);// If print handler valid print the display resolution message
	return true;													// Return successful
}


/*-[GetConsole_FrameBuffer]-------------------------------------------------}
. Simply returns the console frame buffer. If PiConsole_Init has not yet been
. called it will return 0, which sort of forms an error check value.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_FrameBuffer(void) {
	return (uint32_t)WINAPI_CB.fb;									// Return the console frame buffer
}

/*-[GetConsole_Width]-------------------------------------------------------}
. Simply returns the console width. If PiConsole_Init has not yet been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected width setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Width(void) {
	return (uint32_t)WINAPI_CB.wth;									// Return the console width in pixels
}

/*-[GetConsole_Height]------------------------------------------------------}
. Simply returns the console height. If PiConsole_Init has not been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected height setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Height(void) {
	return (uint32_t)WINAPI_CB.ht;									// Return the console height in pixels
}

/*-[GetConsole_Depth]-------------------------------------------------------}
. Simply returns the console depth. If PiConsole_Init has not yet been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected colour depth setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Depth(void) {
	return (uint32_t)WINAPI_CB.depth;								// Return the colour depth in bits per pixel
}

/*==========================================================================}
{			  PI BASIC DISPLAY CONSOLE CURSOR POSITION ROUTINES 			}
{==========================================================================*/

/*-[WhereXY]----------------------------------------------------------------}
. Simply returns the console cursor x,y position into any valid pointer.
. If a value is not required simply set the pointer to 0 to signal ignore.
.--------------------------------------------------------------------------*/
void WhereXY(uint32_t * x, uint32_t * y)
{
	if (x) (*x) = extDC[0].cursor.x;								// If x pointer is valid write x cursor position to it
	if (y) (*y) = extDC[0].cursor.y;								// If y pointer is valid write y cursor position to it 
}

/*-[GotoXY]-----------------------------------------------------------------}
. Simply set the console cursor to the x,y position. Subsequent text writes
. using console write will then be to that cursor position. This can even be
. set before any screen initialization as it just sets internal variables.
.--------------------------------------------------------------------------*/
void GotoXY(uint32_t x, uint32_t y)
{
	extDC[0].cursor.x = x;											// Set cursor x position to that requested
	extDC[0].cursor.y = y;											// Set cursor y position to that requested
}

/*-[WriteText]--------------------------------------------------------------}
. Simply writes the given null terminated string out to the the console at
. current cursor x,y position. If PiConsole_Init has not yet been called it
. will simply return, as it does for empty of invalid string pointer.
.--------------------------------------------------------------------------*/
void WriteText(char* lpString) {
	while ((WINAPI_CB.fb) && (lpString) && (*lpString != 0))		// While console initialize, string pointer valid and not '\0'
	{
		switch (*lpString) {
		case '\r': {												// Carriage return character
			extDC[0].cursor.x = 0;									// Cursor back to line start
		}
				   break;
		case '\t': {												// Tab character character
			extDC[0].cursor.x += 8;									// Cursor increment to by 8
			extDC[0].cursor.x -= (extDC[0].cursor.x % 8);			// align it to 8
		}
				   break;
		case '\n': {												// New line character
			extDC[0].cursor.x = 0;									// Cursor back to line start
			extDC[0].cursor.y++;									// Increment cursor down a line
		}
				   break;
		default: {													// All other characters
			extDC[0].curPos.x = extDC[0].cursor.x * BitFontWth;
			extDC[0].curPos.y = extDC[0].cursor.y * BitFontHt;
			WINAPI_CB.WriteChar(&extDC[0], *lpString);				// Write the character to graphics screen
			extDC[0].cursor.x++;									// Cursor.x forward one character
		}
				 break;
		}
		lpString++;													// Next character
	}
}

/*==========================================================================}
{							GDI OBJECT ROUTINES								}
{==========================================================================*/

/*-[SelectObject]-----------------------------------------------------------}
. Matches WIN32 API, SelectObject function selects an object to the specified
. device context(DC). The new object replaces the previous object on the DC.
. RETURN: Success the return value is a handle to the object being replaced.
. Failure is indicated by a return of HGDI_ERROR
.--------------------------------------------------------------------------*/
HGDIOBJ SelectObject (HDC hdc,									// Handle to the DC (0 means use standard console DC)
					 HGDIOBJ h)									// Handle to GDI object
{
	HGDIOBJ retVal = { HGDI_ERROR };
	if (h.objType) {
		INTDC* intDC = (hdc == 0) ? &extDC[0] : (INTDC*)hdc;	// If hdc is zero then we want console otherwise convert handle
		switch (*h.objType)
		{
		case 0:													// Object is a bitmap
		{
			retVal.bitmap = intDC->bmp;							// Return the old bitmap handle 
			if (WINAPI_CB.depth == h.bitmap->bmBitsPixel)		// If colour depths match simply put image to DC
			{
				intDC->curPos.x = 0;							// Zero the x graphics position on the DC
				intDC->curPos.y = 0;							// zero the Y graphics position on the DC
				WINAPI_CB.PutImage(intDC, h.bitmap->bmWidth,
					h.bitmap->bmHeight, h.bitmap->bmWidthBytes,
					h.bitmap->bmImage, h.bitmap->bmBottomUp);	// Output the bitmap directly to DC
			}
			else {												// Otherwise we need to run conversion
				uint8_t __attribute__((aligned(4))) buffer[4096];
				HIMAGE imgSrc = h.bitmap->bmImage;				// Transfer pointer
				HIMAGE src;
				src.rawImage = &buffer[0];						// Pointer src now needs to point to the temp buffer
				intDC->curPos.x = 0;							// Zero the x graphics position on the DC
				intDC->curPos.y = (h.bitmap->bmBottomUp) ?
					h.bitmap->bmHeight : 0;						// Set the Y graphics position on the DC depending on bottom up flag
				for (int y = 0; y < h.bitmap->bmHeight; y++)
				{
					switch (WINAPI_CB.depth) {					// For each of the DC colour depths we need to provide a conversion
					case 16:									// DC is RGB656
					{
						RGB565 out;								// So we need RGB565 data
						for (unsigned int i = 0; i < h.bitmap->bmWidth; i++) {
							if (h.bitmap->bmBitsPixel == 24) {
								out.R = imgSrc.ptrRGB[i].rgbRed >> 3;// Set red bits from RGB
								out.G = imgSrc.ptrRGB[i].rgbGreen >> 2;// Set green bits from RGB
								out.B = imgSrc.ptrRGB[i].rgbBlue >> 3;// Set bule bits from RGB
							}
							else {
								out.R = imgSrc.ptrRGBA[i].rgbRed >> 3;// Set red bits from RGBA
								out.G = imgSrc.ptrRGBA[i].rgbGreen >> 2;// Set green bits from RGBA
								out.B = imgSrc.ptrRGBA[i].rgbBlue >> 3;// Set blue bits from RGBA
							}
							src.ptrRGB565[i] = out;				// Write the converted RGB565 out
						}
						break;
					}
					case 24:									// DC is RGB
					{
						RGB out;								// So we need RGB data
						for (unsigned int i = 0; i < h.bitmap->bmWidth; i++) {
							if (h.bitmap->bmBitsPixel == 16) {
								out.rgbRed = imgSrc.ptrRGB565[i].R << 3;// Red value from RGB565 red
								out.rgbGreen = imgSrc.ptrRGB565[i].G << 2;// Green value from RGB565 green
								out.rgbBlue = imgSrc.ptrRGB565[i].B << 3;// Blue value from RGB565 blue
							}
							else {
								out.rgbRed = imgSrc.ptrRGBA[i].rgbRed;// Red value from RGBA red
								out.rgbGreen = imgSrc.ptrRGBA[i].rgbGreen;// Green value from RGBA green
								out.rgbBlue = imgSrc.ptrRGBA[i].rgbBlue;// Blue value from RGBA blue
							}
							src.ptrRGB[i] = out;				// Write the converted RGB out
						}
						break;
					}
					case 32:									// DC is RGBA
					{
						RGBA out;								// So we need RGBA data
						out.rgbAlpha = 0xFF;					// Set the alpha
						for (unsigned int i = 0; i < h.bitmap->bmWidth; i++) {
							if (h.bitmap->bmBitsPixel == 16) {
								out.rgbRed = imgSrc.ptrRGB565[i].R << 3;// Red value from RGB565 red
								out.rgbGreen = imgSrc.ptrRGB565[i].G << 2;// Green value from RGB565 green
								out.rgbBlue = imgSrc.ptrRGB565[i].B << 3;// Blue value from RGB565 blue
							}
							else {
								out.rgbRed = imgSrc.ptrRGB[i].rgbRed;// Red value from RGB red
								out.rgbGreen = imgSrc.ptrRGB[i].rgbGreen;// Green value from RGB green
								out.rgbBlue = imgSrc.ptrRGB[i].rgbBlue;// Blue value from RGB blue
							}
							src.ptrRGBA[i] = out;				// Write the converted RGB out
						}
						break;
					}
					}
					WINAPI_CB.PutImage(intDC, h.bitmap->bmWidth, 1,
						h.bitmap->bmWidthBytes, src, true);		// Output the single line
					imgSrc.rawImage += h.bitmap->bmWidthBytes;	// Adjust by power line power of two value
					(h.bitmap->bmBottomUp) ? intDC->curPos.y-- : intDC->curPos.y++;
				}
			}
			intDC->bmp = h.bitmap;								// Hold the bitmap pointer
		}
		}
	}
	return retVal;
}

/*==========================================================================}
{								DC ROUTINES									}
{==========================================================================*/
HDC CreateExternalDC (int num)
{
	if (num < MAX_EXT_DC) {
		if (extDC[num].usedDC == 0)
		{
			extDC[num].curPos = extDC[0].curPos;
			extDC[num].cursor = extDC[0].cursor;

			extDC[num].TxtColor = extDC[0].TxtColor;
			extDC[num].BkColor = extDC[0].BkColor;
			extDC[num].BrushColor = extDC[0].BrushColor;

			extDC[num].TxtColor565 = extDC[0].TxtColor565;
			extDC[num].BkColor565 = extDC[0].BkColor565;
			extDC[num].BrushColor565 = extDC[0].BrushColor565;

			extDC[num].BkGndTransparent = extDC[0].BkGndTransparent;

			extDC[num].usedDC = 1;
			extDCcount++;
		}
		return((HDC)& extDC[num]);
	}
	return 0;
}


/*==========================================================================}
{						SCREEN RESOLUTION API								}
{==========================================================================*/

/*-[GetScreenWidth]---------------------------------------------------------}
.  Returns the screen width in pixels. If the screen is uninitialized it
.  will simply return 0.
.--------------------------------------------------------------------------*/
unsigned int GetScreenWidth (void) 
{
	return WINAPI_CB.wth;											// Return the screen width in pixels
}

/*-[GetScreenHeight]--------------------------------------------------------}
.  Returns the screen height in pixels. If the screen is uninitialized it
.  will simply return 0.
.--------------------------------------------------------------------------*/
unsigned int GetScreenHeight (void)
{
	return WINAPI_CB.ht;											// Return the screen height in pixels
}

/*-[GetScreenDepth]---------------------------------------------------------}
.  Returns the screen colour depth. If the screen is uninitialized it
.  will simply return 0.
.--------------------------------------------------------------------------*/
unsigned int GetScreenDepth (void)
{
	return WINAPI_CB.depth;											// Return the screen colour depth
}