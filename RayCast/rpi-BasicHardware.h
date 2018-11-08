#ifndef _RPI_BASIC_HARDWARE_					// Check RPI_BASIC_HARDWARE guard
#define _RPI_BASIC_HARDWARE_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, uint64_t etc
#include "rpi-smartstart.h"						// Needed for RPi_IO_Base_Addr 

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{       Filename: rpi-BasicHardware.h							}
{       Copyright(c): Leon de Boer(LdB) 2017					}
{       Version: 1.01											}
{																}
{******************[ THIS CODE IS FREEWARE ]********************}
{																}
{     This sourcecode is released for the purpose to promote	}
{   programming on the Raspberry Pi. You may redistribute it    }
{   and/or modify with the following disclaimer.                }
{																}
{   The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES	}
{   AS TO PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR   } 
{   IMPLIED. Redistributions of source code must retain the     }
{   copyright notices.                                          }	
{																}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/***************************************************************************}
{					     PUBLIC ENUMERATION CONSTANTS			            }
****************************************************************************/

/*--------------------------------------------------------------------------}
;{	      ENUMERATED FSEL REGISTERS ... BCM2835.PDF MANUAL see page 92		}
;{-------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	INPUT = 0b000,				// 0
	OUTPUT = 0b001,				// 1
	ALTFUNC5 = 0b010,			// 2
	ALTFUNC4 = 0b011,			// 3
	ALTFUNC0 = 0b100,			// 4
	ALTFUNC1 = 0b101,			// 5
	ALTFUNC2 = 0b110,			// 6
	ALTFUNC3 = 0b111,			// 7
} GPIOMODE;

/*--------------------------------------------------------------------------}
;{	    ENUMERATED GPIO FIX RESISTOR ... BCM2835.PDF MANUAL see page 101	}
;{-------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	NO_RESISTOR = 0b00,			// 0
	PULLUP = 0b01,				// 1
	PULLDOWN = 0b10,			// 2
} GPIO_FIX_RESISTOR;

/*--------------------------------------------------------------------------}
{	ENUMERATED TIMER CONTROL PRESCALE ... BCM2835.PDF MANUAL see page 197	}
{--------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	Clkdiv1 = 0b00,				// 0
	Clkdiv16 = 0b01,			// 1
	Clkdiv256 = 0b10,			// 2
	Clkdiv_undefined = 0b11,	// 3 
} TIMER_PRESCALE;


/*--------------------------------------------------------------------------}
{	                  ENUMERATED MAILBOX CHANNELS							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	MB_CHANNEL_POWER	= 0x0,										// Mailbox Channel 0: Power Management Interface 
	MB_CHANNEL_FB		= 0x1,										// Mailbox Channel 1: Frame Buffer
	MB_CHANNEL_VUART	= 0x2,										// Mailbox Channel 2: Virtual UART
	MB_CHANNEL_VCHIQ	= 0x3,										// Mailbox Channel 3: VCHIQ Interface
	MB_CHANNEL_LEDS		= 0x4,										// Mailbox Channel 4: LEDs Interface
	MB_CHANNEL_BUTTONS	= 0x5,										// Mailbox Channel 5: Buttons Interface
	MB_CHANNEL_TOUCH	= 0x6,										// Mailbox Channel 6: Touchscreen Interface
	MB_CHANNEL_COUNT	= 0x7,										// Mailbox Channel 7: Counter
	MB_CHANNEL_TAGS		= 0x8,										// Mailbox Channel 8: Tags (ARM to VC)
	MB_CHANNEL_GPU		= 0x9,										// Mailbox Channel 9: GPU (VC to ARM)
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
	MAILBOX_TAG_SET_ENABLE_QPU				= 0x00030012,			// QPU enable

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


/***************************************************************************}
{		 				 PUBLIC STRUCT DEFINITIONS				            }
****************************************************************************/

typedef struct __attribute__((__packed__, aligned(4))) RGBA {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			uint8_t B;
			uint8_t G;
			uint8_t R;
			uint8_t A;
		};
		uint32_t raw32;
	};
} RGBA;

typedef struct FrameBuffer FRAMEBUFFER;
/*--------------------------------------------------------------------------}
{						  FRAME BUFFER STRUCTURE							}
{--------------------------------------------------------------------------*/
struct FrameBuffer {
	uint32_t buffer;												// Frame buffer address
	int	width;														// Physical width
	int height;														// Physical height
	int depth;														// Colour depth
	uintptr_t bitfont;												// Bitmap font	
	int  fontWth;													// Bitmap font width
	int  fontHt;													// Bitmap font height

																	/* Text colour control */
	RGBA TxtColor;													// Text colour to write
	RGBA BkColor;													// Background colour to write

	/* Limit control */
	int clipMinX;
	int clipMaxX;
	int clipMinY;
	int clipMaxY;
	void (*VertLine) (FRAMEBUFFER* self, uint32_t X1, uint32_t Y1, uint32_t Y2, uint32_t col);
	void (*WriteChar) (FRAMEBUFFER* self, uint32_t X1, uint32_t Y1, uint8_t Ch);
};


/***************************************************************************}
{		 	      PUBLIC REGISTER DEFINITION STRUCTURES			            }
****************************************************************************/

/*==========================================================================}
{			     INDIVIDUAL HARDWARE REGISTER DEFINITIONS		            }
===========================================================================*/

/*--------------------------------------------------------------------------}
{	   BSC CONTROL REGISTER BCM2835 ARM Peripheral manual page 29,30		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCControlReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile bool ReadTransfer : 1;							// @0 Read transfer bit
			unsigned reserved : 3;									// @1-3 reserved .. write = zero, read = xxx
			volatile bool ClearFifo : 1;							// @4 Clear fifo
			volatile bool ClearFifo1 : 1;							// @5 Clear fifo compatability either bit works
			unsigned reserved1 : 1;									// @6 reserved .. write = zero, read = x
			volatile bool StartTransfer : 1;						// @7 Start transfer
			volatile bool INTD : 1;									// @8 Interrupt on done
			volatile bool INTT : 1;									// @9 Interrupt on TX
			volatile bool INTR : 1;									// @10 Interrupt on RX
			unsigned reserved2 : 4;									// @11-14 reserved .. write = zero, read = xxxx
			volatile bool I2CEN : 1;								// @15 I2C enable
			unsigned reserved3 : 16;								// @16-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   BSC CONTROL REGISTER BCM2835 ARM Peripheral manual page 31,32		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCStatusReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			const volatile bool TransferActive : 1;					// @0 Transfer Active bit  ** Read only hence const
			volatile bool DoneTransfer : 1;							// @1 Done transfer
			const volatile bool TXW : 1;							// @2 Write TX FIFO needs writing  ** Read only hence const
			const volatile bool RXR : 1;							// @3 Read TX FIFO needs reading  ** Read only hence const
			const volatile bool TXD : 1;							// @4 Write TX FIFO can accept more data  ** Read only hence const
			const volatile bool RXD : 1;							// @5 Read TX FIFO has data  ** Read only hence const
			const volatile bool TXE : 1;							// @6 Write TX FIFO empty  ** Read only hence const
			const volatile bool RXE : 1;							// @7 Read TX FIFO has data  ** Read only hence const
			volatile bool ERR : 1;									// @8 ERR ACK Error 
			volatile bool CLKT : 1;									// @9 Clock stretch Timeout
			unsigned reserved : 22;									// @10-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   BSC DATA LENGTH REGISTER BCM2835 ARM Peripheral manual page 32		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCDataLengthReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile uint16_t DLEN : 16;							// @0-15 Data length
			unsigned reserved : 16;									// @16-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   BSC ADDRESS SLAVE REGISTER BCM2835 ARM Peripheral manual page 33		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCAddressSlaveReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned ADDR : 7;								// @0-6 Slave address
			unsigned reserved : 25;									// @7-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	    BSC DATA FIFO REGISTER BCM2835 ARM Peripheral manual page 33		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCDataFifoReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile uint8_t DATA : 8;								// @0-7 Data
			unsigned reserved : 24;									// @8-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   BSC CLOCK DIVISOR REGISTER BCM2835 ARM Peripheral manual page 34		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCClockDivisorReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile uint16_t CDIV : 16;							// @0-15 Clock divisor
			unsigned reserved : 16;									// @16-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	    BSC DATA EDGE REGISTER BCM2835 ARM Peripheral manual page 34		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCDataEdgeReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile uint16_t REDL : 16;							// @0-15 Rising edge delay
			volatile uint16_t FEDL : 16;							// @16-31 Falling edge delay
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   BSC CLOCK STRETCH REGISTER BCM2835 ARM Peripheral manual page 35		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCClockStretchReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile uint16_t TOUT : 16;							// @0-15 Clock Stretch Timeout Value
			unsigned reserved : 16;									// @16-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{   IRQ BASIC PENDING REGISTER - BCM2835.PDF Manual Section 7 page 113/114  }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) IrqBasicPendingReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			const volatile bool Timer_IRQ_pending : 1;				// @0 Timer Irq pending
			const volatile bool Mailbox_IRQ_pending : 1;			// @1 Mailbox Irq pending
			const volatile bool Doorbell0_IRQ_pending : 1;			// @2 Arm Doorbell0 Irq pending
			const volatile bool Doorbell1_IRQ_pending : 1;			// @3 Arm Doorbell0 Irq pending
			const volatile bool GPU0_halted_IRQ_pending : 1;		// @4 GPU0 halted IRQ pending
			const volatile bool GPU1_halted_IRQ_pending : 1;		// @5 GPU1 halted IRQ pending
			const volatile bool Illegal_access_type1_pending : 1;	// @6 Illegal access type 1 IRQ pending
			const volatile bool Illegal_access_type0_pending : 1;	// @7 Illegal access type 0 IRQ pending
			const volatile bool Bits_set_in_pending_register_1 : 1;	// @8 One or more bits set in pending register 1
			const volatile bool Bits_set_in_pending_register_2 : 1;	// @9 One or more bits set in pending register 2
			const volatile bool GPU_IRQ_7_pending : 1;				// @10 GPU irq 7 pending
			const volatile bool GPU_IRQ_9_pending : 1;				// @11 GPU irq 9 pending
			const volatile bool GPU_IRQ_10_pending : 1;				// @12 GPU irq 10 pending
			const volatile bool GPU_IRQ_18_pending : 1;				// @13 GPU irq 18 pending
			const volatile bool GPU_IRQ_19_pending : 1;				// @14 GPU irq 19 pending
			const volatile bool GPU_IRQ_53_pending : 1;				// @15 GPU irq 53 pending
			const volatile bool GPU_IRQ_54_pending : 1;				// @16 GPU irq 54 pending
			const volatile bool GPU_IRQ_55_pending : 1;				// @17 GPU irq 55 pending
			const volatile bool GPU_IRQ_56_pending : 1;				// @18 GPU irq 56 pending
			const volatile bool GPU_IRQ_57_pending : 1;				// @19 GPU irq 57 pending
			const volatile bool GPU_IRQ_62_pending : 1;				// @20 GPU irq 62 pending
			unsigned reserved : 10;									// @21-31 reserved
		};
		const volatile uint32_t Raw32;								// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   FIQ CONTROL REGISTER BCM2835.PDF ARM Peripheral manual page 116		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) FiqControlReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned SelectFIQSource : 7;					// @0-6 Select FIQ source
			volatile bool EnableFIQ : 1;							// @7 enable FIQ
			unsigned reserved : 24;									// @8-31 reserved
		};
		volatile uint32_t Raw32;									// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	 ENABLE BASIC IRQ REGISTER BCM2835 ARM Peripheral manual page 117		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) EnableBasicIrqReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile bool Enable_Timer_IRQ : 1;						// @0 Timer Irq enable
			volatile bool Enable_Mailbox_IRQ : 1;					// @1 Mailbox Irq enable
			volatile bool Enable_Doorbell0_IRQ : 1;					// @2 Arm Doorbell0 Irq enable
			volatile bool Enable_Doorbell1_IRQ : 1;					// @3 Arm Doorbell0 Irq enable
			volatile bool Enable_GPU0_halted_IRQ : 1;				// @4 GPU0 halted IRQ enable
			volatile bool Enable_GPU1_halted_IRQ : 1;				// @5 GPU1 halted IRQ enable
			volatile bool Enable_Illegal_access_type1 : 1;			// @6 Illegal access type 1 IRQ enable
			volatile bool Enable_Illegal_access_type0 : 1;			// @7 Illegal access type 0 IRQ enable
			unsigned reserved : 24;									// @8-31 reserved
		};
		volatile uint32_t Raw32;									// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	DISABLE BASIC IRQ REGISTER BCM2835 ARM Peripheral manual page 117		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) DisableBasicIrqReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile bool Disable_Timer_IRQ : 1;					// @0 Timer Irq disable
			volatile bool Disable_Mailbox_IRQ : 1;					// @1 Mailbox Irq disable
			volatile bool Disable_Doorbell0_IRQ : 1;				// @2 Arm Doorbell0 Irq disable
			volatile bool Disable_Doorbell1_IRQ : 1;				// @3 Arm Doorbell0 Irq disable
			volatile bool Disable_GPU0_halted_IRQ : 1;				// @4 GPU0 halted IRQ disable
			volatile bool Disable_GPU1_halted_IRQ : 1;				// @5 GPU1 halted IRQ disable
			volatile bool Disable_Illegal_access_type1 : 1;			// @6 Illegal access type 1 IRQ disable
			volatile bool Disable_Illegal_access_type0 : 1;			// @7 Illegal access type 0 IRQ disable
			unsigned reserved : 24;									// @8-31 reserved
		};
		volatile uint32_t Raw32;									// Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{	   TIMER_CONTROL REGISTER BCM2835 ARM Peripheral manual page 197		}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) TimerControlReg {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			unsigned unused : 1;									// @0 Unused bit
			volatile bool Counter32Bit : 1;							// @1 Counter32 bit (16bit if false)
			volatile TIMER_PRESCALE Prescale : 2;					// @2-3 Prescale  
			unsigned unused1 : 1;									// @4 Unused bit
			volatile bool TimerIrqEnable : 1;						// @5 Timer irq enable
			unsigned unused2 : 1;									// @6 Unused bit
			volatile bool TimerEnable : 1;							// @7 Timer enable
			unsigned reserved : 24;									// @8-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
};

/*==========================================================================}
{		 HARDWARE REGISTERS BANKED UP IN GROUPS AS PER MANUAL SECTIONS	    }
===========================================================================*/

/*--------------------------------------------------------------------------}
{     RASPBERRY PI BSC HARDWARE REGISTERS - BCM2835.PDF Manual Section 2	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) BSCRegisters {
	volatile struct BSCControlReg Control;							// 0x00
	volatile struct BSCStatusReg Status;							// 0x04
	volatile struct BSCDataLengthReg DataLen;						// 0x08
	volatile struct BSCAddressSlaveReg SlaveAddr;					// 0x0C
	volatile struct BSCDataFifoReg Fifo;							// 0x10
	volatile struct BSCClockDivisorReg DIV;							// 0x14
	volatile struct BSCDataEdgeReg DEL;								// 0x18
	volatile struct BSCClockStretchReg CLKT;						// 0x1C
};

/*--------------------------------------------------------------------------}
{    RASPBERRY PI GPIO HARDWARE REGISTERS - BCM2835.PDF Manual Section 6	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) GPIORegisters {
	volatile uint32_t GPFSEL[6];									// 0x00  GPFSEL0 - GPFSEL5
	uint32_t reserved1;												// 0x18  reserved
	volatile uint32_t GPSET[2];										// 0x1C  GPSET0 - GPSET1;
	uint32_t reserved2;												// 0x24  reserved
	volatile uint32_t GPCLR[2];										// 0x28  GPCLR0 - GPCLR1
	uint32_t reserved3;												// 0x30  reserved
	const volatile uint32_t GPLEV[2];								// 0x34  GPLEV0 - GPLEV1   ** Read only hence const
	uint32_t reserved4;												// 0x3C  reserved
	volatile uint32_t GPEDS[2];										// 0x40  GPEDS0 - GPEDS1 
	uint32_t reserved5;												// 0x48  reserved
	volatile uint32_t GPREN[2];										// 0x4C  GPREN0 - GPREN1;	 
	uint32_t reserved6;												// 0x54  reserved
	volatile uint32_t GPFEN[2];										// 0x58  GPFEN0 - GPFEN1;
	uint32_t reserved7;												// 0x60  reserved
	volatile uint32_t GPHEN[2];										// 0x64  GPHEN0 - GPHEN1;
	uint32_t reserved8;												// 0x6c  reserved
	volatile uint32_t GPLEN[2];										// 0x70  GPLEN0 - GPLEN1;
	uint32_t reserved9;												// 0x78  reserved
	volatile uint32_t GPAREN[2];									// 0x7C  GPAREN0 - GPAREN1;
	uint32_t reserved10;											// 0x84  reserved
	volatile uint32_t GPAFEN[2]; 									// 0x88  GPAFEN0 - GPAFEN1;
	uint32_t reserved11;											// 0x90  reserved
	volatile uint32_t GPPUD; 										// 0x94  GPPUD 
	volatile uint32_t GPPUDCLK[2]; 									// 0x98  GPPUDCLK0 - GPPUDCLK1;
};

/*--------------------------------------------------------------------------}
{	   RASPBERRY PI IRQ HARDWARE REGISTERS - BCM2835 Manual Section 7	    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) IrqControlRegisters {
	const volatile struct IrqBasicPendingReg IRQBasicPending;		// 0x200   ** Read only hence const
	volatile uint32_t IRQPending1;									// 0x204
	volatile uint32_t IRQPending2;									// 0x208
	volatile struct FiqControlReg FIQControl;						// 0x20C
	volatile uint32_t EnableIRQs1;									// 0x210
	volatile uint32_t EnableIRQs2;									// 0x214
	volatile struct EnableBasicIrqReg EnableBasicIRQs;				// 0x218
	volatile uint32_t DisableIRQs1;									// 0x21C
	volatile uint32_t DisableIRQs2;									// 0x220
	volatile struct DisableBasicIrqReg DisableBasicIRQs;			// 0x224
};

/*--------------------------------------------------------------------------}
{  RASPBERRY PI SYSTEM TIMER HARDWARE REGISTERS - BCM2835 Manual Section 12	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) SystemTimerRegisters {
	volatile uint32_t ControlStatus;								// 0x00
	volatile uint32_t TimerLo;										// 0x04
	volatile uint32_t TimerHi;										// 0x08
	volatile uint32_t Compare0;										// 0x0C
	volatile uint32_t Compare1;										// 0x10
	volatile uint32_t Compare2;										// 0x14
	volatile uint32_t Compare3;										// 0x18
};

/*--------------------------------------------------------------------------}
{   RASPBERRY PI ARM TIMER HARDWARE REGISTERS - BCM2835 Manual Section 14	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__,aligned(4))) ArmTimerRegisters {
	volatile uint32_t Load;											// 0x00
	const volatile uint32_t Value;									// 0x04  ** Read only hence const
	struct TimerControlReg Control;									// 0x08
	volatile uint32_t Clear;										// 0x0C
	const volatile uint32_t RawIRQ;									// 0x10  ** Read only hence const
	const volatile uint32_t MaskedIRQ;								// 0x14  ** Read only hence const
	volatile uint32_t Reload;										// 0x18
};


/*--------------------------------------------------------------------------}
;{               RASPBERRY PI MAILBOX HARRDWARE REGISTERS					}
;{-------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) MailBoxRegisters {
	const volatile uint32_t Read0;									// 0x00         Read data from VC to ARM
	uint32_t Unused[3];												// 0x04-0x0F
	volatile uint32_t Peek0;										// 0x10
	volatile uint32_t Sender0;										// 0x14
	volatile uint32_t Status0;										// 0x18         Status of VC to ARM
	volatile uint32_t Config0;										// 0x1C        
	volatile uint32_t Write1;										// 0x20         Write data from ARM to VC
	uint32_t Unused2[3];											// 0x24-0x2F
	volatile uint32_t Peek1;										// 0x30
	volatile uint32_t Sender1;										// 0x34
	volatile uint32_t Status1;										// 0x38         Status of ARM to VC
	volatile uint32_t Config1;										// 0x3C 
};


/***************************************************************************}
{     PUBLIC POINTERS TO ALL OUR RASPBERRY PI REGISTER BANK STRUCTURES	    }
****************************************************************************/
#define GPIO ((volatile __attribute__((aligned(4))) struct GPIORegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x200000u))
#define SYSTEMTIMER ((volatile __attribute__((aligned(4))) struct SystemTimerRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x3000u))
#define BSC0 ((volatile __attribute__((aligned(4))) struct BSCRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x205000u))
#define BSC1 ((volatile __attribute__((aligned(4))) struct BSCRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x804000u))
#define BSC2 ((volatile __attribute__((aligned(4))) struct BSCRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x805000u))
#define IRQ ((volatile __attribute__((aligned(4))) struct IrqControlRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB200u))
#define ARMTIMER ((volatile __attribute__((aligned(4))) struct  ArmTimerRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB400u))
#define MAILBOX ((volatile __attribute__((aligned(4))) struct MailBoxRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB880u))

/***************************************************************************}
{					      PUBLIC INTERFACE ROUTINES			                }
****************************************************************************/

/*--------------------------------------------------------------------------}
{						   PUBLIC GPIO ROUTINES								}
{--------------------------------------------------------------------------*/

/*-[gpio_setup]-------------------------------------------------------------}
 Given a valid GPIO port number and mode sets GPIO to given mode
 RETURN: true for success, false for any failure
 30Jun17 LdB
---------------------------------------------------------------------------*/
bool gpio_setup (int gpio, GPIOMODE mode);

/*-[gpio_output]------------------------------------------------------------}
 Given a valid GPIO port number the output is set high(true) or Low (false)
 RETURN: true for success, false for any failure
 30Jun17 LdB
---------------------------------------------------------------------------*/
bool gpio_output (int gpio, bool on);

/*-[gpio_input]-------------------------------------------------------------}
 Reads the actual level of the GPIO port number
 RETURN: true = GPIO input high, false = GPIO input low 
 30Jun17 LdB
---------------------------------------------------------------------------*/
bool gpio_input (int gpio);

/*-[gpio_checkEvent]-------------------------------------------------------}
 Checks the given GPIO port number for an event/irq flag.
 RETURN: true for event occured, false for no event
 30Jun17 LdB
--------------------------------------------------------------------------*/
bool gpio_checkEvent (int gpio);

/*-[gpio_clearEvent]--------------------------------------------------------}
 Clears the given GPIO port number event/irq flag. 
 RETURN: true for success, false for any failure
 30Jun17 LdB
--------------------------------------------------------------------------*/
bool gpio_clearEvent (int gpio);

/*-[gpio_edgeDetect]-------------------------------------------------------}
 Sets GPIO port number edge detection to lifting/falling in Async/Sync mode
 RETURN: true for success, false for any failure
 30Jun17 LdB
--------------------------------------------------------------------------*/
bool gpio_edgeDetect (int gpio, bool lifting, bool Async);

/*-[gpio_fixResistor]------------------------------------------------------}
 Set the GPIO port number with fix resistors to pull up/pull down.
 RETURN: true for success, false for any failure
 30Jun17 LdB
--------------------------------------------------------------------------*/
bool gpio_fixResistor (int gpio, GPIO_FIX_RESISTOR resistor);


/*--------------------------------------------------------------------------}
{						   PUBLIC TIMER ROUTINES							}
{--------------------------------------------------------------------------*/

/*-[timer_getTickCount]-----------------------------------------------------}
 Get 1Mhz ARM system timer tick count in full 64 bit.
 The timer read is as per the Broadcom specification of two 32bit reads
 RETURN: tickcount value as an unsigned 64bit value
 30Jun17 LdB
--------------------------------------------------------------------------*/
uint64_t timer_getTickCount (void);

/*-[timer_Wait]-------------------------------------------------------------}
 This will simply wait the requested number of microseconds before return.
 02Jul17 LdB
 --------------------------------------------------------------------------*/
void timer_wait (uint64_t us);

/*-[tick_Difference]--------------------------------------------------------}
 Given two timer tick results it returns the time difference between them.
 02Jul17 LdB
 --------------------------------------------------------------------------*/
uint64_t tick_difference (uint64_t us1, uint64_t us2);

/*--------------------------------------------------------------------------}
{						    PI MAILBOX ROUTINES								}
{--------------------------------------------------------------------------*/

/*-[mailbox_write]----------------------------------------------------------}
 This will execute the sending of the given data block message thru the
 mailbox system on the given channel.
 RETURN: True for success, False for failure.
 04Jul17 LdB
 --------------------------------------------------------------------------*/
bool mailbox_write (MAILBOX_CHANNEL channel, uint32_t message);

/*-[mailbox_read]-----------------------------------------------------------}
 This will read any pending data on the mailbox system on the given channel.
 RETURN: The read value for success, 0xFEEDDEAD for failure.
 04Jul17 LdB
 --------------------------------------------------------------------------*/
uint32_t mailbox_read (MAILBOX_CHANNEL channel);

/*--------------------------------------------------------------------------}
{						  PI ACTIVITY LED ROUTINES							}
{--------------------------------------------------------------------------*/
bool set_Activity_LED (bool on);

bool SetMaxCPUSpeed(void);

void DeadLoop (void);

bool PiConsole_Init (int Width, int Height, int Depth);
void PiConsole_WriteChar (char Ch);
void PiConsole_VertLine (int X1, int Y1, int Y2, uint32_t col);
void WriteText (int x, int y, char* txt);

int printf(const char *fmt, ...);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif											// _RPI_BASIC_HARDWARE_ Guard