#ifndef _RPI_GLES_
#define _RPI_GLES_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: rpi-GLES.h												}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 1.01														}
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
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc
#include "rpi-smartstart.h"						// Need for mailbox

/*--------------------------------------------------------------------------}
;{		  ENUMERATED GPU MEMORY ALLOCATE FLAGS FROM REFERENCE			  	}
;{--------------------------------------------------------------------------}
;{  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface }
;{-------------------------------------------------------------------------*/
	typedef enum {
		MEM_FLAG_DISCARDABLE = 1 << 0,									/* can be resized to 0 at any time. Use for cached data */
		MEM_FLAG_NORMAL = 0 << 2,										/* normal allocating alias. Don't use from ARM			*/
		MEM_FLAG_DIRECT = 1 << 2,										/* 0xC alias uncached									*/
		MEM_FLAG_COHERENT = 2 << 2,										/* 0x8 alias. Non-allocating in L2 but coherent			*/
		MEM_FLAG_L1_NONALLOCATING = (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT), /* Allocating in L2									*/
		MEM_FLAG_ZERO = 1 << 4,											/* initialise buffer to all zeros						*/
		MEM_FLAG_NO_INIT = 1 << 5,										/* don't initialise (default is initialise to all ones	*/
		MEM_FLAG_HINT_PERMALOCK = 1 << 6,								/* Likely to be locked for long periods of time.		*/
	} V3D_MEMALLOC_FLAGS;

/*--------------------------------------------------------------------------}
;{			  DEFINE A GPU MEMORY HANDLE TO STOP CONFUSION				  	}
;{-------------------------------------------------------------------------*/
typedef uint32_t GPU_HANDLE;
typedef uint32_t VC4_ADDR;


/*--------------------------------------------------------------------------}
;{	    DEFINE A RENDER STRUCTURE ... WHICH JUST HOLD RENDER DETAILS	  	}
;{-------------------------------------------------------------------------*/
typedef struct render_t
{
	/* This is the current load position */
	VC4_ADDR loadpos;							// Physical load address as ARM address

	/* These are all the same thing just handle and two different address GPU and ARM */
	GPU_HANDLE rendererHandle;					// Renderer memory handle
	VC4_ADDR rendererDataVC4;					// Renderer data VC4 locked address
	
	uint16_t renderWth;							// Render width
	uint16_t renderHt;							// Render height

	VC4_ADDR shaderStart;						// VC4 address shader code starts 
	VC4_ADDR fragShaderRecStart;				// VC4 start address for fragment shader record

	uint32_t binWth;							// Bin width
	uint32_t binHt;								// Bin height

	VC4_ADDR renderControlVC4;					// VC4 render control start address
	VC4_ADDR renderControlEndVC4;				// VC4 render control end address

	VC4_ADDR vertexVC4;							// VC4 address to vertex data
	uint32_t num_verts;							// number of vertices 

	VC4_ADDR indexVertexVC4;					// VC4 address to start of index vertex data
	uint32_t IndexVertexCt;						// Index vertex count
	uint32_t MaxIndexVertex;					// Maximum Index vertex referenced

	/* TILE DATA MEMORY ... HAS TO BE 4K ALIGN */
	GPU_HANDLE tileHandle;						// Tile memory handle
	uint32_t  tileMemSize;						// Tiel memory size;
	VC4_ADDR tileStateDataVC4;					// Tile data VC4 locked address
	VC4_ADDR tileDataBufferVC4;					// Tile data buffer VC4 locked address

	/* BINNING DATA MEMORY ... HAS TO BE 4K ALIGN */
	GPU_HANDLE binningHandle;					// Binning memory handle
	VC4_ADDR binningDataVC4;					// Binning data VC4 locked address
	VC4_ADDR binningCfgEnd;						// VC4 binning config end address

} RENDER_STRUCT;

/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*==========================================================================}
{						 PUBLIC GPU INITIALIZE FUNCTION						}
{==========================================================================*/

/*-[ InitV3D ]--------------------------------------------------------------}
. This must be called before any other function is called in this unit and
. should be called after the any changes to teh ARM system clocks. That is
. because it needs to make settings based on the system clocks.
. RETURN : TRUE if system could be initialized, FALSE if initialize fails
.--------------------------------------------------------------------------*/
bool InitV3D (void);

/*==========================================================================}
{						  PUBLIC GPU MEMORY FUNCTIONS						}	
{==========================================================================*/

/*-[ V3D_mem_alloc ]--------------------------------------------------------}
. Allocates contiguous memory on the GPU with size and alignment in bytes
. and with the properties of the flags.
. RETURN : GPU handle on successs, 0 if allocation fails
.--------------------------------------------------------------------------*/
GPU_HANDLE V3D_mem_alloc (uint32_t size, uint32_t align, V3D_MEMALLOC_FLAGS flags);


/*-[ V3D_mem_free ]---------------------------------------------------------}
. All memory associated with the GPU handle is released.
. RETURN : TRUE on successs, FALSE if release fails
.--------------------------------------------------------------------------*/
bool V3D_mem_free (GPU_HANDLE handle);

/*-[ V3D_mem_lock ]---------------------------------------------------------}
. Locks the memory associated to the GPU handle to a GPU bus address.
. RETURN : locked gpu address, 0 if lock fails
.--------------------------------------------------------------------------*/
uint32_t V3D_mem_lock (GPU_HANDLE handle);

/*-[ V3D_mem_unlock ]-------------------------------------------------------}
. Unlocks the memory associated to the GPU handle from the GPU bus address.
. RETURN : TRUE if sucessful, FALSE if it fails
.--------------------------------------------------------------------------*/
bool V3D_mem_unlock (GPU_HANDLE handle);


bool V3D_execute_code (uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);
bool V3D_execute_qpu (int32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);


bool V3D_InitializeScene (RENDER_STRUCT* scene, uint32_t renderWth, uint32_t renderHt);
bool V3D_AddVertexesToScene (RENDER_STRUCT* scene);
bool V3D_AddShadderToScene (RENDER_STRUCT* scene, uint32_t* frag_shader, uint32_t frag_shader_emits);
bool V3D_SetupRenderControl(RENDER_STRUCT* scene, VC4_ADDR renderBufferAddr);
bool V3D_SetupBinningConfig (RENDER_STRUCT* scene);

void V3D_RenderScene (RENDER_STRUCT* scene);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif