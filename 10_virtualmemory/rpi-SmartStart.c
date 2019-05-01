/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: rpi-smartstart.c											}
{       Copyright(c): Leon de Boer(LdB) 2017, 2018, 2019					}
{       Version: 2.14														}
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
{++++++++++++++++++++++++[ REVISIONS ]++++++++++++++++++++++++++++++++++++++}
{  2.08 Added setIrqFuncAddress & setFiqFuncAddress							}
{  2.09 Added Hard/Soft float compiler support								}
{  2.10 Context Switch support API calls added								}
{  2.11 MiniUart, PL011 Uart and console uart support added					}
{  2.12 New FIQ, DAIF flag support added									}
{  2.13	Graphics routines relocated to there own unit						}
{  2.14 Multicore task switcher support Added								}
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdbool.h>			// C standard unit needed for bool and true/false
#include <stdint.h>				// C standard unit needed for uint8_t, uint32_t, etc
#include <stdarg.h>				// C standard unit needed for variadic functions
#include "rpi-SmartStart.h"		// This units header

/***************************************************************************}
{       PRIVATE INTERNAL RASPBERRY PI REGISTER STRUCTURE DEFINITIONS        }
****************************************************************************/

/*--------------------------------------------------------------------------}
{    RASPBERRY PI GPIO HARDWARE REGISTERS - BCM2835.PDF Manual Section 6	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) GPIORegisters {
	uint32_t GPFSEL[6];												// 0x00  GPFSEL0 - GPFSEL5
	uint32_t reserved1;												// 0x18  reserved
	uint32_t GPSET[2];												// 0x1C  GPSET0 - GPSET1;
	uint32_t reserved2;												// 0x24  reserved
	uint32_t GPCLR[2];												// 0x28  GPCLR0 - GPCLR1
	uint32_t reserved3;												// 0x30  reserved
	const uint32_t GPLEV[2];										// 0x34  GPLEV0 - GPLEV1   ** Read only hence const
	uint32_t reserved4;												// 0x3C  reserved
	uint32_t GPEDS[2];												// 0x40  GPEDS0 - GPEDS1 
	uint32_t reserved5;												// 0x48  reserved
	uint32_t GPREN[2];												// 0x4C  GPREN0 - GPREN1;	 
	uint32_t reserved6;												// 0x54  reserved
	uint32_t GPFEN[2];												// 0x58  GPFEN0 - GPFEN1;
	uint32_t reserved7;												// 0x60  reserved
	uint32_t GPHEN[2];												// 0x64  GPHEN0 - GPHEN1;
	uint32_t reserved8;												// 0x6c  reserved
	uint32_t GPLEN[2];												// 0x70  GPLEN0 - GPLEN1;
	uint32_t reserved9;												// 0x78  reserved
	uint32_t GPAREN[2];												// 0x7C  GPAREN0 - GPAREN1;
	uint32_t reserved10;											// 0x84  reserved
	uint32_t GPAFEN[2]; 											// 0x88  GPAFEN0 - GPAFEN1;
	uint32_t reserved11;											// 0x90  reserved
	uint32_t GPPUD; 												// 0x94  GPPUD 
	uint32_t GPPUDCLK[2]; 											// 0x98  GPPUDCLK0 - GPPUDCLK1;
};

/*--------------------------------------------------------------------------}
{  RASPBERRY PI SYSTEM TIMER HARDWARE REGISTERS - BCM2835 Manual Section 12	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) SystemTimerRegisters {
	uint32_t ControlStatus;											// 0x00
	uint32_t TimerLo;												// 0x04
	uint32_t TimerHi;												// 0x08
	uint32_t Compare0;												// 0x0C
	uint32_t Compare1;												// 0x10
	uint32_t Compare2;												// 0x14
	uint32_t Compare3;												// 0x18
};

/*--------------------------------------------------------------------------}
{	   TIMER_CONTROL REGISTER BCM2835 ARM Peripheral manual page 197		}
{--------------------------------------------------------------------------*/
typedef union  
{
	struct 
	{
		unsigned unused : 1;										// @0 Unused bit
		unsigned Counter32Bit : 1;									// @1 Counter32 bit (16bit if false)
		TIMER_PRESCALE Prescale : 2;								// @2-3 Prescale  
		unsigned unused1 : 1;										// @4 Unused bit
		unsigned TimerIrqEnable : 1;								// @5 Timer irq enable
		unsigned unused2 : 1;										// @6 Unused bit
		unsigned TimerEnable : 1;									// @7 Timer enable
		unsigned reserved : 24;										// @8-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} time_ctrl_reg_t;


/*--------------------------------------------------------------------------}
{   RASPBERRY PI ARM TIMER HARDWARE REGISTERS - BCM2835 Manual Section 14	}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) ArmTimerRegisters {
	uint32_t Load;													// 0x00
	const uint32_t Value;											// 0x04  ** Read only hence const
	time_ctrl_reg_t Control;										// 0x08
	uint32_t Clear;													// 0x0C
	const uint32_t RawIRQ;											// 0x10  ** Read only hence const
	const uint32_t MaskedIRQ;										// 0x14  ** Read only hence const
	uint32_t Reload;												// 0x18
};

/*--------------------------------------------------------------------------}
{   IRQ BASIC PENDING REGISTER - BCM2835.PDF Manual Section 7 page 113/114  }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__, aligned(4)))
	{
		const unsigned Timer_IRQ_pending : 1;						// @0 Timer Irq pending  ** Read only
		const unsigned Mailbox_IRQ_pending : 1;						// @1 Mailbox Irq pending  ** Read only
		const unsigned Doorbell0_IRQ_pending : 1;					// @2 Arm Doorbell0 Irq pending  ** Read only
		const unsigned Doorbell1_IRQ_pending : 1;					// @3 Arm Doorbell0 Irq pending  ** Read only
		const unsigned GPU0_halted_IRQ_pending : 1;					// @4 GPU0 halted IRQ pending  ** Read only
		const unsigned GPU1_halted_IRQ_pending : 1;					// @5 GPU1 halted IRQ pending  ** Read only
		const unsigned Illegal_access_type1_pending : 1;			// @6 Illegal access type 1 IRQ pending  ** Read only
		const unsigned Illegal_access_type0_pending : 1;			// @7 Illegal access type 0 IRQ pending  ** Read only
		const unsigned Bits_set_in_pending_register_1 : 1;			// @8 One or more bits set in pending register 1  ** Read only
		const unsigned Bits_set_in_pending_register_2 : 1;			// @9 One or more bits set in pending register 2  ** Read only
		const unsigned GPU_IRQ_7_pending : 1;						// @10 GPU irq 7 pending  ** Read only
		const unsigned GPU_IRQ_9_pending : 1;						// @11 GPU irq 9 pending  ** Read only
		const unsigned GPU_IRQ_10_pending : 1;						// @12 GPU irq 10 pending  ** Read only
		const unsigned GPU_IRQ_18_pending : 1;						// @13 GPU irq 18 pending  ** Read only
		const unsigned GPU_IRQ_19_pending : 1;						// @14 GPU irq 19 pending  ** Read only
		const unsigned GPU_IRQ_53_pending : 1;						// @15 GPU irq 53 pending  ** Read only
		const unsigned GPU_IRQ_54_pending : 1;						// @16 GPU irq 54 pending  ** Read only
		const unsigned GPU_IRQ_55_pending : 1;						// @17 GPU irq 55 pending  ** Read only
		const unsigned GPU_IRQ_56_pending : 1;						// @18 GPU irq 56 pending  ** Read only
		const unsigned GPU_IRQ_57_pending : 1;						// @19 GPU irq 57 pending  ** Read only
		const unsigned GPU_IRQ_62_pending : 1;						// @20 GPU irq 62 pending  ** Read only
		unsigned reserved : 10;										// @21-31 reserved
	};
	const uint32_t Raw32;											// Union to access all 32 bits as a uint32_t  ** Read only
} irq_basic_pending_reg_t;

/*--------------------------------------------------------------------------}
{	   FIQ CONTROL REGISTER BCM2835.PDF ARM Peripheral manual page 116		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__, aligned(4)))
	{
		unsigned SelectFIQSource : 7;								// @0-6 Select FIQ source
		unsigned EnableFIQ : 1;										// @7 enable FIQ
		unsigned reserved : 24;										// @8-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} fiq_control_reg_t;

/*--------------------------------------------------------------------------}
{	 ENABLE BASIC IRQ REGISTER BCM2835 ARM Peripheral manual page 117		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__, aligned(4)))
	{
		unsigned Enable_Timer_IRQ : 1;								// @0 Timer Irq enable
		unsigned Enable_Mailbox_IRQ : 1;							// @1 Mailbox Irq enable
		unsigned Enable_Doorbell0_IRQ : 1;							// @2 Arm Doorbell0 Irq enable
		unsigned Enable_Doorbell1_IRQ : 1;							// @3 Arm Doorbell0 Irq enable
		unsigned Enable_GPU0_halted_IRQ : 1;						// @4 GPU0 halted IRQ enable
		unsigned Enable_GPU1_halted_IRQ : 1;						// @5 GPU1 halted IRQ enable
		unsigned Enable_Illegal_access_type1 : 1;					// @6 Illegal access type 1 IRQ enable
		unsigned Enable_Illegal_access_type0 : 1;					// @7 Illegal access type 0 IRQ enable
		unsigned reserved : 24;										// @8-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} irq_enable_basic_reg_t;

/*--------------------------------------------------------------------------}
{	DISABLE BASIC IRQ REGISTER BCM2835 ARM Peripheral manual page 117		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__, aligned(4)))
	{
		unsigned Disable_Timer_IRQ : 1;								// @0 Timer Irq disable
		unsigned Disable_Mailbox_IRQ : 1;							// @1 Mailbox Irq disable
		unsigned Disable_Doorbell0_IRQ : 1;							// @2 Arm Doorbell0 Irq disable
		unsigned Disable_Doorbell1_IRQ : 1;							// @3 Arm Doorbell0 Irq disable
		unsigned Disable_GPU0_halted_IRQ : 1;						// @4 GPU0 halted IRQ disable
		unsigned Disable_GPU1_halted_IRQ : 1;						// @5 GPU1 halted IRQ disable
		unsigned Disable_Illegal_access_type1 : 1;					// @6 Illegal access type 1 IRQ disable
		unsigned Disable_Illegal_access_type0 : 1;					// @7 Illegal access type 0 IRQ disable
		unsigned reserved : 24;										// @8-31 reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} irq_disable_basic_reg_t;

/*--------------------------------------------------------------------------}
{	   RASPBERRY PI IRQ HARDWARE REGISTERS - BCM2835 Manual Section 7	    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) IrqControlRegisters {
	const irq_basic_pending_reg_t IRQBasicPending;					// 0x200   ** Read only hence const
	uint32_t IRQPending1;											// 0x204
	uint32_t IRQPending2;											// 0x208
	fiq_control_reg_t FIQControl;									// 0x20C
	uint32_t EnableIRQs1;											// 0x210
	uint32_t EnableIRQs2;											// 0x214
	irq_enable_basic_reg_t EnableBasicIRQs;							// 0x218
	uint32_t DisableIRQs1;											// 0x21C
	uint32_t DisableIRQs2;											// 0x220
	irq_disable_basic_reg_t DisableBasicIRQs;						// 0x224
};

/*--------------------------------------------------------------------------}
;{               RASPBERRY PI MAILBOX HARRDWARE REGISTERS					}
;{-------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) MailBoxRegisters {
	const uint32_t Read0;											// 0x00         Read data from VC to ARM
	uint32_t Unused[3];												// 0x04-0x0F
	uint32_t Peek0;													// 0x10
	uint32_t Sender0;												// 0x14
	uint32_t Status0;												// 0x18         Status of VC to ARM
	uint32_t Config0;												// 0x1C        
	uint32_t Write1;												// 0x20         Write data from ARM to VC
	uint32_t Unused2[3];											// 0x24-0x2F
	uint32_t Peek1;													// 0x30
	uint32_t Sender1;												// 0x34
	uint32_t Status1;												// 0x38         Status of ARM to VC
	uint32_t Config1;												// 0x3C 
};

/*--------------------------------------------------------------------------}
{         MINI UART IO Register BCM2835 ARM Peripheral manual page 11		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned DATA : 8;											// @0-7 Transmit Read/write data if DLAB=0, DLAB = 1 Lower 8 bits of 16 bit baud rate generator 
		unsigned reserved : 24;										// @8-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_io_reg_t;

/*--------------------------------------------------------------------------}
{ MINI UART INTERRUPT ENABLE Register BCM2835 ARM Peripheral manual page 12	}
{   PAGE HAS ERRORS: https://elinux.org/BCM2835_datasheet_errata            }
{   It is essentially same as standard 16550 register IER                   }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned RXDI : 1;											// @0	 If this bit is set the interrupt line is asserted if the receive FIFO holds at least 1 byte.
		unsigned TXEI : 1;											// @1	 If this bit is set the interrupt line is asserted if the transmit FIFO is empty.  
		unsigned LSI : 1;											// @2	 If this bit is set the Receiver Line Status interrupt is asserted on overrun error, parity error, framing error etc
		unsigned MSI : 1;											// @3	 If this bit is set the Modem Status interrupt is asserted on a change To Send(CTS), Data Set Ready(DSR)
		unsigned reserved : 28;										// @4-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_ie_reg_t;

/*--------------------------------------------------------------------------}
{   MINI UART INTERRUPT ID Register BCM2835 ARM Peripheral manual page 13	}
{--------------------------------------------------------------------------*/
typedef union
{
	const struct __attribute__((__packed__))
	{																// THE REGISTER READ DEFINITIONS 
		unsigned PENDING : 1;										// @0	 This bit is clear whenever an interrupt is pending 
		enum {
			MU_NO_INTERRUPTS = 0,									//		 No interrupts pending
			MU_TXE_INTERRUPT = 1,									//		 Transmit buffer empty causing interrupt
			MU_RXD_INTERRUPTS = 2,									//		 receive fifa has data causing interrupt 
		} SOURCE : 2;												// @1-2	 READ this register shows the interrupt ID bits 
		unsigned reserved_rd : 29;									// @3-31 Reserved - Write as 0, read as don't care 
	};
	struct __attribute__((__packed__))
	{																// THE REGISTER WRITE DEFINITIONS 
		unsigned unused : 1;										// @0	 This bit has no use when writing 
		unsigned RXFIFO_CLEAR : 1;									// @1	 Clear the receive fifo by writing a 1
		unsigned TXFIFO_CLEAR : 1;									// @2	 Clear the transmit fifo by writing a 1
		unsigned reserved_wr : 29;									// @3-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_ii_reg_t;

/*--------------------------------------------------------------------------}
{	MINI UART LINE CONTROL Register BCM2835 ARM Peripheral manual page 14	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned DATA_LENGTH : 1;									// @0	 If clear the UART works in 7-bit mode, If set the UART works in 8-bit mode 
		unsigned reserved : 5;										// @1-5	 Reserved, write zero, read as don’t care Some of these bits have functions in a 16550 compatible UART but are ignored here
		unsigned BREAK : 1;											// @6	 If set high the UART1_TX line is pulled low continuously
		unsigned DLAB : 1;											// @7	 DLAB access control bit.
		unsigned reserved1 : 24;									// @8-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_lcr_reg_t;

/*--------------------------------------------------------------------------}
{	MINI UART MODEM CONTROL Register BCM2835 ARM Peripheral manual page 14	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned reserved : 1;										// @0	 Reserved, write zero, read as don’t care 
		unsigned RTS : 1;											// @1	 If clear the UART1_RTS line is high, If set the UART1_RTS line is low 
		unsigned reserved1 : 30;									// @2-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_mcr_reg_t;

/*--------------------------------------------------------------------------}
{	MINI UART LINE STATUS Register BCM2835 ARM Peripheral manual page 15	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned RXFDA : 1;											// @0	 This bit is set if the receive FIFO holds at least 1 
		unsigned RXOE : 1;											// @1	 This bit is set if there was a receiver overrun
		unsigned reserved : 3;										// @2-4	 Reserved, write zero, read as don’t care 
		unsigned TXFE : 1;											// @5	 This bit is set if the transmit FIFO can accept at least one byte
		unsigned TXIdle : 1;										// @6	 This bit is set if the transmit FIFO is empty and the transmitter is idle. (Finished shifting out the last bit). 
		unsigned reserved1 : 25;									// @7-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_lsr_reg_t;

/*--------------------------------------------------------------------------}
{	MINI UART MODEM STATUS Register BCM2835 ARM Peripheral manual page 15	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned reserved : 4;										// @0-3	 Reserved, write zero, read as don’t care 
		unsigned CTS : 1;											// @4	 This bit is the inverse of the CTS input, If set the UART1_CTS pin is low If clear the UART1_CTS pin is high 
		unsigned reserved1 : 27;									// @5-31 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_msr_reg_t;

/*--------------------------------------------------------------------------}
{	  MINI UART SCRATCH Register BCM2835 ARM Peripheral manual page 16	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned USER_DATA : 8;										// @0-7		One whole byte extra on top of the 134217728 provided by the SDC  
		unsigned reserved : 24;										// @8-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_scratch_reg_t;

/*--------------------------------------------------------------------------}
{     MINI UART CONTROL Register BCM2835 ARM Peripheral manual page 16	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned RXE : 1;											// @0		If this bit is set the mini UART receiver is enabled, If this bit is clear the mini UART receiver is disabled  
		unsigned TXE : 1;											// @1		If this bit is set the mini UART transmitter is enabled, If this bit is clear the mini UART transmitter is disabled
		unsigned EnableRTS : 1;										// @2		If this bit is set the RTS line will de-assert if the rc FIFO reaches it 'auto flow' level. If this bit is clear RTS is controlled by the AUX_MU_MCR_REG register bit 1. 
		unsigned EnableCTS : 1;										// @3		If this bit is set the transmitter will stop if the CTS line is de-asserted. If this bit is clear the transmitter will ignore the status of the CTS line
		enum {
			FIFOhas3spaces = 0,
			FIFOhas2spaces = 1,
			FIFOhas1spaces = 2,
			FIFOhas4spaces = 3,
		} RTSflowLevel : 2;											// @4-5		These two bits specify at what receiver FIFO level the RTS line is de-asserted in auto-flow mode
		unsigned RTSassertLevel : 1;								// @6		If set the RTS auto flow assert level is low, If clear the RTS auto flow assert level is high
		unsigned CTSassertLevel : 1;								// @7		If set the CTS auto flow assert level is low, If clear the CTS auto flow assert level is high
		unsigned reserved : 24;										// @8-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_cntl_reg_t;

/*--------------------------------------------------------------------------}
{	  MINI UART STATUS Register BCM2835 ARM Peripheral manual page 18	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned RXFDA : 1;											// @0		This bit is set if the receive FIFO holds at least 1
		unsigned TXFE : 1;											// @1		If this bit is set the mini UART transmitter FIFO can accept at least one more symbol
		unsigned RXIdle : 1;										// @2		If this bit is set the receiver is idle. If this bit is clear the receiver is busy
		unsigned TXIdle : 1;										// @3		If this bit is set the transmitter is idle. If this bit is clear the transmitter is idle
		unsigned RXOE : 1;											// @4		This bit is set if there was a receiver overrun
		unsigned TXFF : 1;											// @5		The inverse of bit 0
		unsigned RTS : 1;											// @6		This bit shows the status of the RTS line
		unsigned CTS : 1;											// @7		This bit shows the status of the CTS line
		unsigned TXFCE : 1;											// @8		If this bit is set the transmitter FIFO is empty. Thus it can accept 8 symbols
		unsigned TX_DONE : 1;										// @9		This bit is set if the transmitter is idle and the transmit FIFO is empty. It is a logic AND of bits 2 and 8 
		unsigned reserved : 6;										// @10-15	Reserved - Write as 0, read as don't care 
		unsigned RXFIFOLEVEL : 4;									// @16-19	These bits shows how many symbols are stored in the receive FIFO The value is in the range 0-8 
		unsigned reserved1 : 4;										// @20-23	Reserved - Write as 0, read as don't care 
		unsigned TXFIFOLEVEL : 4;									// @24-27	These bits shows how many symbols are stored in the transmit FIFO The value is in the range 0-8
		unsigned reserved2 : 4;										// @28-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_stat_reg_t;

/*--------------------------------------------------------------------------}
{	  MINI UART BAUDRATE Register BCM2835 ARM Peripheral manual page 19  	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned DIVISOR : 16;										// @0-15	 Baudrate divisor  
		unsigned reserved : 16;										// @16-31	 Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mu_baudrate_reg_t;

/*--------------------------------------------------------------------------}
{	  MINIUART STRUCTURE LAYOUT BCM2835 ARM Peripheral manual page 8	    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) MiniUARTRegisters {
	mu_io_reg_t IO;													// +0x0
	mu_ie_reg_t IER;												// +0x4
	mu_ii_reg_t IIR;												// +0x8
	mu_lcr_reg_t LCR;												// +0xC
	mu_mcr_reg_t MCR;												// +0x10
	const mu_lsr_reg_t LSR;											// +0x14	** READ ONLY HENCE CONST **
	const mu_msr_reg_t MSR;											// +0x18	** READ ONLY HENCE CONST **
	mu_scratch_reg_t SCRATCH;										// +0x1C
	mu_cntl_reg_t CNTL;												// +0x20
	const mu_stat_reg_t STAT;										// +0x24	** READ ONLY HENCE CONST **
	mu_baudrate_reg_t BAUD;											// +0x28;
};

/*--------------------------------------------------------------------------}
{     PL011 UART DATA Register BCM2835 ARM Peripheral manual page 179/180	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned DATA : 8;											// @0-7		Transmit Read/write data
		unsigned FE : 1;											// @8		Framing error
		unsigned PE : 1;											// @9		Parity error
		unsigned BE : 1;											// @10		Break error
		unsigned OE : 1;											// @11		Overrun error
		unsigned _reserved : 20;									// @12-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_data_reg_t;

/*--------------------------------------------------------------------------}
{     PL011 UART FR Register BCM2835 ARM Peripheral manual page 181/182	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned CTS : 1;											// @0		Clear to send. This bit is the complement of the UART clear to send, nUARTCTS
		unsigned DSR : 1;											// @1		Unsupported, write zero, read as don't care 
		unsigned DCD : 1;											// @2		Unsupported, write zero, read as don't care  
		unsigned BUSY : 1;											// @3		UART busy. If this bit is set to 1, the UART is busy transmitting data.
		unsigned RXFE : 1;											// @4		Receive FIFO empty. The meaning of this bit depends on the state of the FEN bit
		unsigned TXFF : 1;											// @5		Transmit FIFO full. The meaning of this bit depends on the state of the FEN bit
		unsigned RXFF : 1;											// @6		Receive FIFO full. The meaning of this bit depends on the state of the FEN bit 
		unsigned TXFE : 1;											// @7		Transmit FIFO empty. The meaning of this bit depends on the state of the FEN bit 
		unsigned _reserved : 24;									// @8-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_fr_reg_t;

/*--------------------------------------------------------------------------}
{      PL011 UART IBRD Register BCM2835 ARM Peripheral manual page 183	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned DIVISOR : 16;										// @0-15	Integer baud rate divisor
		unsigned _reserved : 16;									// @12-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_ibrd_reg_t;

/*--------------------------------------------------------------------------}
{      PL011 UART FBRD Register BCM2835 ARM Peripheral manual page 184	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned DIVISOR : 6;										// @0-5		Factional baud rate divisor
		unsigned _reserved : 26;									// @6-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_fbrd_reg_t;

/*--------------------------------------------------------------------------}
{    PL011 UART LRCH Register BCM2835 ARM Peripheral manual page 184/185	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned BRK : 1;											// @0		Send break. If this bit is set to 1, a low-level is continually output on the TXD output
		unsigned PEN : 1;											// @1		Parity enable, 0 = parity is disabled, 1 = parity via bit 2. 
		unsigned EPS : 1;											// @2		If PEN = 1 then 0 = odd parity, 1 = even parity
		unsigned STP2 : 1;											// @3		Two stops = 1, 1 stop bit = 0
		unsigned FEN : 1;											// @4		FIFO's enable = 1, No FIFO's uart becomes 1 character deep = 0
		enum {
			PL011_DATA_5BITS = 0,
			PL011_DATA_6BITS = 1,
			PL011_DATA_7BITS = 2,
			PL011_DATA_8BITS = 3,
		} DATALEN : 2;												// @5-6		Data length for transmission
		unsigned SPS : 1;											// @7		Stick parity select 1 = enabled, 0 = stick parity is disabled 
		unsigned _reserved : 24;									// @8-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_lrch_reg_t;

/*--------------------------------------------------------------------------}
{    PL011 UART CR Register BCM2835 ARM Peripheral manual page 185/186/187	}
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned UARTEN : 1;										// @0		UART enable = 1, disabled = 0
		unsigned _unused : 6;										// @2-6		unused bits
		unsigned LBE : 1;											// @7		Loop back enable = 1
		unsigned TXE : 1;											// @8		TX enabled = 1, disabled = 0
		unsigned RXE : 1;											// @9		RX enabled = 1, disabled = 0
		unsigned DTR_unused : 1;									// @10		DTR unused
		unsigned RTS : 1;											// @11		RTS complement when the bit is programmed to a 1 then nUARTRTS is LOW.
		unsigned OUT : 2;											// @12-13	Unsupported
		unsigned RTSEN : 1;											// @14		RTS hardware flow control enable if this bit is set to 1. 
		unsigned CTSEN : 1;											// @15		CTS hardware flow control enable if this bit is set to 1.
		unsigned _reserved : 16;									// @16-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_cr_reg_t;

/*--------------------------------------------------------------------------}
{       PL011 UART ICR Register BCM2835 ARM Peripheral manual page 192	    }
{--------------------------------------------------------------------------*/
typedef union
{
	struct __attribute__((__packed__))
	{
		unsigned RIMIC : 1;											// @0		Unsupported, write zero, read as don't care 
		unsigned CTSMIC : 1;										// @1		nUARTCTS modem masked interrupt status 
		unsigned DCDMIC : 1;										// @2		Unsupported, write zero, read as don't care 
		unsigned DSRMIC : 1;										// @3		Unsupported, write zero, read as don't care 
		unsigned RXIC : 1;											// @4		Receive interrupt clear. 
		unsigned TXIC : 1;											// @5		Transmit interrupt clear
		unsigned RTIC : 1;											// @6		Receive timeout interrupt clear. 
		unsigned FEIC : 1;											// @7		Framing error interrupt clear.
		unsigned PEIC : 1;											// @8		Parity error interrupt clear.
		unsigned BEIC : 1;											// @9		Break error interrupt clear.
		unsigned OEIC : 1;											// @10		Overrun error interrupt clear.
		unsigned _reserved : 21;									// @11-31	Reserved - Write as 0, read as don't care 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} pl011_icr_reg_t;

/*--------------------------------------------------------------------------}
{	 PL011 UART STRUCTURE LAYOUT BCM2835 ARM Peripheral manual page 175	    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) PL011UARTRegisters {
	pl011_data_reg_t DR;											// +0x0
	uint32_t RSRECR;												// +0x4
	uint32_t _unused[4];											// +0x8, +0xC, +0x10, +0x14
	pl011_fr_reg_t FR;												// +0x18
	uint32_t _unused1[2];											// +0x1C, 0x20
	pl011_ibrd_reg_t IBRD;											// +0x24
	pl011_fbrd_reg_t FBRD;											// +0x28
	pl011_lrch_reg_t LCRH;											// +0x2C
	pl011_cr_reg_t CR;												// +0x30
	uint32_t IFLS;													// +0x34
	uint32_t IMSC;													// +0x38
	uint32_t RIS;													// +0x3C
	uint32_t MIS;													// +0x40
	pl011_icr_reg_t ICR;											// +0x44
	uint32_t DMACR;													// +0x48
};

/*--------------------------------------------------------------------------}
{    LOCAL TIMER INTERRUPT ROUTING REGISTER - QA7_rev3.4.pdf page 18		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		enum {
			LOCALTIMER_TO_CORE0_IRQ = 0,
			LOCALTIMER_TO_CORE1_IRQ = 1,
			LOCALTIMER_TO_CORE2_IRQ = 2,
			LOCALTIMER_TO_CORE3_IRQ = 3,
			LOCALTIMER_TO_CORE0_FIQ = 4,
			LOCALTIMER_TO_CORE1_FIQ = 5,
			LOCALTIMER_TO_CORE2_FIQ = 6,
			LOCALTIMER_TO_CORE3_FIQ = 7,
		} Routing : 3;												// @0-2		Local Timer routing 
		unsigned unused : 29;										// @3-31
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} local_timer_int_route_reg_t;

/*--------------------------------------------------------------------------}
{    LOCAL TIMER CONTROL AND STATUS REGISTER - QA7_rev3.4.pdf page 17		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned ReloadValue : 28;									// @0-27	Reload value 
		unsigned TimerEnable : 1;									// @28		Timer enable (1 = enabled) 
		unsigned IntEnable : 1;										// @29		Interrupt enable (1= enabled)
		unsigned unused : 1;										// @30		Unused
		unsigned IntPending : 1;									// @31		Timer Interrupt flag (Read-Only) 
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} local_timer_ctrl_status_reg_t;

/*--------------------------------------------------------------------------}
{     LOCAL TIMER CLEAR AND RELOAD REGISTER - QA7_rev3.4.pdf page 18		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned unused : 30;										// @0-29	unused 
		unsigned Reload : 1;										// @30		Local timer-reloaded when written as 1 
		unsigned IntClear : 1;										// @31		Interrupt flag clear when written as 1  
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} local_timer_clr_reload_reg_t;

/*--------------------------------------------------------------------------}
{    GENERIC TIMER INTERRUPT CONTROL REGISTER - QA7_rev3.4.pdf page 13		}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned nCNTPSIRQ_IRQ : 1;									// @0		Secure physical timer event IRQ enabled, This bit is only valid if bit 4 is clear otherwise it is ignored. 
		unsigned nCNTPNSIRQ_IRQ : 1;								// @1		Non-secure physical timer event IRQ enabled, This bit is only valid if bit 5 is clear otherwise it is ignored
		unsigned nCNTHPIRQ_IRQ : 1;									// @2		Hypervisor physical timer event IRQ enabled, This bit is only valid if bit 6 is clear otherwise it is ignored
		unsigned nCNTVIRQ_IRQ : 1;									// @3		Virtual physical timer event IRQ enabled, This bit is only valid if bit 7 is clear otherwise it is ignored
		unsigned nCNTPSIRQ_FIQ : 1;									// @4		Secure physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (0) 
		unsigned nCNTPNSIRQ_FIQ : 1;								// @5		Non-secure physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (1)
		unsigned nCNTHPIRQ_FIQ : 1;									// @6		Hypervisor physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (2)
		unsigned nCNTVIRQ_FIQ : 1;									// @7		Virtual physical timer event FIQ enabled, If set, this bit overrides the IRQ bit (3)
		unsigned reserved : 24;										// @8-31	reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} generic_timer_int_ctrl_reg_t;


/*--------------------------------------------------------------------------}
{       MAILBOX INTERRUPT CONTROL REGISTER - QA7_rev3.4.pdf page 14			}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned Mailbox0_IRQ : 1;									// @0		Set IRQ enabled, This bit is only valid if bit 4 is clear otherwise it is ignored. 
		unsigned Mailbox1_IRQ : 1;									// @1		Set IRQ enabled, This bit is only valid if bit 5 is clear otherwise it is ignored
		unsigned Mailbox2_IRQ : 1;									// @2		Set IRQ enabled, This bit is only valid if bit 6 is clear otherwise it is ignored
		unsigned Mailbox3_IRQ : 1;									// @3		Set IRQ enabled, This bit is only valid if bit 7 is clear otherwise it is ignored
		unsigned Mailbox0_FIQ : 1;									// @4		Set FIQ enabled, If set, this bit overrides the IRQ bit (0) 
		unsigned Mailbox1_FIQ : 1;									// @5		Set FIQ enabled, If set, this bit overrides the IRQ bit (1)
		unsigned Mailbox2_FIQ : 1;									// @6		Set FIQ enabled, If set, this bit overrides the IRQ bit (2)
		unsigned Mailbox3_FIQ : 1;									// @7		Set FIQ enabled, If set, this bit overrides the IRQ bit (3)
		unsigned reserved : 24;										// @8-31	reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} mailbox_int_ctrl_reg_t;

/*--------------------------------------------------------------------------}
{		 CORE INTERRUPT SOURCE REGISTER - QA7_rev3.4.pdf page 16			}
{--------------------------------------------------------------------------*/
typedef union
{
	struct
	{
		unsigned CNTPSIRQ : 1;										// @0		CNTPSIRQ  interrupt 
		unsigned CNTPNSIRQ : 1;										// @1		CNTPNSIRQ  interrupt 
		unsigned CNTHPIRQ : 1;										// @2		CNTHPIRQ  interrupt 
		unsigned CNTVIRQ : 1;										// @3		CNTVIRQ  interrupt 
		unsigned Mailbox0_Int : 1;									// @4		Set FIQ enabled, If set, this bit overrides the IRQ bit (0) 
		unsigned Mailbox1_Int : 1;									// @5		Set FIQ enabled, If set, this bit overrides the IRQ bit (1)
		unsigned Mailbox2_Int : 1;									// @6		Set FIQ enabled, If set, this bit overrides the IRQ bit (2)
		unsigned Mailbox3_Int : 1;									// @7		Set FIQ enabled, If set, this bit overrides the IRQ bit (3)
		unsigned GPU_Int : 1;										// @8		GPU interrupt <Can be high in one core only> 
		unsigned PMU_Int : 1;										// @9		PMU interrupt 
		unsigned AXI_Int : 1;										// @10		AXI-outstanding interrupt <For core 0 only!> all others are 0 
		unsigned Timer_Int : 1;										// @11		Local timer interrupt
		unsigned GPIO_Int : 16;										// @12-27	Peripheral 1..15 interrupt (Currently not used
		unsigned reserved : 4;										// @28-31	reserved
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} core_int_source_reg_t;

/*--------------------------------------------------------------------------}
{					 ALL QA7 REGISTERS IN ONE BIG STRUCTURE					}
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) QA7Registers {
	local_timer_int_route_reg_t TimerRouting;						// 0x24
	uint32_t GPIORouting;											// 0x28
	uint32_t AXIOutstandingCounters;								// 0x2C
	uint32_t AXIOutstandingIrq;										// 0x30
	local_timer_ctrl_status_reg_t TimerControlStatus;				// 0x34
	local_timer_clr_reload_reg_t TimerClearReload;					// 0x38
	uint32_t unused;												// 0x3C
	generic_timer_int_ctrl_reg_t CoreTimerIntControl[4];			// 0x40, 0x44, 0x48, 0x4C  .. One per core
	mailbox_int_ctrl_reg_t  CoreMailboxIntControl[4];				// 0x50, 0x54, 0x58, 0x5C  .. One per core
	core_int_source_reg_t CoreIRQSource[4];							// 0x60, 0x64, 0x68, 0x6C  .. One per core
	core_int_source_reg_t CoreFIQSource[4];							// 0x70, 0x74, 0x78, 0x7C  .. One per core
};

/*--------------------------------------------------------------------------}
{						RASPBERRY PI MODEL NUMBER 							}
{--------------------------------------------------------------------------*/
/* https://github.com/raspberrypi/documentation/blob/master/hardware/raspberrypi/revision-codes/README.md */
typedef union
{
	struct
	{
		unsigned revision : 4;										// @0-3			revision 0,1,2 .. etc
		unsigned model : 8;											// @4-11		model type
		unsigned processor : 4;										// @12-15		processor 0 = BCM2835, 1 = BCM2836, 2 = BCM2837
		unsigned manufacturer : 4;									// @16-19		manufacturer 
		unsigned memory_size : 3;									// @20-22		memory size 0 = 256 MB, 1 = 512MB, 2 = 1024MB 
		unsigned new_revision : 1;									// @23			new style revision if set
		unsigned _reserved : 8;										// @24-31		reserved for future use
	};
	uint32_t Raw32;													// Union to access all 32 bits as a uint32_t
} model_number_t;

/***************************************************************************}
{        PRIVATE INTERNAL RASPBERRY PI REGISTER STRUCTURE CHECKS            }
****************************************************************************/

/*--------------------------------------------------------------------------}
{					 CODE TYPE STRUCTURE COMPILE TIME CHECKS	            }
{--------------------------------------------------------------------------*/
/* If you have never seen compile time assertions it's worth google search */
/* on "Compile Time Assertions". It is part of the C11++ specification and */
/* all compilers that support the standard will have them (GCC, MSC inc)   */
/*-------------------------------------------------------------------------*/
#include <assert.h>								// Need for compile time static_assert

/* Check the code type structure size */
static_assert(sizeof(CODE_TYPE) == 0x04, "Structure CODE_TYPE should be 0x04 bytes in size");
static_assert(sizeof(CPU_ID) == 0x04, "Structure CPU_ID should be 0x04 bytes in size");
static_assert(sizeof(model_number_t) == 0x04, "Structure model_number_t should be 0x04 bytes in size");
static_assert(sizeof(struct GPIORegisters) == 0xA0, "Structure GPIORegisters should be 0xA0 bytes in size");
static_assert(sizeof(struct SystemTimerRegisters) == 0x1C, "Structure SystemTimerRegisters should be 0x1C bytes in size");
static_assert(sizeof(struct ArmTimerRegisters) == 0x1C, "Structure ArmTimerRegisters should be 0x1C bytes in size");
static_assert(sizeof(struct IrqControlRegisters) == 0x28, "Structure IrqControlRegisters should be 0x28 bytes in size");
static_assert(sizeof(struct MailBoxRegisters) == 0x40, "Structure MailBoxRegisters should be 0x40 bytes in size");
static_assert(sizeof(struct MiniUARTRegisters) == 0x2C, "MiniUARTRegisters should be 0x2C bytes in size");
static_assert(sizeof(struct PL011UARTRegisters) == 0x4C, "PL011UARTRegisters should be 0x4C bytes in size");
static_assert(sizeof(struct QA7Registers) == 0x5C, "QA7Registers should be 0x5C bytes in size");

/***************************************************************************}
{     PRIVATE POINTERS TO ALL OUR RASPBERRY PI REGISTER BANK STRUCTURES	    }
****************************************************************************/
#define GPIO ((volatile __attribute__((aligned(4))) struct GPIORegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x200000))
#define SYSTEMTIMER ((volatile __attribute__((aligned(4))) struct SystemTimerRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x3000))
#define IRQ ((volatile __attribute__((aligned(4))) struct IrqControlRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB200))
#define ARMTIMER ((volatile __attribute__((aligned(4))) struct ArmTimerRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB400))
#define MAILBOX ((volatile __attribute__((aligned(4))) struct MailBoxRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB880))
#define MINIUART ((volatile struct MiniUARTRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x00215040))
#define PL011UART ((volatile struct PL011UARTRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x00201000))
#define QA7 ((volatile __attribute__((aligned(4))) struct QA7Registers*)(uintptr_t)(0x40000024))

/***************************************************************************}
{				   ARM CPU ID STRINGS THAT WILL BE RETURNED				    }
****************************************************************************/
static const char* ARM6_STR = "arm1176jzf-s";
static const char* ARM7_STR = "cortex-a7";
static const char* ARM8_STR = "cortex-a53";
static const char* ARMX_STR = "unknown ARM cpu";

/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*==========================================================================}
{			 PUBLIC CPU ID ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/

/*-[ RPi_CpuIdString]-------------------------------------------------------}
. Returns the CPU id string of the CPU auto-detected by the SmartStart code 
.--------------------------------------------------------------------------*/
const char* RPi_CpuIdString (void)
{
	switch (RPi_CpuId.PartNumber) 
	{
	case 0xb76:														// ARM 6 CPU
		return ARM6_STR;											// Return the ARM6 string
	case 0xc07:														// ARM 7 CPU
		return ARM7_STR;											// Return the ARM7 string
	case 0xd03:														// ARM 8 CPU
		return ARM8_STR;											// Return the ARM string
	default:
		return ARMX_STR;											// Unknown CPU
	}																// End switch RPi_CpuId.PartNumber
}

/*==========================================================================}
{			 PUBLIC GPIO ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/
#define MAX_GPIO_NUM 54												// GPIO 0..53 are valid

/*-[gpio_setup]-------------------------------------------------------------}
. Given a valid GPIO port number and mode sets GPIO to given mode
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_setup (unsigned int gpio, GPIOMODE mode) 
{
	if ((gpio < MAX_GPIO_NUM) && (mode <= 7))						// Check GPIO pin number and mode valid
	{
		uint32_t bit = ((gpio % 10) * 3);							// Create bit mask
		uint32_t mem = GPIO->GPFSEL[gpio / 10];						// Read register
		mem &= ~(7 << bit);											// Clear GPIO mode bits for that port
		mem |= (mode << bit);										// Logical OR GPIO mode bits
		GPIO->GPFSEL[gpio / 10] = mem;								// Write value to register
		return true;												// Return true
	}
	return false;													// Return false is something is invalid
}

/*-[gpio_output]------------------------------------------------------------}
. Given a valid GPIO port number the output is set high(true) or Low (false)
. RETURN: true for success, false for any failure
.--------------------------------------------------------------------------*/
bool gpio_output (unsigned int gpio, bool on) 
{
	if (gpio < MAX_GPIO_NUM) 										// Check GPIO pin number valid
	{
		volatile uint32_t* p;
		uint32_t bit = 1 << (gpio % 32);							// Create mask bit
		p = (on) ? &GPIO->GPSET[0] : &GPIO->GPCLR[0];				// Set pointer depending on on/off state
		p[gpio / 32] = bit;											// Output bit to register number	
		return true;												// Return true
	}
	return false;													// Return false
}

/*-[gpio_input]-------------------------------------------------------------}
. Reads the actual level of the GPIO port number
. RETURN: true = GPIO input high, false = GPIO input low
.--------------------------------------------------------------------------*/
bool gpio_input (unsigned int gpio) 
{
	if (gpio < MAX_GPIO_NUM)										// Check GPIO pin number valid
	{
		uint32_t bit = 1 << (gpio % 32);							// Create mask bit
		uint32_t mem = GPIO->GPLEV[gpio / 32];						// Read port level
		if (mem & bit) return true;									// Return true if bit set
	}
	return false;													// Return false
}

/*-[gpio_checkEvent]-------------------------------------------------------}
. Checks the given GPIO port number for an event/irq flag.
. RETURN: true for event occured, false for no event
.-------------------------------------------------------------------------*/
bool gpio_checkEvent (unsigned int gpio) 
{
	if (gpio < MAX_GPIO_NUM)										// Check GPIO pin number valid
	{
		uint32_t bit = 1 << (gpio % 32);							// Create mask bit
		uint32_t mem = GPIO->GPEDS[gpio / 32];						// Read event detect status register
		if (mem & bit) return true;									// Return true if bit set
	}
	return false;													// Return false
}

/*-[gpio_clearEvent]-------------------------------------------------------}
. Clears the given GPIO port number event/irq flag.
. RETURN: true for success, false for any failure
.-------------------------------------------------------------------------*/
bool gpio_clearEvent (unsigned int gpio) 
{
	if (gpio < MAX_GPIO_NUM)										// Check GPIO pin number valid
	{
		uint32_t bit = 1 << (gpio % 32);							// Create mask bit
		GPIO->GPEDS[gpio / 32] = bit;								// Clear the event from GPIO register
		return true;												// Return true
	}
	return false;													// Return false
}

/*-[gpio_edgeDetect]-------------------------------------------------------}
. Sets GPIO port number edge detection to lifting/falling in Async/Sync mode
. RETURN: true for success, false for any failure
.-------------------------------------------------------------------------*/
bool gpio_edgeDetect (unsigned int gpio, bool lifting, bool Async) 
{
	if (gpio < MAX_GPIO_NUM)										// Check GPIO pin number valid
	{
		volatile uint32_t* p;
		uint32_t bit = 1 << (gpio % 32);							// Create mask bit
		if (lifting) {												// Lifting edge detect
			p = (Async) ? &GPIO->GPAREN[0] : &GPIO->GPREN[0];		// Select Synchronous/Asynchronous lifting edge detect register
		}
		else {														// Falling edge detect
			p = (Async) ? &GPIO->GPAFEN[0] : &GPIO->GPFEN[0];		// Select Synchronous/Asynchronous falling edge detect register
		}
		p[gpio / 32] = bit;											// Set the register with the mask
		return true;												// Return true
	}
	return false;													// Return false
}

/*-[gpio_fixResistor]------------------------------------------------------}
. Set the GPIO port number with fix resistors to pull up/pull down.
. RETURN: true for success, false for any failure
.-------------------------------------------------------------------------*/
bool gpio_fixResistor (unsigned int gpio, GPIO_FIX_RESISTOR resistor) 
{
	if ((gpio < MAX_GPIO_NUM) && (resistor <= 2))					// Check GPIO pin number and resistor mode valid
	{
		uint32_t regnum = gpio / 32;								// Create register number
		uint32_t bit = 1 << (gpio % 32);							// Create mask bit
		GPIO->GPPUD = resistor;										// Set fixed resistor request to PUD register
		timer_wait(2);												// Wait 2 microseconds
		GPIO->GPPUDCLK[regnum] = bit;								// Set the PUD clock bit register
		timer_wait(2);												// Wait 2 microseconds	
		GPIO->GPPUD = 0;											// Clear GPIO resistor setting
		GPIO->GPPUDCLK[regnum] = 0;									// Clear PUDCLK from GPIO
		return true;												// Return true
	}
	return false;													// Return false
}


/*==========================================================================}
{		   PUBLIC TIMER ROUTINES PROVIDED BY RPi-SmartStart API				}
{==========================================================================*/

/*-[timer_getTickCount64]---------------------------------------------------}
. Get 1Mhz ARM system timer tick count in full 64 bit.
. The timer read is as per the Broadcom specification of two 32bit reads
. This is lockfree reads and can be done off any core and concurrently.
. RETURN: tickcount value as an unsigned 64bit value in microseconds (usec)
.--------------------------------------------------------------------------*/
uint64_t timer_getTickCount64 (void)
{
	uint32_t hiCount;
	uint32_t loCount;
	do {
		hiCount = SYSTEMTIMER->TimerHi; 							// Read Arm system timer high count
		loCount = SYSTEMTIMER->TimerLo;								// Read Arm system timer low count
	} while (hiCount != SYSTEMTIMER->TimerHi);						// Check hi counter hasn't rolled as we did low read
	return (((uint64_t)hiCount << 32) | loCount);					// Join the 32 bit values to a full 64 bit
}

/*-[timer_Wait]-------------------------------------------------------------}
. This will simply wait the requested number of microseconds before return.
. This is lockfree reads and can be done off any core and concurrently.
. This is fully blocking and in an O/S situations you should use the O/S
. timed wait so the processor can be redeployed while waiting.
.--------------------------------------------------------------------------*/
void timer_wait (uint64_t usec) 
{
	usec += timer_getTickCount64();									// Add current tickcount onto delay .. usec may roll but ok look at next test
	while (timer_getTickCount64() < usec) {};						// Loop on timeout function until timeout 
}

/*-[tick_Difference]--------------------------------------------------------}
. Given two timer tick results it returns the time difference between them.
. If (us1 > us2) it is assumed the timer rolled as we expect (us2 > us1)
. This is lockfree reads and can be done off any core and concurrently.
.--------------------------------------------------------------------------*/
uint64_t tick_difference (uint64_t us1, uint64_t us2) 
{
	if (us1 > us2) {												// If timer one is greater than two then timer rolled
		uint64_t td = UINT64_MAX - us1 + 1;							// Counts left to roll value
		return us2 + td;											// Add that to new count
	}
	return us2 - us1;												// Return difference between values
}


/*==========================================================================}
{		  PUBLIC PI MAILBOX ROUTINES PROVIDED BY RPi-SmartStart API			}
{==========================================================================*/
#define MAIL_EMPTY	0x40000000		/* Mailbox Status Register: Mailbox Empty */
#define MAIL_FULL	0x80000000		/* Mailbox Status Register: Mailbox Full  */

/*-[mailbox_write]----------------------------------------------------------}
. This will execute the sending of the given data block message thru the
. mailbox system on the given channel. It is normal for a response back so
. usually you need to follow the write up with a read.
. RETURN: True for success, False for failure.
.--------------------------------------------------------------------------*/
bool mailbox_write (MAILBOX_CHANNEL channel, uint32_t message) 
{
	if ((channel >= 0) && (channel <= MB_CHANNEL_GPU))				// Check channel valid
	{
		message &= ~(0xF);											// Make sure 4 low channel bits are clear 
		message |= channel;											// OR the channel bits to the value							
		while ((MAILBOX->Status1 & MAIL_FULL) != 0);				// Make sure arm mailbox is not full
		MAILBOX->Write1 = message;									// Write value to mailbox
		return true;												// Write success
	}
	return false;													// Return false
}

/*-[mailbox_read]-----------------------------------------------------------}
. This will read any pending data on the mailbox system on the given channel.
. RETURN: The read value for success, 0xFEEDDEAD for failure.
.--------------------------------------------------------------------------*/
uint32_t mailbox_read (MAILBOX_CHANNEL channel) 
{
	if ((channel >= 0) && (channel <= MB_CHANNEL_GPU))				// Check channel valid
	{
		uint32_t value;
		do {
			while ((MAILBOX->Status0 & MAIL_EMPTY) != 0);			// Wait for data in mailbox
			value = MAILBOX->Read0;									// Read the mailbox	
		} while ((value & 0xF) != channel);							// We have response back
		value &= ~(0xF);											// Lower 4 low channel bits are not part of message
		return value;												// Return the value
	}
	return 0xFEEDDEAD;												// Channel was invalid
}

/*-[mailbox_tag_message]----------------------------------------------------}
. This will post and execute the given variadic data onto the tags channel
. on the mailbox system. You must provide the correct number of response
. uint32_t variables and a pointer to the response buffer. You nominate the
. number of data uint32_t for the call and fill the variadic data in. If you
. do not want the response data back the use NULL for response_buf pointer.
. RETURN: True for success and the response data will be set with data
.         False for failure and the response buffer is untouched.
.--------------------------------------------------------------------------*/
bool mailbox_tag_message (uint32_t* response_buf,					// Pointer to response buffer
						  uint8_t data_count,						// Number of uint32_t variadic data following
						  ...)										// Variadic uint32_t values for call
{
	uint32_t __attribute__((aligned(16))) message[data_count+3];
	uint32_t addr = (uint32_t)(uintptr_t)&message[0];
	va_list list;
	va_start(list, data_count);										// Start variadic argument
	message[0] = (data_count + 3) * 4;								// Size of message needed
	message[data_count + 2] = 0;									// Set end pointer to zero
	message[1] = 0;													// Zero response message
	for (int i = 0; i < data_count; i++) {
		message[2 + i] = va_arg(list, uint32_t);					// Fetch next variadic
	}
	va_end(list);													// variadic cleanup	

#if __aarch64__ == 1
	__asm volatile ("dc civac, %0" : : "r" (addr) : "memory");		// Ensure coherence
#else
	__asm volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r" (addr));	// Ensure coherence
#endif

	mailbox_write(MB_CHANNEL_TAGS, ARMaddrToGPUaddr(addr));			// Write message to mailbox
	mailbox_read(MB_CHANNEL_TAGS);									// Read the response

#if __aarch64__ == 1
	__asm volatile ("dc civac, %0" : : "r" (addr) : "memory");		// Ensure coherence
#else 
	__asm volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r" (addr));	// Ensure coherence
#endif

	if (message[1] == 0x80000000) {									// Check success flag
		if (response_buf) {											// If buffer NULL used then don't want response
			for (int i = 0; i < data_count; i++)
				response_buf[i] = message[2 + i];					// Transfer out each response message
		}
		return true;												// message success
	}
	return false;													// Message failed
}


/*==========================================================================}
{	  PUBLIC PI TIMER INTERRUPT ROUTINES PROVIDED BY RPi-SmartStart API		}
{==========================================================================*/

/*-[ClearTimerIrq]----------------------------------------------------------}
. Simply clear the timer interupt by hitting the clear register. Any timer
. irq/fiq interrupt should call this before exiting.
.--------------------------------------------------------------------------*/
void ClearTimerIrq (void)
{
	ARMTIMER->Clear = 0x1;											// Hit clear register
}

/*-[TimerSetup]-------------------------------------------------------------}
. Sets the clock rate to the period in usec for the peripheral clock timer.
. Largest period is around 16 million usec (16 sec) it varies on core speed.
. RETURN: TRUE if successful,  FALSE for any failure
.--------------------------------------------------------------------------*/
bool TimerSetup (uint32_t period_in_us)								// Peripheral clock timer period in usec
{
	uint32_t Buffer[5] = { 0 };
	bool resValue = false;
	ARMTIMER->Control.TimerEnable = 0;								// Make sure clock is stopped, illegal to change anything while running
	if (mailbox_tag_message(&Buffer[0], 5,							// Two uint32_t responses being clock id and speed and 5 variadics passed
		MAILBOX_TAG_GET_CLOCK_RATE,	8, 8, 4, 0))					// Get GPU clock (it varies between 200-450Mhz)
	{
		volatile uint32_t divisor = Buffer[4] / 250;				// The prescaler divider is set to 250 (based on GPU=250MHz to give 1Mhz clock)
		divisor /= 10000;											// Divid by 10000 we are trying to hold some precision should be in low hundreds (160'ish)
		divisor *= period_in_us;									// Multiply by the micro seconds result
		divisor /= 100;												// This completes the division by 1000000 (done as /10000 and /100)
		if (divisor != 0) {											// Invalid divisor of zero will return with fail
			ARMTIMER->Load = divisor;								// Set the load value to divisor
			ARMTIMER->Control.Counter32Bit = 1;						// Counter in 32 bit mode
			ARMTIMER->Control.Prescale = Clkdiv1;					// Clock divider = 1
			resValue = true;										// Set success result
		}
		ARMTIMER->Control.TimerEnable = 1;							// Now start the clock
	}
	return resValue;												// Return result value	
}

/*-[TimerIrqSetup]----------------------------------------------------------}
. The irq interrupt rate is set to the period in usec between triggers.
. Largest period is around 16 million usec (16 sec) it varies on core speed.
. The ARM timer and interrupt flags are both enabled.
. RETURN: TRUE if successful,  FALSE for any failure
.--------------------------------------------------------------------------*/
bool TimerIrqSetup (uint32_t period_in_us)							// Period between timer interrupts in usec
{
	if (TimerSetup(period_in_us))									// Peripheral time set successful
	{
		IRQ->EnableBasicIRQs.Enable_Timer_IRQ = 1;					// Enable the timer interrupt IRQ
		ARMTIMER->Control.TimerIrqEnable = 1;						// Enable timer irq
		return true;												// Return success									
	}
	return false;													// Return failure	
}

/*-[TimerFiqSetup]----------------------------------------------------------}
. Allocates the C function pointer (ARMaddress) to be the fiq called on timer 
. interrupt. The interrupt rate is set by providing a period in usec between 
. triggers of the interrupt. The C registers that need to be preserved are
. saved by a prolog and epilog code before and after the C function call.
. If an ARMaddress of 0 is used it causes handler setting to be ignored.
. Largest period is around 16 million usec (16 sec) it varies on core speed
. RETURN: The old function pointer that was in use (will return 0 for 1st).
.--------------------------------------------------------------------------*/
uintptr_t TimerFiqSetup (uint32_t period_in_us, 					// Period between timer interrupts in usec
						 void(*ARMaddress)(void))					// C function pointer to call (0 = ignored)
{
	uintptr_t oldHandler = 0;
	if (TimerSetup(period_in_us))									// Peripheral time set successful
	{
		if (ARMaddress)	oldHandler = setFiqFuncAddress(ARMaddress);	// Change the handler			
		ARMTIMER->Control.TimerIrqEnable = 1;						// Enable timer irq
		IRQ->FIQControl.SelectFIQSource = 64;						// Select FIQ source as the ARM timer
		IRQ->FIQControl.EnableFIQ = 1;								// Enable the FIQ
	}
	return oldHandler;												// Return result	
}

/*==========================================================================}
{  PUBLIC PI MULTICORE LOCAL TIMER ROUTINES PROVIDED BY RPi-SmartStart API	}
{==========================================================================*/

/*-[ClearLocalTimerIrq]-----------------------------------------------------}
. Simply clear the local timer interupt by hitting the clear registers. Any
. irq/fiq local timer interrupt should call this before exiting.
.--------------------------------------------------------------------------*/
void ClearLocalTimerIrq(void)
{
	QA7->TimerClearReload.IntClear = 1;								// Clear interrupt
	QA7->TimerClearReload.Reload = 1;								// Reload now
}

/*-[LocalTimerSetup]--------------------------------------------------------}
. Sets the clock rate to the period in usec for the local clock timer.
. Largest period is around 16 million usec (16 sec) it varies on core speed.
. All cores share this clock so setting it from any core changes all cores.
. RETURN: TRUE if successful,  FALSE for any failure
.--------------------------------------------------------------------------*/
bool LocalTimerSetup (uint32_t period_in_us)						// Period between timer interrupts in usec
{
	if (RPi_CpuId.PartNumber != 0xB76)								// ARM6 cpu is single core and this clock does not exist
	{
		volatile uint32_t divisor = 384 * period_in_us;				// Transfer the period * 384
		divisor /= 10;												// That is divisor required as clock is 38.4Mhz (2*19.2Mhz)
		QA7->TimerControlStatus.ReloadValue = divisor;				// Timer period set
		QA7->TimerControlStatus.TimerEnable = 1;					// Timer enabled
		QA7->TimerClearReload.Reload = 1;							// Reload now
		return true;												// Timer successfully set
	}
	return false;
}


/*-[LocalTimerIrqSetup]-----------------------------------------------------}
. The local timer irq interrupt rate is set to the period in usec between
. triggers. On BCM2835 (ARM6) it does not have core timer so call fails.
. Largest period is around 16 million usec (16 sec) it varies on core speed
. RETURN: TRUE if successful, FALSE for any failure
.--------------------------------------------------------------------------*/
bool LocalTimerIrqSetup (uint32_t period_in_us,						// Period between timer interrupts in usec
						 uint8_t coreNum)							// Core number
{
	if (LocalTimerSetup(period_in_us))								// Peripheral time set successful
	{
		QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_IRQ + coreNum;// Route local timer IRQ to given Core
		QA7->TimerControlStatus.IntEnable = 1;						// Timer IRQ enabled
		QA7->TimerClearReload.IntClear = 1;							// Clear interrupt
		QA7->TimerClearReload.Reload = 1;							// Reload now
		QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_IRQ = 1;		// We are in NS EL1 so enable IRQ to core
		QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_FIQ = 0;		// Make sure FIQ is zero
		return true;												// Return success									
	}
	return false;													// Return failure	
}

/*-[LocalTimerFiqSetup]-----------------------------------------------------}
. The local timer fiq interrupt rate is set to the period in usec between
. triggers. On BCM2835 (ARM6) it does not have core timer so call fails.
. Largest period is around 16 million usec (16 sec) it varies on core speed
. RETURN: TRUE if successful, FALSE for any failure
.--------------------------------------------------------------------------*/
bool LocalTimerFiqSetup (uint32_t period_in_us,						// Period between timer interrupts in usec
						 uint8_t coreNum)							// Core number
{
	if (LocalTimerSetup(period_in_us))								// Peripheral time set successful
	{
		QA7->TimerRouting.Routing = LOCALTIMER_TO_CORE0_FIQ + coreNum;// Route local timer FIQ to given Core
		QA7->TimerControlStatus.IntEnable = 1;						// Timer IRQ enabled
		QA7->TimerClearReload.IntClear = 1;							// Clear interrupt
		QA7->TimerClearReload.Reload = 1;							// Reload now
		QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_FIQ = 1;		// We are in NS EL1 so enable FIQ to core
		QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_IRQ = 0;		// Make sure IRQ is zero
		return true;												// Return success									
	}
	return false;													// Return failure	
}

/*==========================================================================}
{				           MINIUART ROUTINES								}
{==========================================================================*/

#define AUX_ENABLES     (RPi_IO_Base_Addr+0x00215004)

/*-[miniuart_init]----------------------------------------------------------}
. Initializes the miniuart (NS16550 compatible one) to the given baudrate
. with 8 data bits, no parity and 1 stop bit. On the PI models 1A, 1B, 1B+
. and PI ZeroW this is external GPIO14 & GPIO15 (Pin header 8 & 10).
. RETURN: true if successfuly set, false on any error
.--------------------------------------------------------------------------*/
bool miniuart_init (unsigned int baudrate)
{
	uint32_t Buffer[5] = { 0 };
	if (mailbox_tag_message(&Buffer[0], 5,							// Two uint32_t responses being clock id and clock speed and 5 variadics follow
		MAILBOX_TAG_GET_CLOCK_RATE,	8, 8, CLK_CORE_ID, 0)) 			// Get core clock frequency check for fail 
	{
		uint32_t Divisor = (Buffer[4] / (baudrate * 8)) - 1;		// Calculate divisor
		if (Divisor <= 0xFFFF) {
			PUT32(AUX_ENABLES, 1);									// Enable miniuart

			MINIUART->CNTL.RXE = 0;									// Disable receiver
			MINIUART->CNTL.TXE = 0;									// Disable transmitter

			MINIUART->LCR.DATA_LENGTH = 1;							// Data length = 8 bits
			MINIUART->MCR.RTS = 0;									// Set RTS line high
			MINIUART->IIR.RXFIFO_CLEAR = 1;							// Clear RX FIFO
			MINIUART->IIR.TXFIFO_CLEAR = 1;							// Clear TX FIFO

			MINIUART->BAUD.DIVISOR = Divisor;						// Set the divisor

			gpio_setup(14, GPIO_ALTFUNC5 );							// GPIO 14 to ALT FUNC5 mode
			gpio_setup(15, GPIO_ALTFUNC5 );							// GPIO 15 to ALT FUNC5 mode

			MINIUART->CNTL.RXE = 1;									// Enable receiver
			MINIUART->CNTL.TXE = 1;									// Enable transmitter
			return true;											// Return success
		}
	}
	return false;													// Invalid baudrate or mailbox clock error
}

/*-[miniuart_getc]----------------------------------------------------------}
. Wait for an retrieve a character from the uart.
.--------------------------------------------------------------------------*/
char miniuart_getc (void)
{
	while (MINIUART->LSR.RXFDA == 0) {};							// Wait for a character to arrive
	return(MINIUART->IO.DATA);										// Return that character
}

/*-[miniuart_putc]----------------------------------------------------------}
. Send a character out via uart.
.--------------------------------------------------------------------------*/
void miniuart_putc (char c)
{
	while (MINIUART->LSR.TXFE == 0) {};								// Wait for transmitt buffer to have a space
	MINIUART->IO.DATA = c;											// Write character to transmit buffer
}

/*-[miniuart_puts]----------------------------------------------------------}
. Send a '\0' terminated character string out via uart.
.--------------------------------------------------------------------------*/
void miniuart_puts (char *str)
{
	if (str) {														// Make sure string is not null
		while (*str) {												// For each character that is not \0 terminator
			miniuart_putc(*str++);									// Output each character
		}
	}
}

/*==========================================================================}
{				           PL011 UART ROUTINES								}
{==========================================================================*/

/*-[pl011_uart_init]--------------------------------------------------------}
. Initializes the pl011 uart to the given baudrate with 8 data, no parity 
. and 1 stop bit. On the PI models 2B, Pi3, Pi3B+, Pi Zero and CM this is 
. external GPIO14 & GPIO15 (Pin header 8 & 10).
. RETURN: true if successfuly set, false on any error
.--------------------------------------------------------------------------*/
bool pl011_uart_init (unsigned int baudrate)
{
	PL011UART->CR.Raw32 = 0;										// Disable all the UART
	gpio_setup(14, GPIO_ALTFUNC0);									// GPIO 14 to ALT FUNC0 mode
	gpio_setup(15, GPIO_ALTFUNC0);									// GPIO 15 to ALT FUNC0 mode
	if (mailbox_tag_message(0, 5,									// No response required and 5 variadics follow
		MAILBOX_TAG_SET_CLOCK_RATE,	8, 8, CLK_UART_ID, 4000000))	// Set UART clock frequency to 4Mhz
	{
		uint32_t divisor, fracpart;
		divisor = 4000000 / (baudrate * 16);						// Calculate divisor
		PL011UART->IBRD.DIVISOR = divisor;							// Set the 16 integer divisor
		fracpart = 4000000 - (divisor*baudrate * 16);				// Fraction left
		fracpart *= 4;												// fraction part *64 /16 = *4 																		
		fracpart += baudrate / 2;									// Add half baudrate for rough round
		fracpart /= baudrate;										// Divid the baud rate
		PL011UART->FBRD.DIVISOR = fracpart;							// Write the 6bits of fraction
		PL011UART->LCRH.DATALEN = PL011_DATA_8BITS;					// 8 data bits
		PL011UART->LCRH.PEN = 0;									// No parity
		PL011UART->LCRH.STP2 = 0;									// One stop bits AKA not 2 stop bits
		PL011UART->LCRH.FEN = 0;									// Fifo's enabled
		PL011UART->CR.UARTEN = 1;									// Uart enable
		PL011UART->CR.RXE = 1;										// Transmit enable
		PL011UART->CR.TXE = 1;										// Receive enable
		return true;												// Return success
	}
	return false;													// Invalid baudrate or mailbox clock error
}

/*-[pl011_uart_getc]--------------------------------------------------------}
. Wait for an retrieve a character from the uart.
.--------------------------------------------------------------------------*/
char pl011_uart_getc (void) 
{
	while (PL011UART->FR.RXFE != 0) {};								// Check recieve fifo is not empty
	return(PL011UART->DR.DATA);										// Read the receive data
}

/*-[pl011_uart_putc]--------------------------------------------------------}
. Send a character out via uart.
.--------------------------------------------------------------------------*/
void pl011_uart_putc (char c)
{
	while (PL011UART->FR.TXFF != 0) {};								// Check tx fifo is not full
	PL011UART->DR.DATA = c;											// Transfer character
}

/*-[pl011_uart_puts]--------------------------------------------------------}
. Send a '\0' terminated character string out via uart.
.--------------------------------------------------------------------------*/
void pl011_uart_puts (char *str)
{
	if (str) {														// Make sure string is not null
		while (*str) {												// For each character that is not \0 terminator
			pl011_uart_putc(*str++);								// Output that character
		}
	}
}

/*==========================================================================}
{				     PUBLIC PI ACTIVITY LED ROUTINES						}
{==========================================================================*/
static bool ModelCheckHasRun = false;								// Flag set if model check has run
static bool UseExpanderGPIO = false;								// Flag set if we need to use GPIO expander								
static uint_fast8_t ActivityGPIOPort = 47;							// Default GPIO for activity led is 47

/*-[set_Activity_LED]-------------------------------------------------------}
. This will set the PI activity LED on or off as requested. The SmartStart
. stub provides the Pi board autodetection so the right GPIO port is used.
. RETURN: True the LED state was successfully change, false otherwise
.--------------------------------------------------------------------------*/
bool set_Activity_LED (bool on) {
	// THIS IS ALL BASED ON PI MODEL HISTORY: 
    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/README.md
	if (!ModelCheckHasRun) {
		uint32_t model[4];
		ModelCheckHasRun = true;									// Set we have run the model check
		if (mailbox_tag_message(&model[0], 4,						// Model id and 4 variadics follow
			MAILBOX_TAG_GET_BOARD_REVISION, 4, 4, 0))				// Fetch the model revision from mailbox   
		{
			model[0] &= 0x00FFFFFF;									// Mask off the warranty upper 8 bits
			if ((model[3] >= 0x0002) && (model[3] <= 0x000f))		// These are Model A,B which use GPIO 16
			{														// Model A, B return 0x0002 to 0x000F
				ActivityGPIOPort = 16;								// GPIO port 16 is activity led
				UseExpanderGPIO = false;							// Dont use expander GPIO
			}
			else if (model[3] < 0xa02082) {							// These are Pi2, PiZero or Compute models (They may be ARM7 or ARM8)
				ActivityGPIOPort = 47;								// GPIO port 47 is activity led
				UseExpanderGPIO = false;							// Dont use expander GPIO
			}
			else if ( (model[3] == 0xa02082) ||
					  (model[3] == 0xa020a0) ||
					  (model[3] == 0xa22082) ||
					  (model[3] == 0xa32082) )						// These are Pi3B series originals (ARM8)
			{
				ActivityGPIOPort = 130;								// GPIO port 130 is activity led
				UseExpanderGPIO = true;								// Must use expander GPIO
			}
			else {													// These are Pi3B+ series (ARM8)
				ActivityGPIOPort = 29;								// GPIO port 29 is activity led
				UseExpanderGPIO = false;							// Don't use expander GPIO
			}
		} else return (false);										// Model check message failed
	}
	if (UseExpanderGPIO) {											// Activity LED uses expander GPIO
		return (mailbox_tag_message(0, 5, MAILBOX_TAG_SET_GPIO_STATE, 
			8, 8, ActivityGPIOPort, (uint32_t)on));					// Mailbox message,set GPIO port 130, on/off
	} else gpio_output(ActivityGPIOPort, on);						// Not using GPIO expander so use standard GPIO port
	return (true);													// Return success
}


/*==========================================================================}
{				     PUBLIC ARM CPU SPEED SET ROUTINES						}
{==========================================================================*/

/*-[ARM_setmaxspeed]--------------------------------------------------------}
. This will set the ARM cpu to the maximum. You can optionally print confirm
. message to screen or uart etc by providing a print function handler.
. The print handler can be screen or a uart/usb/eth handler for monitoring.
. RETURN: True maxium speed was successfully set, false otherwise
.--------------------------------------------------------------------------*/
bool ARM_setmaxspeed (int(*prn_handler) (const char *fmt, ...)) {
	uint32_t Buffer[5] = { 0 };
	if (mailbox_tag_message(&Buffer[0], 5, MAILBOX_TAG_GET_MAX_CLOCK_RATE, 8, 8, CLK_ARM_ID, 0))
		if (mailbox_tag_message(&Buffer[0], 5, MAILBOX_TAG_SET_CLOCK_RATE, 8, 8, CLK_ARM_ID, Buffer[4])) {
			if (prn_handler) prn_handler("CPU frequency set to %u Hz\n", Buffer[4]);
			return true;											// Return success
		}
	return false;													// Max speed set failed
}


/*==========================================================================}
{				      SMARTSTART DISPLAY ROUTINES							}
{==========================================================================*/

/*-[displaySmartStart]------------------------------------------------------}
. Will print 2 lines of basic smart start details to given print handler
. The print handler can be screen or a uart/usb/eth handler for monitoring.
.--------------------------------------------------------------------------*/
void displaySmartStart (int (*prn_handler) (const char *fmt, ...)) {
	if (prn_handler) {
		prn_handler("SmartStart v%x.%i%i, ARM%d AARCH%d code, CPU: %#03X, Cores: %u FPU: %s\n",
			(unsigned int)(RPi_SmartStartVer.HiVersion), (unsigned int)(RPi_SmartStartVer.LoVersion >> 8), (unsigned int)(RPi_SmartStartVer.LoVersion & 0xFF),
			(unsigned int)RPi_CompileMode.ArmCodeTarget, (unsigned int)RPi_CompileMode.AArchMode * 32 + 32,
			(unsigned int)RPi_CpuId.PartNumber, (unsigned int)RPi_CoresReady, (RPi_CompileMode.HardFloats == 1) ? "HARD" : "SOFT");
	}
}
