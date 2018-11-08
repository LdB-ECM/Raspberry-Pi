#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc
#include <math.h>
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


// Flags for allocate memory.
enum {
	MEM_FLAG_DISCARDABLE = 1 << 0,				/* can be resized to 0 at any time. Use for cached data */
	MEM_FLAG_NORMAL = 0 << 2,					/* normal allocating alias. Don't use from ARM */
	MEM_FLAG_DIRECT = 1 << 2,					/* 0xC alias uncached */
	MEM_FLAG_COHERENT = 2 << 2,					/* 0x8 alias. Non-allocating in L2 but coherent */
	MEM_FLAG_L1_NONALLOCATING = (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT), /* Allocating in L2 */
	MEM_FLAG_ZERO = 1 << 4,						/* initialise buffer to all zeros */
	MEM_FLAG_NO_INIT = 1 << 5,					/* don't initialise (default is initialise to all ones */
	MEM_FLAG_HINT_PERMALOCK = 1 << 6,			/* Likely to be locked for long periods of time. */
};

/* primitive typo\e in the GL pipline */
typedef enum {
	PRIM_POINT = 0,
	PRIM_LINE = 1,
	PRIM_LINE_LOOP = 2,
	PRIM_LINE_STRIP = 3,
	PRIM_TRIANGLE = 4,
	PRIM_TRIANGLE_STRIP = 5,
	PRIM_TRIANGLE_FAN = 6,
} PRIMITIVE;


/* These come from the blob information header .. they start at   KHRN_HW_INSTR_HALT */
/* https://github.com/raspberrypi/userland/blob/a1b89e91f393c7134b4cdc36431f863bb3333163/middleware/khronos/common/2708/khrn_prod_4.h */
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


/* Our vertex shader text we will compile */
char vShaderStr[] =
"uniform mat4 u_rotateMx;  \n"
"attribute vec4 a_position;   \n"
"attribute vec2 a_texCoord;   \n"
"varying vec2 v_texCoord;     \n"
"void main()                  \n"
"{                            \n"
"   gl_Position = u_rotateMx * a_position; \n"
"   v_texCoord = a_texCoord;  \n"
"}                            \n";

/* Our fragment shader text we will compile */
char fShaderStr[] =
"precision mediump float;                            \n"
"varying vec2 v_texCoord;                            \n"
"uniform sampler2D s_texture;                        \n"
"void main()                                         \n"
"{                                                   \n"
"  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
"}                                                   \n";

bool InitV3D (void) {
	if (mailbox_tag_message(0, 9,
		MAILBOX_TAG_SET_CLOCK_RATE, 8, 8,
		CLK_V3D_ID, 250000000,
		MAILBOX_TAG_ENABLE_QPU, 4, 4, 1)) {
		if (v3d[V3D_IDENT0] == 0x02443356) { // Magic number.
			return true;
		}
	}
	return false;
}

uint32_t V3D_mem_alloc (uint32_t size, uint32_t align, uint32_t flags)
{
	uint32_t buffer[6];
	if (mailbox_tag_message(&buffer[0], 6,
		MAILBOX_TAG_ALLOCATE_MEMORY, 12, 12, size, align, flags)) {
		return buffer[3];
	}
	return 0;
}

bool V3D_mem_free (uint32_t handle)
{
	return (mailbox_tag_message(0, 4,
		MAILBOX_TAG_RELEASE_MEMORY, 4, 4, handle));
}

uint32_t V3D_mem_lock (uint32_t handle)
{
	uint32_t buffer[4];
	if (mailbox_tag_message(&buffer[0], 4,
		MAILBOX_TAG_LOCK_MEMORY, 4, 4, handle)) {
		return buffer[3];
	}
	return 0;
}

bool V3D_mem_unlock (uint32_t handle)
{
	return (mailbox_tag_message(0, 4,
		MAILBOX_TAG_UNLOCK_MEMORY, 4, 4, handle));
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


float cosTheta = 1.0f;
//float sinTheta = 0.0f;
float angle = 0.0f;

static int32_t rotate_x (uint_fast32_t xOld) {
	return (int32_t)(xOld * cosTheta);
}

static int32_t rotate_y(uint_fast32_t yOld) {
	return (int32_t)(yOld * cosTheta);
}

void DoRotate (float delta) {
	angle += delta;
	if (angle >= (3.1415926384 * 2)) angle -= (3.1415926384 * 2);
	cosTheta = cosf(angle);
	//sinTheta = sinf(angle);
}

// Render a single triangle to memory.
void testTriangle (uint16_t renderWth, uint16_t renderHt, uint32_t renderBufferAddr, printhandler prn_handler) {
	//  We allocate/lock some videocore memory
	// I'm just shoving everything in a single buffer because I'm lazy 8Mb, 4k alignment
	// Normally you would do individual allocations but not sure of interface I want yet
	// So lets just define some address in that one single buffer for now 
	// You need to make sure they don't overlap if you expand sample
	#define BUFFER_VERTEX_INDEX		0x70 
	#define BUFFER_SHADER_OFFSET	0x80
	#define BUFFER_VERTEX_DATA		0x100 
	#define BUFFER_TILE_STATE		0x200
	#define BUFFER_TILE_DATA		0x6000
	#define BUFFER_RENDER_CONTROL	0xe200
	#define BUFFER_FRAGMENT_SHADER	0xfe00
	#define BUFFER_FRAGMENT_UNIFORM	0xff00

	uint32_t handle = V3D_mem_alloc(0x800000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
	if (!handle) {
		if (prn_handler) prn_handler("Error: Unable to allocate memory");
		return;
	}
	uint32_t bus_addr = V3D_mem_lock(handle);
	uint8_t *list = (uint8_t*)(uintptr_t)GPUaddrToARMaddr(bus_addr);

	uint8_t *p = list;

	// Configuration stuff
	// Tile Binning Configuration.
	//   Tile state data is 48 bytes per tile, I think it can be thrown away
	//   as soon as binning is finished.
	//   A tile itself is 64 x 64 pixels 
	//   we will need binWth of them across to cover the render width
	//   we will need binHt of them down to cover the render height
	//   we add 63 before the division because any fraction at end must have a bin
	uint_fast32_t binWth = (renderWth + 63) / 64;					// Tiles across 
	uint_fast32_t binHt = (renderHt + 63) / 64;						// Tiles down 

	emit_uint8_t(&p, GL_TILE_BINNING_CONFIG);						// tile binning config control 
	emit_uint32_t(&p, bus_addr + BUFFER_TILE_DATA);					// tile allocation memory address
	emit_uint32_t(&p, 0x4000);										// tile allocation memory size
	emit_uint32_t(&p, bus_addr + BUFFER_TILE_STATE);				// Tile state data address
	emit_uint8_t(&p, binWth);										// renderWidth/64
	emit_uint8_t(&p, binHt);										// renderHt/64
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
	emit_uint16_t(&p, renderWth);									// width
	emit_uint16_t(&p, renderHt);									// height

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
	emit_uint32_t(&p, bus_addr + BUFFER_SHADER_OFFSET);				// Shader Record

	// primitive index list
	emit_uint8_t(&p, GL_INDEXED_PRIMITIVE_LIST);					// Indexed primitive list command
	emit_uint8_t(&p, PRIM_TRIANGLE);								// 8bit index, triangles
	emit_uint32_t(&p, 9);											// Length
	emit_uint32_t(&p, bus_addr + BUFFER_VERTEX_INDEX);				// address
	emit_uint32_t(&p, 6);											// Maximum index


	// End of bin list
	// So Flush
	emit_uint8_t(&p, GL_FLUSH_ALL_STATE);
	// Nop
	emit_uint8_t(&p, GL_NOP);
	// Halt
	emit_uint8_t(&p, GL_HALT);

	int length = p - list;

	// Okay now we need Shader Record to buffer
	p = list + BUFFER_SHADER_OFFSET;
	emit_uint8_t(&p, 0x01);											// flags
	emit_uint8_t(&p, 6 * 4);										// stride
	emit_uint8_t(&p, 0xcc);											// num uniforms (not used)
	emit_uint8_t(&p, 3);											// num varyings
	emit_uint32_t(&p, bus_addr + BUFFER_FRAGMENT_SHADER);			// Fragment shader code
	emit_uint32_t(&p, bus_addr + BUFFER_FRAGMENT_UNIFORM);			// Fragment shader uniforms
	emit_uint32_t(&p, bus_addr + BUFFER_VERTEX_DATA);				// Vertex Data




	/* Setup triangle vertices from OpenGL tutorial which used this */
	// fTriangle[0] = -0.4f; fTriangle[1] = 0.1f; fTriangle[2] = 0.0f;
	// fTriangle[3] = 0.4f; fTriangle[4] = 0.1f; fTriangle[5] = 0.0f;
	// fTriangle[6] = 0.0f; fTriangle[7] = 0.7f; fTriangle[8] = 0.0f;
	uint_fast32_t centreX = renderWth / 2;							// triangle centre x
	uint_fast32_t centreY = (uint_fast32_t)(0.4f * (renderHt / 2));	// triangle centre y
	uint_fast32_t half_shape_wth = (uint_fast32_t)(0.4f * (renderWth / 2));// Half width of triangle
	uint_fast32_t half_shape_ht = (uint_fast32_t)(0.3f * (renderHt / 2));  // half height of tringle



	// Vertex Data
	p = list + BUFFER_VERTEX_DATA;
	// Vertex: Top, vary red
	emit_uint16_t(&p, centreX << 4);								// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY - half_shape_ht) << 4);				// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 1.0f);											// Varying 0 (Red)
	emit_float(&p, 0.0f);											// Varying 1 (Green)
	emit_float(&p, 0.0f);											// Varying 2 (Blue)



	// Vertex: bottom left, vary blue
	emit_uint16_t(&p, (centreX - rotate_x(half_shape_wth)) << 4);	// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY + half_shape_ht) << 4);				// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 0.0f);											// Varying 0 (Red)
	emit_float(&p, 0.0f);											// Varying 1 (Green)
	emit_float(&p, 1.0f);											// Varying 2 (Blue)

	// Vertex: bottom right, vary green 
	emit_uint16_t(&p, (centreX + rotate_x(half_shape_wth)) << 4);	// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY + half_shape_ht) << 4);				// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 0.0f);											// Varying 0 (Red)
	emit_float(&p, 1.0f);											// Varying 1 (Green)
	emit_float(&p, 0.0f);											// Varying 2 (Blue)


    /* Setup quad vertices from OpenGL tutorial which used this */
	// fQuad[0] = -0.2f; fQuad[1] = -0.1f; fQuad[2] = 0.0f;
	// fQuad[3] = -0.2f; fQuad[4] = -0.6f; fQuad[5] = 0.0f;
	// fQuad[6] = 0.2f; fQuad[7] = -0.1f; fQuad[8] = 0.0f;
	// fQuad[9] = 0.2f; fQuad[10] = -0.6f; fQuad[11] = 0.0f;
	centreY = (uint_fast32_t)(1.35f * (renderHt / 2));				// quad centre y

	// Vertex: Top, left  vary blue
	emit_uint16_t(&p, (centreX - half_shape_wth) << 4);				// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY - rotate_y(half_shape_ht)) << 4);	// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 0.0f);											// Varying 0 (Red)
	emit_float(&p, 0.0f);											// Varying 1 (Green)
	emit_float(&p, 1.0f);											// Varying 2 (Blue)

	// Vertex: bottom left, vary Green
	emit_uint16_t(&p, (centreX - half_shape_wth) << 4);				// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY + rotate_y(half_shape_ht)) << 4);	// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 0.0f);											// Varying 0 (Red)
	emit_float(&p, 1.0f);											// Varying 1 (Green)
	emit_float(&p, 0.0f);											// Varying 2 (Blue)

	// Vertex: top right, vary red
	emit_uint16_t(&p, (centreX + half_shape_wth) << 4);				// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY - rotate_y(half_shape_ht)) << 4);	// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 1.0f);											// Varying 0 (Red)
	emit_float(&p, 0.0f);											// Varying 1 (Green)
	emit_float(&p, 0.0f);											// Varying 2 (Blue)

	// Vertex: bottom right, vary yellow
	emit_uint16_t(&p, (centreX + half_shape_wth) << 4);				// X in 12.4 fixed point
	emit_uint16_t(&p, (centreY + rotate_y(half_shape_ht)) << 4);	// Y in 12.4 fixed point
	emit_float(&p, 1.0f);											// Z
	emit_float(&p, 1.0f);											// 1/W
	emit_float(&p, 0.0f);											// Varying 0 (Red)
	emit_float(&p, 1.0f);											// Varying 1 (Green)
	emit_float(&p, 1.0f);											// Varying 2 (Blue)

	// Vertex list
	p = list + BUFFER_VERTEX_INDEX;
	emit_uint8_t(&p, 0);											// tri - top
	emit_uint8_t(&p, 1);											// tri - bottom left
	emit_uint8_t(&p, 2);											// tri - bottom right

	emit_uint8_t(&p, 3);											// quad - top left
	emit_uint8_t(&p, 4);											// quad - bottom left
	emit_uint8_t(&p, 5);											// quad - top right

	emit_uint8_t(&p, 4);											// quad - bottom left
	emit_uint8_t(&p, 6);											// quad - bottom right
	emit_uint8_t(&p, 5);											// quad - top right

	// fragment shader
	p = list + BUFFER_FRAGMENT_SHADER;
	emit_uint32_t(&p, 0x958e0dbf);
	emit_uint32_t(&p, 0xd1724823); /* mov r0, vary; mov r3.8d, 1.0 */
	emit_uint32_t(&p, 0x818e7176);
	emit_uint32_t(&p, 0x40024821); /* fadd r0, r0, r5; mov r1, vary */
	emit_uint32_t(&p, 0x818e7376);
	emit_uint32_t(&p, 0x10024862); /* fadd r1, r1, r5; mov r2, vary */
	emit_uint32_t(&p, 0x819e7540);
	emit_uint32_t(&p, 0x114248a3); /* fadd r2, r2, r5; mov r3.8a, r0 */
	emit_uint32_t(&p, 0x809e7009);
	emit_uint32_t(&p, 0x115049e3); /* nop; mov r3.8b, r1 */
	emit_uint32_t(&p, 0x809e7012);
	emit_uint32_t(&p, 0x116049e3); /* nop; mov r3.8c, r2 */
	emit_uint32_t(&p, 0x159e76c0);
	emit_uint32_t(&p, 0x30020ba7); /* mov tlbc, r3; nop; thrend */
	emit_uint32_t(&p, 0x009e7000);
	emit_uint32_t(&p, 0x100009e7); /* nop; nop; nop */
	emit_uint32_t(&p, 0x009e7000);
	emit_uint32_t(&p, 0x500009e7); /* nop; nop; sbdone */

	// Render control list
	p = list + BUFFER_RENDER_CONTROL;

	// Clear colors
	emit_uint8_t(&p, GL_CLEAR_COLORS);
	emit_uint32_t(&p, 0xff000000);			// Opaque Black
	emit_uint32_t(&p, 0xff000000);			// 32 bit clear colours need to be repeated twice
	emit_uint32_t(&p, 0);
	emit_uint8_t(&p, 0);

	// Tile Rendering Mode Configuration
	emit_uint8_t(&p, GL_TILE_RENDER_CONFIG);

	emit_uint32_t(&p, renderBufferAddr);	// render address

	emit_uint16_t(&p, renderWth);			// width
	emit_uint16_t(&p, renderHt);			// height
	emit_uint8_t(&p, 0x04);					// framebuffer mode (linear rgba8888)
	emit_uint8_t(&p, 0x00);

	// Do a store of the first tile to force the tile buffer to be cleared
	// Tile Coordinates
	emit_uint8_t(&p, GL_TILE_COORDINATES);
	emit_uint8_t(&p, 0);
	emit_uint8_t(&p, 0);
	// Store Tile Buffer General
	emit_uint8_t(&p, GL_STORE_TILE_BUFFER);
	emit_uint16_t(&p, 0);					// Store nothing (just clear)
	emit_uint32_t(&p, 0);					// no address is needed

	// Link all binned lists together
	for (int x = 0; x < binWth; x++) {
		for (int y = 0; y < binHt; y++) {

			// Tile Coordinates
			emit_uint8_t(&p, GL_TILE_COORDINATES);
			emit_uint8_t(&p, x);
			emit_uint8_t(&p, y);

			// Call Tile sublist
			emit_uint8_t(&p, GL_BRANCH_TO_SUBLIST);
			emit_uint32_t(&p, bus_addr + BUFFER_TILE_DATA + (y * binWth + x) * 32);

			// Last tile needs a special store instruction
			if (x == (binWth - 1) && (y == binHt - 1)) {
				// Store resolved tile color buffer and signal end of frame
				emit_uint8_t(&p, GL_STORE_MULTISAMPLE_END);
			}
			else {
				// Store resolved tile color buffer
				emit_uint8_t(&p, GL_STORE_MULTISAMPLE);
			}
		}
	}


	int render_length = p - (list + BUFFER_RENDER_CONTROL);


	// Run our control list
	v3d[V3D_BFC] = 1;                         // reset binning frame count
	v3d[V3D_CT0CA] = bus_addr;
	v3d[V3D_CT0EA] = bus_addr + length;

	// Wait for control list to execute
	while (v3d[V3D_CT0CS] & 0x20);

	// wait for binning to finish
	while (v3d[V3D_BFC] == 0) {}            

	// stop the thread
	v3d[V3D_CT0CS] = 0x20;

	// Run our render
	v3d[V3D_RFC] = 1;						// reset rendering frame count
	v3d[V3D_CT1CA] = bus_addr + BUFFER_RENDER_CONTROL;
	v3d[V3D_CT1EA] = bus_addr + BUFFER_RENDER_CONTROL + render_length;

	// Wait for render to execute
	while (v3d[V3D_CT1CS] & 0x20);

	// wait for render to finish
	while(v3d[V3D_RFC] == 0) {} 

	// stop the thread
	v3d[V3D_CT1CS] = 0x20;

	// Release resources
	V3D_mem_unlock(handle);
	V3D_mem_free(handle);

}

