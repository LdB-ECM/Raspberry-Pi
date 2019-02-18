#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc
#include <string.h>								// Needed for strlen, strcpy
#include <ctype.h>
#include "rpi-smartstart.h"						// Need for mailbox
#include "rpi-GLES.h"

/* REFERENCES */
/* https://docs.broadcom.com/docs/12358545 */
/* http://latchup.blogspot.com.au/2016/02/life-of-triangle.html */
/* https://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/vc4/vc4_draw.c */

#define v3d ((volatile __attribute__((aligned(4))) uint32_t*)(uintptr_t)(RPi_IO_Base_Addr + 0xc00000))

/* Registers shamelessly copied from Eric AnHolt */

// Defines for v3d register offsets
#define V3D_IDENT0  (0x000>>2) // V3D Identification 0 (V3D block identity)
#define V3D_IDENT1  (0x004>>2) // V3D Identification 1 (V3D Configuration A)
#define V3D_IDENT2  (0x008>>2) // V3D Identification 1 (V3D Configuration B)

#define V3D_SCRATCH (0x010>>2) // Scratch Register

#define V3D_L2CACTL (0x020>>2) // 2 Cache Control
#define V3D_SLCACTL (0x024>>2) // Slices Cache Control

#define V3D_INTCTL  (0x030>>2) // Interrupt Control
#define V3D_INTENA  (0x034>>2) // Interrupt Enables
#define V3D_INTDIS  (0x038>>2) // Interrupt Disables

#define V3D_CT0CS   (0x100>>2) // Control List Executor Thread 0 Control and Status.
#define V3D_CT1CS   (0x104>>2) // Control List Executor Thread 1 Control and Status.
#define V3D_CT0EA   (0x108>>2) // Control List Executor Thread 0 End Address.
#define V3D_CT1EA   (0x10c>>2) // Control List Executor Thread 1 End Address.
#define V3D_CT0CA   (0x110>>2) // Control List Executor Thread 0 Current Address.
#define V3D_CT1CA   (0x114>>2) // Control List Executor Thread 1 Current Address.
#define V3D_CT00RA0 (0x118>>2) // Control List Executor Thread 0 Return Address.
#define V3D_CT01RA0 (0x11c>>2) // Control List Executor Thread 1 Return Address.
#define V3D_CT0LC   (0x120>>2) // Control List Executor Thread 0 List Counter
#define V3D_CT1LC   (0x124>>2) // Control List Executor Thread 1 List Counter
#define V3D_CT0PC   (0x128>>2) // Control List Executor Thread 0 Primitive List Counter
#define V3D_CT1PC   (0x12c>>2) // Control List Executor Thread 1 Primitive List Counter

#define V3D_PCS     (0x130>>2) // V3D Pipeline Control and Status
#define V3D_BFC     (0x134>>2) // Binning Mode Flush Count
#define V3D_RFC     (0x138>>2) // Rendering Mode Frame Count

#define V3D_BPCA    (0x300>>2) // Current Address of Binning Memory Pool
#define V3D_BPCS    (0x304>>2) // Remaining Size of Binning Memory Pool
#define V3D_BPOA    (0x308>>2) // Address of Overspill Binning Memory Block
#define V3D_BPOS    (0x30c>>2) // Size of Overspill Binning Memory Block
#define V3D_BXCF    (0x310>>2) // Binner Debug

#define V3D_SQRSV0  (0x410>>2) // Reserve QPUs 0-7
#define V3D_SQRSV1  (0x414>>2) // Reserve QPUs 8-15
#define V3D_SQCNTL  (0x418>>2) // QPU Scheduler Control

#define V3D_SRQPC   (0x430>>2) // QPU User Program Request Program Address
#define V3D_SRQUA   (0x434>>2) // QPU User Program Request Uniforms Address
#define V3D_SRQUL   (0x438>>2) // QPU User Program Request Uniforms Length
#define V3D_SRQCS   (0x43c>>2) // QPU User Program Request Control and Status

#define V3D_VPACNTL (0x500>>2) // VPM Allocator Control
#define V3D_VPMBASE (0x504>>2) // VPM base (user) memory reservation

#define V3D_PCTRC   (0x670>>2) // Performance Counter Clear
#define V3D_PCTRE   (0x674>>2) // Performance Counter Enables

#define V3D_PCTR0   (0x680>>2) // Performance Counter Count 0
#define V3D_PCTRS0  (0x684>>2) // Performance Counter Mapping 0
#define V3D_PCTR1   (0x688>>2) // Performance Counter Count 1
#define V3D_PCTRS1  (0x68c>>2) // Performance Counter Mapping 1
#define V3D_PCTR2   (0x690>>2) // Performance Counter Count 2
#define V3D_PCTRS2  (0x694>>2) // Performance Counter Mapping 2
#define V3D_PCTR3   (0x698>>2) // Performance Counter Count 3
#define V3D_PCTRS3  (0x69c>>2) // Performance Counter Mapping 3
#define V3D_PCTR4   (0x6a0>>2) // Performance Counter Count 4
#define V3D_PCTRS4  (0x6a4>>2) // Performance Counter Mapping 4
#define V3D_PCTR5   (0x6a8>>2) // Performance Counter Count 5
#define V3D_PCTRS5  (0x6ac>>2) // Performance Counter Mapping 5
#define V3D_PCTR6   (0x6b0>>2) // Performance Counter Count 6
#define V3D_PCTRS6  (0x6b4>>2) // Performance Counter Mapping 6
#define V3D_PCTR7   (0x6b8>>2) // Performance Counter Count 7
#define V3D_PCTRS7  (0x6bc>>2) // Performance Counter Mapping 7 
#define V3D_PCTR8   (0x6c0>>2) // Performance Counter Count 8
#define V3D_PCTRS8  (0x6c4>>2) // Performance Counter Mapping 8
#define V3D_PCTR9   (0x6c8>>2) // Performance Counter Count 9
#define V3D_PCTRS9  (0x6cc>>2) // Performance Counter Mapping 9
#define V3D_PCTR10  (0x6d0>>2) // Performance Counter Count 10
#define V3D_PCTRS10 (0x6d4>>2) // Performance Counter Mapping 10
#define V3D_PCTR11  (0x6d8>>2) // Performance Counter Count 11
#define V3D_PCTRS11 (0x6dc>>2) // Performance Counter Mapping 11
#define V3D_PCTR12  (0x6e0>>2) // Performance Counter Count 12
#define V3D_PCTRS12 (0x6e4>>2) // Performance Counter Mapping 12
#define V3D_PCTR13  (0x6e8>>2) // Performance Counter Count 13
#define V3D_PCTRS13 (0x6ec>>2) // Performance Counter Mapping 13
#define V3D_PCTR14  (0x6f0>>2) // Performance Counter Count 14
#define V3D_PCTRS14 (0x6f4>>2) // Performance Counter Mapping 14
#define V3D_PCTR15  (0x6f8>>2) // Performance Counter Count 15
#define V3D_PCTRS15 (0x6fc>>2) // Performance Counter Mapping 15

#define V3D_DBGE    (0xf00>>2) // PSE Error Signals
#define V3D_FDBGO   (0xf04>>2) // FEP Overrun Error Signals
#define V3D_FDBGB   (0xf08>>2) // FEP Interface Ready and Stall Signals, FEP Busy Signals
#define V3D_FDBGR   (0xf0c>>2) // FEP Internal Ready Signals
#define V3D_FDBGS   (0xf10>>2) // FEP Internal Stall Input Signals

#define V3D_ERRSTAT (0xf20>>2) // Miscellaneous Error Signals (VPM, VDW, VCD, VCM, L2C)

#define ALIGN_128BIT_MASK  0xFFFFFF80


/* primitive typoe in the GL pipline */
typedef enum {
	PRIM_POINT = 0,
	PRIM_LINE = 1,
	PRIM_LINE_LOOP = 2,
	PRIM_LINE_STRIP = 3,
	PRIM_TRIANGLE = 4,
	PRIM_TRIANGLE_STRIP = 5,
	PRIM_TRIANGLE_FAN = 6,
} PRIMITIVE;

/* GL pipe control commands */
typedef enum {
	GL_HALT = 0,
	GL_NOP = 1,
	GL_FLUSH = 4,
	GL_FLUSH_ALL_STATE = 5,
	GL_START_TILE_BINNING = 6,
	GL_INCREMENT_SEMAPHORE = 7,
	GL_WAIT_ON_SEMAPHORE = 8,
	GL_BRANCH = 16,
	GL_BRANCH_TO_SUBLIST = 17,
	GL_RETURN_FROM_SUBLIST = 18,
	GL_STORE_MULTISAMPLE = 24,
	GL_STORE_MULTISAMPLE_END = 25,
	GL_STORE_FULL_TILE_BUFFER = 26,
	GL_RELOAD_FULL_TILE_BUFFER = 27,
	GL_STORE_TILE_BUFFER = 28,
	GL_LOAD_TILE_BUFFER = 29,
	GL_INDEXED_PRIMITIVE_LIST = 32,
	GL_VERTEX_ARRAY_PRIMITIVES = 33,
	GL_VG_COORDINATE_ARRAY_PRIMITIVES = 41,
	GL_COMPRESSED_PRIMITIVE_LIST = 48,
	GL_CLIP_COMPRESSD_PRIMITIVE_LIST = 49,
	GL_PRIMITIVE_LIST_FORMAT = 56,
	GL_SHADER_STATE = 64,
	GL_NV_SHADER_STATE = 65,
	GL_VG_SHADER_STATE = 66,
	GL_VG_INLINE_SHADER_RECORD = 67,
	GL_CONFIG_STATE = 96,
	GL_FLAT_SHADE_FLAGS = 97,
	GL_POINTS_SIZE = 98,
	GL_LINE_WIDTH = 99,
	GL_RHT_X_BOUNDARY = 100,
	GL_DEPTH_OFFSET = 101,
	GL_CLIP_WINDOW = 102,
	GL_VIEWPORT_OFFSET = 103,
	GL_Z_CLIPPING_PLANES = 104,
	GL_CLIPPER_XY_SCALING = 105,
	GL_CLIPPER_Z_ZSCALE_OFFSET = 106, 
	GL_TILE_BINNING_CONFIG = 112,
	GL_TILE_RENDER_CONFIG = 113,
	GL_CLEAR_COLORS = 114,
	GL_TILE_COORDINATES = 115
}  GL_CONTROL;


struct __attribute__((__packed__, aligned(1))) EMITDATA {
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;
	uint8_t byte4;
};

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


/*==========================================================================}
{						 PUBLIC GPU INITIALIZE FUNCTION						}
{==========================================================================*/

/*-[ InitV3D ]--------------------------------------------------------------}
. This must be called before any other function is called in this unit and
. should be called after the any changes to teh ARM system clocks. That is
. because it needs to make settings based on the system clocks.
. RETURN : TRUE if system could be initialized, FALSE if initialize fails
.--------------------------------------------------------------------------*/
bool InitV3D (void) {
	if (mailbox_tag_message(0, 9,
		MAILBOX_TAG_SET_CLOCK_RATE, 8, 8, CLK_V3D_ID, 250000000,	// Set V3D clock to 250Mhz
		MAILBOX_TAG_ENABLE_QPU, 4, 4, 1))							// Enable the QPU untis
	{																// Message was successful
		if (v3d[V3D_IDENT0] == 0x02443356) return true;				// We can read V3D ID number.
	}
	return false;													// Initialize failed
}

/*==========================================================================}
{						  PUBLIC GPU MEMORY FUNCTIONS						}
{==========================================================================*/

/*-[ V3D_mem_alloc ]--------------------------------------------------------}
. Allocates contiguous memory on the GPU with size and alignment in bytes
. and with the properties of the flags.
. RETURN : GPU handle on successs, 0 if allocation fails
.--------------------------------------------------------------------------*/
GPU_HANDLE V3D_mem_alloc (uint32_t size, uint32_t align, V3D_MEMALLOC_FLAGS flags)
{
	uint32_t buffer[6];
	if (mailbox_tag_message(&buffer[0], 6,
		MAILBOX_TAG_ALLOCATE_MEMORY, 12, 12, size, align, flags))	// Allocate memory tag 
	{																// Message was successful
		return buffer[3];											// GPU handle returned										
	}
	return 0;														// Memory allocate failed
}

/*-[ V3D_mem_free ]---------------------------------------------------------}
. All memory associated with the GPU handle is released.
. RETURN : TRUE on successs, FALSE if release fails
.--------------------------------------------------------------------------*/
bool V3D_mem_free (GPU_HANDLE handle)
{
	uint32_t buffer[4] = { 0 };
	if (mailbox_tag_message(0, 4,
		MAILBOX_TAG_RELEASE_MEMORY, 4, 4, handle))					// Release memory tag
	{																// Release was successful
		if (buffer[3] == 0) return true;							// Return true
	}
	return false;													// Return false
}

/*-[ V3D_mem_lock ]---------------------------------------------------------}
. Locks the memory associated to the GPU handle to a GPU bus address.
. RETURN : locked gpu address, 0 if lock fails
.--------------------------------------------------------------------------*/
uint32_t V3D_mem_lock (GPU_HANDLE handle)
{
	uint32_t buffer[4] = { 0 };
	if (mailbox_tag_message(&buffer[0], 4,
		MAILBOX_TAG_LOCK_MEMORY, 4, 4, handle))						// Lock memory tag
	{																// message was successful
		return buffer[3];											// Return the bus address
	}
	return 0;														// Return failure
}

/*-[ V3D_mem_unlock ]-------------------------------------------------------}
. Unlocks the memory associated to the GPU handle from the GPU bus address.
. RETURN : TRUE if sucessful, FALSE if it fails
.--------------------------------------------------------------------------*/
bool V3D_mem_unlock (GPU_HANDLE handle)
{
	uint32_t buffer[4] = { 0 };
	if (mailbox_tag_message(0, 4,
		MAILBOX_TAG_UNLOCK_MEMORY, 4, 4, handle))					// Memory unlock tag
	{																// Message was successful
		if (buffer[3] == 0) return true;							// Return true
	}
	return false;													// Return false
}

bool V3D_execute_code (uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5)
{
	return (mailbox_tag_message(0, 10,
		MAILBOX_TAG_EXECUTE_CODE, 28, 28,
		code, r0, r1, r2, r3, r4, r5));
}

bool V3D_execute_qpu (int32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout) 
{
	return (mailbox_tag_message(0, 7,
		MAILBOX_TAG_EXECUTE_QPU, 16, 16,
		num_qpus, control, noflush, timeout));
}

static void emit_uint8_t(uint8_t **list, uint8_t d) {
	*((*list)++) = d;
}

static void emit_uint16_t (uint8_t **list, uint16_t d) {
	struct EMITDATA* data = (struct EMITDATA*)&d;
	*((*list)++) = (*data).byte1;
	*((*list)++) = (*data).byte2;
}

static void emit_uint32_t(uint8_t **list, uint32_t d) {
	struct EMITDATA* data = (struct EMITDATA*)&d;
	*((*list)++) = (*data).byte1;
	*((*list)++) = (*data).byte2;
	*((*list)++) = (*data).byte3;
	*((*list)++) = (*data).byte4;
}

static void emit_float(uint8_t **list, float f) {
	struct EMITDATA* data = (struct EMITDATA*)&f;
	*((*list)++) = (*data).byte1;
	*((*list)++) = (*data).byte2;
	*((*list)++) = (*data).byte3;
	*((*list)++) = (*data).byte4;
}


bool V3D_InitializeScene (RENDER_STRUCT* scene, uint32_t renderWth, uint32_t renderHt )
{
	if (scene) 
	{
		scene->rendererHandle = V3D_mem_alloc(0x10000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
		if (!scene->rendererHandle) return false;
		scene->rendererDataVC4 = V3D_mem_lock(scene->rendererHandle);
		scene->loadpos = scene->rendererDataVC4;					// VC4 load from start of memory

		scene->renderWth = renderWth;								// Render width
		scene->renderHt = renderHt;									// Render height
		scene->binWth = (renderWth + 63) / 64;						// Tiles across 
		scene->binHt = (renderHt + 63) / 64;						// Tiles down 

		scene->tileMemSize = 0x4000;
		scene->tileHandle = V3D_mem_alloc(scene->tileMemSize + 0x4000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
		scene->tileStateDataVC4 = V3D_mem_lock(scene->tileHandle);
		scene->tileDataBufferVC4 = scene->tileStateDataVC4 + 0x4000;

		scene->binningHandle = V3D_mem_alloc(0x10000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
		scene->binningDataVC4 = V3D_mem_lock(scene->binningHandle);
		return true;
	}
	return false;
}

bool V3D_AddVertexesToScene (RENDER_STRUCT* scene, int vertex_count)
{
	if (scene) 
	{
		scene->vertexVC4 = (scene->loadpos + 127) & ALIGN_128BIT_MASK;	// Hold vertex start adderss .. aligned to 128bits

		scene->loadpos = scene->vertexVC4 + (vertex_count * 24);		// Update load position for vertex_count * 24 (2+2+4+4+4+4+4 bytes)

		scene->indexVertexVC4 = (scene->loadpos + 127) & ALIGN_128BIT_MASK;// Hold index vertex start adderss .. align it to 128 bits

		scene->loadpos = scene->indexVertexVC4 + (vertex_count * 6);	// Move loaad pos to new position
		return true;
	}
	return false;
}


bool V3D_AddShadderToScene (RENDER_STRUCT* scene, uint32_t* frag_shader, uint32_t frag_shader_emits)
{
	if (scene)
	{
		scene->shaderStart = (scene->loadpos + 127) & ALIGN_128BIT_MASK;// Hold shader start adderss .. aligned to 127 bits
		uint8_t *p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->shaderStart);	// ARM address for load
		uint8_t *q = p;												// Hold start address

		for (int i = 0; i < frag_shader_emits; i++)					// For the number of fragment shader emits
			emit_uint32_t(&p, frag_shader[i]);						// Emit fragment shader into our allocated memory

		scene->loadpos = scene->shaderStart + (p - q);				// Update load position

		scene->fragShaderRecStart = (scene->loadpos + 127) & ALIGN_128BIT_MASK;// Hold frag shader start adderss .. .aligned to 128bits
		p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->fragShaderRecStart);
		q = p;

		// Okay now we need Shader Record to buffer
		emit_uint8_t(&p, 0x01);										// flags
		emit_uint8_t(&p, 6 * 4);									// stride
		emit_uint8_t(&p, 0xcc);										// num uniforms (not used)
		emit_uint8_t(&p, 3);										// num varyings
		emit_uint32_t(&p, scene->shaderStart);						// Shader code address
		emit_uint32_t(&p, 0);										// Fragment shader uniforms (not in use)
		emit_uint32_t(&p, scene->vertexVC4);						// Vertex Data

		scene->loadpos = scene->fragShaderRecStart + (p - q);		// Adjust VC4 load poistion

		return true;
	}
	return false;
}


bool V3D_SetupRenderControl (RENDER_STRUCT* scene, VC4_ADDR renderBufferAddr)
{
	if (scene)
	{
		scene->renderControlVC4 = (scene->loadpos + 127) & ALIGN_128BIT_MASK;// Hold render control start adderss .. aligned to 128 bits
		uint8_t *p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->renderControlVC4); // ARM address for load
		uint8_t *q = p;												// Hold start address

		// Clear colors
		emit_uint8_t(&p, GL_CLEAR_COLORS);
		emit_uint32_t(&p, 0xff000000);								// Opaque Black
		emit_uint32_t(&p, 0xff000000);								// 32 bit clear colours need to be repeated twice
		emit_uint32_t(&p, 0);
		emit_uint8_t(&p, 0);

		// Tile Rendering Mode Configuration
		emit_uint8_t(&p, GL_TILE_RENDER_CONFIG);

		emit_uint32_t(&p, renderBufferAddr);						// render address (will be framebuffer)

		emit_uint16_t(&p, scene->renderWth);						// render width
		emit_uint16_t(&p, scene->renderHt);							// render height
		emit_uint8_t(&p, 0x04);										// framebuffer mode (linear rgba8888)
		emit_uint8_t(&p, 0x00);

		// Do a store of the first tile to force the tile buffer to be cleared
		// Tile Coordinates
		emit_uint8_t(&p, GL_TILE_COORDINATES);
		emit_uint8_t(&p, 0);
		emit_uint8_t(&p, 0);

		// Store Tile Buffer General
		emit_uint8_t(&p, GL_STORE_TILE_BUFFER);
		emit_uint16_t(&p, 0);										// Store nothing (just clear)
		emit_uint32_t(&p, 0);										// no address is needed

		// Link all binned lists together
		for (int x = 0; x < scene->binWth; x++) {
			for (int y = 0; y < scene->binHt; y++) {

				// Tile Coordinates
				emit_uint8_t(&p, GL_TILE_COORDINATES);
				emit_uint8_t(&p, x);
				emit_uint8_t(&p, y);

				// Call Tile sublist
				emit_uint8_t(&p, GL_BRANCH_TO_SUBLIST);
				emit_uint32_t(&p, scene->tileDataBufferVC4 + (y * scene->binWth + x) * 32);

				// Last tile needs a special store instruction
				if (x == (scene->binWth - 1) && (y == scene->binHt - 1)) {
					// Store resolved tile color buffer and signal end of frame
					emit_uint8_t(&p, GL_STORE_MULTISAMPLE_END);
				}
				else {
					// Store resolved tile color buffer
					emit_uint8_t(&p, GL_STORE_MULTISAMPLE);
				}
			}
		}

		scene->loadpos = scene->renderControlVC4 + (p - q);			// Adjust VC4 load poistion
		scene->renderControlEndVC4 = scene->loadpos;				// Hold end of render control data

		return true;
	}
	return false;
}


bool V3D_SetupBinningConfig (RENDER_STRUCT* scene) 
{
	if (scene)
	{
		uint8_t *p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->binningDataVC4); // ARM address for binning data load
		uint8_t *list = p;												// Hold start address

		emit_uint8_t(&p, GL_TILE_BINNING_CONFIG);						// tile binning config control 
		emit_uint32_t(&p, scene->tileDataBufferVC4);					// tile allocation memory address
		emit_uint32_t(&p, scene->tileMemSize);							// tile allocation memory size
		emit_uint32_t(&p, scene->tileStateDataVC4);						// Tile state data address
		emit_uint8_t(&p, scene->binWth);								// renderWidth/64
		emit_uint8_t(&p, scene->binHt);									// renderHt/64
		emit_uint8_t(&p, 0x04);											// config

		// Start tile binning.
		emit_uint8_t(&p, GL_START_TILE_BINNING);						// Start binning command

		// Primitive type
		emit_uint8_t(&p, GL_PRIMITIVE_LIST_FORMAT);
		emit_uint8_t(&p, 0x32);											// 16 bit triangle

		// Clip Window
		emit_uint8_t(&p, GL_CLIP_WINDOW);								// Clip window 
		emit_uint16_t(&p, 0);											// 0
		emit_uint16_t(&p, 0);											// 0
		emit_uint16_t(&p, scene->renderWth);							// width
		emit_uint16_t(&p, scene->renderHt);								// height

		// GL State
		emit_uint8_t(&p, GL_CONFIG_STATE);
		emit_uint8_t(&p, 0x03);											// enable both foward and back facing polygons
		emit_uint8_t(&p, 0x00);											// depth testing disabled
		emit_uint8_t(&p, 0x02);											// enable early depth write

		// Viewport offset
		emit_uint8_t(&p, GL_VIEWPORT_OFFSET);							// Viewport offset
		emit_uint16_t(&p, 0);											// 0
		emit_uint16_t(&p, 0);											// 0

		// The triangle
		// No Vertex Shader state (takes pre-transformed vertexes so we don't have to supply a working coordinate shader.)
		emit_uint8_t(&p, GL_NV_SHADER_STATE);
		emit_uint32_t(&p, scene->fragShaderRecStart);					// Shader Record

		// primitive index list
#define INDEX_TYPE_8  0x00   //  Indexed_Primitive_List: Index Type = 8 - Bit
#define INDEX_TYPE_16 0x10   //  Indexed_Primitive_List: Index Type = 16 - Bit
		emit_uint8_t(&p, GL_INDEXED_PRIMITIVE_LIST);					// Indexed primitive list command
		emit_uint8_t(&p, PRIM_TRIANGLE | INDEX_TYPE_16);				// 8bit index, triangles
		emit_uint32_t(&p, scene->IndexVertexCt);						// Number of index vertex
		emit_uint32_t(&p, scene->indexVertexVC4);						// Address of index vertex data
		emit_uint32_t(&p, scene->MaxIndexVertex);						// Maximum index


		// End of bin list
		// So Flush
		emit_uint8_t(&p, GL_FLUSH_ALL_STATE);
		// Nop
		emit_uint8_t(&p, GL_NOP);
		// Halt
		emit_uint8_t(&p, GL_HALT);
		scene->binningCfgEnd = scene->binningDataVC4 + (p-list);		// Hold binning data end address

		return true;
	}
	return false;
}



/*-[ V3D_RenderScene ]------------------------------------------------------}
. Pretty obvious asks the VC4 to renders the given scene
.--------------------------------------------------------------------------*/
void V3D_RenderScene (RENDER_STRUCT* scene) 
{
	if (scene) {
		// clear caches
		v3d[V3D_L2CACTL] = 4;
		v3d[V3D_SLCACTL] = 0x0F0F0F0F;

		// stop the thread
		v3d[V3D_CT0CS] = 0x20;
		// wait for it to stop
		while (v3d[V3D_CT0CS] & 0x20);

		// Run our control list
		v3d[V3D_BFC] = 1;											// reset binning frame count
		v3d[V3D_CT0CA] = scene->binningDataVC4;						// Start binning config address
		v3d[V3D_CT0EA] = scene->binningCfgEnd;						// End binning config address is at render control start

		// wait for binning to finish
		while (v3d[V3D_BFC] == 0) {}

		// stop the thread
		v3d[V3D_CT1CS] = 0x20;

		// Wait for thread to stop
		while (v3d[V3D_CT1CS] & 0x20);

		// Run our render
		v3d[V3D_RFC] = 1;											// reset rendering frame count
		v3d[V3D_CT1CA] = scene->renderControlVC4;					// Start address for render control
		v3d[V3D_CT1EA] = scene->renderControlEndVC4;				// End address for render control

		// wait for render to finish
		while (v3d[V3D_RFC] == 0) {}

	}
}



bool V3D_AddWindowToScene (RENDER_STRUCT* scene, int window_index, int x1, int y1, int x2, int y2, RGBA colour)
{
	if (scene)
	{
		double r, g, b;
		uint16_t xa, ya, xb, yb;
		uint8_t * p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->vertexVC4); // Start of vertex index
		p += (window_index * (4 * 24));									// Adjust vertex position

		r = colour.rgbRed;
		r /= 255.0;
		g = colour.rgbGreen;
		g /= 255.0;
		b = colour.rgbBlue;
		b /= 255.0;

		xa = (x1 < 0) ? 0 : (x1 > 0xFFFF) ? 0xFFFF : x1;
		ya = (y1 < 0) ? 0 : (y1 > 0xFFFF) ? 0xFFFF : y1;
		xb = (x2 < 0) ? 0 : (x2 > 0xFFFF) ? 0xFFFF : x2;
		yb = (y2 < 0) ? 0 : (y2 > 0xFFFF) ? 0xFFFF : y2;


		// Vertex: Top left
		emit_uint16_t(&p, xa << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, ya << 4);										// Y in 12.4 fixed point
		emit_float(&p, 1.0f);											// Z
		emit_float(&p, 1.0f);											// 1/W
		emit_float(&p, r);												// Varying 0 (Red)
		emit_float(&p, g);												// Varying 1 (Green)
		emit_float(&p, b);												// Varying 2 (Blue)

		// Vertex: top right
		emit_uint16_t(&p, xb << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, ya << 4);										// Y in 12.4 fixed point
		emit_float(&p, 1.0f);											// Z
		emit_float(&p, 1.0f);											// 1/W
		emit_float(&p, r);												// Varying 0 (Red)
		emit_float(&p, g);												// Varying 1 (Green)
		emit_float(&p, b);												// Varying 2 (Blue)

		// Vertex: bottom right 
		emit_uint16_t(&p, xb << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, yb << 4);										// Y in 12.4 fixed point
		emit_float(&p, 1.0f);											// Z
		emit_float(&p, 1.0f);											// 1/W
		emit_float(&p, r);												// Varying 0 (Red)
		emit_float(&p, g);												// Varying 1 (Green)
		emit_float(&p, b);												// Varying 2 (Blue)


		// Vertex: bottom left
		emit_uint16_t(&p, xa << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, yb << 4);										// Y in 12.4 fixed point
		emit_float(&p, 1.0f);											// Z
		emit_float(&p, 1.0f);											// 1/W
		emit_float(&p, r);												// Varying 0 (Red)
		emit_float(&p, g);												// Varying 1 (Green)
		emit_float(&p, b);												// Varying 2 (Blue)


		scene->num_verts += 4;

		p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->indexVertexVC4);
		p += (window_index * 12);										// Adjust vertex position

		uint32_t w = window_index * 4;
		emit_uint16_t(&p, w);											// quad - top left
		emit_uint16_t(&p, w+3);											// quad - bottom left
		emit_uint16_t(&p, w+1);											// quad - top right

		emit_uint16_t(&p, w+3);											// quad - bottom left
		emit_uint16_t(&p, w+2);											// quad - bottom right
		emit_uint16_t(&p, w+1);											// quad - top right
		
		scene->IndexVertexCt += 6;
		scene->MaxIndexVertex = w+4;

		return true;
	}
	return false;
}


bool V3D_MoveWindowInScene(RENDER_STRUCT* scene, int window_index, int x1, int y1, int x2, int y2)
{
	if (scene)
	{
		uint16_t xa, ya, xb, yb;
		uint8_t* p = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(scene->vertexVC4); // Start of vertex index
		p += (window_index * (4 * 24));									// Adjust vertex position

		xa = (x1 < 0) ? 0 : (x1 > 0xFFFF) ? 0xFFFF : x1;
		ya = (y1 < 0) ? 0 : (y1 > 0xFFFF) ? 0xFFFF : y1;
		xb = (x2 < 0) ? 0 : (x2 > 0xFFFF) ? 0xFFFF : x2;
		yb = (y2 < 0) ? 0 : (y2 > 0xFFFF) ? 0xFFFF : y2;

		// Vertex: Top left
		emit_uint16_t(&p, xa << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, ya << 4);										// Y in 12.4 fixed point
		p += 20;

		// Vertex: top right
		emit_uint16_t(&p, xb << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, ya << 4);										// Y in 12.4 fixed point
		p += 20;

		// Vertex: bottom right 
		emit_uint16_t(&p, xb << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, yb << 4);										// Y in 12.4 fixed point
		p += 20;


		// Vertex: bottom left
		emit_uint16_t(&p, xa << 4);										// X in 12.4 fixed point
		emit_uint16_t(&p, yb << 4);										// Y in 12.4 fixed point
		p += 20;

		return true;
	}
	return false;
}



static uint32_t shader1[18] = {  // Vertex Color Shader
		0x958e0dbf, 0xd1724823,   /* mov r0, vary; mov r3.8d, 1.0 */
		0x818e7176, 0x40024821,   /* fadd r0, r0, r5; mov r1, vary */
		0x818e7376, 0x10024862,   /* fadd r1, r1, r5; mov r2, vary */
		0x819e7540, 0x114248a3,   /* fadd r2, r2, r5; mov r3.8a, r0 */
		0x809e7009, 0x115049e3,   /* nop; mov r3.8b, r1 */
		0x809e7012, 0x116049e3,   /* nop; mov r3.8c, r2 */
		0x159e76c0, 0x30020ba7,   /* mov tlbc, r3; nop; thrend */
		0x009e7000, 0x100009e7,   /* nop; nop; nop */
		0x009e7000, 0x500009e7,   /* nop; nop; sbdone */
};

static RENDER_STRUCT scene = { 0 };

static int window_dx[3] = { 1, 1, 1 };
static int window_dy[3] = { 1, 1, 1 };
static int window_xdir[3] = { 1, -1, 1 };
static int window_ydir[3] = { 1, -1, 1 };
static int window_x[3] = { 20, 700, 400 };
static int window_y[3] = { 20, 600, 200 };
static int window_width[3] = { 200, 200, 400 };
static int window_height[3] = { 200, 200, 200 };


#include <stdlib.h> // needed for rand
void Desktop_Start (void)
{
	InitV3D();														// Initialize 3D graphics

	// Step1: Initialize scene
	V3D_InitializeScene(&scene, GetConsole_Width(), GetConsole_Height());

	// Step2: Add vertexes to scene
	V3D_AddVertexesToScene(&scene, 100);


	V3D_AddWindowToScene(&scene, 0, window_x[0], window_y[0], window_x[0] + window_width[0], window_y[0] + window_height[0], (RGBA) { .rgbBlue = 0xFF });
	V3D_AddWindowToScene(&scene, 1, window_x[1], window_y[1], window_x[1] + window_width[1], window_y[1] + window_height[1], (RGBA) { .rgbGreen = 0xFF });
	V3D_AddWindowToScene(&scene, 2, window_x[2], window_y[2], window_x[2] + window_width[2], window_y[2] + window_height[2], (RGBA) { .rgbRed = 0xFF });

	// Step3: Add shader to scene
	V3D_AddShadderToScene(&scene, &shader1[0], _countof(shader1));

	// Step4: Setup render control
	V3D_SetupRenderControl(&scene, GetConsole_FrameBuffer());

	// Step5: Setup binning
	V3D_SetupBinningConfig(&scene);

	// Step 6: Render the scene
	V3D_RenderScene(&scene);

	for (int i = 0; i < 2; i++)
	{
		window_dx[i] = rand() % 10 + 1;
		window_dy[i] = rand() % 10 + 1;
	}
}

void Move_Windows (void)
{
	for (int i = 0; i < 2; i++)
	{
		if (window_xdir[i] > 0) {
			if (window_x[i] + window_width[i] + window_dx[i] < scene.renderWth)
			{
				window_x[i] += window_dx[i];
			}
			else {
				window_x[i] = scene.renderWth - window_width[i];
				window_dx[i] = rand() % 10 + 1;
				window_dy[i] = rand() % 10 + 1;
				window_xdir[i] = -1;
			}
		}
		else {
			if (window_x[i] > window_dx[i]) {
				window_x[i] -= window_dx[i];
			}
			else {
				window_x[i] = 0;
				window_dx[i] = rand() % 10 + 1;
				window_dy[i] = rand() % 10 + 1;
				window_xdir[i] = 1;
			}
		}

		if (window_ydir[i] > 0) {
			if (window_y[i] + window_height[i] + window_dy[i] < scene.renderHt)
			{
				window_y[i] += window_dy[i];
			}
			else {
				window_y[i] = scene.renderHt - window_height[i];
				window_dx[i] = rand() % 10 + 1;
				window_dy[i] = rand() % 10 + 1;
				window_ydir[i] = -1;
			}
		}
		else {
			if (window_y[i] > window_dy[i]) {
				window_y[i] -= window_dy[i];
			}
			else {
				window_y[i] = 0;
				window_dx[i] = rand() % 10 + 1;
				window_dy[i] = rand() % 10 + 1;
				window_ydir[i] = 1;
			}
		}

		V3D_MoveWindowInScene(&scene, i, window_x[i], window_y[i], window_x[i] + window_width[i], window_y[i] + window_height[i]);
	}
	V3D_RenderScene(&scene);
}
