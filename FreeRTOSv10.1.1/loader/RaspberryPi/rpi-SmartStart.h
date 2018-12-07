#ifndef _SMART_START_H_
#define _SMART_START_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: rpi-smartstart.h											}
{       Copyright(c): Leon de Boer(LdB) 2017, 2018							}
{       Version: 2.12														}
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
{      This code provides a 32bit or 64bit C wrapper around the assembler   }
{  stub code. In AARCH32 that file is SmartStart32.S, while in AARCH64 the  }
{  file is SmartStart64.S.													}
{	   There file also provides access to the basic hardware of the PI.     }
{  It is also the easy point to insert a couple of important very useful    }
{  Macros that make C development much easier.		        				} 
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  2.08 Added setIrqFuncAddress & setFiqFuncAddress							}
{  2.09 Added Hard/Soft float compiler support								}
{  2.10 Context Switch support API calls added								}
{  2.11 MiniUart, PL011 Uart and console uart support added					}
{  2.12 New FIQ, DAIF flag support added									}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>		// C standard unit needed for bool and true/false
#include <stdint.h>			// C standard unit needed for uint8_t, uint32_t, etc
#include <stdarg.h>			// C standard unit needed for variadic functions

/***************************************************************************}
{		  PUBLIC MACROS MUCH AS WE HATE THEM SOMETIMES YOU NEED THEM        }
{***************************************************************************/

/* Most versions of C don't have _countof macro so provide it if not available */
#if !defined(_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0])) 
#endif

/* As we are compiling for Raspberry Pi if winmain make it main */
#define WinMain(...) main(uint32_t r0, uint32_t r1, uint32_t atags)

/* System font is 8 wide and 16 height so these are preset for the moment */
#define BitFontHt 16
#define BitFontWth 8

/***************************************************************************}
{					     PUBLIC ENUMERATION CONSTANTS			            }
****************************************************************************/

/*--------------------------------------------------------------------------}
;{				  ENUMERATED AN ID FOR DIFFERENT PI MODELS					}
;{-------------------------------------------------------------------------*/
typedef enum {
	MODEL_1A		= 0,
	MODEL_1B		= 1,
	MODEL_1A_PLUS	= 2,
	MODEL_1B_PLUS	= 3,
	MODEL_2B		= 4,
	MODEL_ALPHA		= 5,
	MODEL_CM		= 6,		// Compute Module
	MODEL_2A        = 7,
	MODEL_PI3		= 8,
	MODEL_PI_ZERO	= 9,
	MODEL_CM3       = 0xA,      // Compute module 3
	MODEL_PI_ZEROW	= 0xC,
	MODEL_PI3B_PLUS = 0xD,
} RPI_BOARD_TYPE;

/*--------------------------------------------------------------------------}
;{	      ENUMERATED FSEL REGISTERS ... BCM2835.PDF MANUAL see page 92		}
;{-------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	GPIO_INPUT = 0b000,									// 0
	GPIO_OUTPUT = 0b001,								// 1
	GPIO_ALTFUNC5 = 0b010,								// 2
	GPIO_ALTFUNC4 = 0b011,								// 3
	GPIO_ALTFUNC0 = 0b100,								// 4
	GPIO_ALTFUNC1 = 0b101,								// 5
	GPIO_ALTFUNC2 = 0b110,								// 6
	GPIO_ALTFUNC3 = 0b111,								// 7
} GPIOMODE;

/*--------------------------------------------------------------------------}
;{	    ENUMERATED GPIO FIX RESISTOR ... BCM2835.PDF MANUAL see page 101	}
;{-------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	NO_RESISTOR = 0b00,									// 0
	PULLUP = 0b01,										// 1
	PULLDOWN = 0b10,									// 2
} GPIO_FIX_RESISTOR;

/*--------------------------------------------------------------------------}
{	ENUMERATED TIMER CONTROL PRESCALE ... BCM2835.PDF MANUAL see page 197	}
{--------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	Clkdiv1 = 0b00,										// 0
	Clkdiv16 = 0b01,									// 1
	Clkdiv256 = 0b10,									// 2
	Clkdiv_undefined = 0b11,							// 3 
} TIMER_PRESCALE;

/*--------------------------------------------------------------------------}
{	                  ENUMERATED MAILBOX CHANNELS							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	MB_CHANNEL_POWER = 0x0,								// Mailbox Channel 0: Power Management Interface 
	MB_CHANNEL_FB = 0x1,								// Mailbox Channel 1: Frame Buffer
	MB_CHANNEL_VUART = 0x2,								// Mailbox Channel 2: Virtual UART
	MB_CHANNEL_VCHIQ = 0x3,								// Mailbox Channel 3: VCHIQ Interface
	MB_CHANNEL_LEDS = 0x4,								// Mailbox Channel 4: LEDs Interface
	MB_CHANNEL_BUTTONS = 0x5,							// Mailbox Channel 5: Buttons Interface
	MB_CHANNEL_TOUCH = 0x6,								// Mailbox Channel 6: Touchscreen Interface
	MB_CHANNEL_COUNT = 0x7,								// Mailbox Channel 7: Counter
	MB_CHANNEL_TAGS = 0x8,								// Mailbox Channel 8: Tags (ARM to VC)
	MB_CHANNEL_GPU = 0x9,								// Mailbox Channel 9: GPU (VC to ARM)
} MAILBOX_CHANNEL;

/*--------------------------------------------------------------------------}
{	            ENUMERATED MAILBOX TAG CHANNEL COMMANDS						}
{  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface  }
{--------------------------------------------------------------------------*/
typedef enum {
	/* Videocore info commands */
	MAILBOX_TAG_GET_VERSION					= 0x00000001,			// Get firmware revision

	/* Hardware info commands */
	MAILBOX_TAG_GET_BOARD_MODEL				= 0x00010001,			// Get board model
	MAILBOX_TAG_GET_BOARD_REVISION			= 0x00010002,			// Get board revision
	MAILBOX_TAG_GET_BOARD_MAC_ADDRESS		= 0x00010003,			// Get board MAC address
	MAILBOX_TAG_GET_BOARD_SERIAL			= 0x00010004,			// Get board serial
	MAILBOX_TAG_GET_ARM_MEMORY				= 0x00010005,			// Get ARM memory
	MAILBOX_TAG_GET_VC_MEMORY				= 0x00010006,			// Get VC memory
	MAILBOX_TAG_GET_CLOCKS					= 0x00010007,			// Get clocks

	/* Power commands */
	MAILBOX_TAG_GET_POWER_STATE				= 0x00020001,			// Get power state
	MAILBOX_TAG_GET_TIMING					= 0x00020002,			// Get timing
	MAILBOX_TAG_SET_POWER_STATE				= 0x00028001,			// Set power state

	/* GPIO commands */
	MAILBOX_TAG_GET_GET_GPIO_STATE			= 0x00030041,			// Get GPIO state
	MAILBOX_TAG_SET_GPIO_STATE				= 0x00038041,			// Set GPIO state

	/* Clock commands */
	MAILBOX_TAG_GET_CLOCK_STATE				= 0x00030001,			// Get clock state
	MAILBOX_TAG_GET_CLOCK_RATE				= 0x00030002,			// Get clock rate
	MAILBOX_TAG_GET_MAX_CLOCK_RATE			= 0x00030004,			// Get max clock rate
	MAILBOX_TAG_GET_MIN_CLOCK_RATE			= 0x00030007,			// Get min clock rate
	MAILBOX_TAG_GET_TURBO					= 0x00030009,			// Get turbo

	MAILBOX_TAG_SET_CLOCK_STATE				= 0x00038001,			// Set clock state
	MAILBOX_TAG_SET_CLOCK_RATE				= 0x00038002,			// Set clock rate
	MAILBOX_TAG_SET_TURBO					= 0x00038009,			// Set turbo

	/* Voltage commands */
	MAILBOX_TAG_GET_VOLTAGE					= 0x00030003,			// Get voltage
	MAILBOX_TAG_GET_MAX_VOLTAGE				= 0x00030005,			// Get max voltage
	MAILBOX_TAG_GET_MIN_VOLTAGE				= 0x00030008,			// Get min voltage

	MAILBOX_TAG_SET_VOLTAGE					= 0x00038003,			// Set voltage

	/* Temperature commands */
	MAILBOX_TAG_GET_TEMPERATURE				= 0x00030006,			// Get temperature
	MAILBOX_TAG_GET_MAX_TEMPERATURE			= 0x0003000A,			// Get max temperature

	/* Memory commands */
	MAILBOX_TAG_ALLOCATE_MEMORY				= 0x0003000C,			// Allocate Memory
	MAILBOX_TAG_LOCK_MEMORY					= 0x0003000D,			// Lock memory
	MAILBOX_TAG_UNLOCK_MEMORY				= 0x0003000E,			// Unlock memory
	MAILBOX_TAG_RELEASE_MEMORY				= 0x0003000F,			// Release Memory
																	
	/* Execute code commands */
	MAILBOX_TAG_EXECUTE_CODE				= 0x00030010,			// Execute code

	/* QPU control commands */
	MAILBOX_TAG_EXECUTE_QPU					= 0x00030011,			// Execute code on QPU
	MAILBOX_TAG_ENABLE_QPU					= 0x00030012,			// QPU enable

	/* Displaymax commands */
	MAILBOX_TAG_GET_DISPMANX_HANDLE			= 0x00030014,			// Get displaymax handle
	MAILBOX_TAG_GET_EDID_BLOCK				= 0x00030020,			// Get HDMI EDID block

	/* SD Card commands */
	MAILBOX_GET_SDHOST_CLOCK				= 0x00030042,			// Get SD Card EMCC clock
	MAILBOX_SET_SDHOST_CLOCK				= 0x00038042,			// Set SD Card EMCC clock

	/* Framebuffer commands */
	MAILBOX_TAG_ALLOCATE_FRAMEBUFFER		= 0x00040001,			// Allocate Framebuffer address
	MAILBOX_TAG_BLANK_SCREEN				= 0x00040002,			// Blank screen
	MAILBOX_TAG_GET_PHYSICAL_WIDTH_HEIGHT	= 0x00040003,			// Get physical screen width/height
	MAILBOX_TAG_GET_VIRTUAL_WIDTH_HEIGHT	= 0x00040004,			// Get virtual screen width/height
	MAILBOX_TAG_GET_COLOUR_DEPTH			= 0x00040005,			// Get screen colour depth
	MAILBOX_TAG_GET_PIXEL_ORDER				= 0x00040006,			// Get screen pixel order
	MAILBOX_TAG_GET_ALPHA_MODE				= 0x00040007,			// Get screen alpha mode
	MAILBOX_TAG_GET_PITCH					= 0x00040008,			// Get screen line to line pitch
	MAILBOX_TAG_GET_VIRTUAL_OFFSET			= 0x00040009,			// Get screen virtual offset
	MAILBOX_TAG_GET_OVERSCAN				= 0x0004000A,			// Get screen overscan value
	MAILBOX_TAG_GET_PALETTE					= 0x0004000B,			// Get screen palette

	MAILBOX_TAG_RELEASE_FRAMEBUFFER			= 0x00048001,			// Release Framebuffer address
	MAILBOX_TAG_SET_PHYSICAL_WIDTH_HEIGHT	= 0x00048003,			// Set physical screen width/heigh
	MAILBOX_TAG_SET_VIRTUAL_WIDTH_HEIGHT	= 0x00048004,			// Set virtual screen width/height
	MAILBOX_TAG_SET_COLOUR_DEPTH			= 0x00048005,			// Set screen colour depth
	MAILBOX_TAG_SET_PIXEL_ORDER				= 0x00048006,			// Set screen pixel order
	MAILBOX_TAG_SET_ALPHA_MODE				= 0x00048007,			// Set screen alpha mode
	MAILBOX_TAG_SET_VIRTUAL_OFFSET			= 0x00048009,			// Set screen virtual offset
	MAILBOX_TAG_SET_OVERSCAN				= 0x0004800A,			// Set screen overscan value
	MAILBOX_TAG_SET_PALETTE					= 0x0004800B,			// Set screen palette
	MAILBOX_TAG_SET_VSYNC					= 0x0004800E,			// Set screen VSync
	MAILBOX_TAG_SET_BACKLIGHT				= 0x0004800F,			// Set screen backlight

	/* VCHIQ commands */
	MAILBOX_TAG_VCHIQ_INIT					= 0x00048010,			// Enable VCHIQ

	/* Config commands */
	MAILBOX_TAG_GET_COMMAND_LINE			= 0x00050001,			// Get command line 

	/* Shared resource management commands */
	MAILBOX_TAG_GET_DMA_CHANNELS			= 0x00060001,			// Get DMA channels

	/* Cursor commands */
	MAILBOX_TAG_SET_CURSOR_INFO				= 0x00008010,			// Set cursor info
	MAILBOX_TAG_SET_CURSOR_STATE			= 0x00008011,			// Set cursor state
} TAG_CHANNEL_COMMAND;

/*--------------------------------------------------------------------------}
{					    ENUMERATED MAILBOX CLOCK ID							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	CLK_EMMC_ID		= 0x1,								// Mailbox Tag Channel EMMC clock ID 
	CLK_UART_ID		= 0x2,								// Mailbox Tag Channel uart clock ID
	CLK_ARM_ID		= 0x3,								// Mailbox Tag Channel ARM clock ID
	CLK_CORE_ID		= 0x4,								// Mailbox Tag Channel SOC core clock ID
	CLK_V3D_ID		= 0x5,								// Mailbox Tag Channel V3D clock ID
	CLK_H264_ID		= 0x6,								// Mailbox Tag Channel H264 clock ID
	CLK_ISP_ID		= 0x7,								// Mailbox Tag Channel ISP clock ID
	CLK_SDRAM_ID	= 0x8,								// Mailbox Tag Channel SDRAM clock ID
	CLK_PIXEL_ID	= 0x9,								// Mailbox Tag Channel PIXEL clock ID
	CLK_PWM_ID		= 0xA,								// Mailbox Tag Channel PWM clock ID
} MB_CLOCK_ID;


/*--------------------------------------------------------------------------}
{			      ENUMERATED MAILBOX POWER BLOCK ID							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	PB_SDCARD		= 0x0,								// Mailbox Tag Channel SD Card power block 
	PB_UART0		= 0x1,								// Mailbox Tag Channel UART0 power block 
	PB_UART1		= 0x2,								// Mailbox Tag Channel UART1 power block 
	PB_USBHCD		= 0x3,								// Mailbox Tag Channel USB_HCD power block 
	PB_I2C0			= 0x4,								// Mailbox Tag Channel I2C0 power block 
	PB_I2C1			= 0x5,								// Mailbox Tag Channel I2C1 power block 
	PB_I2C2			= 0x6,								// Mailbox Tag Channel I2C2 power block 
	PB_SPI			= 0x7,								// Mailbox Tag Channel SPI power block 
	PB_CCP2TX		= 0x8,								// Mailbox Tag Channel CCP2TX power block 
} MB_POWER_ID;

/*--------------------------------------------------------------------------}
;{	  ENUMERATED CODE TARGET ... WHICH ARM CPU THE CODE IS COMPILED FOR		}
;{-------------------------------------------------------------------------*/
typedef enum {
	ARM5_CODE = 5,										// ARM 5 CPU is targetted
	ARM6_CODE = 6,										// ARM 6 CPU is targetted
	ARM7_CODE = 7,										// ARM 7 CPU is targetted
	ARM8_CODE = 8,										// ARM 8 CPU is targetted
} ARM_CODE_TYPE;

/*--------------------------------------------------------------------------}
;{	 ENUMERATED AARCH TARGET ... WHICH AARCH TARGET CODE IS COMPILED FOR	}
;{-------------------------------------------------------------------------*/
typedef enum {
	AARCH32 = 0,										// AARCH32 - 32 bit
	AARCH64 = 1,										// AARCH64 - 64 bit
} AARCH_MODE;


/*--------------------------------------------------------------------------}
;{				 ENUMERATED BACKGROUND MODES AS PER WIN32 API				}
;{-------------------------------------------------------------------------*/
#define TRANSPARENT	1
#define OPAQUE		2

/***************************************************************************}
{		 		    PUBLIC STRUCTURE DEFINITIONS				            }
****************************************************************************/

/*--------------------------------------------------------------------------}
{				 COMPILER TARGET SETTING STRUCTURE DEFINED					}
{--------------------------------------------------------------------------*/
typedef union 
{
	struct 
	{
		ARM_CODE_TYPE ArmCodeTarget : 4;							// @0  Compiler code target
		AARCH_MODE AArchMode : 1;									// @5  Code AARCH type compiler is producing
		unsigned CoresSupported : 3;								// @6  Cores the code is setup to support
		unsigned reserved : 23;										// @9-31 reserved
		unsigned HardFloats : 1;									// @31	 Compiler code for hard floats
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} CODE_TYPE;

/*--------------------------------------------------------------------------}
{						ARM CPU ID STRUCTURE DEFINED						}
{--------------------------------------------------------------------------*/
typedef union 
{
	struct 
	{
		unsigned Revision : 4;										// @0-3  CPU minor revision 
		unsigned PartNumber: 12;									// @4-15  Partnumber
		unsigned Architecture : 4;									// @16-19 Architecture
		unsigned Variant : 4;										// @20-23 Variant
		unsigned Implementer : 8;									// @24-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} CPU_ID;

/*--------------------------------------------------------------------------}
{				SMARTSTART VERSION STRUCTURE DEFINED						}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned LoVersion : 16;									// @0-15  SmartStart minor version 
		unsigned HiVersion : 8;										// @16-23 SmartStart major version
		unsigned _reserved : 8;										// @24-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} SMARTSTART_VER;

/***************************************************************************}
{                      PUBLIC INTERFACE MEMORY VARIABLES                    }
{***************************************************************************/
extern uint32_t RPi_IO_Base_Addr;				// RPI IO base address auto-detected by SmartStartxx.S
extern uint32_t RPi_ARM_TO_GPU_Alias;			// RPI ARM_TO_GPU_Alias auto-detected by SmartStartxx.S
extern uint32_t RPi_BootAddr;					// RPI address processor booted from auto-detected by SmartStartxx.S
extern uint32_t RPi_CoresReady;					// RPI cpu cores made read for use by SmartStartxx.S
extern uint32_t RPi_CPUBootMode;				// RPI cpu mode it was in when it booted
extern CPU_ID RPi_CpuId;						// RPI CPU type auto-detected by SmartStartxx.S
extern CODE_TYPE RPi_CompileMode;				// RPI code type that compiler produced
extern uint32_t RPi_CPUCurrentMode;				// RPI cpu current operation mode
extern SMARTSTART_VER RPi_SmartStartVer;		// SmartStart version

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
} POINT, *LPPOINT;										// Typedef define POINT and LPPOINT


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
typedef struct __attribute__((__packed__)) tagBITMAP  {
	uint32_t   bmType;									// Always zero ... bitmap objtype ID
	uint32_t   bmWidth;									// Width in pixels of the bitmap
	uint32_t   bmHeight;								// Height in pixels of the bitmap
	uint32_t   bmWidthBytes;							// The number of bytes in each scan line as it must be balign4
	uint32_t   bmPlanes;								// The count of color planes
	union {
		void*	 bmBits;
		HIMAGE	 bmImage;
	};
	struct __attribute__((__packed__)) {
		unsigned bmBitsPixel : 16;						// The number of bits required to indicate the color of a pixel
		unsigned bmBottomUp : 1;						// Image draws from bottom up
		unsigned _reserved : 15;						// Reserved but unused bits
	};
} BITMAP, *HBITMAP;

/*--------------------------------------------------------------------------}
{	                    HGDIOBJ - HANDLE TO GDI OBJECT						}
{--------------------------------------------------------------------------*/
typedef union {
	uint32_t* objType;									// Pointer to the type which is always first uint32_t							
	HBITMAP bitmap;										// Bitmap is one type of GDI object
} HGDIOBJ;

#define HGDI_ERROR 0

/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{		  INTERRUPT HELPER ROUTINES PROVIDE BY RPi-SmartStart API	        }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[setFiqFuncAddress]------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Sets the function pointer to be the called when an Fiq interrupt occurs.
. CPU fiq interrupts will be disabled so they can't trigger while changing.
. RETURN: Old function pointer that was in use (will return 0 if never set).
.--------------------------------------------------------------------------*/
uintptr_t setFiqFuncAddress (void(*ARMaddress)(void));

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			GLOBAL INTERRUPT CONTROL PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[EnableInterrupts]-------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Enable global interrupts on any CPU core calling this function.
.--------------------------------------------------------------------------*/
void EnableInterrupts (void);

/*-[DisableInterrupts]------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Disable global interrupts on any CPU core calling this function.
.--------------------------------------------------------------------------*/
void DisableInterrupts (void);

/*-[EnableFIQ]--------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Enable global fast interrupts on any CPU core calling this function.
.--------------------------------------------------------------------------*/
void EnableFIQ(void);

/*-[DisableFIQ]-------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Disable global fast interrupts on any CPU core calling this function.
.--------------------------------------------------------------------------*/
void DisableFIQ(void);

/*-[getDAIF]----------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Return the DAIF flags for any CPU core calling this function.
.--------------------------------------------------------------------------*/
unsigned long getDAIF(void);

/*-[setDAIF]----------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Sets the DAIF flags for any CPU core calling this function.
.--------------------------------------------------------------------------*/
void setDAIF (unsigned long flags);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			 RPi-SmartStart API TO MULTICORE FUNCTIONS					    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[getCoreID]------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Returns the multicore id of the core calling the function
.--------------------------------------------------------------------------*/
unsigned int getCoreID (void);

/*-[CoreExecute]------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Commands the given parked core to execute the function provided. The core
. called must be parked in the secondary spinloop. All secondary cores are
. automatically parked by the normal SmartStart boot so are ready to deploy
.--------------------------------------------------------------------------*/
bool CoreExecute (uint8_t coreNum, void (*func) (void) );


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{		VC4 GPU ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ARMaddrToGPUaddr]-------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Converts an ARM address to GPU address by using the GPU_alias offset
.--------------------------------------------------------------------------*/
uint32_t ARMaddrToGPUaddr (void* ARMaddress);

/*-[GPUaddrToARMaddr]-------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Converts a GPU address to an ARM address by using the GPU_alias offset
.--------------------------------------------------------------------------*/
uint32_t GPUaddrToARMaddr (uint32_t GPUaddress);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{	  RPi-SmartStart Compatability for David Welch CALLS he always uses	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ PUT32 ]----------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Put the 32bit value out to the given address (Match Davids calls)
.--------------------------------------------------------------------------*/
void PUT32 (uint32_t addr, uint32_t value);

/*-[ GET32 ]----------------------------------------------------------------}
. NOTE: Public C interface only to code located in SmartsStartxx.S
. Get the 32bit value from the given address (Match Davids calls)
.--------------------------------------------------------------------------*/
uint32_t GET32 (uint32_t addr);

/*==========================================================================}
{			 PUBLIC CPU ID ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[ RPi_CpuIdString]-------------------------------------------------------}
. Returns the CPU id string of the CPU auto-detected by the SmartStart code
.--------------------------------------------------------------------------*/
const char* RPi_CpuIdString (void);

/*==========================================================================}
{			 PUBLIC GPIO ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[gpio_setup]-------------------------------------------------------------}
. Given a valid GPIO port number and mode sets GPIO to given mode
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_setup (unsigned int gpio, GPIOMODE mode);

/*-[gpio_output]------------------------------------------------------------}
. Given a valid GPIO port number the output is set high(true) or Low (false)
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_output (unsigned int gpio, bool on);

/*-[gpio_input]-------------------------------------------------------------}
. Reads the actual level of the GPIO port number
. RETURN: true = GPIO input high, false = GPIO input low
.--------------------------------------------------------------------------*/
bool gpio_input (unsigned int gpio);

/*-[gpio_checkEvent]--------------------------------------------------------}
. Checks the given GPIO port number for an event/irq flag.
. RETURN: true for event occured, false for no event
.--------------------------------------------------------------------------*/
bool gpio_checkEvent (unsigned int gpio);

/*-[gpio_clearEvent]--------------------------------------------------------}
. Clears the given GPIO port number event/irq flag.
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_clearEvent (unsigned int gpio);

/*-[gpio_edgeDetect]--------------------------------------------------------}
. Sets GPIO port number edge detection to lifting/falling in Async/Sync mode
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_edgeDetect (unsigned int gpio, bool lifting, bool Async);

/*-[gpio_fixResistor]-------------------------------------------------------}
. Set the GPIO port number with fix resistors to pull up/pull down.
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_fixResistor (unsigned int gpio, GPIO_FIX_RESISTOR resistor);

/*==========================================================================}
{		   PUBLIC TIMER ROUTINES PROVIDED BY RPi-SmartStart API				}
{==========================================================================*/

/*-[timer_getTickCount64]---------------------------------------------------}
. Get 1Mhz ARM system timer tick count in full 64 bit.
. The timer read is as per the Broadcom specification of two 32bit reads
. RETURN: tickcount value as an unsigned 64bit value in microseconds (usec)
.--------------------------------------------------------------------------*/
uint64_t timer_getTickCount64 (void);

/*-[timer_Wait]-------------------------------------------------------------}
. This will simply wait the requested number of microseconds before return.
.--------------------------------------------------------------------------*/
void timer_wait (uint64_t usec);

/*-[tick_Difference]--------------------------------------------------------}
. Given two timer tick results it returns the time difference between them.
. If (us1 > us2) it is assumed the timer rolled as we expect (us2 > us1)
.--------------------------------------------------------------------------*/
uint64_t tick_difference (uint64_t us1, uint64_t us2);

/*==========================================================================}
{		  PUBLIC PI MAILBOX ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[mailbox_write]----------------------------------------------------------}
. This will execute the sending of the given data block message thru the
. mailbox system on the given channel. It is normal for a response back so
. usually you need to follow the write up with a read.
. RETURN: True for success, False for failure.
.--------------------------------------------------------------------------*/
bool mailbox_write (MAILBOX_CHANNEL channel, uint32_t message);

/*-[mailbox_read]-----------------------------------------------------------}
. This will read any pending data on the mailbox system on the given channel.
. RETURN: The read value for success, 0xFEEDDEAD for failure.
.--------------------------------------------------------------------------*/
uint32_t mailbox_read (MAILBOX_CHANNEL channel);

/*-[mailbox_tag_message]----------------------------------------------------}
. This will post and execute the given variadic data onto the tags channel
. on the mailbox system. You must provide the correct number of response
. uint32_t variables and a pointer to the response buffer. You nominate the
. number of data uint32_t for the call and fill the variadic data in. If you
. do not want the response data back the use NULL for response_buffer.
. RETURN: True for success and the response data will be set with data
.         False for failure and the response buffer is untouched.
.--------------------------------------------------------------------------*/
bool mailbox_tag_message (uint32_t* response_buf,					// Pointer to response buffer (NULL = no response wanted)
						  uint8_t data_count,						// Number of uint32_t data to be set for call
						  ...);										// Variadic uint32_t values for call

/*==========================================================================}
{	  PUBLIC PI TIMER INTERRUPT ROUTINES PROVIDED BY RPi-SmartStart API		}
{==========================================================================*/

/*-[ClearTimerIrq]----------------------------------------------------------}
. Simply clear the timer interupt by hitting the clear register. Any timer
. fiq/irq interrupt should call this before exiting.
.--------------------------------------------------------------------------*/
void ClearTimerIrq (void);

/*-[TimerIrqSetup]----------------------------------------------------------}
. The timer irq interrupt rate is set to the period in usec between triggers.
. Largest period is around 16 million usec (16 sec) it varies on core speed
. RETURN: TRUE if successful,  FALSE for any failure
.--------------------------------------------------------------------------*/
bool TimerIrqSetup (uint32_t period_in_us);							// Period between timer interrupts in usec

/*-[TimerFiqSetup]----------------------------------------------------------}
. Allocates the given TimerFiqHandler function pointer to be the fiq call
. when a timer interrupt occurs. The interrupt rate is set by providing a
. period in usec between triggers of the interrupt. If the FIQ function is
. redirected by another means use 0 which causes handler set to be ignored.
. Largest period is around 16 million usec (16 sec) it varies on core speed
. RETURN: The old function pointer that was in use (will return 0 for 1st).
.--------------------------------------------------------------------------*/
uintptr_t TimerFiqSetup (uint32_t period_in_us, 					// Period between timer interrupts in usec
						 void (*ARMaddress)(void));					// Function to call (0 = ignored)

/*-[ClearLocalTimerIrq]-----------------------------------------------------}
. Simply clear the local timer interupt by hitting the clear registers. Any
. local timer irq/fiq interrupt should call this before exiting.
.--------------------------------------------------------------------------*/
void ClearLocalTimerIrq (void);

/*-[LocalTimerIrqSetup]-----------------------------------------------------}
. The local timer irq interrupt rate is set to the period in usec between 
. triggers. On BCM2835 (ARM6) it does not have core timer so call fails. 
. Largest period is around 16 million usec (16 sec) it varies on core speed
. RETURN: TRUE if successful, FALSE for any failure
.--------------------------------------------------------------------------*/
bool LocalTimerSetup (uint32_t period_in_us,						// Period between timer interrupts in usec
					  uint8_t coreNum);								// Core number

/*==========================================================================}
{				           MINIUART ROUTINES								}
{==========================================================================*/

/*-[miniuart_init]----------------------------------------------------------}
. Initializes the miniuart (NS16550 compatible one) to the given baudrate
. with 8 data bits, no parity and 1 stop bit. On the PI models 1A, 1B, 1B+
. and PI ZeroW this is external GPIO14 & GPIO15 (Pin header 8 & 10).
. RETURN: true if successfuly set, false on any error
.--------------------------------------------------------------------------*/
bool miniuart_init (unsigned int baudrate);

/*-[miniuart_getc]----------------------------------------------------------}
. Wait for an retrieve a character from the uart.
.--------------------------------------------------------------------------*/
char miniuart_getc (void);

/*-[miniuart_putc]----------------------------------------------------------}
. Send a character out via uart.
.--------------------------------------------------------------------------*/
void miniuart_putc (char c);

/*-[miniuart_puts]----------------------------------------------------------}
. Send a '\0' terminated character string out via uart.
.--------------------------------------------------------------------------*/
void miniuart_puts (char *str);

/*==========================================================================}
{				           PL011 UART ROUTINES								}
{==========================================================================*/

/*-[pl011_uart_init]--------------------------------------------------------}
. Initializes the pl011 uart to the given baudrate with 8 data, no parity
. and 1 stop bit. On the PI models 2B, Pi3, Pi3B+, Pi Zero and CM this is
. external GPIO14 & GPIO15 (Pin header 8 & 10).
. RETURN: true if successfuly set, false on any error
.--------------------------------------------------------------------------*/
bool pl011_uart_init (unsigned int baudrate);

/*-[pl011_uart_getc]--------------------------------------------------------}
. Wait for an retrieve a character from the uart.
.--------------------------------------------------------------------------*/
char pl011_uart_getc (void);

/*-[pl011_uart_putc]--------------------------------------------------------}
. Send a character out via uart.
.--------------------------------------------------------------------------*/
void pl011_uart_putc (char c);

/*-[pl011_uart_puts]--------------------------------------------------------}
. Send a '\0' terminated character string out via uart.
.--------------------------------------------------------------------------*/
void pl011_uart_puts (char *str);

/*==========================================================================}
{				         UART CONSOLE ROUTINES								}
{==========================================================================*/

/*-[console_uart_init]------------------------------------------------------}
. This will detect the Pi model at runtime and set the baud rate and console 
. uart functions to pins 6 & 8 on header for the detected model. It could be 
. either the PL011 or Miniuart port depending on Pi Model but all that is 
. shielded from you. So you can just use these calls and know on whichever 
. model the code is placed it will come out at header pins.
. RETURN: true if successfuly set, false on any error
.--------------------------------------------------------------------------*/
bool console_uart_init (unsigned int baudrate);

/*-[console_uart_getc]------------------------------------------------------}
. Wait for an retrieve a character from the runtime detected uart that goes
. to Pins 6 & 8 on the 40 way header. It could be minuart or PL011 depending 
. on Pi model detected at runtime. If console_uart_init has not been called
. the function will simply immediately return with a character 0.
.--------------------------------------------------------------------------*/
char console_uart_getc (void);

/*-[console_uart_putc]------------------------------------------------------}
. Writes a character to the runtime detected uart that goes to Pins 6 & 8 on
. the 40 way header. It could be minuart or PL011 depending on Pi model that
. was detected at runtime. If console_uart_init has not been called then the
. function will simply immediately return.
.--------------------------------------------------------------------------*/
void console_uart_putc (char ch);

/*-[console_uart_puts]------------------------------------------------------}
. Writes a a '\0' terminated character string to the runtime detected uart
. that goes to Pins 6 & 8 on the 40 way header. It could be minuart or PL011
. depending on Pi model that was detected at runtime. If console_uart_init
. has not been called then the function will simply immediately return.
.--------------------------------------------------------------------------*/
void console_uart_puts (char *str);

/*==========================================================================}
{	   PUBLIC PI ACTIVITY LED ROUTINE PROVIDED BY RPi-SmartStart API		}
{==========================================================================*/

/*-[set_Activity_LED]-------------------------------------------------------}
. This will set the PI activity LED on or off as requested. The SmartStart
. stub provides the Pi board autodetection so the right GPIO port is used.
. RETURN: True the LED state was successfully change, false otherwise
.--------------------------------------------------------------------------*/
bool set_Activity_LED (bool on);

/*==========================================================================}
{	   PUBLIC ARM CPU SPEED SET ROUTINES PROVIDED BY RPi-SmartStart API	 	}
{==========================================================================*/

/*-[ARM_setmaxspeed]--------------------------------------------------------}
. This will set the ARM cpu to the maximum. You can optionally print confirm
. message to screen or uart etc by providing a print function handler.
. The print handler can be screen or a uart/usb/eth handler for monitoring.
. RETURN: True maxium speed was successfully set, false otherwise
.--------------------------------------------------------------------------*/
bool ARM_setmaxspeed (int (*prn_handler) (const char *fmt, ...) );

/*==========================================================================}
{				      SMARTSTART DISPLAY ROUTINES							}
{==========================================================================*/

/*-[displaySmartStart]------------------------------------------------------}
. Will print 2 lines of basic smart start details to given print handler
. The print handler can be screen or a uart/usb/eth handler for monitoring.
.--------------------------------------------------------------------------*/
void displaySmartStart (int (*prn_handler) (const char *fmt, ...));

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
bool PiConsole_Init (int Width,										// Screen width request (Use 0 if you wish autodetect width) 
					 int Height,									// Screen height request (Use 0 if you wish autodetect height) 	
					 int Depth,										// Screen colour depth request (Use 0 if you wish autodetect colour depth) 
					 int(*prn_handler) (const char *fmt, ...));		// Print handler to display setting result on if successful

/*-[GetConsole_FrameBuffer]-------------------------------------------------}
. Simply returns the console frame buffer. If PiConsole_Init has not yet been
. called it will return 0, which sort of forms an error check value.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_FrameBuffer (void);

/*-[GetConsole_Width]-------------------------------------------------------}
. Simply returns the console width. If PiConsole_Init has not yet been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected width setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Width (void);

/*-[GetConsole_Height]------------------------------------------------------}
. Simply returns the console height. If PiConsole_Init has not been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected height setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Height (void);

/*-[GetConsole_Depth]-------------------------------------------------------}
. Simply returns the console depth. If PiConsole_Init has not yet been called
. it will return 0, which sort of forms an error check value. If autodetect
. setting was used it is a simple way to get the detected colour depth setup.
.--------------------------------------------------------------------------*/
uint32_t GetConsole_Depth (void);

/*==========================================================================}
{			  PI BASIC DISPLAY CONSOLE CURSOR POSITION ROUTINES 			}
{==========================================================================*/

/*-[WhereXY]----------------------------------------------------------------}
. Simply returns the console cursor x,y position into any valid pointer.
. If a value is not required simply set the pointer to 0 to signal ignore.
.--------------------------------------------------------------------------*/
void WhereXY (uint32_t* x, uint32_t* y);

/*-[GotoXY]-----------------------------------------------------------------}
. Simply set the console cursor to the x,y position. Subsequent text writes
. using console write will then be to that cursor position. This can even be
. set before any screen initialization as it just sets internal variables.
.--------------------------------------------------------------------------*/
void GotoXY (uint32_t x, uint32_t y);

/*==========================================================================}
{			  PI BASIC DISPLAY CONSOLE TEXT OUTPUT ROUTINES					}
{==========================================================================*/

/*-[WriteText]--------------------------------------------------------------}
. Simply writes the given null terminated string out to the the console at
. current cursor x,y position. If PiConsole_Init has not yet been called it 
. will simply return, as it does for empty of invalid string pointer.
.--------------------------------------------------------------------------*/
void WriteText (char* lpString);

/*==========================================================================}
{							GDI OBJECT ROUTINES								}
{==========================================================================*/

HGDIOBJ SelectObject (HDC hdc,										// Handle to the DC (0 means use standard console DC)
					  HGDIOBJ h);									// Handle to GDI object


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif

