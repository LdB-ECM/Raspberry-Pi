#ifndef _SMART_START_
#define _SMART_START_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

#ifndef __ASSEMBLER__							// Don't include standard headers for ASSEMBLER
#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: rpi-smartstart.h											}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 2.01														}
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
{	   There is not much to the file as it mostly provides the external     }
{  C definitions of the underlying assembler code. It is also the easy      }
{  point to insert a couple of important system Macros.						} 
{																            }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/***************************************************************************}
{		  PUBLIC MACROS MUCH AS WE HATE THEM SOMETIMES YOU NEED THEM        }
{***************************************************************************/

/* Most versions of C don't have _countof macro so provide it if not available */
#if !defined(_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0])) 
#endif

/* Often I define In and Out for routine interfaces as per MSC */
#define _In_							// Swallow any _In_ declaration
#define _Out_							// Swallow any _Out_ declaration

/* As we are compiling for Raspberry Pi if main, winmain make them kernel_main */
#define WinMain(...) kernel_main (uint32_t r0, uint32_t r1, uint32_t atags)
#define main(...) kernel_main (uint32_t r0, uint32_t r1, uint32_t atags)

/* System font is 8 wide and 16 height so these are preset for the moment */
#define BitFontHt 16
#define BitFontWth 8


/***************************************************************************}
{		 		    PUBLIC STRUCTURE DEFINITIONS				            }
****************************************************************************/

/*--------------------------------------------------------------------------}
{				 COMPILER TARGET SETTING STRUCTURE DEFINED					}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(4))) CodeType {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile enum {
				ARM5_CODE = 5,
				ARM6_CODE = 6,
				ARM7_CODE = 7,
				ARM8_CODE = 8,
			} CodeType : 4;											// @0  Compiler target code set
			volatile enum {
				AARCH32 = 0,
				AARCH64 = 1,
			} AArchMode : 1;										// @5  Code AARCH type compiler is producing
			volatile unsigned CoresSupported : 3;					// @6  Cores the code is setup to support
			unsigned reserved : 24;									// @9-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
} CODE_TYPE;

/*--------------------------------------------------------------------------}
{						ARM CPU ID STRUCTURE DEFINED						}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(4))) CpuId {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			volatile unsigned Revision : 4;							// @0-3  CPU minor revision 
			volatile unsigned PartNumber: 12;						// @4-15  Partnumber
			volatile unsigned Architecture : 4;						// @16-19 Architecture
			volatile unsigned Variant : 4;							// @20-23 Variant
			volatile unsigned Implementer : 8;						// @24-31 reserved
		};
		uint32_t Raw32;												// Union to access all 32 bits as a uint32_t
	};
} CPU_ID;

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

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{					CPU ID ROUTINES PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
extern const char* RPi_CpuIdString (void);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			GLOBAL INTERRUPT CONTROL PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
extern void EnableInterrupts (void);			// Enable global interrupts
extern void DisableInterrupts (void);			// Disable global interrupts


typedef void (*CORECALLFUNC) (void);
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{	   	RPi-SmartStart API TO SET CORE EXECUTE ROUTINE AT ADDRESS 		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Execute function on (1..CoresReady) */
extern bool CoreExecute (uint8_t coreNum, CORECALLFUNC func); 

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{			MEMORY HELPER ROUTINES PROVIDE BY RPi-SmartStart API		    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* ARM bus address to GPU bus address */
extern uint32_t ARMaddrToGPUaddr (void* ARMaddress);

/* GPU bus address to ARM bus address */
extern uint32_t GPUaddrToARMaddr (uint32_t GPUaddress);


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif

