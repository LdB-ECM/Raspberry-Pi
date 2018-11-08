#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, uint64_t etc
#include <ctype.h>								// Needed for toupper for wildcard string match
#include <wchar.h>								// Needed for UTF for long file name support
#include <string.h>								// Needed for string copy
#include "rpi-smartstart.h"						// Provides all basic hardware access
#include "emb-stdio.h"							// Provides printf functionality
#include "SDCard.h"								// This units header

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: SDCard.c													}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 1.01														}
{       Original start code from hldswrth use from the Pi Forum             }
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
{***************************************************************************/

/*--------------------------------------------------------------------------}
{    These are SmartStart functions we use. If you wish to port this code   }
{  you will need to provide equivalent 3 functions on any new system.       }
{--------------------------------------------------------------------------*/
#define waitMicro timer_wait							// Waits a number of microseconds
#define TICKCOUNT timer_getTickCount					// Gets current system clock value (1Mhz accuracy)
#define TIMEDIFF  tick_difference						// Given two TICKCOUNT values calculates microseconds between them.
/* Smartstart also defines a function pointer ....  int (*printhandler) (const char *fmt, ...); */
printhandler LOG_ERROR = NULL;							// LOG_ERROR is a function pointer of that printhandler format

/*--------------------------------------------------------------------------}
{  This controls if debugging code is compiled or removed at compile time   }
{--------------------------------------------------------------------------*/
#define DEBUG_INFO 0									// Compile debugging message code .... set to 1 and other value means no compilation 

/*--------------------------------------------------------------------------}
{  The working part of the DEBUG_INFO macro to make compilation on or off   }
{--------------------------------------------------------------------------*/
#if DEBUG_INFO == 1
#define LOG_DEBUG(...) printf( __VA_ARGS__ )			// This displays debugging messages to function given
#else
#define LOG_DEBUG(...)									// This just swallows debug code, it doesn't even get compiled
#endif

/***************************************************************************}
{    PRIVATE INTERNAL SD HOST REGISTER STRUCTURES AS PER BCM2835 MANUAL     }
****************************************************************************/

/*--------------------------------------------------------------------------}
{      EMMC BLKSIZECNT register - BCM2835.PDF Manual Section 5 page 68      }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regBLKSIZECNT {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned BLKSIZE : 10;				// @0-9		Block size in bytes
			unsigned reserved : 6;						// @10-15	Write as zero read as don't care
			volatile unsigned BLKCNT : 16;				// @16-31	Number of blocks to be transferred 
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{      EMMC CMDTM register - BCM2835.PDF Manual Section 5 pages 69-70       }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regCMDTM {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			unsigned reserved : 1;						// @0		Write as zero read as don't care
			volatile unsigned TM_BLKCNT_EN : 1;			// @1		Enable the block counter for multiple block transfers
			volatile enum {	TM_NO_COMMAND = 0,			//			no command 
							TM_CMD12 = 1,				//			command CMD12 
							TM_CMD23 = 2,				//			command CMD23
							TM_RESERVED = 3,
						  } TM_AUTO_CMD_EN : 2;			// @2-3		Select the command to be send after completion of a data transfer
			volatile unsigned TM_DAT_DIR : 1;			// @4		Direction of data transfer (0 = host to card , 1 = card to host )
			volatile unsigned TM_MULTI_BLOCK : 1;		// @5		Type of data transfer (0 = single block, 1 = muli block)
			unsigned reserved1 : 10;					// @6-15	Write as zero read as don't care
			volatile enum {	CMD_NO_RESP = 0,			//			no response
							CMD_136BIT_RESP = 1,		//			136 bits response 
							CMD_48BIT_RESP = 2,			//			48 bits response 
							CMD_BUSY48BIT_RESP = 3,		//			48 bits response using busy 
						  } CMD_RSPNS_TYPE : 2;			// @16-17
			unsigned reserved2 : 1;						// @18		Write as zero read as don't care
			volatile unsigned CMD_CRCCHK_EN : 1;		// @19		Check the responses CRC (0=disabled, 1= enabled)
			volatile unsigned CMD_IXCHK_EN : 1;			// @20		Check that response has same index as command (0=disabled, 1= enabled)
			volatile unsigned CMD_ISDATA : 1;			// @21		Command involves data transfer (0=disabled, 1= enabled)
			volatile enum {	CMD_TYPE_NORMAL = 0,		//			normal command
							CMD_TYPE_SUSPEND = 1,		//			suspend command 
							CMD_TYPE_RESUME = 2,		//			resume command 
							CMD_TYPE_ABORT = 3,			//			abort command 
						  } CMD_TYPE : 2;				// @22-23 
			volatile unsigned CMD_INDEX : 6;			// @24-29
			unsigned reserved3 : 2;						// @30-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{        EMMC STATUS register - BCM2835.PDF Manual Section 5 page 72        }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regSTATUS {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned CMD_INHIBIT : 1;			// @0		Command line still used by previous command
			volatile unsigned DAT_INHIBIT : 1;			// @1		Data lines still used by previous data transfer
			volatile unsigned DAT_ACTIVE : 1;			// @2		At least one data line is active
			unsigned reserved : 5;						// @3-7		Write as zero read as don't care
			volatile unsigned WRITE_TRANSFER : 1;		// @8		New data can be written to EMMC
			volatile unsigned READ_TRANSFER : 1;		// @9		New data can be read from EMMC
			unsigned reserved1 : 10;					// @10-19	Write as zero read as don't care
			volatile unsigned DAT_LEVEL0 : 4;			// @20-23	Value of data lines DAT3 to DAT0
			volatile unsigned CMD_LEVEL : 1;			// @24		Value of command line CMD 
			volatile unsigned DAT_LEVEL1 : 4;			// @25-28	Value of data lines DAT7 to DAT4
			unsigned reserved2 : 3;						// @29-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{      EMMC CONTROL0 register - BCM2835.PDF Manual Section 5 page 73/74     }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regCONTROL0 {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			unsigned reserved : 1;						// @0		Write as zero read as don't care
			volatile unsigned HCTL_DWIDTH : 1;			// @1		Use 4 data lines (true = enable)
			volatile unsigned HCTL_HS_EN : 1;			// @2		Select high speed mode (true = enable)
			unsigned reserved1 : 2;						// @3-4		Write as zero read as don't care
			volatile unsigned HCTL_8BIT : 1;			// @5		Use 8 data lines (true = enable)
			unsigned reserved2 : 10;					// @6-15	Write as zero read as don't care
			volatile unsigned GAP_STOP : 1;				// @16		Stop the current transaction at the next block gap
			volatile unsigned GAP_RESTART : 1;			// @17		Restart a transaction last stopped using the GAP_STOP
			volatile unsigned READWAIT_EN : 1;			// @18		Use DAT2 read-wait protocol for cards supporting this
			volatile unsigned GAP_IEN : 1;				// @19		Enable SDIO interrupt at block gap 
			volatile unsigned SPI_MODE : 1;				// @20		SPI mode enable
			volatile unsigned BOOT_EN : 1;				// @21		Boot mode access
			volatile unsigned ALT_BOOT_EN : 1;			// @22		Enable alternate boot mode access
			unsigned reserved3 : 9;						// @23-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{      EMMC CONTROL1 register - BCM2835.PDF Manual Section 5 page 74/75     }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regCONTROL1 {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned CLK_INTLEN : 1;			// @0		Clock enable for internal EMMC clocks for power saving
			volatile const unsigned CLK_STABLE : 1;		// @1		SD clock stable  0=No 1=yes   **read only
			volatile unsigned CLK_EN : 1;				// @2		SD clock enable  0=disable 1=enable
			unsigned reserved : 2;						// @3-4		Write as zero read as don't care
			volatile unsigned CLK_GENSEL : 1;			// @5		Mode of clock generation (0=Divided, 1=Programmable)
			volatile unsigned CLK_FREQ_MS2 : 2;			// @6-7		SD clock base divider MSBs (Version3+ only)
			volatile unsigned CLK_FREQ8 : 8;			// @8-15	SD clock base divider LSBs
			volatile unsigned DATA_TOUNIT : 4;			// @16-19	Data timeout unit exponent
			unsigned reserved1 : 4;						// @20-23	Write as zero read as don't care
			volatile unsigned SRST_HC : 1;				// @24		Reset the complete host circuit
			volatile unsigned SRST_CMD : 1;				// @25		Reset the command handling circuit
			volatile unsigned SRST_DATA : 1;			// @26		Reset the data handling circuit
			unsigned reserved2 : 5;						// @27-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{    EMMC CONTROL2 register - BCM2835.PDF Manual Section 5 pages 81-84      }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regCONTROL2 {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile const unsigned ACNOX_ERR : 1;		// @0		Auto command not executed due to an error **read only
			volatile const unsigned ACTO_ERR : 1;		// @1		Timeout occurred during auto command execution **read only
			volatile const unsigned ACCRC_ERR : 1;		// @2		Command CRC error occurred during auto command execution **read only
			volatile const unsigned ACEND_ERR : 1;		// @3		End bit is not 1 during auto command execution **read only
			volatile const unsigned ACBAD_ERR : 1;		// @4		Command index error occurred during auto command execution **read only
			unsigned reserved : 2;						// @5-6		Write as zero read as don't care
			volatile const unsigned NOTC12_ERR : 1;		// @7		Error occurred during auto command CMD12 execution **read only
			unsigned reserved1 : 8;						// @8-15	Write as zero read as don't care			
			volatile enum { SDR12 = 0,
							SDR25 = 1, 
							SDR50 = 2,
							SDR104 = 3,
							DDR50 = 4,
						  } UHSMODE : 3;				// @16-18	Select the speed mode of the SD card (SDR12, SDR25 etc)
			unsigned reserved2 : 3;						// @19-21	Write as zero read as don't care
			volatile unsigned TUNEON : 1;				// @22		Start tuning the SD clock
			volatile unsigned TUNED : 1;				// @23		Tuned clock is used for sampling data
			unsigned reserved3 : 8;						// @24-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{     EMMC INTERRUPT register - BCM2835.PDF Manual Section 5 pages 75-77    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regINTERRUPT {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned CMD_DONE : 1;				// @0		Command has finished
			volatile unsigned DATA_DONE : 1;			// @1		Data transfer has finished
			volatile unsigned BLOCK_GAP : 1;			// @2		Data transfer has stopped at block gap
			unsigned reserved : 1;						// @3		Write as zero read as don't care
			volatile unsigned WRITE_RDY : 1;			// @4		Data can be written to DATA register
			volatile unsigned READ_RDY : 1;				// @5		DATA register contains data to be read
			unsigned reserved1 : 2;						// @6-7		Write as zero read as don't care
			volatile unsigned CARD : 1;					// @8		Card made interrupt request
			unsigned reserved2 : 3;						// @9-11	Write as zero read as don't care
			volatile unsigned RETUNE : 1;				// @12		Clock retune request was made
			volatile unsigned BOOTACK : 1;				// @13		Boot acknowledge has been received
			volatile unsigned ENDBOOT : 1;				// @14		Boot operation has terminated
			volatile unsigned ERR : 1;					// @15		An error has occured
			volatile unsigned CTO_ERR : 1;				// @16		Timeout on command line
			volatile unsigned CCRC_ERR : 1;				// @17		Command CRC error
			volatile unsigned CEND_ERR : 1;				// @18		End bit on command line not 1
			volatile unsigned CBAD_ERR : 1;				// @19		Incorrect command index in response
			volatile unsigned DTO_ERR : 1;				// @20		Timeout on data line
			volatile unsigned DCRC_ERR : 1;				// @21		Data CRC error
			volatile unsigned DEND_ERR : 1;				// @22		End bit on data line not 1
			unsigned reserved3 : 1;						// @23		Write as zero read as don't care
			volatile unsigned ACMD_ERR : 1;				// @24		Auto command error
			unsigned reserved4 : 7;						// @25-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{     EMMC IRPT_MASK register - BCM2835.PDF Manual Section 5 pages 77-79    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regIRPT_MASK {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned CMD_DONE : 1;				// @0		Command has finished
			volatile unsigned DATA_DONE : 1;			// @1		Data transfer has finished
			volatile unsigned BLOCK_GAP : 1;			// @2		Data transfer has stopped at block gap
			unsigned reserved : 1;						// @3		Write as zero read as don't care
			volatile unsigned WRITE_RDY : 1;			// @4		Data can be written to DATA register
			volatile unsigned READ_RDY : 1;				// @5		DATA register contains data to be read
			unsigned reserved1 : 2;						// @6-7		Write as zero read as don't care
			volatile unsigned CARD : 1;					// @8		Card made interrupt request
			unsigned reserved2 : 3;						// @9-11	Write as zero read as don't care
			volatile unsigned RETUNE : 1;				// @12		Clock retune request was made
			volatile unsigned BOOTACK : 1;				// @13		Boot acknowledge has been received
			volatile unsigned ENDBOOT : 1;				// @14		Boot operation has terminated
			volatile unsigned ERR : 1;					// @15		An error has occured
			volatile unsigned CTO_ERR : 1;				// @16		Timeout on command line
			volatile unsigned CCRC_ERR : 1;				// @17		Command CRC error
			volatile unsigned CEND_ERR : 1;				// @18		End bit on command line not 1
			volatile unsigned CBAD_ERR : 1;				// @19		Incorrect command index in response
			volatile unsigned DTO_ERR : 1;				// @20		Timeout on data line
			volatile unsigned DCRC_ERR : 1;				// @21		Data CRC error
			volatile unsigned DEND_ERR : 1;				// @22		End bit on data line not 1
			unsigned reserved3 : 1;						// @23		Write as zero read as don't care
			volatile unsigned ACMD_ERR : 1;				// @24		Auto command error
			unsigned reserved4 : 7;						// @25-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{      EMMC IRPT_EN register - BCM2835.PDF Manual Section 5 pages 79-71     }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regIRPT_EN {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned CMD_DONE : 1;				// @0		Command has finished
			volatile unsigned DATA_DONE : 1;			// @1		Data transfer has finished
			volatile unsigned BLOCK_GAP : 1;			// @2		Data transfer has stopped at block gap
			unsigned reserved : 1;						// @3		Write as zero read as don't care
			volatile unsigned WRITE_RDY : 1;			// @4		Data can be written to DATA register
			volatile unsigned READ_RDY : 1;				// @5		DATA register contains data to be read
			unsigned reserved1 : 2;						// @6-7		Write as zero read as don't care
			volatile unsigned CARD : 1;					// @8		Card made interrupt request
			unsigned reserved2 : 3;						// @9-11	Write as zero read as don't care
			volatile unsigned RETUNE : 1;				// @12		Clock retune request was made
			volatile unsigned BOOTACK : 1;				// @13		Boot acknowledge has been received
			volatile unsigned ENDBOOT : 1;				// @14		Boot operation has terminated
			volatile unsigned ERR : 1;					// @15		An error has occured
			volatile unsigned CTO_ERR : 1;				// @16		Timeout on command line
			volatile unsigned CCRC_ERR : 1;				// @17		Command CRC error
			volatile unsigned CEND_ERR : 1;				// @18		End bit on command line not 1
			volatile unsigned CBAD_ERR : 1;				// @19		Incorrect command index in response
			volatile unsigned DTO_ERR : 1;				// @20		Timeout on data line
			volatile unsigned DCRC_ERR : 1;				// @21		Data CRC error
			volatile unsigned DEND_ERR : 1;				// @22		End bit on data line not 1
			unsigned reserved3 : 1;						// @23		Write as zero read as don't care
			volatile unsigned ACMD_ERR : 1;				// @24		Auto command error
			unsigned reserved4 : 7;						// @25-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{       EMMC TUNE_STEP  register - BCM2835.PDF Manual Section 5 page 86     }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regTUNE_STEP {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile enum {	TUNE_DELAY_200ps  = 0,
							TUNE_DELAY_400ps  = 1,
							TUNE_DELAY_400psA = 2,		// I dont understand the duplicate value???
							TUNE_DELAY_600ps  = 3,
							TUNE_DELAY_700ps  = 4,
							TUNE_DELAY_900ps  = 5,
							TUNE_DELAY_900psA = 6,		// I dont understand the duplicate value??
							TUNE_DELAY_1100ps = 7,
						   } DELAY : 3;					// @0-2		Select the speed mode of the SD card (SDR12, SDR25 etc)
			unsigned reserved : 29;						// @3-31	Write as zero read as don't care
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};

/*--------------------------------------------------------------------------}
{    EMMC SLOTISR_VER register - BCM2835.PDF Manual Section 5 pages 87-88   }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regSLOTISR_VER {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned SLOT_STATUS : 8;			// @0-7		Logical OR of interrupt and wakeup signal for each slot
			unsigned reserved : 8;						// @8-15	Write as zero read as don't care
			volatile unsigned SDVERSION : 8;			// @16-23	Host Controller specification version 
			volatile unsigned VENDOR : 8;				// @24-31	Vendor Version Number 
		};
		volatile uint32_t Raw32;						// @0-31	Union to access all 32 bits as a uint32_t
	};
};


/***************************************************************************}
{         PRIVATE POINTERS TO ALL THE BCM2835 EMMC HOST REGISTERS           }
****************************************************************************/
#define EMMC_ARG2			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x300000))
#define EMMC_BLKSIZECNT		((volatile struct __attribute__((aligned(4))) regBLKSIZECNT*)(uintptr_t)(RPi_IO_Base_Addr + 0x300004))
#define EMMC_ARG1			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x300008))
#define EMMC_CMDTM			((volatile struct __attribute__((aligned(4))) regCMDTM*)(uintptr_t)(RPi_IO_Base_Addr + 0x30000c))
#define EMMC_RESP0			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x300010))
#define EMMC_RESP1			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x300014))
#define EMMC_RESP2			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x300018))
#define EMMC_RESP3			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x30001C))
#define EMMC_DATA			((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0x300020))
#define EMMC_STATUS			((volatile struct __attribute__((aligned(4))) regSTATUS*)(uintptr_t)(RPi_IO_Base_Addr + 0x300024))
#define EMMC_CONTROL0		((volatile struct __attribute__((aligned(4))) regCONTROL0*)(uintptr_t)(RPi_IO_Base_Addr + 0x300028))
#define EMMC_CONTROL1		((volatile struct __attribute__((aligned(4))) regCONTROL1*)(uintptr_t)(RPi_IO_Base_Addr + 0x30002C))
#define EMMC_INTERRUPT		((volatile struct __attribute__((aligned(4))) regINTERRUPT*)(uintptr_t)(RPi_IO_Base_Addr + 0x300030))
#define EMMC_IRPT_MASK		((volatile struct __attribute__((aligned(4))) regIRPT_MASK*)(uintptr_t)(RPi_IO_Base_Addr + 0x300034))
#define EMMC_IRPT_EN		((volatile struct __attribute__((aligned(4))) regIRPT_EN*)(uintptr_t)(RPi_IO_Base_Addr + 0x300038))
#define EMMC_CONTROL2		((volatile struct __attribute__((aligned(4))) regCONTROL2*)(uintptr_t)(RPi_IO_Base_Addr + 0x30003C))
#define EMMC_TUNE_STEP 		((volatile struct __attribute__((aligned(4))) regTUNE_STEP*)(uintptr_t)(RPi_IO_Base_Addr + 0x300088))
#define EMMC_SLOTISR_VER	((volatile struct __attribute__((aligned(4))) regSLOTISR_VER*)(uintptr_t)(RPi_IO_Base_Addr + 0x3000fC))


/***************************************************************************}
{   PRIVATE INTERNAL SD CARD REGISTER STRUCTURES AS PER SD CARD STANDARD    }
****************************************************************************/

/*--------------------------------------------------------------------------}
{						   SD CARD OCR register							    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regOCR {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			unsigned reserved : 15;						// @0-14	Write as zero read as don't care
			unsigned voltage2v7to2v8 : 1;				// @15		Voltage window 2.7v to 2.8v
			unsigned voltage2v8to2v9 : 1;				// @16		Voltage window 2.8v to 2.9v
			unsigned voltage2v9to3v0 : 1;				// @17		Voltage window 2.9v to 3.0v
			unsigned voltage3v0to3v1 : 1;				// @18		Voltage window 3.0v to 3.1v
			unsigned voltage3v1to3v2 : 1;				// @19		Voltage window 3.1v to 3.2v
			unsigned voltage3v2to3v3 : 1;				// @20		Voltage window 3.2v to 3.3v
			unsigned voltage3v3to3v4 : 1;				// @21		Voltage window 3.3v to 3.4v
			unsigned voltage3v4to3v5 : 1;				// @22		Voltage window 3.4v to 3.5v
			unsigned voltage3v5to3v6 : 1;				// @23		Voltage window 3.5v to 3.6v
			unsigned reserved1 : 6;						// @24-29	Write as zero read as don't care
			unsigned card_capacity : 1;					// @30		Card Capacity status
			unsigned card_power_up_busy : 1;			// @31		Card power up status (busy)
		};
		volatile uint32_t Raw32;						// @0-31	Union to access 32 bits as a uint32_t		
	};
};

/*--------------------------------------------------------------------------}
{						   SD CARD SCR register							    }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) regSCR {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile enum { SD_SPEC_1_101 = 0,			// ..enum..	Version 1.0-1.01 
							SD_SPEC_11 = 1,				// ..enum..	Version 1.10 
							SD_SPEC_2_3 = 2,			// ..enum..	Version 2.00 or Version 3.00 (check bit SD_SPEC3)
						  } SD_SPEC : 4;				// @0-3		SD Memory Card Physical Layer Specification version
			volatile enum {	SCR_VER_1 = 0,				// ..enum..	SCR version 1.0
						  } SCR_STRUCT : 4;				// @4-7		SCR structure version
			volatile enum { BUS_WIDTH_1 = 1,			// ..enum..	Card supports bus width 1
							BUS_WIDTH_4 = 4,			// ..enum.. Card supports bus width 4
						   } BUS_WIDTH : 4;				// @8-11	SD Bus width
			volatile enum { SD_SEC_NONE = 0,			// ..enum..	No Security
							SD_SEC_NOT_USED = 1,		// ..enum..	Security Not Used
							SD_SEC_101 = 2,				// ..enum..	SDSC Card (Security Version 1.01)
							SD_SEC_2 = 3,				// ..enum..	SDHC Card (Security Version 2.00)
							SD_SEC_3 = 4,				// ..enum..	SDXC Card (Security Version 3.xx)
						  } SD_SECURITY : 3;			// @12-14	Card security in use
			volatile unsigned DATA_AFTER_ERASE : 1;		// @15		Defines the data status after erase, whether it is 0 or 1
			unsigned reserved : 3;						// @16-18	Write as zero read as don't care
			volatile enum {	EX_SEC_NONE = 0,			// ..enum..	No extended Security
						  } EX_SECURITY : 4;			// @19-22	Extended security
			volatile unsigned SD_SPEC3 : 1;				// @23		Spec. Version 3.00 or higher
			volatile enum { CMD_SUPP_SPEED_CLASS = 1,
							CMD_SUPP_SET_BLKCNT = 2,
						   } CMD_SUPPORT : 2;			// @24-25	CMD support
			unsigned reserved1 : 6;						// @26-63	Write as zero read as don't care
		};
		volatile uint32_t Raw32_Lo;						// @0-31	Union to access low 32 bits as a uint32_t		
	};
	volatile uint32_t Raw32_Hi;							// @32-63	Access upper 32 bits as a uint32_t
};

/*--------------------------------------------------------------------------}
{						  PI SD CARD CID register						    }
{--------------------------------------------------------------------------*/
/* The CID is Big Endian and secondly the Pi butchers it by not having CRC */
/*  So the CID appears shifted 8 bits right with first 8 bits reading zero */
struct __attribute__((__packed__, aligned(4))) regCID {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile uint8_t OID_Lo;					
			volatile uint8_t OID_Hi;					// @0-15	Identifies the card OEM. The OID is assigned by the SD-3C, LLC
			volatile uint8_t MID;						// @16-23	Manufacturer ID, assigned by the SD-3C, LLC
			unsigned reserved : 8;						// @24-31	PI butcher with CRC removed these bits end up empty
		};
		volatile uint32_t Raw32_0;						// @0-31	Union to access 32 bits as a uint32_t		
	};
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile char ProdName4 : 8;				// @0-7		Product name character four
			volatile char ProdName3 : 8;				// @8-15	Product name character three
			volatile char ProdName2 : 8;				// @16-23	Product name character two
			volatile char ProdName1 : 8;				// @24-31	Product name character one	
		};
		volatile uint32_t Raw32_1;						// @0-31	Union to access 32 bits as a uint32_t		
	};
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned SerialNumHi : 16;			// @0-15	Serial number upper 16 bits
			volatile unsigned ProdRevLo : 4;			// @16-19	Product revision low value in BCD
			volatile unsigned ProdRevHi : 4;			// @20-23	Product revision high value in BCD
			volatile char ProdName5 : 8;				// @24-31	Product name character five
		};
		volatile uint32_t Raw32_2;						// @0-31	Union to access 32 bits as a uint32_t		
	};
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned ManufactureMonth : 4;		// @0-3		Manufacturing date month (1=Jan, 2=Feb, 3=Mar etc)
			volatile unsigned ManufactureYear : 8;		// @4-11	Manufacturing dateyear (offset from 2000 .. 1=2001,2=2002,3=2003 etc)
			unsigned reserved1 : 4;						// @12-15 	Write as zero read as don't care
			volatile unsigned SerialNumLo : 16;			// @16-23	Serial number lower 16 bits
		};
		volatile uint32_t Raw32_3;						// @0-31	Union to access 32 bits as a uint32_t		
	};
};

/***************************************************************************}
{			 PRIVATE INTERNAL SD CARD REGISTER STRUCTURE CHECKS             }
****************************************************************************/
/*--------------------------------------------------------------------------}
{					 INTERNAL STRUCTURE COMPILE TIME CHECKS		            }
{--------------------------------------------------------------------------*/
/* GIVEN THE AMOUNT OF PRECISE PACKING OF THESE STRUCTURES .. IT'S PRUDENT */
/* TO CHECK THEM AT COMPILE TIME. USE IS POINTLESS IF THE SIZES ARE WRONG. */
/*-------------------------------------------------------------------------*/
/* If you have never seen compile time assertions it's worth google search */
/* on "Compile Time Assertions". It is part of the C11++ specification and */
/* all compilers that support the standard will have them (GCC, MSC inc)   */
/* This actually produces no code it only executes as a compile time check */
/*-------------------------------------------------------------------------*/
#include <assert.h>								// Need for compile time static_assert

/* Check the main register section group sizes */
static_assert(sizeof(struct regBLKSIZECNT) == 0x04, "EMMC register BLKSIZECNT should be 0x04 bytes in size");
static_assert(sizeof(struct regCMDTM) == 0x04, "EMMC register CMDTM should be 0x04 bytes in size");
static_assert(sizeof(struct regSTATUS) == 0x04, "EMMC register STATUS should be 0x04 bytes in size");
static_assert(sizeof(struct regCONTROL0) == 0x04, "EMMC register CONTROL0 should be 0x04 bytes in size");
static_assert(sizeof(struct regCONTROL1) == 0x04, "EMMC register CONTROL1 should be 0x04 bytes in size");
static_assert(sizeof(struct regCONTROL2) == 0x04, "EMMC register CONTROL2 should be 0x04 bytes in size");
static_assert(sizeof(struct regINTERRUPT) == 0x04, "EMMC register INTERRUPT should be 0x04 bytes in size");
static_assert(sizeof(struct regIRPT_MASK) == 0x04, "EMMC register IRPT_MASK should be 0x04 bytes in size");
static_assert(sizeof(struct regIRPT_EN) == 0x04, "EMMC register IRPT_EN should be 0x04 bytes in size");
static_assert(sizeof(struct regTUNE_STEP) == 0x04, "EMMC register TUNE_STEP should be 0x04 bytes in size");
static_assert(sizeof(struct regSLOTISR_VER) == 0x04, "EMMC register SLOTISR_VER should be 0x04 bytes in size");

static_assert(sizeof(struct regOCR) == 0x04, "EMMC register OCR should be 0x04 bytes in size");
static_assert(sizeof(struct regSCR) == 0x08, "EMMC register SCR should be 0x08 bytes in size");
static_assert(sizeof(struct regCID) == 0x10, "EMMC register CID should be 0x10 bytes in size");

/*--------------------------------------------------------------------------}
{			  INTERRUPT REGISTER TURN TO MASK BIT DEFINITIONS			    }
{--------------------------------------------------------------------------*/
#define INT_AUTO_ERROR   0x01000000									// ACMD_ERR bit in register
#define INT_DATA_END_ERR 0x00400000									// DEND_ERR bit in register
#define INT_DATA_CRC_ERR 0x00200000									// DCRC_ERR bit in register
#define INT_DATA_TIMEOUT 0x00100000									// DTO_ERR bit in register
#define INT_INDEX_ERROR  0x00080000									// CBAD_ERR bit in register
#define INT_END_ERROR    0x00040000									// CEND_ERR bit in register
#define INT_CRC_ERROR    0x00020000									// CCRC_ERR bit in register
#define INT_CMD_TIMEOUT  0x00010000									// CTO_ERR bit in register
#define INT_ERR          0x00008000									// ERR bit in register
#define INT_ENDBOOT      0x00004000									// ENDBOOT bit in register
#define INT_BOOTACK      0x00002000									// BOOTACK bit in register
#define INT_RETUNE       0x00001000									// RETUNE bit in register
#define INT_CARD         0x00000100									// CARD bit in register
#define INT_READ_RDY     0x00000020									// READ_RDY bit in register
#define INT_WRITE_RDY    0x00000010									// WRITE_RDY bit in register
#define INT_BLOCK_GAP    0x00000004									// BLOCK_GAP bit in register
#define INT_DATA_DONE    0x00000002									// DATA_DONE bit in register
#define INT_CMD_DONE     0x00000001									// CMD_DONE bit in register
#define INT_ERROR_MASK   (INT_CRC_ERROR|INT_END_ERROR|INT_INDEX_ERROR| \
                          INT_DATA_TIMEOUT|INT_DATA_CRC_ERR|INT_DATA_END_ERR| \
                          INT_ERR|INT_AUTO_ERROR)
#define INT_ALL_MASK     (INT_CMD_DONE|INT_DATA_DONE|INT_READ_RDY|INT_WRITE_RDY|INT_ERROR_MASK)


/*--------------------------------------------------------------------------}
{						  SD CARD FREQUENCIES							    }
{--------------------------------------------------------------------------*/
#define FREQ_SETUP				400000  // 400 Khz
#define FREQ_NORMAL			  25000000  // 25 Mhz

/*--------------------------------------------------------------------------}
{						  CMD 41 BIT SELECTIONS							    }
{--------------------------------------------------------------------------*/
#define ACMD41_HCS           0x40000000
#define ACMD41_SDXC_POWER    0x10000000
#define ACMD41_S18R          0x04000000
#define ACMD41_VOLTAGE       0x00ff8000
/* PI DOES NOT SUPPORT VOLTAGE SWITCH */
#define ACMD41_ARG_HC        (ACMD41_HCS|ACMD41_SDXC_POWER|ACMD41_VOLTAGE)//(ACMD41_HCS|ACMD41_SDXC_POWER|ACMD41_VOLTAGE|ACMD41_S18R)
#define ACMD41_ARG_SC        (ACMD41_VOLTAGE)   //(ACMD41_VOLTAGE|ACMD41_S18R)

/*--------------------------------------------------------------------------}
{						   SD CARD COMMAND RECORD						    }
{--------------------------------------------------------------------------*/
typedef struct EMMCCommand
{
	const char cmd_name[16];
	struct regCMDTM code;
	struct __attribute__((__packed__)) {
		unsigned use_rca : 1;										// @0		Command uses rca										
		unsigned reserved : 15;										// @1-15	Write as zero read as don't care
		uint16_t delay;												// @16-31	Delay to apply after command
	};
} EMMCCommand;

/*--------------------------------------------------------------------------}
{						SD CARD COMMAND INDEX DEFINITIONS				    }
{--------------------------------------------------------------------------*/
#define IX_GO_IDLE_STATE    0
#define IX_ALL_SEND_CID     1
#define IX_SEND_REL_ADDR    2
#define IX_SET_DSR          3
#define IX_SWITCH_FUNC      4
#define IX_CARD_SELECT      5
#define IX_SEND_IF_COND     6
#define IX_SEND_CSD         7
#define IX_SEND_CID         8
#define IX_VOLTAGE_SWITCH   9
#define IX_STOP_TRANS       10
#define IX_SEND_STATUS      11
#define IX_GO_INACTIVE      12
#define IX_SET_BLOCKLEN     13
#define IX_READ_SINGLE      14
#define IX_READ_MULTI       15
#define IX_SEND_TUNING      16
#define IX_SPEED_CLASS      17
#define IX_SET_BLOCKCNT     18
#define IX_WRITE_SINGLE     19
#define IX_WRITE_MULTI      20
#define IX_PROGRAM_CSD      21
#define IX_SET_WRITE_PR     22
#define IX_CLR_WRITE_PR     23
#define IX_SND_WRITE_PR     24
#define IX_ERASE_WR_ST      25
#define IX_ERASE_WR_END     26
#define IX_ERASE            27
#define IX_LOCK_UNLOCK      28
#define IX_APP_CMD          29
#define IX_APP_CMD_RCA      30
#define IX_GEN_CMD          31

// Commands hereafter require APP_CMD.
#define IX_APP_CMD_START    32
#define IX_SET_BUS_WIDTH    32
#define IX_SD_STATUS        33
#define IX_SEND_NUM_WRBL    34
#define IX_SEND_NUM_ERS     35
#define IX_APP_SEND_OP_COND 36
#define IX_SET_CLR_DET      37
#define IX_SEND_SCR         38

/*--------------------------------------------------------------------------}
{							  SD CARD COMMAND TABLE						    }
{--------------------------------------------------------------------------*/
static EMMCCommand sdCommandTable[IX_SEND_SCR + 1] =  {
	[IX_GO_IDLE_STATE] =	{ "GO_IDLE_STATE", .code.CMD_INDEX = 0x00, .code.CMD_RSPNS_TYPE = CMD_NO_RESP        , .use_rca = 0 , .delay = 0},
	[IX_ALL_SEND_CID] =		{ "ALL_SEND_CID" , .code.CMD_INDEX = 0x02, .code.CMD_RSPNS_TYPE = CMD_136BIT_RESP    , .use_rca = 0 , .delay = 0},
	[IX_SEND_REL_ADDR] =	{ "SEND_REL_ADDR", .code.CMD_INDEX = 0x03, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_SET_DSR] =			{ "SET_DSR"      , .code.CMD_INDEX = 0x04, .code.CMD_RSPNS_TYPE = CMD_NO_RESP        , .use_rca = 0 , .delay = 0},
	[IX_SWITCH_FUNC] =		{ "SWITCH_FUNC"  , .code.CMD_INDEX = 0x06, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_CARD_SELECT] =		{ "CARD_SELECT"  , .code.CMD_INDEX = 0x07, .code.CMD_RSPNS_TYPE = CMD_BUSY48BIT_RESP , .use_rca = 1 , .delay = 0},
	[IX_SEND_IF_COND] = 	{ "SEND_IF_COND" , .code.CMD_INDEX = 0x08, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 100},
	[IX_SEND_CSD] =			{ "SEND_CSD"     , .code.CMD_INDEX = 0x09, .code.CMD_RSPNS_TYPE = CMD_136BIT_RESP    , .use_rca = 1 , .delay = 0},
	[IX_SEND_CID] =			{ "SEND_CID"     , .code.CMD_INDEX = 0x0A, .code.CMD_RSPNS_TYPE = CMD_136BIT_RESP    , .use_rca = 1 , .delay = 0},
	[IX_VOLTAGE_SWITCH] =	{ "VOLT_SWITCH"  , .code.CMD_INDEX = 0x0B, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_STOP_TRANS] =		{ "STOP_TRANS"   , .code.CMD_INDEX = 0x0C, .code.CMD_RSPNS_TYPE = CMD_BUSY48BIT_RESP , .use_rca = 0 , .delay = 0},
	[IX_SEND_STATUS] =		{ "SEND_STATUS"  , .code.CMD_INDEX = 0x0D, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 1 , .delay = 0},
	[IX_GO_INACTIVE] =		{ "GO_INACTIVE"  , .code.CMD_INDEX = 0x0F, .code.CMD_RSPNS_TYPE = CMD_NO_RESP        , .use_rca = 1 , .delay = 0},
	[IX_SET_BLOCKLEN] =		{ "SET_BLOCKLEN" , .code.CMD_INDEX = 0x10, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_READ_SINGLE] =		{ "READ_SINGLE"  , .code.CMD_INDEX = 0x11, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , 
	                                           .code.CMD_ISDATA = 1  , .code.TM_DAT_DIR = 1,					   .use_rca = 0 , .delay = 0},
	[IX_READ_MULTI] =		{ "READ_MULTI"   , .code.CMD_INDEX = 0x12, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , 
											   .code.CMD_ISDATA = 1 ,  .code.TM_DAT_DIR = 1,
											   .code.TM_BLKCNT_EN =1 , .code.TM_MULTI_BLOCK = 1,                   .use_rca = 0 , .delay = 0},
	[IX_SEND_TUNING] =		{ "SEND_TUNING"  , .code.CMD_INDEX = 0x13, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_SPEED_CLASS] =		{ "SPEED_CLASS"  , .code.CMD_INDEX = 0x14, .code.CMD_RSPNS_TYPE = CMD_BUSY48BIT_RESP , .use_rca = 0 , .delay = 0},
	[IX_SET_BLOCKCNT] =		{ "SET_BLOCKCNT" , .code.CMD_INDEX = 0x17, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_WRITE_SINGLE] =		{ "WRITE_SINGLE" , .code.CMD_INDEX = 0x18, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , 
											   .code.CMD_ISDATA = 1  ,											   .use_rca = 0 , .delay = 0},
	[IX_WRITE_MULTI] =		{ "WRITE_MULTI"  , .code.CMD_INDEX = 0x19, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , 
											   .code.CMD_ISDATA = 1  ,  
											   .code.TM_BLKCNT_EN = 1, .code.TM_MULTI_BLOCK = 1,				   .use_rca = 0 , .delay = 0},
	[IX_PROGRAM_CSD] =		{ "PROGRAM_CSD"  , .code.CMD_INDEX = 0x1B, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_SET_WRITE_PR] =		{ "SET_WRITE_PR" , .code.CMD_INDEX = 0x1C, .code.CMD_RSPNS_TYPE = CMD_BUSY48BIT_RESP , .use_rca = 0 , .delay = 0},
	[IX_CLR_WRITE_PR] =		{ "CLR_WRITE_PR" , .code.CMD_INDEX = 0x1D, .code.CMD_RSPNS_TYPE = CMD_BUSY48BIT_RESP , .use_rca = 0 , .delay = 0},
	[IX_SND_WRITE_PR] =		{ "SND_WRITE_PR" , .code.CMD_INDEX = 0x1E, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_ERASE_WR_ST] =		{ "ERASE_WR_ST"  , .code.CMD_INDEX = 0x20, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_ERASE_WR_END] =		{ "ERASE_WR_END" , .code.CMD_INDEX = 0x21, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_ERASE] =			{ "ERASE"        , .code.CMD_INDEX = 0x26, .code.CMD_RSPNS_TYPE = CMD_BUSY48BIT_RESP , .use_rca = 0 , .delay = 0},
	[IX_LOCK_UNLOCK] =		{ "LOCK_UNLOCK"  , .code.CMD_INDEX = 0x2A, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_APP_CMD] =			{ "APP_CMD"      , .code.CMD_INDEX = 0x37, .code.CMD_RSPNS_TYPE = CMD_NO_RESP        , .use_rca = 0 , .delay = 100},
	[IX_APP_CMD_RCA] =		{ "APP_CMD"      , .code.CMD_INDEX = 0x37, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 1 , .delay = 0},
	[IX_GEN_CMD] =			{ "GEN_CMD"      , .code.CMD_INDEX = 0x38, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},

	// APP commands must be prefixed by an APP_CMD.
	[IX_SET_BUS_WIDTH] =	{ "SET_BUS_WIDTH", .code.CMD_INDEX = 0x06, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_SD_STATUS] =		{ "SD_STATUS"    , .code.CMD_INDEX = 0x0D, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 1 , .delay = 0},
	[IX_SEND_NUM_WRBL] =	{ "SEND_NUM_WRBL", .code.CMD_INDEX = 0x16, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_SEND_NUM_ERS] =		{ "SEND_NUM_ERS" , .code.CMD_INDEX = 0x17, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_APP_SEND_OP_COND] =	{ "SD_SENDOPCOND", .code.CMD_INDEX = 0x29, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 1000},
	[IX_SET_CLR_DET] =		{ "SET_CLR_DET"  , .code.CMD_INDEX = 0x2A, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , .use_rca = 0 , .delay = 0},
	[IX_SEND_SCR] =			{ "SEND_SCR"     , .code.CMD_INDEX = 0x33, .code.CMD_RSPNS_TYPE = CMD_48BIT_RESP     , 
											   .code.CMD_ISDATA = 1  , .code.TM_DAT_DIR = 1,					   .use_rca = 0 , .delay = 0},
};

static const char* SD_TYPE_NAME[] = { "Unknown", "MMC", "Type 1", "Type 2 SC", "Type 2 HC" };

/*--------------------------------------------------------------------------}
{						  SD CARD DESCRIPTION RECORD					    }
{--------------------------------------------------------------------------*/
typedef struct SDDescriptor {
	struct regCID cid;							// Card cid
	struct CSD csd;								// Card csd
	struct regSCR scr;							// Card scr
	uint64_t CardCapacity;						// Card capacity expanded .. calculated from card details
	SDCARD_TYPE type;							// Card type
	uint32_t rca;								// Card rca
	struct regOCR ocr;							// Card ocr
	uint32_t status;							// Card last status

	EMMCCommand* lastCmd;

	struct {
		uint32_t rootCluster;					// Active partition rootCluster
		uint32_t sectorPerCluster;				// Active partition sectors per cluster
		uint32_t bytesPerSector;				// Active partition bytes per sector
		uint32_t firstDataSector;				// Active partition first data sector
		uint32_t dataSectors;					// Active partition data sectors
		uint32_t unusedSectors;					// Active partition unused sectors
		uint32_t reservedSectorCount;			// Active partition reserved sectors
	} partition;

	SFN_NAME partitionLabe1;					// Partition label
} SDDescriptor;

/*--------------------------------------------------------------------------}
{					    CURRENT SD CARD DATA STORAGE					    }
{--------------------------------------------------------------------------*/
static SDDescriptor sdCard = { 0 };


//**************************************************************************
// SD Card PUBLIC functions.
//**************************************************************************

static int sdDebugResponse( int resp ) 
{
	LOG_DEBUG("EMMC: Status: %08x, control1: %08x, interrupt: %08x\n", 
		(unsigned int)EMMC_STATUS->Raw32, (unsigned int)EMMC_CONTROL1->Raw32, 
		(unsigned int)EMMC_INTERRUPT->Raw32);
	LOG_DEBUG("EMMC: Command %s resp %08x: %08x %08x %08x %08x\n",
		sdCard.lastCmd->cmd_name, (unsigned int)resp,(unsigned int)*EMMC_RESP3, 
		(unsigned int)*EMMC_RESP2, (unsigned int)*EMMC_RESP1, 
		(unsigned int)*EMMC_RESP0);
	return resp;
}

/*-[INTERNAL: sdWaitForInterrupt]-------------------------------------------}
. Given an interrupt mask the routine loops polling for the condition for up
. to 1 second.
. RETURN: SD_TIMEOUT - the condition mask flags where not met in 1 second
.		  SD_ERROR - an identifiable error occurred
.		  SD_OK - the wait completed with a mask state as requested
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static SDRESULT sdWaitForInterrupt (uint32_t mask ) 
{
	uint64_t td = 0;												// Zero time difference
	uint64_t start_time = 0;										// Zero start time
	uint32_t tMask = mask | INT_ERROR_MASK;							// Add fatal error masks to mask provided
	while (!(EMMC_INTERRUPT->Raw32 & tMask) && (td < 1000000)) {
		if (!start_time) start_time = TICKCOUNT();					// If start time not set the set start time
			else td = TIMEDIFF(start_time, TICKCOUNT());			// Time difference between start time and now
	}
	uint32_t ival = EMMC_INTERRUPT->Raw32;							// Fetch all the interrupt flags
	if( td >= 1000000 ||											// No reponse timeout occurred
		(ival & INT_CMD_TIMEOUT) ||									// Command timeout occurred 
		(ival & INT_DATA_TIMEOUT) )									// Data timeout occurred
	{
		if (LOG_ERROR) LOG_ERROR("EMMC: Wait for interrupt %08x timeout: %08x %08x %08x\n", 
			(unsigned int)mask, (unsigned int)EMMC_STATUS->Raw32, 
			(unsigned int)ival, (unsigned int)*EMMC_RESP0);			// Log any error if requested

		// Clear the interrupt register completely.
		EMMC_INTERRUPT->Raw32 = ival;								// Clear any interrupt that occured

		return SD_TIMEOUT;											// Return SD_TIMEOUT
	} else if ( ival & INT_ERROR_MASK ) {
		if (LOG_ERROR) LOG_ERROR("EMMC: Error waiting for interrupt: %08x %08x %08x\n", 
			(unsigned int)EMMC_STATUS->Raw32, (unsigned int)ival, 
			(unsigned int)*EMMC_RESP0);								// Log any error if requested

		// Clear the interrupt register completely.
		EMMC_INTERRUPT->Raw32 = ival;								// Clear any interrupt that occured

		return SD_ERROR;											// Return SD_ERROR
    }

	// Clear the interrupt we were waiting for, leaving any other (non-error) interrupts.
	EMMC_INTERRUPT->Raw32 = mask;									// Clear any interrupt we are waiting on

	return SD_OK;													// Return SD_OK
}


/*-[INTERNAL: sdWaitForCommand]---------------------------------------------}
. Waits for up to 1 second for any command that may be in progress.
. RETURN: SD_BUSY - the command was not completed within 1 second period
.		  SD_OK - the wait completed sucessfully
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static SDRESULT sdWaitForCommand (void) 
{
	uint64_t td = 0;												// Zero time difference
	uint64_t start_time = 0;										// Zero start time
	while ((EMMC_STATUS->CMD_INHIBIT) &&							// Command inhibit signal
		  !(EMMC_INTERRUPT->Raw32 & INT_ERROR_MASK) &&				// No error occurred
		   (td < 1000000))											// Timeout not reached
	{
		if (!start_time) start_time = TICKCOUNT();					// Get start time
			else td = TIMEDIFF(start_time, TICKCOUNT());			// Time difference between start and now
	}
	if( (td >= 1000000) || (EMMC_INTERRUPT->Raw32 & INT_ERROR_MASK) )// Error occurred or it timed out
    {
		if (LOG_ERROR) LOG_ERROR("EMMC: Wait for command aborted: %08x %08x %08x\n", 
			(unsigned int)EMMC_STATUS->Raw32, (unsigned int)EMMC_INTERRUPT->Raw32, 
			(unsigned int)*EMMC_RESP0);								// Log any error if requested
		return SD_BUSY;												// return SD_BUSY
    }

	return SD_OK;													// return SD_OK
}


/*-[INTERNAL: sdWaitForData]------------------------------------------------}
. Waits for up to 1 second for any data transfer that may be in progress.
. RETURN: SD_BUSY - the transfer was not completed within 1 second period
.		  SD_OK - the transfer completed sucessfully
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static SDRESULT sdWaitForData (void) 
{
	uint64_t td = 0;												// Zero time difference
	uint64_t start_time = 0;										// Zero start time
	while ((EMMC_STATUS->DAT_INHIBIT) &&							// Data inhibit signal
		  !(EMMC_INTERRUPT->Raw32 & INT_ERROR_MASK) &&				// Some error occurred
		   (td < 500000))											// Timeout not reached
	{
		if (!start_time) start_time = TICKCOUNT();					// If start time not set the set start time
			else td = TIMEDIFF(start_time, TICKCOUNT());			// Time difference between start time and now
	}
	if ( (td >= 500000) || (EMMC_INTERRUPT->Raw32 & INT_ERROR_MASK) )
    {
		if (LOG_ERROR) LOG_ERROR("EMMC: Wait for data aborted: %08x %08x %08x\n", 
			(unsigned int)EMMC_STATUS->Raw32, (unsigned int)EMMC_INTERRUPT->Raw32, 
			(unsigned int)*EMMC_RESP0);								// Log any error if requested
		return SD_BUSY;												// return SD_BUSY
    }
	return SD_OK;													// return SD_OK
}


/*-[INTERNAL: unpack_csd]---------------------------------------------------}
. Unpacks a CSD in Resp0,1,2,3 into the proper CSD structure we defined.
. 16Aug17 LdB
.--------------------------------------------------------------------------*/
static void unpack_csd(struct CSD* csd)
{
	uint8_t buf[16] = { 0 };

	/* Fill buffer CSD comes IN MSB so I will invert so its sort of right way up so I can debug it */
	__attribute__((aligned(4))) uint32_t* p;
	p = (uint32_t*)&buf[12];
	*p = *EMMC_RESP0;
	p = (uint32_t*)&buf[8];
	*p = *EMMC_RESP1;
	p = (uint32_t*)&buf[4];
	*p = *EMMC_RESP2;
	p = (uint32_t*)&buf[0];
	*p = *EMMC_RESP3;

	/* Display raw CSD - values of my SANDISK ultra 16GB shown under each */
	LOG_DEBUG("CSD Contents : %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		buf[2], buf[1], buf[0], buf[7], buf[6], buf[5], buf[4],
		/*    40       e0      00     32        5b     59     00               */
		buf[11], buf[10], buf[9], buf[8], buf[15], buf[14], buf[13], buf[12]);
	/*    00       73       a7     7f       80       0a       40       00 */

	/* Populate CSD structure */
	csd->csd_structure = (buf[2] & 0xc0) >> 6;								// @126-127 ** correct 
	csd->spec_vers = buf[2] & 0x3F;											// @120-125 ** correct
	csd->taac = buf[1];														// @112-119 ** correct
	csd->nsac = buf[0];														// @104-111 ** correct
	csd->tran_speed = buf[7];												// @96-103  ** correct
	csd->ccc = (((uint16_t)buf[6]) << 4) | ((buf[5] & 0xf0) >> 4);			// @84-95   ** correct
	csd->read_bl_len = buf[5] & 0x0f;										// @80-83   ** correct
	csd->read_bl_partial = (buf[4] & 0x80) ? 1 : 0;							// @79		** correct
	csd->write_blk_misalign = (buf[4] & 0x40) ? 1 : 0;						// @78      ** correct
	csd->read_blk_misalign = (buf[4] & 0x20) ? 1 : 0;						// @77		** correct
	csd->dsr_imp = (buf[4] & 0x10) ? 1 : 0;									// @76		** correct

	if (csd->csd_structure == 0x1) {										// CSD VERSION 2.0 
		/* Basically absorbs bottom of buf[4] to align to next byte */		// @@75-70  ** Correct
		csd->ver2_c_size = (uint32_t)(buf[11] & 0x3F) << 16;				// @69-64
		csd->ver2_c_size += (uint32_t)buf[10] << 8;							// @63-56
		csd->ver2_c_size += (uint32_t)buf[9];								// @55-48
		sdCard.CardCapacity = csd->ver2_c_size;
		sdCard.CardCapacity *= (512 * 1024);								// Calculate Card capacity
	}
	else {																	// CSD VERSION 1.0
		csd->c_size = (uint32_t)(buf[4] & 0x03) << 8;
		csd->c_size += (uint32_t)buf[11];
		csd->c_size <<= 2;
		csd->c_size += (buf[10] & 0xc0) >> 6;								// @62-73
		csd->vdd_r_curr_min = (buf[10] & 0x38) >> 3;						// @59-61
		csd->vdd_r_curr_max = buf[10] & 0x07;								// @56-58
		csd->vdd_w_curr_min = (buf[9] & 0xe0) >> 5;							// @53-55
		csd->vdd_w_curr_max = (buf[9] & 0x1c) >> 2;							// @50-52	
		csd->c_size_mult = ((buf[9] & 0x03) << 1) | ((buf[8] & 0x80) >> 7);	// @47-49
		sdCard.CardCapacity = (csd->c_size + 1) * (1 << (csd->c_size_mult + 2)) * (1 << csd->read_bl_len);
	}

	csd->erase_blk_en = (buf[8] & 0x40) >> 6;								// @46
	csd->sector_size = ((buf[15] & 0x80) >> 1) | (buf[8] & 0x3F);			// @39-45
	csd->wp_grp_size = buf[15] & 0x7f;										// @32-38
	csd->wp_grp_enable = (buf[14] & 0x80) ? 1 : 0;							// @31  
	csd->default_ecc = (buf[14] & 0x60) >> 5;								// @29-30
	csd->r2w_factor = (buf[14] & 0x1c) >> 2;								// @26-28   ** correct
	csd->write_bl_len = ((buf[14] & 0x03) << 2) | ((buf[13] & 0xc0) >> 6);  // @22-25   **correct
	csd->write_bl_partial = (buf[13] & 0x20) ? 1 : 0;						// @21 
																			// @16-20 are reserved
	csd->file_format_grp = (buf[12] & 0x80) ? 1 : 0;						// @15
	csd->copy = (buf[12] & 0x40) ? 1 : 0;									// @14
	csd->perm_write_protect = (buf[12] & 0x20) ? 1 : 0;						// @13
	csd->tmp_write_protect = (buf[12] & 0x10) ? 1 : 0;						// @12
	csd->file_format = (buf[12] & 0x0c) >> 2;								// @10-11    **correct
	csd->ecc = buf[12] & 0x03;												// @8-9      **corrrect   

	LOG_DEBUG("  csd_structure=%d\t  spec_vers=%d\t  taac=%02x\t nsac=%02x\t  tran_speed=%02x\t  ccc=%04x\n"
		"  read_bl_len=%d\t  read_bl_partial=%d\t  write_blk_misalign=%d\t  read_blk_misalign=%d\n"
		"  dsr_imp=%d\t  sector_size =%d\t  erase_blk_en=%d\n",
		csd->csd_structure, csd->spec_vers, csd->taac, csd->nsac, csd->tran_speed, csd->ccc,
		csd->read_bl_len, csd->read_bl_partial, csd->write_blk_misalign, csd->read_blk_misalign,
		csd->dsr_imp, csd->sector_size, csd->erase_blk_en);

	if (csd->csd_structure == 0x1) {
		LOG_DEBUG("CSD 2.0: ver2_c_size = %d\t  card capacity: %lu\n",
			csd->ver2_c_size, sdCard.CardCapacity);
	}
	else {
		LOG_DEBUG("CSD 1.0: c_size = %d\t  c_size_mult=%d\t card capacity: %lu\n"
			"  vdd_r_curr_min = %d\t  vdd_r_curr_max=%d\t  vdd_w_curr_min = %d\t  vdd_w_curr_max=%d\n",
			csd->c_size, csd->c_size_mult, sdCard.CardCapacity,
			csd->vdd_r_curr_min, csd->vdd_r_curr_max, csd->vdd_w_curr_min, csd->vdd_w_curr_max);
	}

	LOG_DEBUG("  wp_grp_size=%d\t  wp_grp_enable=%d\t  default_ecc=%d\t  r2w_factor=%d\n"
		"  write_bl_len=%d\t  write_bl_partial=%d\t  file_format_grp=%d\t  copy=%d\n"
		"  perm_write_protect=%d\t  tmp_write_protect=%d\t  file_format=%d\t  ecc=%d\n",
		csd->wp_grp_size, csd->wp_grp_enable, csd->default_ecc, csd->r2w_factor,
		csd->write_bl_len, csd->write_bl_partial, csd->file_format_grp, csd->copy,
		csd->perm_write_protect, csd->tmp_write_protect, csd->file_format, csd->ecc);
}

/*-[INTERNAL: sdSendCommandP]-----------------------------------------------}
. Send command and handle response.
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
#define R1_ERRORS_MASK       0xfff9c004
static int sdSendCommandP( EMMCCommand* cmd, uint32_t arg )
{
	SDRESULT res;

	/* Check for command in progress */
	if ( sdWaitForCommand() != SD_OK ) return SD_BUSY;				// Check command wait

	LOG_DEBUG("EMMC: Sending command %s code %08x arg %08x\n",
		cmd->cmd_name, (unsigned int)cmd->code.CMD_INDEX, (unsigned int)arg);
	sdCard.lastCmd = cmd;

	/* Clear interrupt flags.  This is done by setting the ones that are currently set */
	EMMC_INTERRUPT->Raw32 = EMMC_INTERRUPT->Raw32;					// Clear interrupts

	/* Set the argument and the command code, Some commands require a delay before reading the response */
	*EMMC_ARG1 = arg;												// Set argument to SD card
	*EMMC_CMDTM = cmd->code;										// Send command to SD card								
	if ( cmd->delay ) waitMicro(cmd->delay);						// Wait for required delay

	/* Wait until command complete interrupt */
	if ( (res = sdWaitForInterrupt(INT_CMD_DONE))) return res;		// In non zero return result 

	/* Get response from RESP0 */
	uint32_t resp0 = *EMMC_RESP0;									// Fetch SD card response 0 to command

	/* Handle response types for command */
	switch ( cmd->code.CMD_RSPNS_TYPE) {
		// no response
		case CMD_NO_RESP:
			return SD_OK;											// Return okay then

		case CMD_BUSY48BIT_RESP:
			sdCard.status = resp0;
			// Store the card state.  Note that this is the state the card was in before the
			// command was accepted, not the new state.
			//sdCard.cardState = (resp0 & ST_CARD_STATE) >> R1_CARD_STATE_SHIFT;
			return resp0 & R1_ERRORS_MASK;

		// RESP0 contains card status, no other data from the RESP* registers.
		// Return value non-zero if any error flag in the status value.
		case CMD_48BIT_RESP:
			switch (cmd->code.CMD_INDEX) {
				case 0x03:											// SEND_REL_ADDR command
					// RESP0 contains RCA and status bits 23,22,19,12:0
					sdCard.rca = resp0 & 0xffff0000;				// RCA[31:16] of response
					sdCard.status = ((resp0 & 0x00001fff)) |		// 12:0 map directly to status 12:0
						((resp0 & 0x00002000) << 6) |				// 13 maps to status 19 ERROR
						((resp0 & 0x00004000) << 8) |				// 14 maps to status 22 ILLEGAL_COMMAND
						((resp0 & 0x00008000) << 8);				// 15 maps to status 23 COM_CRC_ERROR
					// Store the card state.  Note that this is the state the card was in before the
					// command was accepted, not the new state.
					// sdCard.cardState = (resp0 & ST_CARD_STATE) >> R1_CARD_STATE_SHIFT;
					return sdCard.status & R1_ERRORS_MASK;
				case 0x08:											// SEND_IF_COND command
					// RESP0 contains voltage acceptance and check pattern, which should match
					// the argument.
					sdCard.status = 0;
					return resp0 == arg ? SD_OK : SD_ERROR;
					// RESP0 contains OCR register
					// TODO: What is the correct time to wait for this?
				case 0x29:											// SD_SENDOPCOND command
					sdCard.status = 0;
					sdCard.ocr.Raw32 = resp0;
					return SD_OK;
				default:
					sdCard.status = resp0;
					// Store the card state.  Note that this is the state the card was in before the
					// command was accepted, not the new state.
					//sdCard.cardState = (resp0 & ST_CARD_STATE) >> R1_CARD_STATE_SHIFT;
					return resp0 & R1_ERRORS_MASK;
			}
		// RESP0..3 contains 128 bit CID or CSD shifted down by 8 bits as no CRC
		// Note: highest bits are in RESP3.
		case CMD_136BIT_RESP:		
			sdCard.status = 0;
			if (cmd->code.CMD_INDEX == 0x09) {
				unpack_csd(&sdCard.csd);
			} else {
				uint32_t* data = (uint32_t*)(uintptr_t)&sdCard.cid;
				data[3] = resp0;
				data[2] = *EMMC_RESP1;
				data[1] = *EMMC_RESP2;
				data[0] = *EMMC_RESP3;
			}
			return SD_OK;
    }

	return SD_ERROR;
}


/*-[INTERNAL: sdSendAppCommand]---------------------------------------------}
. Send APP command and handle response.
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
#define ST_APP_CMD           0x00000020
static SDRESULT sdSendAppCommand (void)
{
	SDRESULT resp;
	// If no RCA, send the APP_CMD and don't look for a response.
	if ( !sdCard.rca )
		sdSendCommandP(&sdCommandTable[IX_APP_CMD], 0x00000000);
	// If there is an RCA, include that in APP_CMD and check card accepted it.
	else {
		if( (resp = sdSendCommandP(&sdCommandTable[IX_APP_CMD_RCA], sdCard.rca)) ) 
			return sdDebugResponse(resp);
		// Debug - check that status indicates APP_CMD accepted.
		if( !(sdCard.status & ST_APP_CMD) ) return SD_ERROR;
	}
	return SD_OK;
}

 /*-[INTERNAL: sdSendCommand]------------------------------------------------}
 . Send a command with no argument. RCA automatically added if required.
 . APP_CMD sent automatically if required.
 . 10Aug17 LdB
 .--------------------------------------------------------------------------*/
static SDRESULT sdSendCommand ( int index )
{
	// Issue APP_CMD if needed.
	SDRESULT resp;
	if ( index >= IX_APP_CMD_START && (resp = sdSendAppCommand()) )
		return sdDebugResponse(resp);

	// Get the command and set RCA if required.
	EMMCCommand* cmd = &sdCommandTable[index];
	uint32_t arg = 0;
	if( cmd->use_rca == 1 ) arg = sdCard.rca;

	if( (resp = sdSendCommandP(cmd, arg)) ) return resp;

	// Check that APP_CMD was correctly interpreted.
	if( index >= IX_APP_CMD_START && sdCard.rca && !(sdCard.status & ST_APP_CMD) )
		return SD_ERROR_APP_CMD;

	return resp;
}

 /*-[INTERNAL: sdSendCommandA]-----------------------------------------------}
 . Send a command with argument. APP_CMD sent automatically if required.
 . APP_CMD sent automatically if required.
 . 10Aug17 LdB
 .--------------------------------------------------------------------------*/
static SDRESULT sdSendCommandA ( int index, uint32_t arg )
{
	// Issue APP_CMD if needed.
	SDRESULT resp;
	if( index >= IX_APP_CMD_START && (resp = sdSendAppCommand()) )
		return sdDebugResponse(resp);

	// Get the command and pass the argument through.
	if( (resp = sdSendCommandP(&sdCommandTable[index],arg)) ) return resp;

	// Check that APP_CMD was correctly interpreted.
	if( index >= IX_APP_CMD_START && sdCard.rca && !(sdCard.status & ST_APP_CMD) )
		return SD_ERROR_APP_CMD;

	return resp;
}


/*-[INTERNAL: sdReadSCR]----------------------------------------------------}
. Read card's SCR
. APP_CMD sent automatically if required.
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static SDRESULT sdReadSCR (void)
{
	// SEND_SCR command is like a READ_SINGLE but for a block of 8 bytes.
	// Ensure that any data operation has completed before reading the block.
	if( sdWaitForData() ) return SD_TIMEOUT;

	// Set BLKSIZECNT to 1 block of 8 bytes, send SEND_SCR command
	EMMC_BLKSIZECNT->BLKCNT = 1;
	EMMC_BLKSIZECNT->BLKSIZE = 8;
	int resp;
	if( (resp = sdSendCommand(IX_SEND_SCR)) ) return sdDebugResponse(resp);

	// Wait for READ_RDY interrupt.
	if( (resp = sdWaitForInterrupt(INT_READ_RDY)) )
	{
		if (LOG_ERROR) LOG_ERROR("EMMC: Timeout waiting for ready to read\n");
		return sdDebugResponse(resp);
	}

	// Allow maximum of 100ms for the read operation.
	int numRead = 0, count = 100000;
	while( numRead < 2 )  {
		if (EMMC_STATUS->READ_TRANSFER) {
			//sdCard.scr[numRead++] = *EMMC_DATA;
			if (numRead == 0) sdCard.scr.Raw32_Lo = *EMMC_DATA;
				else sdCard.scr.Raw32_Hi = *EMMC_DATA;
			numRead++;
		} else {
			waitMicro(1);
			if( --count == 0 ) break;
		}
	}

	// If SCR not fully read, the operation timed out.
	if( numRead != 2 )
	{
		if (LOG_ERROR) {
			LOG_ERROR("EMMC: SEND_SCR ERR: %08x %08x %08x\n", 
				(unsigned int)EMMC_STATUS->Raw32, 
				(unsigned int)EMMC_INTERRUPT->Raw32, 
				(unsigned int)*EMMC_RESP0);
			LOG_ERROR("EMMC: Reading SCR, only read %d words\n", numRead);
		}
		return SD_TIMEOUT;
	}

	return SD_OK;
}


/*-[INTERNAL: fls_uint32_t]-------------------------------------------------}
. Find Last Set bit in given uint32_t value. That is find the bit index of 
. the MSB that is set in the value. 
. RETURN: 0 - 32 are only possible answers given uint32_t is 32 bits.
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static uint_fast8_t fls_uint32_t (uint32_t x) 
{
	uint_fast8_t r = 32;											// Start at 32
	if (!x)  return 0;												// If x is zero answer must be zero
	if (!(x & 0xffff0000u)) {										// If none of the upper word bits are set
		x <<= 16;													// We can roll it up 16 bits
		r -= 16;													// Reduce r by 16
	}
	if (!(x & 0xff000000u)) {										// If none of uppermost byte bits are set
		x <<= 8;													// We can roll it up by 8 bits
		r -= 8;														// Reduce r by 8
	}
	if (!(x & 0xf0000000u)) {										// If none of the uppermost 4 bits are set
		x <<= 4;													// We can roll it up by 4 bits
		r -= 4;														// Reduce r by 4
	}
	if (!(x & 0xc0000000u)) {										// If none of the uppermost 2 bits are set
		x <<= 2;													// We can roll it up by 2 bits
		r -= 2;														// Reduce r by 2
	}
	if (!(x & 0x80000000u)) {										// If the uppermost bit is not set
		x <<= 1;													// We can roll it up by 1 bit
		r -= 1;														// Reduce r by 1
	}
	return r;														// Return the number of the uppermost set bit
}

/*-[INTERNAL: sdGetClockDivider]--------------------------------------------}
. Get clock divider for the given requested frequency. This is calculated 
. relative to the SD base clock of 41.66667Mhz
. RETURN: 3 - 0x3FF are only possible answers for the divisor
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static uint32_t sdGetClockDivider (uint32_t freq) 
{
	uint32_t divisor = (41666667 + freq - 1) / freq;				// Pi SD frequency is always 41.66667Mhz on baremetal
	if (divisor > 0x3FF) divisor = 0x3FF;							// Constrain divisor to max 0x3FF
	if (EMMC_SLOTISR_VER->SDVERSION < 2) {							// Any version less than HOST SPECIFICATION 3 (Aka numeric 2)						
		uint_fast8_t shiftcount = fls_uint32_t(divisor);			// Only 8 bits and set pwr2 div on Hosts specs 1 & 2
		if (shiftcount > 0) shiftcount--;							// Note the offset of shift by 1 (look at the spec)
		if (shiftcount > 7) shiftcount = 7;							// It's only 8 bits maximum on HOST_SPEC_V2
		divisor = ((uint32_t)1 << shiftcount);						// Version 1,2 take power 2
	} else if (divisor < 3) divisor = 4;							// Set minimum divisor limit
	LOG_DEBUG("Divisor = %i, Freq Set = %i\n", (int)divisor, (int)(41666667/divisor));
	return divisor;													// Return divisor that would be required
}

/*-[INTERNAL: sdGetClockDivider]--------------------------------------------}
. Set the SD clock to the given frequency from the 41.66667Mhz base clock
. RETURN: SD_ERROR_CLOCK - A fatal error occurred setting the clock
.		  SD_OK - the clock change to given frequency
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static SDRESULT sdSetClock (uint32_t freq)
{
	uint64_t td = 0;												// Zero time difference
	uint64_t start_time = 0;										// Zero start time
	LOG_DEBUG("EMMC: setting clock speed to %i.\n", (unsigned int)freq);
	while ((EMMC_STATUS->CMD_INHIBIT || EMMC_STATUS->DAT_INHIBIT)	// Data inhibit signal
  		   && (td < 100000))										// Timeout not reached
	{
		if (!start_time) start_time = TICKCOUNT();					// If start time not set the set start time
			else td = TIMEDIFF(start_time, TICKCOUNT());			// Time difference between start time and now
	}
	if (td >= 100000) {												// Timeout waiting for inghibit flags
		if (LOG_ERROR) LOG_ERROR("EMMC: Set clock: timeout waiting for inhibit flags. Status %08x.\n",
			(unsigned int)EMMC_STATUS->Raw32);
		return SD_ERROR_CLOCK;										// Return clock error
	}

	/* Switch clock off */
	EMMC_CONTROL1->CLK_EN = 0;										// Disable clock
	waitMicro(10);													// We must now wait 10 microseconds

	/* Request the divisor for new clock setting */
	uint_fast32_t cdiv = sdGetClockDivider(freq);					// Fetch divisor for new frequency
	uint_fast32_t divlo = (cdiv & 0xff) << 8;						// Create divisor low bits value
	uint_fast32_t divhi = ((cdiv & 0x300) >> 2);					// Create divisor high bits value

	/* Set new clock frequency by setting new divisor */
	EMMC_CONTROL1->Raw32 = (EMMC_CONTROL1->Raw32 & 0xffff001f) | divlo | divhi;
	waitMicro(10);													// We must now wait 10 microseconds

	/* Enable the clock. */
	EMMC_CONTROL1->CLK_EN = 1;										// Enable the clock

	/* Wait for clock to be stablize */
	td = 0;															// Zero time difference
	start_time = 0;													// Zero start time
	while (!(EMMC_CONTROL1->CLK_STABLE)								// Clock not stable yet
		&& (td < 100000))											// Timeout not reached
	{
		if (!start_time) start_time = TICKCOUNT();					// If start time not set the set start time
			else td = TIMEDIFF(start_time, TICKCOUNT());			// Time difference between start time and now
	}
	if (td >= 100000) {												// Timeout waiting for stability flag
		if (LOG_ERROR) LOG_ERROR("EMMC: ERROR: failed to get stable clock.\n");
		return SD_ERROR_CLOCK;										// Return clock error
	}
	return SD_OK;													// Clock frequency set worked
}

/*-[INTERNAL: sdResetCard]--------------------------------------------------}
. Reset the SD Card
. RETURN: SD_ERROR_RESET - A fatal error occurred resetting the SD Card
.		  SD_OK - SD Card reset correctly
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static SDRESULT sdResetCard (void)
{
	SDRESULT resp;
	uint64_t td = 0;												// Zero time difference
	uint64_t start_time = 0;										// Zero start time


	/* Send reset host controller and wait for complete */
	EMMC_CONTROL0->Raw32 = 0;										// Zero control0 register
	EMMC_CONTROL1->Raw32 = 0;										// Zero control1 register
	EMMC_CONTROL1->SRST_HC = 1;										// Reset the complete host circuit
  	//EMMC_CONTROL2->UHSMODE = SDR12;
	waitMicro(10);													// Wait 10 microseconds
	LOG_DEBUG("EMMC: reset card.\n");
	while ((EMMC_CONTROL1->SRST_HC)									// Host circuit reset not clear
			&& (td < 100000))										// Timeout not reached
	{
		if (!start_time) start_time = TICKCOUNT();					// If start time not set the set start time
			else td = TIMEDIFF(start_time, TICKCOUNT());			// Time difference between start time and now
	}
	if (td >= 100000) {												// Timeout waiting for reset flag
		if (LOG_ERROR) LOG_ERROR("EMMC: ERROR: failed to reset.\n");
		return SD_ERROR_RESET;										// Return reset SD Card error
    }

	/* Enable internal clock and set data timeout */
	EMMC_CONTROL1->DATA_TOUNIT = 0xE;								// Maximum timeout value
	EMMC_CONTROL1->CLK_INTLEN = 1;									// Enable internal clock
	waitMicro(10);													// Wait 10 microseconds

	/* Set clock to setup frequency */
	if ( (resp = sdSetClock(FREQ_SETUP)) ) return resp;				// Set low speed setup frequency (400Khz)

	/* Enable interrupts for command completion values */
	EMMC_IRPT_EN->Raw32   = 0xffffffff;
	EMMC_IRPT_MASK->Raw32 = 0xffffffff;

	/* Reset our card structure entries */
	sdCard.rca = 0;													// Zero rca
	sdCard.ocr.Raw32 = 0;											// Zero ocr
	sdCard.lastCmd = 0;												// Zero lastCmd
	sdCard.status = 0;												// Zero status
	sdCard.type = SD_TYPE_UNKNOWN;									// Set card type unknown

	/* Send GO_IDLE_STATE to card */
	resp = sdSendCommand(IX_GO_IDLE_STATE);							// Send GO idle state

	return resp;													// Return response
}

 /*-[INTERNAL: sdAppSendOpCond]---------------------------------------------}
 . Common routine for APP_SEND_OP_COND
 . This is used for both SC and HC cards based on the parameter
 . 10Aug17 LdB
 .-------------------------------------------------------------------------*/
static SDRESULT sdAppSendOpCond (uint32_t arg )
{
	// Send APP_SEND_OP_COND with the given argument (for SC or HC cards).
	// Note: The host shall set ACMD41 timeout more than 1 second to abort repeat of issuing ACMD41
	SDRESULT  resp;
	if( (resp = sdSendCommandA(IX_APP_SEND_OP_COND,arg)) && resp != SD_TIMEOUT )
    {
		if (LOG_ERROR) LOG_ERROR("EMMC: ACMD41 returned non-timeout error %d\n",resp);
			return resp;
    }
	int count = 6;
	while( (sdCard.ocr.card_power_up_busy == 0) && count-- )
	{
		//scPrintf("EMMC: Retrying ACMD SEND_OP_COND status %08x\n",*EMMC_STATUS);
		waitMicro(400000);
		if( (resp = sdSendCommandA(IX_APP_SEND_OP_COND,arg)) && resp != SD_TIMEOUT )
		{
			if (LOG_ERROR) LOG_ERROR("EMMC: ACMD41 returned non-timeout error %d\n",resp);
			return resp;
		}
	}

	// Return timeout error if still not busy.
	if( sdCard.ocr.card_power_up_busy == 0 )
	return SD_TIMEOUT;

	// Pi is 3.3v SD only so check that one voltage values around 3.3v was returned.
	if(  sdCard.ocr.voltage3v2to3v3 == 0  && sdCard.ocr.voltage3v3to3v4 == 0)
	return SD_ERROR_VOLTAGE;

	return SD_OK;
}

/*-[sdTransferBlocks]-------------------------------------------------------}
. Transfer the count blocks starting at given block to/from SD Card.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
SDRESULT sdTransferBlocks (uint32_t startBlock, uint32_t numBlocks, uint8_t* buffer, bool write )
{
	if ( sdCard.type == SD_TYPE_UNKNOWN ) return SD_NO_RESP;		// If card not known return error
	if ( sdWaitForData() ) return SD_TIMEOUT;						// Ensure any data operation has completed before doing the transfer.

	// Work out the status, interrupt and command values for the transfer.
	int readyInt = write ? INT_WRITE_RDY : INT_READ_RDY;

	int transferCmd = write ? ( numBlocks == 1 ? IX_WRITE_SINGLE : IX_WRITE_MULTI) :
							( numBlocks == 1 ? IX_READ_SINGLE : IX_READ_MULTI);

	// If more than one block to transfer, and the card supports it,
	// send SET_BLOCK_COUNT command to indicate the number of blocks to transfer.
	SDRESULT resp;
	if ( numBlocks > 1 &&
		(sdCard.scr.CMD_SUPPORT == CMD_SUPP_SET_BLKCNT) &&
		(resp = sdSendCommandA(IX_SET_BLOCKCNT, numBlocks)) ) return sdDebugResponse(resp);

	// Address is different depending on the card type.
	// HC pass address as block # so just pass it thru.
	// SC pass address so need to multiply by 512 which is shift left 9.
	uint32_t blockAddress = sdCard.type == SD_TYPE_2_SC ? (uint32_t)(startBlock << 9) : (uint32_t)startBlock;

	// Set BLKSIZECNT to number of blocks * 512 bytes, send the read or write command.
	// Once the data transfer has started and the TM_BLKCNT_EN bit in the CMDTM register is
	// set the EMMC module automatically decreases the BLKCNT value as the data blocks
	// are transferred and stops the transfer once BLKCNT reaches 0.
	// TODO: TM_AUTO_CMD12 - is this needed?  What effect does it have?
	EMMC_BLKSIZECNT->BLKCNT = numBlocks;
	EMMC_BLKSIZECNT->BLKSIZE = 512;
	if ((resp = sdSendCommandA(transferCmd, blockAddress))) {
		return sdDebugResponse(resp);
	}

	// Transfer all blocks.
	uint_fast32_t blocksDone = 0;
	while ( blocksDone < numBlocks )
    {
		// Wait for ready interrupt for the next block.
		if( (resp = sdWaitForInterrupt(readyInt)) )
		{
			if (LOG_ERROR) LOG_ERROR("EMMC: Timeout waiting for ready to read\n");
			return sdDebugResponse(resp);
		}

		// Handle non-word-aligned buffers byte-by-byte.
		// Note: the entire block is sent without looking at status registers.
		if ((uintptr_t)buffer & 0x03) {
			for (uint_fast16_t i = 0; i < 512; i++ ) {
				if ( write ) {
					uint32_t data = (buffer[i]      );
					data |=    (buffer[i+1] << 8 );
					data |=    (buffer[i+2] << 16);
					data |=    (buffer[i+3] << 24);
					*EMMC_DATA = data;
				} else {
					uint32_t data = *EMMC_DATA;
					buffer[i] =   (data      ) & 0xff;
					buffer[i+1] = (data >> 8 ) & 0xff;
					buffer[i+2] = (data >> 16) & 0xff;
					buffer[i+3] = (data >> 24) & 0xff;
				}
			}
		}
	    // Handle word-aligned buffers more efficiently.
		// Hopefully people smart enough to privide aligned data buffer
		else {
			uint32_t* intbuff = (uint32_t*)buffer;
			for (uint_fast16_t i = 0; i < 128; i++ ) {
				if ( write ) *EMMC_DATA = intbuff[i];
					else intbuff[i] = *EMMC_DATA;
			}
		}

		blocksDone++;
		buffer += 512;
	}

	// If not all bytes were read, the operation timed out.
	if( blocksDone != numBlocks ) {
		if (LOG_ERROR) LOG_ERROR("EMMC: Transfer error only done %d/%d blocks\n",blocksDone,numBlocks);
		LOG_DEBUG("EMMC: Transfer: %08x %08x %08x %08x\n", (unsigned int)EMMC_STATUS->Raw32, 
			(unsigned int)EMMC_INTERRUPT->Raw32, (unsigned int)*EMMC_RESP0, 
			(unsigned int)EMMC_BLKSIZECNT->Raw32);
		if( !write && numBlocks > 1 && (resp = sdSendCommand(IX_STOP_TRANS)) )
			LOG_DEBUG("EMMC: Error response from stop transmission: %d\n",resp);

		return SD_TIMEOUT;
    }

	// For a write operation, ensure DATA_DONE interrupt before we stop transmission.
	if( write && (resp = sdWaitForInterrupt(INT_DATA_DONE)) )
	{
		if (LOG_ERROR) LOG_ERROR("EMMC: Timeout waiting for data done\n");
		return sdDebugResponse(resp);
	}

	// For a multi-block operation, if SET_BLOCKCNT is not supported, we need to indicate
	// that there are no more blocks to be transferred.
	if( (numBlocks > 1) && (sdCard.scr.CMD_SUPPORT != CMD_SUPP_SET_BLKCNT) &&
		(resp = sdSendCommand(IX_STOP_TRANS)) ) return sdDebugResponse(resp);

	return SD_OK;
}

/*-[sdClearBlocks]----------------------------------------------------------}
. Clears the count blocks starting at given block from SD Card.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
SDRESULT sdClearBlocks(uint32_t startBlock , uint32_t numBlocks)
{
	if (sdCard.type == SD_TYPE_UNKNOWN) return SD_NO_RESP;

	// Ensure that any data operation has completed before doing the transfer.
	if ( sdWaitForData() ) return SD_TIMEOUT;

	// Address is different depending on the card type.
	// HC pass address as block # which is just address/512.
	// SC pass address straight through.
	uint32_t startAddress = sdCard.type == SD_TYPE_2_SC ? (uint32_t)(startBlock << 9) : (uint32_t)startBlock;
	uint32_t endAddress = sdCard.type == SD_TYPE_2_SC ? (uint32_t)( (startBlock +numBlocks) << 9) : (uint32_t)(startBlock + numBlocks);
	SDRESULT resp;
	LOG_DEBUG("EMMC: erasing blocks from %d to %d\n", startAddress, endAddress);
	if ( (resp = sdSendCommandA(IX_ERASE_WR_ST,startAddress)) ) return sdDebugResponse(resp);
	if ( (resp = sdSendCommandA(IX_ERASE_WR_END,endAddress)) ) return sdDebugResponse(resp);
	if ( (resp = sdSendCommand(IX_ERASE)) ) return sdDebugResponse(resp);

	// Wait for data inhibit status to drop.
	int count = 1000000;
	while( EMMC_STATUS->DAT_INHIBIT )
	{
	if ( --count == 0 )
		{
		if (LOG_ERROR) LOG_ERROR("EMMC: Timeout waiting for erase: %08x %08x\n", 
			(unsigned int)EMMC_STATUS->Raw32, (unsigned int)EMMC_INTERRUPT->Raw32);
		return SD_TIMEOUT;
		}

		waitMicro(10);
	}

	//scPrintf("EMMC: completed erase command int %08x\n",*EMMC_INTERRUPT);

	return SD_OK;
}

/*==========================================================================}
{				      FAT32 STRUCTURES AND ROUTINES							}
{==========================================================================*/

/*--------------------------------------------------------------------------}
{                        FAT12/16 SPECIFIC STRUCTURE		 	            }
{--------------------------------------------------------------------------*/
// Offset 36 into the BPB, structure for a FAT16/12 table entry varies this defines it
struct __attribute__((packed, aligned(1))) BPBFAT1612_struct {
	uint8_t			BS_DriveNumber;			// 36       ( As used in INT 13)
	uint8_t			BS_Reserved1;           // 37
	uint8_t			BS_BootSig;             // 38		(0x29)
	uint32_t		BS_VolumeID;            // 42
	uint8_t			BS_VolumeLabel[11];     // 43-53	(Label or  "NO NAME    ")
	char			BS_FileSystemType[8];   // 54-61    ("FAT16   ", "FAT     ", or all zero.)
};

/*--------------------------------------------------------------------------}
{                         FAT32 SPECIFIC STRUCTURE				            }
{--------------------------------------------------------------------------*/
// Offset 36 into the BPB, structure for a FAT32 table entry varies this defines it
struct __attribute__((packed, aligned(1))) BPBFAT32_struct {
	uint32_t		FATSize32;				// 36-39   (Number of sectors per FAT on FAT32)
	uint16_t		ExtFlags;				// 40-41   (Mirror flags Bits 0-3: number of active FAT (if bit 7 is 1) Bit 7: 1 = single active FAT; zero: all FATs are updated at runtime; Bits 4-6 & 8-15 : reserved)
	uint16_t		FSVersion;				// 42-43
	uint32_t		RootCluster;			// 44-47   (usually 2)
	uint16_t		FSInfo;					// 48-49   (usually 1)
	uint16_t		BkBootSec;				// 50-51   (usually 6)
	uint8_t			Reserved[12];			// 52-63
	uint8_t			BS_DriveNumber;			// 64
	uint8_t			BS_Reserved1;           // 65
	uint8_t		    BS_BootSig;				// 66      (0x29: extend signature .. Early FAT32 stop here ) 
	uint32_t		BS_VolumeID;			// 67-70
	char			BS_VolumeLabel[11];     // 71-81   (Label or  "NO NAME    ")
	char			BS_FileSystemType[8];   // 82-89   ("FAT32   ", "FAT     ", or all zero.)
};

/*--------------------------------------------------------------------------}
{          FAT12/16/32 UNIONIZED BIOS PARAMETER BLOCK STRUCTURE		        }
{--------------------------------------------------------------------------*/
// Full unionized BPB struct valid for 12/16/32 bit
struct __attribute__((packed, aligned(4))) FAT_BPB_struct {			// OFFSET LOCATIONS
	uint8_t			BS_JmpBoot[3];			// 0-2   (eb xx 90, or e9 xx xx)
	char			BS_OEMName[8];			// 3-10
	uint16_t		BytesPerSector;			// 11-12 (valid 512, 1024, 2048, 4096)
	uint8_t			SectorsPerCluster;		// 13    (valid 1, 2, 4, 8, 16, 32, 64, 128, 32768, 65536)
	uint16_t		ReservedSectorCount;	// 14-15 
	uint8_t			NumFATs;				// 16    (2)
	uint16_t		RootEntryCount;			// 17-18 (224 = FAT12, 512 = FAT16, 0 = FAT 32**)
	uint16_t		TotalSectors16;			// 19-20
	uint8_t			Media;					// 21    (Media descriptor f0: 1.4 MB floppy, f8 : hard disk; etc)
	uint16_t		FATSize16;				// 22-23 (Number of sectors per FAT .. 0 on FAT32**)
	uint16_t		SectorsPerTrack;		// 24-25
	uint16_t		NumberOfHeads;			// 26-27
	uint32_t		HiddenSectors;			// 28-31
	uint32_t		TotalSectors32;			// 32-35
	union {									// Unionize the two structures at offset 36
		struct BPBFAT1612_struct fat1612;	// Fat 16/12 structure
		struct BPBFAT32_struct	 fat32;		// Fat 32 structure
	} FSTypeData;
};

/*--------------------------------------------------------------------------}
{				    PARTITION DECRIPTION BLOCK STRUCTURE				    }
{--------------------------------------------------------------------------*/
//Structure to access info of a partition on the disk
struct __attribute__((packed, aligned(2))) partition_info {
	uint8_t		status;							// 0x80 - active partition
	uint8_t		headStart;						// starting head
	uint16_t	cylSectStart;					// starting cylinder and sector
	uint8_t		type;							// partition type (01h = 12bit FAT, 04h = 16bit FAT, 05h = Ex MSDOS, 06h = 16bit FAT (>32Mb), 0Bh = 32bit FAT (<2048GB))
	uint8_t		headEnd;						// ending head of the partition
	uint16_t	cylSectEnd;						// ending cylinder and sector
	uint32_t	firstSector;					// total sectors between MBR & the first sector of the partition
	uint32_t	sectorsTotal;					// size of this partition in sectors
};

/*--------------------------------------------------------------------------}
{				     MASTER BOOT RECORD BLOCK STRUCTURE					    }
{--------------------------------------------------------------------------*/
// Structure to access Master Boot Record for getting info about partitions
struct __attribute__((packed, aligned(4))) MBR_info {
	uint8_t					nothing[446];		// Filler the gap in the structure
	struct partition_info partitionData[4];		// partition records (16x4)
	uint16_t				signature;			// 0xaa55
};


/*--------------------------------------------------------------------------}
{				  FAT 12/16/32 (8:3) DIRECTORY ENTRY STRUCTURE			    }
{--------------------------------------------------------------------------*/
//Structure to access Directory Entry in the FAT
struct __attribute__((packed, aligned(1))) dir_Structure {
	SFN_NAME	name;						// 8.3 filename
	uint8_t		attrib;						// file attributes
	uint8_t		NTreserved;					// always 0
	uint8_t		timeTenth;					// tenths of seconds, set to 0 here
	uint16_t	writeTime;					// time file was last written
	uint16_t	writeDate;					// date file was last written
	uint16_t	lastAccessDate;				// date file was last accessed
	uint16_t	firstClusterHI;				// higher word of the first cluster number
	uint16_t	createTime;					// time file was created
	uint16_t	createDate;					// date file was created
	uint16_t	firstClusterLO;				// lower word of the first cluster number
	uint32_t	fileSize;					// size of file in bytes
};

/*--------------------------------------------------------------------------}
{				  FAT 12/16/32 (LFN) DIRECTORY ENTRY STRUCTURE			    }
{--------------------------------------------------------------------------*/
//Structure to access FAT long filename Directory Entry in the FAT
struct __attribute__((packed, aligned(1))) dir_LFN_Structure {
	uint8_t		LDIR_SeqNum;				// Long directory order		
	uint8_t		LDIR_Name1[10];				// Characters 1-5 of long name (UTF 16) but misaligned again
	uint8_t		LDIR_Attr;					// Long directory attribute .. always 0x0f	
	uint8_t		LDIR_Type;					// Long directory type ... always 0x00
	uint8_t		LDIR_ChkSum;				// Long directory checksum
	uint16_t	LDIR_Name2[6];				// Characters 6-11 of long name (UTF 16)
	uint16_t	LDIR_firstClusterLO;		// First cluster lo
	uint16_t	LDIR_Name3[2];				// Characters 12-13 of long name (UTF 16)
};

/*-[INTERNAL: LoadDrivePartition]-------------------------------------------}
. Attempts to load the partition on the SD Card. This involves detecting the
. type of partiton and saving values that will be needed for IO access.
. 10Aug17 LdB
.--------------------------------------------------------------------------*/
static bool LoadDrivePartition (printhandler prn_basic) {
	uint32_t partition_totalClusters;
	uint8_t buffer[512] __attribute__((aligned(4)));

	if (sdTransferBlocks(0, 1, (uint8_t*)&buffer[0], false) != SD_OK) return false;
	struct FAT_BPB_struct* bpb = (struct FAT_BPB_struct *)buffer;
	if (bpb->BS_JmpBoot[0] != 0xE9 && bpb->BS_JmpBoot[0] != 0xEB) { // Check if it is boot sector
		struct MBR_info* mbr = (struct MBR_info*)&buffer[0];		// if it is not boot sector, it must be MBR
		if (mbr->signature != 0xaa55) return false;					// if it is not even MBR then it's not FAT
		struct partition_info* pd = &mbr->partitionData[0];			// First partition
		sdCard.partition.unusedSectors = pd->firstSector;			// FAT16 needs this value so hold it
		sdTransferBlocks(pd->firstSector, 1, &buffer[0], false);	// Read first sector of partition
		if (bpb->BS_JmpBoot[0] != 0xE9 && bpb->BS_JmpBoot[0] != 0xEB)  
			return false;											// Not an MBR
	}
	sdCard.partition.bytesPerSector = bpb->BytesPerSector;			// Bytes per sector on partition
	sdCard.partition.sectorPerCluster = bpb->SectorsPerCluster;		// Hold the sector per cluster count
	sdCard.partition.reservedSectorCount = bpb->ReservedSectorCount;// Hold the reserved sector count
	if ((bpb->FATSize16 == 0) && (bpb->RootEntryCount == 0)) {		// Check if FAT16/FAT32
		// FAT32
		sdCard.partition.rootCluster = bpb->FSTypeData.fat32.RootCluster;// Hold partition root cluster
		sdCard.partition.firstDataSector = bpb->ReservedSectorCount + bpb->HiddenSectors + (bpb->FSTypeData.fat32.FATSize32 * bpb->NumFATs);
		// data sectors x sectorsize = capacity ... I have check this on PC and it gives right calc
		sdCard.partition.dataSectors = bpb->TotalSectors32 - bpb->ReservedSectorCount - (bpb->FSTypeData.fat32.FATSize32 * bpb->NumFATs);
		if (prn_basic) prn_basic("FAT32 Volume Label: %s, ID: %08x\n", 
			bpb->FSTypeData.fat32.BS_VolumeLabel, 
			(unsigned int)bpb->FSTypeData.fat32.BS_VolumeID);		// Basic detail print if requested
	}
	else {
		// FAT16
		sdCard.partition.rootCluster = 2;							// Hold partition root cluster, FAT16 always start at 2
		sdCard.partition.firstDataSector = sdCard.partition.unusedSectors + (bpb->NumFATs * bpb->FATSize16) + 1;
		// data sectors x sectorsize = capacity ... I have check this on PC and gives right calc
		sdCard.partition.dataSectors = bpb->TotalSectors32 - (bpb->NumFATs * bpb->FATSize16) - 33;  // -1 see above +1 and 32 fixed sectors 
		if (bpb->FSTypeData.fat1612.BS_BootSig == 0x29) {
			if (prn_basic) prn_basic("FAT12/16 Volume Label: %s, Volume ID %08x\n", 
				bpb->FSTypeData.fat1612.BS_VolumeLabel, 
				(unsigned int)bpb->FSTypeData.fat1612.BS_VolumeID);	// Basic detail print if requested
		}
	}
	
	// total clusters *  clustersize = capacity  ... see data sectors above ... another way to say same thing 
	partition_totalClusters = sdCard.partition.dataSectors / sdCard.partition.sectorPerCluster;
	if (prn_basic) prn_basic("First Sector: %lu, Data Sectors: %lu, TotalClusters: %lu, RootCluster: %lu\n",
		(unsigned long)sdCard.partition.firstDataSector, (unsigned long)sdCard.partition.dataSectors, 
		(unsigned long)partition_totalClusters, (unsigned long) sdCard.partition.rootCluster);
	return true;
}


/*-[INTERNAL: getFirstSector]-----------------------------------------------}
. Calculate first sector address of any given cluster number on a partition.
. beside the cluster number you need the sectorPerCluster and FirstDataSector
. of the partition on which you wish to calculate.
. 12Feb17 LdB
.--------------------------------------------------------------------------*/
static uint32_t getFirstSector (uint32_t clusterNumber, uint32_t sectorPerCluster, uint32_t firstDataSector) {
	return (((clusterNumber - 2) * sectorPerCluster) + firstDataSector);
}

/*-getSetNextCluster---------------------------------------------------------
Simply gets or sets next cluster entry value of the FAT chain
12Feb17 LdB
--------------------------------------------------------------------------*/
uint32_t getSetNextCluster (uint32_t clusterNumber, bool set, uint32_t clusterEntry) {
	uint32_t FATEntrySector, FATEntryOffset;
	uint32_t* FATEntryValue;
	uint8_t buffer[512];
	FATEntrySector = sdCard.partition.unusedSectors + sdCard.partition.reservedSectorCount +
		((clusterNumber * 4) / sdCard.partition.bytesPerSector);	// Get sector number of the cluster entry in the FAT
	FATEntryOffset = ((clusterNumber * 4) % sdCard.partition.bytesPerSector); // Get the offset address in that sector number
	if (sdTransferBlocks(FATEntrySector, 1, (uint8_t*)&buffer[0], false) != SD_OK)
	{
		return 0xFFFFFFFF;											// Sector read failed twice so fail exit
	}
	FATEntryValue = (uint32_t*)&buffer[FATEntryOffset];				// Get the cluster address from the buffer .. fortunately always aligned
	if (set == false) return (*FATEntryValue);						// If not setting exit with the retrieved value
	*FATEntryValue = clusterEntry;									// Setting new value in cluster entry in FAT
	if (sdTransferBlocks(FATEntrySector, 1, (uint8_t*)&buffer[0], true) != SD_OK)
	{
		return 0xFFFFFFFF;											// Sector write failed twice so fail exit
	}
	return (0);														// return zero
}


/*==========================================================================}
{				      PRIVATE SEARCH STRUCTURES AND ROUTINES				}
{==========================================================================*/

/*--------------------------------------------------------------------------}
{                         PRIVATE SEARCH DATA STRUCTURE			            }
{--------------------------------------------------------------------------*/
struct __attribute__((packed, aligned(4))) PRIV_SEARCH_DATA {
	uint32_t cluster;												// Current cluster
	uint32_t firstSector;											// First sector
	uint32_t sector;												// Current sector
	uint32_t bPos;													// Buffer position
	uint8_t buffer[512] __attribute__((aligned(4)));				// Data buffer
	LFN_NAME searchPattern;											// Search pattern find was started with
};

/*--------------------------------------------------------------------------}
{               INTERNAL SEARCHS THAT ARE CURRENTLY RUNNING		            }
{--------------------------------------------------------------------------*/
#define MAX_SEARCH_RECORDS 4										// Current limit is 4 (if you need more simultaneous searches increase)
static struct PRIV_SEARCH_DATA __attribute__((aligned(4))) intSearchRecord[MAX_SEARCH_RECORDS] = { 0 };

/*-[INTERNAL: SetFindDataFromFATEntry]--------------------------------------}
. Transfers the normal directory entry values to a FIND_DATA structure ptr.
. If both pointers it will transfer the various time, size and name records.
. 18Feb17 LdB
.--------------------------------------------------------------------------*/
static void SetFindDataFromFATEntry (struct dir_Structure* dir, FIND_DATA* lpFD) {
	if ((dir) && (lpFD)) {
		lpFD->dwFileAttributes = dir->attrib;						// Transfer file attributes	
		lpFD->CreateDT.tm_year = ((dir->createDate & 0xFE00) >> 9) + 80;// Looks weird but file date is from 1980 and C standard from 1900 so add 80
		lpFD->CreateDT.tm_mon = ((dir->createDate & 0x01E0) >> 5) - 1;// file date is 1-12 .. C standard is 0-11
		lpFD->CreateDT.tm_mday = (dir->createDate & 0x001F);		// day is strangely correct both 1-31 
		lpFD->CreateDT.tm_hour = ((dir->createTime & 0xF800) >> 11);// Hour when file was created
		lpFD->CreateDT.tm_min = ((dir->createTime & 0x07E0) >> 5);	// Minute when file was created
		lpFD->CreateDT.tm_sec = ((dir->createTime & 0x001F) << 1);	// Seconds when file was created

		lpFD->LastAccessDate.tm_year = ((dir->lastAccessDate & 0xFE00) >> 9) + 80; // Looks weird but file date is from 1980 and C standard from 1900
		lpFD->LastAccessDate.tm_mon = ((dir->lastAccessDate & 0x01E0) >> 5) - 1; // file date is 1-12 .. C standard is 0-11
		lpFD->LastAccessDate.tm_mday = (dir->lastAccessDate & 0x001F);	// day is strangely correct both 1-31 
		

		lpFD->WriteDT.tm_year = ((dir->writeDate & 0xFE00) >> 9) + 80;// Looks weird but file date is from 1980 and C standard from 1900
		lpFD->WriteDT.tm_mon = ((dir->writeDate & 0x01E0) >> 5) - 1;// file date is 1-12 .. C standard is 0-11
		lpFD->WriteDT.tm_mday = (dir->writeDate & 0x001F);			// day is strangely correct both 1-31 
		lpFD->WriteDT.tm_hour = ((dir->writeTime & 0xF800) >> 11);	// Hour of last file write
		lpFD->WriteDT.tm_min = ((dir->writeTime & 0x07E0) >> 5);	// Minute of last file write
		lpFD->WriteDT.tm_sec = ((dir->writeTime & 0x001F) << 1);	// Second of last file write

		lpFD->nFileSizeLow = dir->fileSize;							// Low 32 bits of file size
		lpFD->nFileSizeHigh = 0;									// Zero high 32 bits as not supported

		for (int i = 0; i < sizeof(SFN_NAME); i++)					// This is all unaligned so has to be manhandled
			lpFD->cAlternateFileName[i] = dir->name[i];				// Transfer short name (8:3)
	}
}

/*-[INTERNAL: ReadLFNEntry]-------------------------------------------------}
. Transfers up to 13 characters from the LFN directory entry to the buffer
. and returns the count of characters placed in the buffer. The first five
. characters in Name1 have alignment issues for the ARM 7/8, care needed.
. 18Feb17 LdB
.--------------------------------------------------------------------------*/
static uint8_t ReadLFNEntry (struct dir_LFN_Structure* LFdir, char LFNtext[14]) {
	uint8_t utf1, utf2;
	uint16_t utf, j;
	bool lfn_done = false;
	uint8_t LFN_blockcount = 0;
	if (LFdir) {													// Check the directory pointer valid
		for (j = 0; ((j < 5) && (lfn_done == false)); j++) {		// For each of the first 5 characters
			/* Alignment problems on this 1st block .. please leave alone */
			utf1 = LFdir->LDIR_Name1[j * 2];						// Load the low byte  ***unaligned access
			utf2 = LFdir->LDIR_Name1[j * 2 + 1];					// Load the high byte
			utf = (utf2 << 8) | utf1;								// Roll and join to make word
			if (utf == 0) lfn_done = true;							// If the character is zero we are done
				else LFNtext[LFN_blockcount++] = (char)wctob(utf);	// Otherwise narrow to ascii and place in buffer
		}
		for (j = 0; ((j < 6) && (lfn_done == false)); j++) {		// For the next six characters
			utf = LFdir->LDIR_Name2[j];								// Fetch the character
			if (utf == 0) lfn_done = true;							// If the character is zero we are done
				else LFNtext[LFN_blockcount++] = (char)wctob(utf);	// Otherwise narrow to ascii and place in buffer
		}
		for (j = 0; ((j < 2) && (lfn_done == false)); j++) {		// For the last two characters
			utf = LFdir->LDIR_Name3[j];								// Fetch the character
			if (utf == 0) lfn_done = true;							// If the character is zero we are done
				else LFNtext[LFN_blockcount++] = (char)wctob(utf);	// Otherwise narrow to ascii and place in buffer
		}
	}
	LFNtext[LFN_blockcount] = '\0';									// Make ascizz incase it needs to be used as string									
	return LFN_blockcount;											// Return characters in buffer 0-13 is valid range
}


/*-[INTERNAL: WildcardMatch ]-----------------------------------------------}
. Given a pattern string which may include wildcard characters "*","?" and
. sets the given text string is compared for a match. It is the classic code
. that checks if a given filename matches the provided pattern. 
. 18Feb17 LdB
.--------------------------------------------------------------------------*/
bool WildcardMatch (const char * pattern, const char * text)
{
	const char * retryPat = NULL;
	const char * retryText = NULL;
	int	ch;
	bool found;
	while (toupper(*text) || toupper(*pattern)) {					// While neither string terminated
		ch = *pattern++;											// Fetch next pattern character
		switch (ch)
		{
			case '*':												// If character is wildcard
				retryPat = pattern;									// Hold pattern for retry
				retryText = text;									// Hold text for retry
				break;
			case '[':												// Multi character select wildcard open
				found = false;										// found = false
				while ((ch = *pattern++) != ']')					// While not close multicharacter
				{
					if (ch == '\\')	ch = *pattern++;				// Literal request
					if (ch == '\0')	return false;					// Pattern terminated abnormally without match
					if (*text == ch) found = false;					// character not found
				}
				if (!found)											// If not found
				{
					pattern = retryPat;								// Reset pattern to retry string
					text = ++retryText;								// Set text string to next character on from retry
				}
				/* fall into next case */
			case '?':												// Single character wildcard
				if (*text++ == '\0') return false;					// text string ended so no match
				break;
			case '\\':												// Literal slash
				ch = *pattern++;									// Load next character
				if (ch == '\0')	return false;						// Pattern terminated abnormally without match
				/* fall into next case */
			default:
				if (toupper(*text) == toupper(ch))					// If pattern character matches text character
				{
					if (*text) text++;								// Move the text pointer forward
					break;
				}
				if (*text)											// If end of text string
				{
					pattern = retryPat;								// Reset pattern to retry point
					text = ++retryText;								// Set text string to next character on from retry
					break;
				}
				return false;										// No match
		}
		if (pattern == NULL) return false;							// No pattern provided
	}
	return true;													// Text string matches pattern
}

/*--------------------------------------------------------------------------}
{						  FAT SYSTEM ERROR CONSTANTS		 	            }
{--------------------------------------------------------------------------*/
#define FAT_RESULT_OK				0								// FAT function success
#define FAT_END_REACHED				1								// FAT table end reached (did not find requested before)
#define FAT_INVALID_DATAPTR			2								// FAT function given invalid pointer
#define FAT_READSECTOR_FAIL			3								// FAT sector read failed
#define FAT_WRITESECTOR_FAIL		4								// FAT sector write failed

/*-[INTERNAL: LocateFATEntry]-----------------------------------------------}
. Return the next diretory entry on the file system. It will return NULL if
. an error occurs and if you provide an ErrorID pointer it will set value
. to a value to help identify the error.
. 18Feb17 LdB
.--------------------------------------------------------------------------*/
#define FILE_EMPTY					0x00							// File empty
#define FILE_DELETED				0xE5							// File was deleted
#define FILE_ATTRIBUTE_LONGNAME		0x0F							// Long filename

static struct dir_Structure* LocateFATEntry (const char* searchPat, struct PRIV_SEARCH_DATA* priv, bool dirChange, LFN_NAME LFN_Name, uint32_t* ErrorID) {
	struct dir_Structure* dir;
	uint32_t LFN_count = 0;
	while ((priv->cluster < 0x0ffffff6)	&& (priv->cluster != 0)) {	// Check cluster valid and read successful	
		while (priv->sector < sdCard.partition.sectorPerCluster) {
			while (priv->bPos <  512) {								// While not a buffer end
				dir = (struct dir_Structure *) &priv->buffer[priv->bPos];// Transfer buffer position to pointer				
				if (dir->name[0] == FILE_EMPTY) {
					if (ErrorID) *ErrorID = FAT_END_REACHED;		// End of FAT entry chain reached
					return NULL;									// Return null pointer
				}
				if (dir->name[0] != FILE_DELETED) {					// If entry not deleted
					if (dir->attrib == FILE_ATTRIBUTE_LABEL) {		// FAT LABEL ENTRY
						/*printf("LABEL: %c%c%c%c%c%c%c%c%c%c%c\n",
							dir->name[0], dir->name[1], dir->name[2], dir->name[3],
							dir->name[4], dir->name[5], dir->name[6], dir->name[7],
							dir->name[8], dir->name[9], dir->name[10]);*/
					} else if (dir->attrib == FILE_ATTRIBUTE_LONGNAME)// LFN block precede the normal block
					{
						uint8_t LFN_blockcount;									
						char LFNtext[14] = { 0 };					// Each block can hold max 13 characters						
						// Read the LFN
						LFN_blockcount = ReadLFNEntry((struct dir_LFN_Structure*)&priv->buffer[priv->bPos], LFNtext);
						// Transfer the max 13 characters to front of reverse growing string
						for (int j = 0; j < LFN_blockcount; j++)
							LFN_Name[255 - LFN_count - LFN_blockcount + j] = LFNtext[j];
						LFN_count += LFN_blockcount;				// Increment long filename count
					} else {
						uint_fast32_t dotadded = 0;					// Track if we add a dot .. it starts zero
						if (LFN_count != 0) {						// Filename is LFN
							bool hasdot = false;					// Track if filname has dot in it, start false
							for (int j = 0; j < LFN_count; j++) {	// For each character in LFN
								LFN_Name[j] = LFN_Name[255 - LFN_count + j]; // Transfer it from back to front
								if (LFN_Name[j] == '.')				// If we see a dot 
									hasdot = true;					// Mark it as true
							}
							if ((!hasdot) &&						// If do not have a dot
							   (dirChange == false))				// This is not a directory change call
							{							
								dotadded = LFN_count;				// Set where we are going to add dot
								LFN_Name[LFN_count++] = '.';		// Add dot as req for wildcard name check
							}
							LFN_Name[LFN_count] = '\0';				// Make asciiz
							LFN_count = 0;							// Make sure we clear the internal LFN_count
						} else {
							int j = 0;								// Start on zero
							while (j < 8 && dir->name[j] != 0 && dir->name[j] != ' ') {
								LFN_Name[j] = dir->name[j];			// From SFN string to LFN
								j++;
							}
							if (dirChange == false) {
								LFN_Name[j++] = '.';				// Add a "." at end of name
								if (dir->attrib != FILE_ATTRIBUTE_DIRECTORY) {
									int k = 0;						// Start on zero
									while (k < 3 && dir->name[8 + k] != 0 && dir->name[8 + k] != ' ') {
										LFN_Name[j++] = dir->name[8 + k];// From SFN string to LFN
										k++;						// Next ext character
									}
								} else  dotadded = j - 1;
							}
							LFN_Name[j] = '\0';						// Make LFN asciiz
						}
						if ((dir->attrib != FILE_ATTRIBUTE_LABEL) &&// If not a label 
						   (WildcardMatch(searchPat, &LFN_Name[0])))// And LFN matches search pattern
						{
							if (dotadded) LFN_Name[dotadded] = '\0';// Make asciiz
							priv->bPos += sizeof(struct dir_Structure);// Move pointer down 	
							if (ErrorID) *ErrorID = FAT_RESULT_OK;	// Return result of FAT_RESULT_OK if requested
							return dir;								// Return directory pointer	
						}
						/*printf("Rejected match %s vs %s\n", searchPat, &LFN_Name[0]);*/
					}
				}
				priv->bPos += sizeof(struct dir_Structure);			// Buffer position moves forward
			}
			priv->sector++;											// Increment sector
			if (priv->sector < sdCard.partition.sectorPerCluster) { // Next sector valid do disk read
				priv->bPos = 0;										// Reset buffer position to top of buffer
				if (sdTransferBlocks(priv->firstSector + priv->sector, 
					1,	&priv->buffer[0], false) != SD_OK)
				{
					if (ErrorID) *ErrorID = FAT_READSECTOR_FAIL;	// Check for read failure
					return NULL;									// Return null pointer
				}
			}
		}
		priv->cluster = getSetNextCluster(priv->cluster, false, 0); // Fetch the next cluster
		priv->firstSector = getFirstSector(priv->cluster, 
			sdCard.partition.sectorPerCluster, 
			sdCard.partition.firstDataSector);						// Hold the first sector of this new cluster		
		priv->sector = 0;											// Zero the sector count of this new cluster 
		priv->bPos = 0;												// Reset buffer position to top of buffer
		if (sdTransferBlocks(priv->firstSector + priv->sector, 
			1,	&priv->buffer[0], false) != SD_OK) 
		{
			if (ErrorID) *ErrorID = FAT_READSECTOR_FAIL;			// Check for read failure
			return NULL;											// Return null pointer
		}
	}
	if (ErrorID) *ErrorID = FAT_INVALID_DATAPTR;					// If valid pointer return error id
	return NULL;													// Find FAT entry failed
}

/*-[CopyUnAlignedString]----------------------------------------------------}
. Copy a string byte by byte allowing for unaligned allocation
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
static bool CopyUnAlignedString (char* Dest, const char* Src) {
	if ((Src) && (Dest)) {
		while (*Src) *Dest++ = *Src++;								// Transfer source byte to dest and increment pointers
		*Dest = '\0';												// Make sure Dest is terminated
		return true;												// Return success
	}
	return false;													// One of the pointers was invalid
}


/*==========================================================================}
{						 PUBLIC FILE SEARCH ROUTINES						}
{==========================================================================*/

/*-[sdFindFirstFile]--------------------------------------------------------}
. This is an exact replica of Windows FindFirst function but restricted to
. this SD card.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
HANDLE sdFindFirstFile (const char* lpFileName, FIND_DATA* lpFFData) 
{												
	uint32_t errID = FAT_RESULT_OK;
	struct dir_Structure* dir;
	int i = 0;
	LFN_NAME tempName;												// We will cut and chop name so we need local copy
	if ((lpFileName == 0) || (lpFFData == 0)) return 0;				// One of the pointers is invalid so fail
	CopyUnAlignedString(&tempName[0], lpFileName);					// Copy the name string as we will chop and change it
	const char* searchStr = &tempName[0];							// Search string starts as lpFilename
	if (searchStr[0] == '\\') searchStr++;							// We sometimes write root directory with leadslash .. remove it
	while ((intSearchRecord[i].firstSector != 0) && i < MAX_SEARCH_RECORDS) i++;
	if (i != MAX_SEARCH_RECORDS) {									// Empty search record found
		intSearchRecord[i].cluster = sdCard.partition.rootCluster;	// We are going to start on FAT which starts at rootCluster
		intSearchRecord[i].firstSector = getFirstSector(
			sdCard.partition.rootCluster,
			sdCard.partition.sectorPerCluster,
			sdCard.partition.firstDataSector);						// Hold the first sector of FAT32
		intSearchRecord[i].sector = 0;								// Zero sector count
		intSearchRecord[i].bPos = 0;								// Zero buffer position
		if (sdTransferBlocks(intSearchRecord[i].firstSector, 1,
			&intSearchRecord[i].buffer[0], false) == SD_OK) {
			char* p;
			do {
				p = strchr(searchStr, '\\');						// Check for sub-directory slash
				if (p) {											// Directory slash found	
					*p = '\0';										// Turn the slash into a terminate
					dir = LocateFATEntry(searchStr, &intSearchRecord[i],
						true, lpFFData->cFileName, &errID);			// Locate directory FAT entry
					if (dir == NULL) return 0;						// Did not find subdirectory
					intSearchRecord[i].cluster = (((uint32_t)dir->firstClusterHI) << 16) | dir->firstClusterLO;
					intSearchRecord[i].firstSector = getFirstSector(
						intSearchRecord[i].cluster,
						sdCard.partition.sectorPerCluster,
						sdCard.partition.firstDataSector);			// Hold the first sector
					intSearchRecord[i].sector = 0;					// Zero sector count
					intSearchRecord[i].bPos = 0;					// Buffer pos starts at top of buffer
					if (sdTransferBlocks(intSearchRecord[i].firstSector, 1,
						&intSearchRecord[i].buffer[0], false) != SD_OK)
						return 0;									// Some read error occured
					searchStr = p + 1;								// Search string now starts after directory
				}
			} while (p != NULL);
			dir = LocateFATEntry(searchStr, &intSearchRecord[i],
				false, lpFFData->cFileName, &errID);				// Locate first FAT entry
			if ((errID == FAT_RESULT_OK) && (dir)) {				// First FAT entry returned		
				int j = 0;
				while (searchStr[j] != 0) {
					intSearchRecord[i].searchPattern[j] = searchStr[j];// Hold the search pattern
					j++;
				}
				SetFindDataFromFATEntry(dir, lpFFData);				// Set all the find data fields from FAT entry
				return (i+1);										// Return internal search record index as handle
			}
		}
		intSearchRecord[i].firstSector = 0;							// Clear first sector which is used to mark in use
	}
	return 0;														// Return the handle and its error
}

/*-[sdFindNextFile]---------------------------------------------------------}
. Continues a file search from a previous call to the FindFirstFile.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
HANDLE sdFindNextFile (HANDLE hFindFile, FIND_DATA* lpFFData)
{
	uint32_t errID = FAT_RESULT_OK;
	struct dir_Structure* dir;
	if ((hFindFile > 0) && (hFindFile <= MAX_SEARCH_RECORDS) &&
		(intSearchRecord[hFindFile-1].firstSector != 0)) {			// File handle maps to an internal search record in use 
		dir = LocateFATEntry(&intSearchRecord[hFindFile-1].searchPattern[0],
			&intSearchRecord[hFindFile-1],
			false, lpFFData->cFileName, &errID);					// Locate first FAT entry
		if ((errID == FAT_RESULT_OK) && (dir)) {					// First FAT entry returned
			SetFindDataFromFATEntry(dir, lpFFData);					// Set all the find data fields from FAT entry
			return (hFindFile);										// Return internal serach record index as handle
		}
		intSearchRecord[hFindFile - 1].firstSector = 0;				// Clear first sector which is used to mark in use
	}
	return 0;														// Return the handle and its error
}

/*-[sdFindClose]------------------------------------------------------------}
. Closes and releases a file search handle opened by the FindFirstFile.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
bool sdFindClose (HANDLE hFindFile)
{
	if ((hFindFile > 0) && (hFindFile <= MAX_SEARCH_RECORDS) &&
		(intSearchRecord[hFindFile - 1].firstSector != 0)) {		// File handle maps to an internal search record in use 
		intSearchRecord[hFindFile - 1].firstSector = 0;				// Clear first sector which is used to mark in use
		return true;
	}
	return false;
}

/*==========================================================================}
{						 PUBLIC FILE IO ROUTINES							}
{==========================================================================*/


/*--------------------------------------------------------------------------}
{                        PRIVATE FILE IO DATA STRUCTURE		            }
{--------------------------------------------------------------------------*/
struct __attribute__((packed, aligned(4))) PRIV_FILE_IO_DATA {
	struct PRIV_SEARCH_DATA srec;									// Search record
	uint32_t fileStart;												// File start sector
	uint32_t filePos;												// Current file position
	uint32_t fileSize;												// Current file size
};

/*--------------------------------------------------------------------------}
{               INTERNAL FILE IO THAT ARE CURRENTLY RUNNING		            }
{--------------------------------------------------------------------------*/
#define MAX_FILE_IO_RECORDS 8										// Maximum number of file IO that can occur at same time
static struct PRIV_FILE_IO_DATA __attribute__((aligned(4))) intFileIORecord[MAX_FILE_IO_RECORDS] = { 0 };

/*-[sdCreateFile]-----------------------------------------------------------}
. Creates or opens a file or I/O device. The function returns a handle that 
. can be used to access the file for followup I/O operations. The file or 
. device will be openned with flags and attributes specified.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
HANDLE sdCreateFile (const char* lpFileName,						// Filename or device to open
					 uint32_t	dwDesiredAccess,					// GENERIC_READ or GENERIC_WRITE
					 uint32_t	dwShareMode,						// Currently not supported (use 0)
					 uint32_t	lpSecurityAttributes,				// Currently not supported (use 0)
					 uint32_t	dwCreationDisposition,				// CREATE_ALWAYS, CREATE_NEW, OPEN_EXISTING, OPEN_ALWAYS
					 uint32_t	dwFlagsAndAttributes,				// Standard file attributes
					 HANDLE hTemplateFile)							// Currently not supported (use 0)
{
	uint32_t errID = FAT_RESULT_OK;
	struct dir_Structure* dir;
	LFN_NAME openName;
	int i = 0;
	LFN_NAME tempName;												// We will cut and chop name so we need local copy
	if (lpFileName == 0) return 0;									// Name pointer is invalid so fail
	CopyUnAlignedString(&tempName[0], lpFileName);					// Make local copy of name as we will chop and change it
	const char* searchStr = &tempName[0];							// Search string starts as lpFilename
	if (searchStr[0] == '\\') searchStr++;							// We sometimes write root directory with lead slash .. remove it
	while ((intFileIORecord[i].srec.firstSector != 0) && i < MAX_FILE_IO_RECORDS) i++;
	if (i != MAX_FILE_IO_RECORDS) {									// Empty search record found
		intFileIORecord[i].srec.cluster = sdCard.partition.rootCluster;
		intFileIORecord[i].srec.firstSector = getFirstSector(
			sdCard.partition.rootCluster,
			sdCard.partition.sectorPerCluster,
			sdCard.partition.firstDataSector);						// Hold the first sector
		intFileIORecord[i].srec.sector = 0;							// Zero sector count
		intFileIORecord[i].srec.bPos = 0;							// Zero buffer position
		if (sdTransferBlocks(intFileIORecord[i].srec.firstSector, 1,
			&intFileIORecord[i].srec.buffer[0], false) == SD_OK)	// Read a sector of data
		{
			char* p;
			do {
				p = strchr(searchStr, '\\');						// Check for sub-directory slash
				if (p) {											// Directory slash found	
					*p = '\0';										// Turn the slash into a terminate
					dir = LocateFATEntry(searchStr, &intFileIORecord[i].srec,
						true, openName, &errID);					// Locate directory FAT entry
					if (dir == NULL) return 0;						// Did not find subdirectory
					intFileIORecord[i].srec.cluster = (((uint32_t)dir->firstClusterHI) << 16) | dir->firstClusterLO;
					intFileIORecord[i].srec.firstSector = getFirstSector(
						intFileIORecord[i].srec.cluster,
						sdCard.partition.sectorPerCluster,
						sdCard.partition.firstDataSector);			// Hold the first sector
					intFileIORecord[i].srec.sector = 0;				// Zero sector count
					intFileIORecord[i].srec.bPos = 0;				// Buffer pos starts at top of buffer
					if (sdTransferBlocks(intFileIORecord[i].srec.firstSector, 1,
						&intFileIORecord[i].srec.buffer[0], false) != SD_OK)
						return 0;									// Some read error occured
					searchStr = p + 1;								// Search string now starts after directory
				}
			} while (p != NULL);
			dir = LocateFATEntry(searchStr, &intFileIORecord[i].srec,
				false, openName, &errID);							// Locate file first FAT entry
			if ((errID == FAT_RESULT_OK) && (dir)) {				// First FAT matching entry returned
				intFileIORecord[i].fileStart = (((uint32_t)dir->firstClusterHI) << 16) | dir->firstClusterLO;
				intFileIORecord[i].filePos = 0;						// Zero file position
				intFileIORecord[i].fileSize = dir->fileSize;		// Transfer file size
				intFileIORecord[i].srec.cluster = intFileIORecord[i].fileStart;
				intFileIORecord[i].srec.firstSector = getFirstSector(
					intFileIORecord[i].srec.cluster,
					sdCard.partition.sectorPerCluster,
					sdCard.partition.firstDataSector);				// Hold the first sector
				intFileIORecord[i].srec.sector = 0;					// Zero sector count
				intFileIORecord[i].srec.bPos = 0;					// Buffer pos starts at top of buffer		
				if (sdTransferBlocks(intFileIORecord[i].srec.firstSector,
					1, &intFileIORecord[i].srec.buffer[0], false) == SD_OK) 
				{
					return (i + 1);									// Return internal file io record index as handle
				}
			}
		}
	}
	return 0;														// Create file failed
}


/*-[sdReadFile]-------------------------------------------------------------}
. Reads data from the specified file or input/output (I/O) device. The file
. must have been opened with CreateFile and the handle is returned from that
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
bool sdReadFile (HANDLE hFile,										// Handle as returned from CreateFile
				 void* lpBuffer,									// Pointer to buffer read data will be placed
				 uint32_t nNumberOfBytesToRead,						// Number of bytes requested to read
				 uint32_t* lpNumberOfBytesRead,						// Provide a pointer to a value which updated to bytes actually placed in buffer
				 void* lpOverlapped)								// Currently not supported (use 0)
{
	if ((hFile > 0) && (hFile <= MAX_FILE_IO_RECORDS) &&
		(intFileIORecord[hFile - 1].srec.firstSector != 0)) {		// File handle maps to an internal file io record in use 
		uint32_t bytesRead = 0;										// Zero bytes read
		uint32_t retErrId = FAT_RESULT_OK;							// Preset error id clear

		while ((retErrId == FAT_RESULT_OK) && (intFileIORecord[hFile - 1].srec.cluster < 0x0ffffff6)
			&& (intFileIORecord[hFile - 1].srec.cluster != 0)) {	// Check cluster valid and read successful	

			while (	(nNumberOfBytesToRead > bytesRead) &&			// Still more bytes to read
				(intFileIORecord[hFile - 1].filePos < intFileIORecord[hFile - 1].fileSize) && // Still inside fileSize
				(intFileIORecord[hFile - 1].srec.bPos <  512) )		// While not a buffer end
			{
				((uint8_t*)lpBuffer)[bytesRead] = intFileIORecord[hFile - 1].srec.buffer[intFileIORecord[hFile - 1].srec.bPos++]; // Transfer byte and increment pointer
				bytesRead++;										// Increment bytes read	
				intFileIORecord[hFile - 1].filePos++;				// Increment file position
			}
			if (nNumberOfBytesToRead == bytesRead) {				// Finished
				if (lpNumberOfBytesRead) *lpNumberOfBytesRead = bytesRead;
				return true;										// Read completed successfully
			}
			if (intFileIORecord[hFile - 1].filePos >= intFileIORecord[hFile - 1].fileSize) 
			{														// File end reached
				if (lpNumberOfBytesRead) *lpNumberOfBytesRead = bytesRead;
				return false;										// Read ended as file ran out of data
			}

			intFileIORecord[hFile - 1].srec.sector++;				// Increment sector
			if (intFileIORecord[hFile - 1].srec.sector < sdCard.partition.sectorPerCluster) // Next sector still in current cluster
			{
				intFileIORecord[hFile - 1].srec.bPos = 0;			// Reset buffer position to top of buffer
				if (sdTransferBlocks(intFileIORecord[hFile - 1].srec.firstSector + intFileIORecord[hFile - 1].srec.sector,
					1, &intFileIORecord[hFile - 1].srec.buffer[0], false) != SD_OK) {
					return false;									// Return read failure
				}
			} else {												// Need to move to next cluster
				intFileIORecord[hFile - 1].srec.cluster = getSetNextCluster(
					intFileIORecord[hFile - 1].srec.cluster, false, 0);// Fetch the next cluster
				intFileIORecord[hFile - 1].srec.firstSector = getFirstSector(
					intFileIORecord[hFile - 1].srec.cluster,
					sdCard.partition.sectorPerCluster,
					sdCard.partition.firstDataSector);				// Hold the first sector of this new cluster
				intFileIORecord[hFile - 1].srec.sector = 0;			// Zero the sector count 
				intFileIORecord[hFile - 1].srec.bPos = 0;			// Reset buffer position to top of buffer
				if (sdTransferBlocks(intFileIORecord[hFile - 1].srec.firstSector + intFileIORecord[hFile - 1].srec.sector,
					1, &intFileIORecord[hFile - 1].srec.buffer[0], false) != SD_OK) {
					return false;									// Return read failure
				}
			}
		}
	}
	return false;													// Return read failure
}
	
/*-[sdCloseHandle]----------------------------------------------------------}
. Closes file or device as per the handle that was given when it opened.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
bool sdCloseHandle (HANDLE hFile)
{
	if ((hFile > 0) && (hFile <= MAX_FILE_IO_RECORDS) &&
		(intFileIORecord[hFile - 1].srec.firstSector != 0)) {		// File handle maps to an internal file io record in use 
		intFileIORecord[hFile - 1].srec.firstSector = 0;			// Clear first sector which is used to mark in use
		return true;												// Return handle successfully closed
	}
	return false;													// Return handle did not close
}

/*-[sdSetFilePointer]-------------------------------------------------------}
. The function stores the file pointer in two LONG values. To work with file
. pointers that are larger than a single LONG value, it is easier to use the
. SetFilePointerEx function.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
uint32_t sdSetFilePointer (HANDLE hFile,							// Handle as returned from CreateFile
						   uint32_t lDistanceToMove,				// low order 32-bit signed value of bytes to move the file pointer
						   uint32_t* lpDistanceToMoveHigh,			// A pointer to high order 32-bits of the signed 64-bit distance to move
						   uint32_t dwMoveMethod)					// FILE_BEGIN, FILE_CURRENT, FILE_END
{
	if ((hFile > 0) && (hFile <= MAX_FILE_IO_RECORDS) &&
		(intFileIORecord[hFile - 1].srec.firstSector != 0)) {		// File handle maps to an internal file io record in use 

		switch (dwMoveMethod) {
			case FILE_CURRENT:										// Movement from current position
				lDistanceToMove += intFileIORecord[hFile - 1].filePos;// So simply add current position
				break;
			case FILE_END:											// Movement from end of file
				lDistanceToMove += intFileIORecord[hFile - 1].fileSize;// So simply add filesize
				break;
			default:												// Default will be file begin and do nothing
				break;
		}

		if (lDistanceToMove > intFileIORecord[hFile - 1].fileSize)	// You cant request a move larger than filesize
			return INVALID_SET_FILE_POINTER;						// Return set file position failure

		if (lDistanceToMove == intFileIORecord[hFile - 1].filePos)  // Request move is where we already are
			return (intFileIORecord[hFile - 1].filePos);			// So return position as if we did something

		intFileIORecord[hFile - 1].srec.cluster = intFileIORecord[hFile - 1].fileStart;
		intFileIORecord[hFile - 1].srec.firstSector = getFirstSector(
			intFileIORecord[hFile - 1].srec.cluster,
			sdCard.partition.sectorPerCluster,
			sdCard.partition.firstDataSector);						// Hold the first sector
		intFileIORecord[hFile - 1].srec.sector = 0;					// Zero sector count
		intFileIORecord[hFile - 1].srec.bPos = 0;					// Buffer pos starts at top of buffer	
		intFileIORecord[hFile - 1].filePos = 0;						// Zero file position
		if (sdTransferBlocks(intFileIORecord[hFile - 1].srec.firstSector,
			1, &intFileIORecord[hFile - 1].srec.buffer[0], false) != SD_OK) {
			return INVALID_SET_FILE_POINTER;						// Return set file position failure
		}

		while (lDistanceToMove > 512) {								// Position not in current sector
			intFileIORecord[hFile - 1].srec.sector++;				// Increment sector
			if (intFileIORecord[hFile - 1].srec.sector < sdCard.partition.sectorPerCluster) // Next sector still in current cluster
			{
				intFileIORecord[hFile - 1].srec.bPos = 0;			// Reset buffer position to top of buffer
				if (sdTransferBlocks(intFileIORecord[hFile - 1].srec.firstSector + intFileIORecord[hFile - 1].srec.sector,
					1, &intFileIORecord[hFile - 1].srec.buffer[0], false) != SD_OK) {
					return INVALID_SET_FILE_POINTER;				// Return set file position failure
				}
			} else {												// Need to move to next cluster
				intFileIORecord[hFile - 1].srec.cluster = getSetNextCluster(
					intFileIORecord[hFile - 1].srec.cluster, false, 0);  // Fetch the next cluster
				intFileIORecord[hFile - 1].srec.firstSector = getFirstSector(
					intFileIORecord[hFile - 1].srec.cluster, 
					sdCard.partition.sectorPerCluster, 
					sdCard.partition.firstDataSector);				// Hold the first sector of this new cluster
				intFileIORecord[hFile - 1].srec.sector = 0;			// Zero the sector count 
				intFileIORecord[hFile - 1].srec.bPos = 0;			// Reset buffer position to top of buffer
				if (sdTransferBlocks(intFileIORecord[hFile - 1].srec.firstSector + intFileIORecord[hFile - 1].srec.sector,
					1, &intFileIORecord[hFile - 1].srec.buffer[0], false) != SD_OK) {
					return INVALID_SET_FILE_POINTER;				// Return set file position failure
				}
			}
			lDistanceToMove -= 512;									// We have moved 512 bytes
			intFileIORecord[hFile - 1].filePos += 512;				// Increment file position
		}
		intFileIORecord[hFile - 1].srec.bPos = lDistanceToMove;		// Buffer position is whatever is left below 512
		intFileIORecord[hFile - 1].filePos += lDistanceToMove;		// Add left over movement to file position
		if (lpDistanceToMoveHigh) *lpDistanceToMoveHigh = 0;		// We don't use this
		return (intFileIORecord[hFile - 1].filePos);				// Return the file position
	}
	return INVALID_SET_FILE_POINTER;								// Return set file position failure
}


/*-[sdGetFileSize]----------------------------------------------------------}
. Retrieves the size of the specified file, in bytes. As we don't support
. individual file size beyond 4GB lpFileSizeHigh is unused.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
uint32_t sdGetFileSize (HANDLE  hFile, uint32_t* lpFileSizeHigh) {
	if ((hFile > 0) && (hFile <= MAX_FILE_IO_RECORDS) &&
		(intFileIORecord[hFile - 1].srec.firstSector != 0)) 		// File handle maps to an internal file io record in use
	{
		return(intFileIORecord[hFile - 1].fileSize);				// Return the file size
	}
	return 0;														// Function failed
}


/*-[sdInitCard]-------------------------------------------------------------}
. Attempts to initializes current SD Card and returns success/error status.
. This call should be done before any attempt to do anything with a SD card.
. RETURN: SD_OK indicates the current card successfully initialized.
.         !SD_OK if card initialize failed with code identifying error.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
SDRESULT sdInitCard (printhandler prn_basic, printhandler prn_error, bool mount)
{
	SDRESULT resp;

	// First ensure log error handler set
	LOG_ERROR = prn_error;											// Hold the provided print error handler

	// Reset the card.
	if( (resp = sdResetCard()) ) return resp;						// Reset SD card

	// Send SEND_IF_COND,0x000001AA (CMD8) voltage range 0x1 check pattern 0xAA
	// If voltage range and check pattern don't match, look for older card.
	resp = sdSendCommandA(IX_SEND_IF_COND,0x000001AA);
	if( resp == SD_OK )
	{
	// Card responded with voltage and check pattern.
	// Resolve voltage and check for high capacity card.
	if( (resp = sdAppSendOpCond(ACMD41_ARG_HC)) ) return sdDebugResponse(resp);

	// Check for high or standard capacity.
	if( sdCard.ocr.card_capacity )
		sdCard.type = SD_TYPE_2_HC;
	else
		sdCard.type = SD_TYPE_2_SC;
	}
	else if( resp == SD_BUSY ) return resp;
	// No response to SEND_IF_COND, treat as an old card.
	else
    {
		// If there appears to be a command in progress, reset the card.
		if( (EMMC_STATUS->CMD_INHIBIT) &&
			(resp = sdResetCard()) )
			return resp;

		// wait(50);
		// Resolve voltage.
		if( (resp = sdAppSendOpCond(ACMD41_ARG_SC)) ) return sdDebugResponse(resp);

		sdCard.type = SD_TYPE_1;
    }

	// Send ALL_SEND_CID (CMD2)
	if( (resp = sdSendCommand(IX_ALL_SEND_CID)) ) return sdDebugResponse(resp);

	// Send SEND_REL_ADDR (CMD3)
	// TODO: In theory, loop back to SEND_IF_COND to find additional cards.
	if( (resp = sdSendCommand(IX_SEND_REL_ADDR)) ) return sdDebugResponse(resp);

	// From now on the card should be in standby state.
	// Actually cards seem to respond in identify state at this point.
	// Check this with a SEND_STATUS (CMD13)
	//if( (resp = sdSendCommand(IX_SEND_STATUS)) ) return sdDebugResponse(resp);
	//printf("Card current state: %08x %s\n",sdCard.status,STATUS_NAME[sdCard.cardState]);

	// Send SEND_CSD (CMD9) and parse the result.
	if( (resp = sdSendCommand(IX_SEND_CSD)) ) return sdDebugResponse(resp);

	// At this point, set the clock to full speed.
	if( (resp = sdSetClock(FREQ_NORMAL)) ) return sdDebugResponse(resp);

	// Send CARD_SELECT  (CMD7)
	// TODO: Check card_is_locked status in the R1 response from CMD7 [bit 25], if so, use CMD42 to unlock
	// CMD42 structure [4.3.7] same as a single block write; data block includes
	// PWD setting mode, PWD len, PWD data.
	if( (resp = sdSendCommand(IX_CARD_SELECT)) ) return sdDebugResponse(resp);

	// Get the SCR as well.
	// Need to do this before sending ACMD6 so that allowed bus widths are known.
	if( (resp = sdReadSCR()) ) return sdDebugResponse(resp);

	// Send APP_SET_BUS_WIDTH (ACMD6)
	// If supported, set 4 bit bus width and update the CONTROL0 register.
	if (sdCard.scr.BUS_WIDTH == BUS_WIDTH_4)
	{
		if( (resp = sdSendCommandA(IX_SET_BUS_WIDTH, sdCard.rca | 2)) ) 
			return sdDebugResponse(resp);
		EMMC_CONTROL0->HCTL_DWIDTH = 1;
		LOG_DEBUG("EMMC: Bus width set to 4\n");
	}

	// Send SET_BLOCKLEN (CMD16)
	if( (resp = sdSendCommandA(IX_SET_BLOCKLEN,512)) ) return sdDebugResponse(resp);

	// Print out the CID having got this far.
	unsigned int serial = sdCard.cid.SerialNumHi;
	serial <<= 16;
	serial |= sdCard.cid.SerialNumLo;
	if (prn_basic) prn_basic("EMMC: SD Card %s %dMb mfr %d '%c%c:%c%c%c%c%c' r%d.%d %d/%d, #%08x RCA %04x\n",
		SD_TYPE_NAME[sdCard.type], (int)(sdCard.CardCapacity >> 20),
		sdCard.cid.MID, sdCard.cid.OID_Hi, sdCard.cid.OID_Lo,
		sdCard.cid.ProdName1, sdCard.cid.ProdName2, sdCard.cid.ProdName3, sdCard.cid.ProdName4, sdCard.cid.ProdName5,
		sdCard.cid.ProdRevHi, sdCard.cid.ProdRevLo, sdCard.cid.ManufactureMonth, 2000+sdCard.cid.ManufactureYear, serial,
		sdCard.rca >> 16);

	if (mount) if (!LoadDrivePartition(prn_basic)) return SD_MOUNT_FAIL;
	return SD_OK;
}

/*-[sdCardCSD]--------------------------------------------------------------}
. Returns the pointer to the CSD structure for the current SD Card.
. RETURN: Valid CSD pointer if the current card successfully initialized
.         NULL if current card initialize not called or failed.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
struct CSD* sdCardCSD (void) {
	if (sdCard.type != SD_TYPE_UNKNOWN) return (&sdCard.csd);		// Card success so return structure pointer
		else return NULL;											// Return fail result of null
}
