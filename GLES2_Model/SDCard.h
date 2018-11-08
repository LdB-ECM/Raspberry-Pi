#ifndef SDCARD_H
#define SDCARD_H
#include <time.h>										// C standard for time needed for that and struct tm
#include "rpi-smartstart.h"								// Provides all basic hardware access and printhandler definition and HANDLE type

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}
{       Filename: SDCard.h													}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 1.01														}
{       Original start code from Holdswrthy use from the Pi Forum           }
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

/***************************************************************************}
{                    PUBLIC ENUMERATIONS FOR THIS UNIT					    }
****************************************************************************/

/*--------------------------------------------------------------------------}
{					     PUBLIC SD RESULT CODES							    }
{--------------------------------------------------------------------------*/
typedef enum {
	SD_OK				= 0,							// NO error
	SD_ERROR			= 1,							// General non specific SD error
	SD_TIMEOUT			= 2,							// SD Timeout error
	SD_BUSY				= 3,							// SD Card is busy
	SD_NO_RESP			= 5,							// SD Card did not respond
	SD_ERROR_RESET		= 6,							// SD Card did not reset
	SD_ERROR_CLOCK		= 7,							// SD Card clock change failed
	SD_ERROR_VOLTAGE	= 8,							// SD Card does not support requested voltage
	SD_ERROR_APP_CMD	= 9,							// SD Card app command failed						
	SD_CARD_ABSENT		= 10,							// SD Card not present
	SD_READ_ERROR		= 11,
	SD_MOUNT_FAIL		= 12,
} SDRESULT;

// SD card types

/*--------------------------------------------------------------------------}
{				    PUBLIC ENUMERATION OF SD CARD TYPE					    }
{--------------------------------------------------------------------------*/
typedef enum {
	SD_TYPE_UNKNOWN = 0,
	SD_TYPE_MMC = 1,
	SD_TYPE_1 = 2,
	SD_TYPE_2_SC = 3,
	SD_TYPE_2_HC = 4,
} SDCARD_TYPE;


/***************************************************************************}
{                    PUBLIC STRUCTURES FOR THIS UNIT					    }
****************************************************************************/

/*--------------------------------------------------------------------------}
{					   PUBLIC CSD DECODED STRUCTURE						    }
{---------------------------------------------------------------------------}
{  You usually do not create this structure you simply ask for the pointer  }
{  to the one automatically created when an SD Card is initialized. It has  }
{  many of the entries of the standard SD Card CSD. However as it allows    }
{  for many SD Card variants the field positions can be very different.     }
{--------------------------------------------------------------------------*/
struct __attribute__((__packed__, aligned(4))) CSD {
	union {
		struct __attribute__((__packed__, aligned(1))) {
			enum { 
				CSD_VERSION_1 = 0,						// enum CSD version 1.0 - 1.1, Version 2.00/Standard Capacity
				CSD_VERSION_2 = 1,						// enum CSD cersion 2.0, Version 2.00/High Capacity and Extended Capacity
			} csd_structure : 2;						// @127-126	CSD Structure Version as on SD CSD bits 
			unsigned spec_vers : 6;						// @125-120 CSD version as on SD CSD bits
			uint8_t taac;								// @119-112	taac as on SD CSD bits
			uint8_t nsac;								// @111-104	nsac as on SD CSD bits
			uint8_t tran_speed;							// @103-96	trans_speed as on SD CSD bits
		};
		uint32_t Raw32_0;								// @127-96	Union to access 32 bits as a uint32_t		
	};
	union {
		struct __attribute__((__packed__, aligned(1))) {
			unsigned ccc : 12;							// @95-84	ccc as on SD CSD bits
			unsigned read_bl_len : 4;					// @83-80	read_bl_len on SD CSD bits
			unsigned read_bl_partial : 1;				// @79		read_bl_partial as on SD CSD bits
			unsigned write_blk_misalign : 1;			// @78		write_blk_misalign as on SD CSD bits
			unsigned read_blk_misalign : 1;				// @77		read_blk_misalign as on SD CSD bits
			unsigned dsr_imp : 1;						// @76		dsr_imp as on SD CSD bits
			unsigned c_size : 12;						// @75-64   Version 1 C_Size as on SD CSD bits
		};
		uint32_t Raw32_1;								// @0-31	Union to access 32 bits as a uint32_t		
	};
	union {
		struct __attribute__((__packed__, aligned(1))) {
			union {
				struct __attribute__((__packed__, aligned(1))) {
					unsigned vdd_r_curr_min : 3;		// @61-59	vdd_r_curr_min as on SD CSD bits
					unsigned vdd_r_curr_max : 3;		// @58-56	vdd_r_curr_max as on SD CSD bits
					unsigned vdd_w_curr_min : 3;		// @55-53	vdd_w_curr_min as on SD CSD bits
					unsigned vdd_w_curr_max : 3;		// @52-50	vdd_w_curr_max as on SD CSD bits
					unsigned c_size_mult : 3;			// @49-47	c_size_mult as on SD CSD bits
					unsigned reserved0 : 7;				// reserved for CSD ver 2.0 size match
				};
				unsigned ver2_c_size : 22;				// Version 2 C_Size
			};
			unsigned erase_blk_en : 1;					// @46		erase_blk_en as on SD CSD bits
			unsigned sector_size : 7;					// @45-39	sector_size as on SD CSD bits
			unsigned reserved1 : 2;						// 2 Spares bit unused
		};
		uint32_t Raw32_2;								// @0-31	Union to access 32 bits as a uint32_t		
	};
	union {
		struct __attribute__((__packed__, aligned(1))) {
			unsigned wp_grp_size : 7;					// @38-32	wp_grp_size as on SD CSD bits
			unsigned wp_grp_enable : 1;					// @31		wp_grp_enable as on SD CSD bits
			unsigned reserved2 : 2;						// @30-29 	Write as zero read as don't care
			unsigned r2w_factor : 3;					// @28-26	r2w_factor as on SD CSD bits
			unsigned write_bl_len : 4;					// @25-22	write_bl_len as on SD CSD bits
			unsigned write_bl_partial : 1;				// @21		write_bl_partial as on SD CSD bits
			unsigned default_ecc : 5;					// @20-16	default_ecc as on SD CSD bits
			unsigned file_format_grp : 1;				// @15		file_format_grp as on SD CSD bits
			unsigned copy : 1;							// @14		copy as on SD CSD bits
			unsigned perm_write_protect : 1;			// @13		perm_write_protect as on SD CSD bits
			unsigned tmp_write_protect : 1;				// @12		tmp_write_protect as on SD CSD bits
			enum {
				FAT_PARTITION_TABLE = 0,				// enum SD card is FAT with partition table
				FAT_NO_PARTITION_TABLE = 1,				// enum SD card is FAT with no partition table
				FS_UNIVERSAL = 2,						// enum	SD card file system is universal
				FS_OTHER = 3,							// enum	SD card file system is other
			} file_format : 2;							// @11-10	File format as on SD CSD bits
			unsigned ecc : 2;							// @9-8		ecc as on SD CSD bits
			unsigned reserved3 : 1;						// 1 spare bit unused
		};
		uint32_t Raw32_3;								// @0-31	Union to access 32 bits as a uint32_t		
	};
};


/***************************************************************************}
{					      PUBLIC INTERFACE ROUTINES			                }
****************************************************************************/

/*==========================================================================}
{						 PUBLIC RAW SD CARD ROUTINES						}
{==========================================================================*/

/*-[sdInitCard]-------------------------------------------------------------}
. Attempts to initializes current SD Card and returns success/error status.
. This call should be done before any attempt to do anything with a SD card.
. RETURN: SD_OK indicates the current card successfully initialized.
.         !SD_OK if card initialize failed with code identifying error.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
SDRESULT sdInitCard (printhandler prn_basic, printhandler prn_error, bool mount);

/*-[sdCardCSD]--------------------------------------------------------------}
. Returns the pointer to the CSD structure for the current SD Card.
. RETURN: Valid CSD pointer if the current card successfully initialized 
.         NULL if current card initialize not called or failed.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
struct CSD* sdCardCSD (void);

/*-[sdTransferBlocks]-------------------------------------------------------}
. Transfer the count blocks starting at given block to/from SD Card.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
SDRESULT sdTransferBlocks (uint32_t startBlock, uint32_t numBlocks, uint8_t* buffer, bool write );

/*-[sdClearBlocks]----------------------------------------------------------}
. Clears the count blocks starting at given block from SD Card.
. 21Aug17 LdB
.--------------------------------------------------------------------------*/
SDRESULT sdClearBlocks (uint32_t startBlock, uint32_t numBlocks);


/*==========================================================================}
{						 PUBLIC FILE SEARCH ROUTINES						}
{==========================================================================*/

/*--------------------------------------------------------------------------}
{					  PUBLIC FILE ATTRIBUTE CONSTANTS			            }
{--------------------------------------------------------------------------*/
#define FILE_ATTRIBUTE_READONLY		0x01							// Read only file
#define FILE_ATTRIBUTE_HIDDEN		0x02							// Hidden file
#define FILE_ATTRIBUTE_SYSTEM		0x04							// System file
#define FILE_ATTRIBUTE_LABEL		0x08							// Label
#define FILE_ATTRIBUTE_DIRECTORY	0x10							// Directory
#define FILE_ATTRIBUTE_ARCHIVE		0x20							// Archive file
#define FILE_ATTRIBUTE_DEVICE		0x40							// Device file
#define FILE_ATTRIBUTE_NORMAL		0x80							// Normal file

typedef char LFN_NAME[256];											// Long file name
typedef char SFN_NAME[11];											// Short file name

/*--------------------------------------------------------------------------}
{					  PUBLIC SEARCH ROUTINE STRUCTURE		 		        }
{--------------------------------------------------------------------------*/
typedef struct FIND_DATA {
	struct tm	CreateDT;											// Creation date/time
	struct tm	LastAccessDate;										// Last access time
	struct tm	WriteDT;											// Last write date/time
	uint32_t	dwFileAttributes;									// File attributes
	uint32_t    nFileSizeHigh;										// High 32 bits of file size
	uint32_t    nFileSizeLow;										// Low 32 bits of file size
	LFN_NAME	cFileName;											// Long filename
	SFN_NAME	cAlternateFileName;									// 8:3 filename		
} FIND_DATA;

/*-[sdFindFirstFile]--------------------------------------------------------}
. This is an exact replica of Windows FindFirst function but restricted to 
. this SD card.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
HANDLE sdFindFirstFile (const char* lpFileName, FIND_DATA* lpFFData);

/*-[sdFindNextFile]---------------------------------------------------------}
. Continues a file search from a previous call to the FindFirstFile.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
HANDLE sdFindNextFile (HANDLE hFindFile, FIND_DATA* lpFFData);

/*-[sdFindClose]------------------------------------------------------------}
. Closes and releases a file search handle opened by the FindFirstFile.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
bool sdFindClose (HANDLE hFindFile);

/*==========================================================================}
{						 PUBLIC FILE IO ROUTINES							}
{==========================================================================*/

/*--------------------------------------------------------------------------}
{					  FILE CREATEFILE CONTROL FLAGS  				        }
{--------------------------------------------------------------------------*/
#define GENERIC_READ        0x80000000								// Provide GENERIC_READ definition
#define GENERIC_WRITE       0x40000000								// Provide GENERIC_WRITE definition

#define CREATE_NEW          1										// Create new file
#define CREATE_ALWAYS       2										// Create always
#define OPEN_EXISTING       3										// Open existing
#define OPEN_ALWAYS         4										// Open always

/*-[sdCreateFile]-----------------------------------------------------------}
. Creates or opens a file on the SD Card. The function returns a handle that
. can be used to access the file for followup I/O operations. The file will 
. be opened with flags and attributes specified.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
HANDLE sdCreateFile (const char* lpFileName,						// Filename or device to open
					 uint32_t	dwDesiredAccess,					// GENERIC_READ or GENERIC_WRITE
					 uint32_t	dwShareMode,						// Currently not supported (use 0)
					 uint32_t	lpSecurityAttributes,				// Currently not supported (use 0)
					 uint32_t	dwCreationDisposition,				// CREATE_ALWAYS, CREATE_NEW, OPEN_EXISTING, OPEN_ALWAYS
					 uint32_t	dwFlagsAndAttributes,				// Standard file attributes
					 HANDLE hTemplateFile);							// Currently not supported (use 0)

/*-[sdReadFile]-------------------------------------------------------------}
. Reads data from the specified file on the SD Card. The file must have been 
. opened with sdCreateFile and the handle is returned from that function.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
bool sdReadFile (HANDLE hFile,										// Handle as returned from CreateFile
				 void* lpBuffer,									// Pointer to buffer read data will be placed
				 uint32_t nNumberOfBytesToRead,						// Number of bytes requested to read
				 uint32_t* lpNumberOfBytesRead,						// Provide a pointer to a value which updated to bytes actually placed in buffer
				 void* lpOverlapped);								// Currently not supported (use 0)

/*-[sdCloseHandle]------------------------------------------------------------}
. Closes file on the SD Card as per the handle that was given when it opened.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
bool sdCloseHandle (HANDLE hFile);									// Handle as returned from CreateFile


/*--------------------------------------------------------------------------}
{					  SET FILE POINTER CONTROL FLAGS  				        }
{--------------------------------------------------------------------------*/
#define  FILE_BEGIN		0
#define  FILE_CURRENT	1
#define	 FILE_END		2
#define INVALID_SET_FILE_POINTER 0xFFFFFFFF

/*-[sdSetFilePointer]-------------------------------------------------------}
. The function stores the file pointer in two LONG values. To work with file 
. pointers that are larger than a single LONG value, it is easier to use the 
. SetFilePointerEx function.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
uint32_t sdSetFilePointer (HANDLE hFile,							// Handle as returned from CreateFile
						   uint32_t lDistanceToMove,				// low order 32-bit signed value of bytes to move the file pointer
						   uint32_t* lpDistanceToMoveHigh,			// A pointer to high order 32-bits of the signed 64-bit distance to move
						   uint32_t dwMoveMethod);					// FILE_BEGIN, FILE_CURRENT, FILE_END

/*-[sdGetFileSize]----------------------------------------------------------}
. Retrieves the size of the specified file, in bytes. As we don't support
. individual file size beyond 4GB lpFileSizeHigh is unused.
. 23Feb17 LdB
.--------------------------------------------------------------------------*/
uint32_t sdGetFileSize (HANDLE  hFile, uint32_t* lpFileSizeHigh);

#endif // SDCARD_H
