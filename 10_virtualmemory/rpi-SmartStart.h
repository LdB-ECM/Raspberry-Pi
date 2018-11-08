#ifndef _SMART_START_
#define _SMART_START_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: rpi-smartstart.h											}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 2.02														}
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
{																            }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc

/***************************************************************************}
{		  PUBLIC MACROS MUCH AS WE HATE THEM SOMETIMES YOU NEED THEM        }
{***************************************************************************/

/* Most versions of C don't have _countof macro so provide it if not available */
#if !defined(_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0])) 
#endif

/* As we are compiling for Raspberry Pi if winmain make it main */
#define WinMain(...) main (uint32_t r0, uint32_t r1, uint32_t atags)

/* System font is 8 wide and 16 height so these are preset for the moment */
#define BitFontHt 16
#define BitFontWth 8

/* Timer interrupt handler function proto type */
typedef void (*TimerIrqHandler) (void);

/***************************************************************************}
{						  PUBLIC MEMORY BARRIER ROUTINES				    }
{***************************************************************************/
#if (__ARM_ARCH == 6)							// Compiling for ARM 6
#define ISB(x) asm volatile ("mcr p15, 0, r3, c7, c5, 4" : : "r3" : "memory")
#define DSB(x) asm volatile ("mcr p15, 0, r3, c7, c10, 4" : : "r3" : "memory")
#define DMB(x) asm volatile ("mcr p15, 0, r3, c7, c10, 5" : : "r3" : "memory")
#endif
#if (__ARM_ARCH == 7)							// Compiling for ARM 7
#define ISB(option) asm volatile ("isb " #option : : : "memory")
#define DSB(option) asm volatile ("dsb " #option : : : "memory")
#define DMB(option) asm volatile ("dmb " #option : : : "memory")
#endif

#if (__ARM_ARCH == 8)  							// Compiling for an ARM 8
#define ISB(option) asm volatile ("isb " #option : : : "memory")
#define DSB(option) asm volatile ("dsb " #option : : : "memory")
#define DMB(option) asm volatile ("dmb " #option : : : "memory")
#endif

/***************************************************************************}
{					     PUBLIC ENUMERATION CONSTANTS			            }
****************************************************************************/

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
		unsigned reserved : 24;										// @9-31 reserved
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

/***************************************************************************}
{                      PUBLIC INTERFACE MEMORY VARIABLES                    }
{***************************************************************************/
extern uint32_t RPi_IO_Base_Addr;				// RPI IO base address auto-detected by SmartStartxx.S
extern uint32_t RPi_BootAddr;					// RPI address processor booted from auto-detected by SmartStartxx.S
extern uint32_t RPi_CoresReady;					// RPI cpu cores made read for use by SmartStartxx.S
extern CPU_ID RPi_CpuId;						// RPI CPU type auto-detected by SmartStartxx.S
extern CODE_TYPE RPi_CompileMode;				// RPI code type that compiler produced
extern uint32_t RPi_CPUBootMode;				// RPI cpu mode it was in when it booted
extern uint32_t RPi_CPUCurrentMode;				// RPI cpu current operation mode

/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*==========================================================================}
{			 PUBLIC CPU ID ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[ RPi_CpuIdString]-------------------------------------------------------}
. Returns the CPU id string of the CPU auto-detected by the SmartStart code
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
const char* RPi_CpuIdString (void);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			GLOBAL INTERRUPT CONTROL PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
extern void EnableInterrupts (void);			// Enable global interrupts
extern void DisableInterrupts (void);			// Disable global interrupts


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{	 	RPi-SmartStart API TO GET CORE ID & EXECUTE ROUTINE AT ADDRESS 	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
uint8_t GetCoreID (void);

/* Execute function on (1..CoresReady) */
bool CoreExecute (uint8_t coreNum, void (*func) (void)); 

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{		VC4 GPU ADDRESS HELPER ROUTINES PROVIDE BY RPi-SmartStart API	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* ARM bus address to GPU bus address */
uint32_t ARMaddrToGPUaddr (void* ARMaddress);

/* GPU bus address to ARM bus address */
uint32_t GPUaddrToARMaddr (uint32_t GPUaddress);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{	  RPi-SmartStart Compatability for David Welch CALLS he always uses	    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ PUT32 ]----------------------------------------------------------------}
. Put the 32bit value out to the given address
. 15May18 LdB
.--------------------------------------------------------------------------*/
void PUT32(uint32_t addr, uint32_t value);

/*-[ GET32 ]----------------------------------------------------------------}
. Get the 32bit value from the given address
. 15May18 LdB
.--------------------------------------------------------------------------*/
uint32_t GET32(uint32_t addr);

/*==========================================================================}
{			 PUBLIC GPIO ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[gpio_setup]-------------------------------------------------------------}
. Given a valid GPIO port number and mode sets GPIO to given mode
. RETURN: true for success, false for any failure
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_setup (uint_fast8_t gpio, GPIOMODE mode);

/*-[gpio_output]------------------------------------------------------------}
. Given a valid GPIO port number the output is set high(true) or Low (false)
. RETURN: true for success, false for any failure
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_output (uint_fast8_t gpio, bool on);

/*-[gpio_input]-------------------------------------------------------------}
. Reads the actual level of the GPIO port number
. RETURN: true = GPIO input high, false = GPIO input low
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_input (uint_fast8_t gpio);

/*-[gpio_checkEvent]--------------------------------------------------------}
. Checks the given GPIO port number for an event/irq flag.
. RETURN: true for event occured, false for no event
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_checkEvent (uint_fast8_t gpio);

/*-[gpio_clearEvent]--------------------------------------------------------}
. Clears the given GPIO port number event/irq flag.
. RETURN: true for success, false for any failure
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_clearEvent (uint_fast8_t gpio);

/*-[gpio_edgeDetect]--------------------------------------------------------}
. Sets GPIO port number edge detection to lifting/falling in Async/Sync mode
. RETURN: true for success, false for any failure
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_edgeDetect (uint_fast8_t gpio, bool lifting, bool Async);

/*-[gpio_fixResistor]-------------------------------------------------------}
. Set the GPIO port number with fix resistors to pull up/pull down.
. RETURN: true for success, false for any failure
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
bool gpio_fixResistor (uint_fast8_t gpio, GPIO_FIX_RESISTOR resistor);

/*==========================================================================}
{		   PUBLIC TIMER ROUTINES PROVIDED BY RPi-SmartStart API				}
{==========================================================================*/

/*-[timer_getTickCount]-----------------------------------------------------}
. Get 1Mhz ARM system timer tick count in full 64 bit.
. The timer read is as per the Broadcom specification of two 32bit reads
. RETURN: tickcount value as an unsigned 64bit value in milliseconds (msec)
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
uint64_t timer_getTickCount (void);

/*-[timer_getTickCount64]---------------------------------------------------}
. Get 1Mhz ARM system timer tick count in full 64 bit.
. The timer read is as per the Broadcom specification of two 32bit reads
. RETURN: tickcount value as an unsigned 64bit value in microseconds (usec)
. 30Jun17 LdB
.--------------------------------------------------------------------------*/
uint64_t timer_getTickCount64 (void);

/*-[timer_Wait]-------------------------------------------------------------}
. This will simply wait the requested number of microseconds before return.
. 02Jul17 LdB
.--------------------------------------------------------------------------*/
void timer_wait (uint64_t us);

/*-[tick_Difference]--------------------------------------------------------}
. Given two timer tick results it returns the time difference between them.
. 02Jul17 LdB
.--------------------------------------------------------------------------*/
uint64_t tick_difference (uint64_t us1, uint64_t us2);

/*==========================================================================}
{		  PUBLIC PI MAILBOX ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[mailbox_write]----------------------------------------------------------}
. This will execute the sending of the given data block message thru the
. mailbox system on the given channel.
. RETURN: True for success, False for failure.
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
bool mailbox_write (MAILBOX_CHANNEL channel, uint32_t message);

/*-[mailbox_read]-----------------------------------------------------------}
. This will read any pending data on the mailbox system on the given channel.
. RETURN: The read value for success, 0xFEEDDEAD for failure.
. 04Jul17 LdB
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
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
bool mailbox_tag_message (uint32_t* response_buf,					// Pointer to response buffer (NULL = no response wanted)
						  uint8_t data_count,						// Number of uint32_t data to be set for call
						  ...);										// Variadic uint32_t values for call

/*==========================================================================}
{	  PUBLIC PI TIMER INTERRUPT ROUTINES PROVIDED BY RPi-SmartStart API		}
{==========================================================================*/

/*-[setTimerIrqAddress]-----------------------------------------------------}
. Allocates the given TimerIrqHandler function pointer to be the call when
. a timer interrupt occurs. As we are undoubtedly setting up the interrupt
. the interrupt is disabled. 
. RETURN: The old function pointer that was in use (will return 0 for 1st).
. 19Sep17 LdB
.--------------------------------------------------------------------------*/
TimerIrqHandler setTimerIrqAddress (TimerIrqHandler ARMaddress);

/*-[TimerIrqSetup]----------------------------------------------------------}
. Allocates the given TimerIrqHandler function pointer to be the irq call 
. when a timer interrupt occurs. The interrupt rate is set by providing a 
. period in usec between triggers of the interrupt.
. RETURN: The old function pointer that was in use (will return 0 for 1st).
. 19Sep17 LdB
.--------------------------------------------------------------------------*/
TimerIrqHandler TimerIrqSetup (uint32_t period_in_us,				// Period between timer interrupts in usec
							   TimerIrqHandler ARMaddress);         // Function to call on interrupt


/*==========================================================================}
{	   PUBLIC PI ACTIVITY LED ROUTINE PROVIDED BY RPi-SmartStart API		}
{==========================================================================*/

/*-[set_Activity_LED]-------------------------------------------------------}
. This will set the PI activity LED on or off as requested. The SmartStart
. stub provides the Pi board autodetection so the right GPIO port is used.
. RETURN: True the LED state was successfully change, false otherwise
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
bool set_Activity_LED (bool on);

/*==========================================================================}
{	   PUBLIC ARM CPU SPEED SET ROUTINES PROVIDED BY RPi-SmartStart API	 	}
{==========================================================================*/

/*-[ARM_setmaxspeed]--------------------------------------------------------}
. This will set the ARM cpu to the maximum. You can optionally print confirm
. message to screen but providing a print handler.
. RETURN: True maxium speed was successfully set, false otherwise
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
bool ARM_setmaxspeed ( int(*printhandler) (const char *fmt, ...) );

/*==========================================================================}
{				      SMARTSTART DISPLAY ROUTINES							}
{==========================================================================*/

/*-[displaySmartStart]------------------------------------------------------}
. This will print 2 lines of basic smart start details to given print handler
. 04Jul17 LdB
.--------------------------------------------------------------------------*/
void displaySmartStart ( int(*printhandler) (const char *fmt, ...) );

/*==========================================================================}
{		 PUBLIC PI MINIUART ROUTINE PROVIDED BY RPi-SmartStart API	    	}
{==========================================================================*/
bool miniuart_init (uint32_t baudrate);
char miniuart_getc (void);
void miniuart_putc (const char c);
void miniuart_puts (const char *str);

/*==========================================================================}
{		PUBLIC PI PL011 UART ROUTINE PROVIDED BY RPi-SmartStart API	    	}
{==========================================================================*/
bool pl011_uart_init (uint32_t baudrate);
char pl011_uart_getc (void);
void pl011_uart_putc (const char c);
void pl011_uart_puts (const char *str);

void semaphore_inc(uint32_t* sem);
void semaphore_dec(uint32_t* sem);


typedef int32_t		BOOL;							// BOOL is defined to an int32_t ... yeah windows is weird -1 is often returned
typedef char		TCHAR;							// TCHAR is a char
typedef uintptr_t	HDC;							// HDC is really a pointer
typedef uintptr_t	HANDLE;							// HANDLE is really a pointer
typedef uintptr_t	HINSTANCE;						// HINSTANCE is really a pointer
typedef uint32_t	UINT;							// UINT is an unsigned 32bit int
typedef char*		LPCTSTR;						// LPCTSTR is a char pointer

#define TRUE 1
#define FALSE 0

#define IMAGE_BITMAP	0
#define IMAGE_ICON		1
#define IMAGE_CURSOR	2

#define LR_DEFAULTCOLOR			0x00000000
#define LR_MONOCHROME			0x00000001
#define LR_COLOR				0x00000002
#define LR_COPYRETURNORG		0x00000004
#define LR_COPYDELETEORG		0x00000008
#define	LR_LOADFROMFILE			0x00000010
#define LR_LOADTRANSPARENT		0x00000020
#define LR_DEFAULTSIZE			0x00000040
#define LR_VGACOLOR				0x00000080
#define LR_LOADMAP3DCOLORS		0x00001000
#define LR_CREATEDIBSECTION		0x00002000
#define LR_COPYFROMRESOURCE		0x00004000
#define LR_SHARED				0x00008000


/***************************************************************************}
{		 		    PUBLIC STRUCTURE DEFINITIONS				            }
****************************************************************************/

typedef struct __attribute__((__packed__, aligned(1)))
{
	uint8_t rgbBlue;								// Blue
	uint8_t rgbGreen;								// Green
	uint8_t rgbRed;									// Red
} RGBTRIPLE;


typedef union 
{
	struct __attribute__((__packed__, aligned(1)))
	{
		uint8_t rgbBlue;							// Blue
		uint8_t rgbGreen;							// Green
		uint8_t rgbRed;								// Red
		uint8_t rgbAlpha;							// Alpha
	};
	RGBTRIPLE rgb;									// RGB triple (1st 3 bytes)
	uint32_t Raw32;									// Raw access to all 32 bits
} RGBA;

typedef union 
{
	struct __attribute__((__packed__, aligned(1)))
	{
		unsigned B : 5;								// Blue
		unsigned G : 6;								// Green
		unsigned R : 5;								// Red
	};
	uint16_t Raw16;									// Access all 16 bits as one
} RGB565;


typedef union
{
	RGBA rgba;										// RGBA 
	RGBTRIPLE rgb;									// RGB triple (1st 3 bytes)
	uint32_t Raw32;									// Raw access to all 32 bits								
} COLORREF;

/*--------------------------------------------------------------------------}
{	                 HBITMAP - HANDLE TO BITMAP IN MEMORY					}
{--------------------------------------------------------------------------*/
typedef union 
{
	uint8_t* rawImage;								// Pointer to raw byte format array
	RGB565* __attribute__((aligned(2))) ptrRGB565;	// Pointer to RGB565 format array
	RGBTRIPLE* __attribute__((aligned(1))) ptrRGB;	// Pointer to RGB format array
	RGBA* __attribute__((aligned(4))) ptrRGBA;		// Pointer to RGBA format array
	uintptr_t rawPtr;								// Pointer address
} HBITMAP;


typedef struct 
{
	int_fast32_t x;									// x co-ordinate
	int_fast32_t y;									// y co-ordinate
} POINT, *LPPOINT;									// Typedef define POINT and LPPOINT


/*--------------------------------------------------------------------------}
{                        BITMAP FILE HEADER DEFINITION                      }
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(2))) 
{
	uint16_t  bfType; 												// Bitmap type should be "BM"
	uint32_t  bfSize; 												// Bitmap size in bytes
	uint16_t  bfReserved1; 											// reserved short1
	uint16_t  bfReserved2; 											// reserved short2
	uint32_t  bfOffBits; 											// Offset to bmp data
} BITMAPFILEHEADER;

/*--------------------------------------------------------------------------}
{                    BITMAP FILE INFO HEADER DEFINITION						}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(2))) 
{
	uint32_t biSize; 												// Bitmap file size
	uint32_t biWidth; 												// Bitmap width
	uint32_t biHeight;												// Bitmap height
	uint16_t biPlanes; 												// Planes in image
	uint16_t biBitCount; 											// Bits per byte
	uint32_t biCompression; 										// Compression technology
	uint32_t biSizeImage; 											// Image byte size
	uint32_t biXPelsPerMeter; 										// Pixels per x meter
	uint32_t biYPelsPerMeter; 										// Pixels per y meter
	uint32_t biClrUsed; 											// Number of color indexes needed
	uint32_t biClrImportant; 										// Min colours needed
} BITMAPINFOHEADER;


bool PiConsole_Init(int Width, int Height, int Depth, int(*printhandler) (const char *fmt, ...) );
void WriteText(int x, int y, char* txt);
void Embedded_Console_WriteChar (char Ch);
bool TransparentTextOut (int nXStart, int nYStart, const char* lpString);

HDC GetConsoleDC(void);
uint32_t GetConsole_FrameBuffer(void);
uint32_t GetConsole_Width (void);
uint32_t GetConsole_Height (void);
void WhereXY(uint32_t* x, uint32_t* y);
void GotoXY (uint32_t x, uint32_t y);


COLORREF SetPixel(HDC hdc, uint_fast32_t x, uint_fast32_t y, COLORREF crColor);

COLORREF SetDCPenColor(HDC      hdc,							// Handle to the DC
					  COLORREF crColor);						// The new pen color

COLORREF SetDCBrushColor(HDC      hdc,						// Handle to the DC
	COLORREF crColor);					// The new brush color

COLORREF SetTextColor(HDC      hdc,
	COLORREF crColor
);

BOOL MoveToEx(HDC hdc,
	int_fast32_t X,
	int_fast32_t Y,
	POINT* lpPoint);

BOOL LineTo(HDC hdc,
	int nXEnd,
	int nYEnd);

BOOL TextOut(HDC				hdc,
	int_fast32_t     nXStart,
	int_fast32_t     nYStart,
	const TCHAR*		lpString,
	int_fast32_t     cchString);

BOOL Rectangle(HDC hdc,
	int_fast32_t nLeftRect,
	int_fast32_t nTopRect,
	int_fast32_t nRightRect,
	int_fast32_t nBottomRect);

BOOL BmpOut(HDC hdc,
	uint32_t nXStart,
	uint32_t nYStart,
	uint32_t cX,
	uint32_t cY,
	uint8_t* imgSrc);

BOOL CvtBmpLine(HDC hdc,
	uint32_t nXStart,
	uint32_t nYStart,
	uint32_t cX,
	uint32_t imgDepth,
	uint8_t* imgSrc);

typedef char* caddr_t;
caddr_t __attribute__((weak)) _sbrk(int incr);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif

