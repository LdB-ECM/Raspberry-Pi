#ifndef _WINDOWS_H_
#define _WINDOWS_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: windows.h													}
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

/* System font is 8 wide and 16 height so these are preset for the moment */
#define BitFontHt 16
#define BitFontWth 8

/***************************************************************************}
{                    PUBLIC WIN32 API COMPATIBLE TYPEDEFS                   }
{***************************************************************************/
typedef int32_t		BOOL;						// BOOL is defined to an int32_t ... yeah windows is weird -1 is often returned
typedef uint32_t	COLORREF;					// COLORREF is a uint32_t
typedef uintptr_t	HDC;						// HDC is really a pointer
typedef uintptr_t	HANDLE;						// HANDLE is really a pointer
typedef uintptr_t	HINSTANCE;					// HINSTANCE is really a pointer
typedef uint32_t	UINT;						// UINT is an unsigned 32bit int
typedef char*		LPCSTR;						// LPCSTR is a char pointer

/***************************************************************************}
{                    PUBLIC WIN32 API COMPATIBLE DEFINES                    }
{***************************************************************************/
#define TRUE 1									// TRUE is 1 (technically any non zero value)
#define FALSE 0									// FALSE is 0

/*--------------------------------------------------------------------------}
;{				 ENUMERATED BACKGROUND MODES AS PER WIN32 API				}
;{-------------------------------------------------------------------------*/
#define TRANSPARENT	1
#define OPAQUE		2

/***************************************************************************}
{		 	        PUBLIC GRAPHICS STRUCTURE DEFINITIONS					}
****************************************************************************/

/*--------------------------------------------------------------------------}
{						 RGB STRUCTURE DEFINITION							}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__))
{
	unsigned rgbBlue : 8;							// Blue
	unsigned rgbGreen : 8;							// Green
	unsigned rgbRed : 8;							// Red
}  RGB;

/*--------------------------------------------------------------------------}
{						 RGBA STRUCTURE DEFINITION							}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned rgbBlue : 8;						// Blue
		unsigned rgbGreen : 8;						// Green
		unsigned rgbRed : 8;						// Red
		unsigned rgbAlpha : 8;						// Alpha
	};
	RGB rgb;										// RGB triple (1st 3 bytes)
	COLORREF ref;									// Color reference								
} RGBA;

/*--------------------------------------------------------------------------}
{						 RGB565 STRUCTURE DEFINITION						}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__))
{
	unsigned B : 5;									// Blue
	unsigned G : 6;									// Green
	unsigned R : 5;									// Red
} RGB565;

/*--------------------------------------------------------------------------}
{						 POINT STRUCTURE DEFINITION							}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__))
{
	int32_t x;											// x co-ordinate
	int32_t y;											// y co-ordinate
} POINT, * LPPOINT;										// Typedef define POINT and LPPOINT


/*--------------------------------------------------------------------------}
{						DIFFERENT IMAGE POINTER UNION						}
{--------------------------------------------------------------------------*/
typedef union {
	uint8_t* __attribute__((aligned(1))) rawImage;		// Pointer to raw byte format array
	RGB565* __attribute__((aligned(2))) ptrRGB565;		// Pointer to RGB565 format array
	RGB* __attribute__((aligned(1))) ptrRGB;			// Pointer to RGB format array
	RGBA* __attribute__((aligned(4))) ptrRGBA;			// Pointer to RGBA format array
	uintptr_t rawPtr;									// Pointer address
} HIMAGE;

/*--------------------------------------------------------------------------}
{						BITMAP STRUCTURE DEFINITIOMN						}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__)) tagBITMAP {
	uint32_t   bmType;									// Always zero ... bitmap objtype ID
	uint32_t   bmWidth;									// Width in pixels of the bitmap
	uint32_t   bmHeight;								// Height in pixels of the bitmap
	uint32_t   bmWidthBytes;							// The number of bytes in each scan line as it must be balign4
	uint32_t   bmPlanes;								// The count of color planes
	union {
		void* bmBits;
		HIMAGE	 bmImage;
	};
	struct __attribute__((__packed__)) {
		unsigned bmBitsPixel : 16;						// The number of bits required to indicate the color of a pixel
		unsigned bmBottomUp : 1;						// Image draws from bottom up
		unsigned _reserved : 15;						// Reserved but unused bits
	};
} BITMAP, * HBITMAP;

/*--------------------------------------------------------------------------}
{	                    HGDIOBJ - HANDLE TO GDI OBJECT						}
{--------------------------------------------------------------------------*/
typedef union {
	uint32_t* objType;									// Pointer to the type which is always first uint32_t							
	HBITMAP bitmap;										// Bitmap is one type of GDI object
} HGDIOBJ;

#define HGDI_ERROR 0

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
				int mode);											// The new mode

/*-[SetDCPenColor]----------------------------------------------------------}
. Matches WIN32 API, Sets the current text color to specified color value,
. or to the nearest physical color if the device cannot represent the color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
COLORREF SetDCPenColor (HDC hdc,									// Handle to the DC (0 means use standard console DC)
						COLORREF crColor);							// The new pen color

/*-[SetDCBrushColor]--------------------------------------------------------}
. Matches WIN32 API, Sets the current brush color to specified color value,
. or to the nearest physical color if the device cannot represent the color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
COLORREF SetDCBrushColor (HDC hdc,									// Handle to the DC (0 means use standard console DC)
						  COLORREF crColor);						// The new brush color

/*-[SetBkColor]-------------------------------------------------------------}
. Matches WIN32 API, Sets the current background color to specified color,
. or to nearest physical color if the device cannot represent the color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
COLORREF SetBkColor (HDC hdc,										// Handle to the DC (0 means use standard console DC)
					 COLORREF crColor);								// The new background color


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
			   POINT* lpPoint);										// Pointer to return old position in (If set as 0 ignored)

/*-[LineTo]-----------------------------------------------------------------}
. Matches WIN32 API, Draws a line from the current position up to, but not
. including, specified endpoint co-ordinate.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL LineTo (HDC hdc,												// Handle to the DC (0 means use standard console DC)
			 int nXEnd,												// End at x graphics position
			 int nYEnd);											// End at y graphics position

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
				int nBottomRect);									// Bottom y value of rectangle (not drawn)

/*-[TextOut]----------------------------------------------------------------}
. Matches WIN32 API, Writes a character string at the specified location,
. using the currently selected font, background color, and text color.
. If DC is passed as 0 the screen console is assumed as the target.
.--------------------------------------------------------------------------*/
BOOL TextOut (HDC hdc,												// Handle to the DC (0 means use standard console DC)
			  int nXStart,											// Start text at x graphics position
			  int nYStart,											// Start test at y graphics position
			  LPCSTR lpString,										// Pointer to character string to print
			  int cchString);										// Number of characters to print


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
bool PiConsole_Init(int Width,										// Screen width request (Use 0 if you wish autodetect width) 
	int Height,									// Screen height request (Use 0 if you wish autodetect height) 	
	int Depth,										// Screen colour depth request (Use 0 if you wish autodetect colour depth) 
	int(*prn_handler) (const char* fmt, ...));		// Print handler to display setting result on if successful

/*-[GetConsole_FrameBuffer]-------------------------------------------------}
. Simply returns the console frame buffer. If PiConsole_Init has not yet been
. called it will return 0, which sort of forms an error check value.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_FrameBuffer(void);

/*-[GetConsole_Width]-------------------------------------------------------}
. Simply returns the console width. If PiConsole_Init has not yet been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected width setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Width(void);

/*-[GetConsole_Height]------------------------------------------------------}
. Simply returns the console height. If PiConsole_Init has not been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected height setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Height(void);

/*-[GetConsole_Depth]-------------------------------------------------------}
. Simply returns the console depth. If PiConsole_Init has not yet been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected colour depth setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Depth(void);

/*==========================================================================}
{			  PI BASIC DISPLAY CONSOLE CURSOR POSITION ROUTINES 			}
{==========================================================================*/

/*-[WhereXY]----------------------------------------------------------------}
. Simply returns the console cursor x,y position into any valid pointer.
. If a value is not required simply set the pointer to 0 to signal ignore.
.--------------------------------------------------------------------------*/
void WhereXY(uint32_t* x, uint32_t* y);

/*-[GotoXY]-----------------------------------------------------------------}
. Simply set the console cursor to the x,y position. Subsequent text writes
. using console write will then be to that cursor position. This can even be
. set before any screen initialization as it just sets internal variables.
.--------------------------------------------------------------------------*/
void GotoXY(uint32_t x, uint32_t y);

/*==========================================================================}
{			  PI BASIC DISPLAY CONSOLE TEXT OUTPUT ROUTINES					}
{==========================================================================*/

/*-[WriteText]--------------------------------------------------------------}
. Simply writes the given null terminated string out to the the console at
. current cursor x,y position. If PiConsole_Init has not yet been called it
. will simply return, as it does for empty of invalid string pointer.
.--------------------------------------------------------------------------*/
void WriteText(char* lpString);

/*==========================================================================}
{							GDI OBJECT ROUTINES								}
{==========================================================================*/

HGDIOBJ SelectObject(HDC hdc,										// Handle to the DC (0 means use standard console DC)
					 HGDIOBJ h);									// Handle to GDI object


/*==========================================================================}
{								DC ROUTINES									}
{==========================================================================*/
HDC CreateExternalDC (int num);


/*==========================================================================}
{						SCREEN RESOLUTION API								}
{==========================================================================*/

/*-[GetScreenWidth]---------------------------------------------------------}
.  Returns the screen width in pixels. If the screen is uninitialized it
.  will simply return 0.
.--------------------------------------------------------------------------*/
unsigned int GetScreenWidth (void);

/*-[GetScreenHeight]--------------------------------------------------------}
.  Returns the screen height in pixels. If the screen is uninitialized it
.  will simply return 0.
.--------------------------------------------------------------------------*/
unsigned int GetScreenHeight (void);

/*-[GetScreenDepth]---------------------------------------------------------}
.  Returns the screen colour depth. If the screen is uninitialized it
.  will simply return 0.
.--------------------------------------------------------------------------*/
unsigned int GetScreenDepth (void);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif