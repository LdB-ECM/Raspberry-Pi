#include <stdbool.h>							// Needed for bool and true/false
#include <stdint.h>								// Needed for uint8_t, uint32_t, etc
#include <math.h>
#include <string.h>
#include "emb-stdio.h"
#include "rpi-smartstart.h"						// Need for mailbox
#include "rpi-GLES.h"
#include "SDCard.h"

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

/* primitive type in the GL pipline */
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


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{                           3D VECTOR ROUTINES                              }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*--------------------------------------------------------------------------}
{ Returns the dot product between the two vectors v1 & v2					}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
GLfloat vec3_dot (VEC3 v1, VEC3 v2) {
	return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

/*--------------------------------------------------------------------------}
{ Returns the cross product between the two vectors v1 & v2					}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
VEC3 vec3_cross (VEC3 v1, VEC3 v2) {
	VEC3 result = { .x = v1.y * v2.z - v1.z * v2.y,
		.y = v1.z * v2.x - v1.x * v2.z,
		.z = v1.x * v2.y - v1.y * v2.x };
	return result;
}

/*--------------------------------------------------------------------------}
{ Returns the subtraction of v2 from v1										}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
VEC3 vec3_sub (VEC3 v1, VEC3 v2) {
	VEC3 result = { .x = v1.x - v2.x, 
		            .y = v1.y - v2.y, 
					.z = v1.z - v2.z };
	return result;
}

/*--------------------------------------------------------------------------}
{ Returns the addition of v2 to v1											}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
VEC3 vec3_add (VEC3 v1, VEC3 v2) {
	VEC3 result = { .x = v1.x - v2.x,
				    .y = v1.y - v2.y,
					.z = v1.z - v2.z };
	return result;
}

/*--------------------------------------------------------------------------}
{ Returns the scale of v1 by scale s										}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
VEC3 vec3_scale (VEC3 v, GLfloat s) {
	VEC3 result = { .x = v.x * s,
					.y = v.y * s,
					.z = v.z * s };
	return result;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{                           3D MATRIX ROUTINES                              }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*--------------------------------------------------------------------------}
{                  IDENTITY MATRIX3D (4x4 MATRIX) DEFINITION                }
{--------------------------------------------------------------------------*/
const MATRIX3D IdentityMatrix3D = {
	.coef[0][0] = GLFLOAT_ONE,.coef[0][1] = GLFLOAT_ZERO,.coef[0][2] = GLFLOAT_ZERO,.coef[0][3] = GLFLOAT_ZERO,
	.coef[1][0] = GLFLOAT_ZERO,.coef[1][1] = GLFLOAT_ONE,.coef[1][2] = GLFLOAT_ZERO,.coef[1][3] = GLFLOAT_ZERO,
	.coef[2][0] = GLFLOAT_ZERO,.coef[2][1] = GLFLOAT_ZERO,.coef[2][2] = GLFLOAT_ONE,.coef[2][3] = GLFLOAT_ZERO,
	.coef[3][0] = GLFLOAT_ZERO,.coef[3][1] = GLFLOAT_ZERO,.coef[3][2] = GLFLOAT_ZERO,.coef[3][3] = GLFLOAT_ONE };

/*--------------------------------------------------------------------------}
{ Returns the X rotation matrix in MATRIX3D from the given angle in radians }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D XRotationMatrix3D(GLfloat angle_in_rad) {
	GLfloat sineValue = sinf(angle_in_rad);						// Sine of angle
	GLfloat cosineValue = cosf(angle_in_rad);						// Cosine of angle
	MATRIX3D m = IdentityMatrix3D;									// Start with identity matrix
	m.coef[1][1] = cosineValue;										// Coef[0][0] = cos
	m.coef[1][2] = -sineValue;										// Coef[0][1] = -sin
	m.coef[2][1] = sineValue;										// Coef[1][0] = sin
	m.coef[2][2] = cosineValue;										// Coef[1][1] = cos
	return(m);														// Return the matrix
}

/*--------------------------------------------------------------------------}
{ Returns the Y rotation matrix in MATRIX3D from the given angle in radians }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D YRotationMatrix3D(GLfloat angle_in_rad) {
	GLfloat sineValue = sinf(angle_in_rad);							// Sine of angle
	GLfloat cosineValue = cosf(angle_in_rad);						// Cosine of angle
	MATRIX3D m = IdentityMatrix3D;									// Start with identity matrix
	m.coef[0][0] = cosineValue;										// Coef[0][0] = cos
	m.coef[0][2] = -sineValue;										// Coef[0][1] = -sin
	m.coef[2][0] = sineValue;										// Coef[1][0] = sin
	m.coef[2][2] = cosineValue;										// Coef[1][1] = cos
	return(m);														// Return the matrix
}

/*--------------------------------------------------------------------------}
{ Returns the Z rotation matrix in MATRIX3D from the given angle in radians }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D ZRotationMatrix3D(GLfloat angle_in_rad) {
	GLfloat sineValue = sinf(angle_in_rad);							// Sine of angle
	GLfloat cosineValue = cosf(angle_in_rad);						// Cosine of angle
	MATRIX3D m = IdentityMatrix3D;									// Start with identity matrix
	m.coef[0][0] = cosineValue;										// Coef[0][0] = cos
	m.coef[0][1] = -sineValue;										// Coef[0][1] = -sin
	m.coef[1][0] = sineValue;										// Coef[1][0] = sin
	m.coef[1][1] = cosineValue;										// Coef[1][1] = cos
	return(m);														// Return the matrix
}

/*--------------------------------------------------------------------------}
{ Returns the translation matrix in MATRIX3D from the x,y,z offset values   }
{ 16Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D TranslationMatrix3D (GLfloat offsetX, GLfloat offsetY, GLfloat offsetZ) {
	MATRIX3D m = IdentityMatrix3D;									// Start with identity matrix
	m.coef[0][3] = offsetX;											// Set x offset value
	m.coef[1][3] = offsetY;											// Set y offset value
	m.coef[2][3] = offsetZ;											// Set z offset value
	return(m);														// Return matrix
}

/*--------------------------------------------------------------------------}
{ Returns the scale matrix in MATRIX3D from the x,y,z axis scale values     }
{ 16Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D ScaleMatrix3D (GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ) {
	MATRIX3D m = IdentityMatrix3D;									// Start with identity matrix
	m.coef[0][0] = scaleX;											// Set x scale value
	m.coef[1][1] = scaleY;											// Set y scale value
	m.coef[2][2] = scaleZ;											// Set z scale value
	return(m);														// Return the matrix
}

/*--------------------------------------------------------------------------}
{ Returns the mirror matrix in MATRIX3D from the x,y,z mirror axis values   }
{ 17Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D MirrorMatrix3D (bool mirrorX, bool mirrorY, bool mirrorZ) {
	MATRIX3D m = IdentityMatrix3D;									// Start with identity matrix
	m.coef[0][0] =  mirrorX ?  -GLFLOAT_ONE : GLFLOAT_ONE;			// Coef[0][0] is -1 rather than 1 if x mirror active
	m.coef[1][1] =  mirrorY ?  -GLFLOAT_ONE : GLFLOAT_ONE;			// Coef[1][1] is -1 rather than 1 if y mirror active
	m.coef[2][2] =  mirrorZ ?  -GLFLOAT_ONE : GLFLOAT_ONE;			// Coef[2][2] is -1 rather than 1 if z mirror active
	return(m);														// Return the matrix
}

/*--------------------------------------------------------------------------}
{ Transposes the 3D matrix from ROW MAJOR <==> COLUMN MAJOR or back again	}
{ 17Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
void TransposeMatrix3D(MATRIX3D* matrix) {
	for (uint_fast8_t i = 0; i < 3; i++) {
		for (uint_fast8_t j = i + 1; j <= 3; j++) {
			GLfloat* p1 = &matrix->coef[i][j];						// Pointer to first coefficient to swap
			GLfloat* p2 = &matrix->coef[j][i];						// Pointer to second coefficient to swap
			GLfloat temp = *p1; 									// Copy whats at p1 to temp
			*p1 = *p2;												// Copy whats at p2 to p1
			*p2 = temp;												// Copy temp to p2
		}
	}
}

/*--------------------------------------------------------------------------}
{ Returns the matrix result of multiplying matrix3D a by matrix3D b         }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
MATRIX3D MultiplyMatrix3D(MATRIX3D a, MATRIX3D  b) {
	MATRIX3D  m = IdentityMatrix3D;									// Temp result matrix

	for (uint_fast8_t i = 0; i < 4; i++) {							// For each row rank
		for (uint_fast8_t j = 0; j < 4; j++) {						// For each column rank
			GLfloat sum = (a.coef[i][0] * b.coef[0][j]);			// 1st calc
			sum = sum + (a.coef[i][1] * b.coef[1][j]);				// 2nd calc
			sum = sum + (a.coef[i][2] * b.coef[2][j]);				// 3rd calc
			sum = sum + (a.coef[i][3] * b.coef[3][j]);				// 4th calc
			m.coef[i][j] = sum;										// Set result to temp result matrix
		}
	}
	return (m);														// Return result matrix
}

/*-TransformPoint3D---------------------------------------------------------}
{ The 3D point(inX, inY, inZ) is transformed by the Matrix3D to return the  }
{ new position to the point ptrs(*outX, *outY, *outZ).You can use NULL 		}
{ for a return pointer if you do not require that individual value, and IN  }
{ and OUT can be the same variable.											}
{ 14Sep2017 LdB 															}
{--------------------------------------------------------------------------*/
void TransformPoint3D (GLfloat inX, GLfloat inY, GLfloat inZ,		// 3D input point (inX,inY,inZ) 
					   MATRIX3D a,									// Matrix to transform point by
					   GLfloat* outX, GLfloat* outY, GLfloat* outZ)	// 3D point result ptrs (*outX, *outY, *outZ)
{
	GLfloat tw, tx, ty, tz;
	tw = a.coef[3][0] * inX + a.coef[3][1] * inY +
		a.coef[3][2] * inZ + a.coef[3][3];							// Calculate 4th value
	tx = a.coef[0][0] * inX + a.coef[0][1] * inY +
		a.coef[0][2] * inZ + a.coef[0][3];							// Calculate temp x value into tx
	ty = a.coef[1][0] * inX + a.coef[1][1] * inY +
		a.coef[1][2] * inZ + a.coef[1][3];							// Calculate temp y value into ty
	tz = a.coef[2][0] * inX + a.coef[2][1] * inY +
		a.coef[2][2] * inZ + a.coef[2][3];							// Calculate temp z value into tz
	if ((tw != GLFLOAT_ZERO) && (tw != GLFLOAT_ONE)) {				// If tw is not zero or one
		tx /= tw;													// Divid out tw from tx
		ty /= tw;													// Divid out tw	from ty
		tz /= tw;													// Divid out tw	from tz
	}
	if (outX) *outX = tx;											// Pointer outX valid so output result
	if (outY) *outY = ty;											// Pointer outY valid so output result
	if (outZ) *outZ = tz;											// Pointer outZ valid so output result
}





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



struct __attribute__((__packed__, aligned(1))) EmitVertex {
	uint16_t  x;													// X in 12.4 fixed point
	uint16_t  y;													// Y in 12.4 fixed point
	float	  z;													// Z
	float	  w;													// 1/W
	float 	  red;													// Varying 0 (Red)
	float	  green;												// Varying 1 (Green)
	float	  blue;													// Varying 2 (Blue)
};

struct __attribute__((__packed__, aligned(1))) OriginalVertex {
	float     x;													// X
	float	  y;													// Y
	float	  z;													// Z
};

struct __attribute__((__packed__, aligned(1))) TileListEntry {
	uint8_t   coord_cmd;											// cmd = GL_TILE_COORDINATES
	uint8_t	  x;													// x
	uint8_t	  y;													// y
	uint8_t   sublist_cmd;											// cmd = GL_BRANCH_TO_SUBLIST
	uint32_t  sublist_ptr;											// ptr to next sublist
	uint8_t   tile_endcmd;											// GL_STORE_MULTISAMPLE or GL_STORE_MULTISAMPLE_END
};


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{                           GL PIPE STRUCTURES                              }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*==========================================================================}
{					 VERTEX PIPE MEMORY (VPM) STRUCTURES 					}
{==========================================================================*/

/* The minimum VPM size is 8Kbytes, which is the amount required for normal 
   pipelined 3D operation with concurrent vertex and coordinate shading and 
   worst case vertex data size  */

/* The FIFO used for the partially interpolated varying results is also used 
   for VPM read and write accesses and VCD control from QPU programs. For this 
   reason a fragment shader program cannot access the VPM or VCD. */

/* VC4 doesn't have any hardware support for blending, alpha test, logic ops,
  or color mask.  Instead, you read the current contents of the destination
  from the tile buffer after having waited for the scoreboard (which is
  handled by vc4_qpu_emit.c), then do math using your output color and that
  destination value, and update the output color appropriately.
 
  Once this pass is done, the color write will either have one component (for
  single sample) with packed argb8888, or 4 components with the per-sample
  argb8888 result.*/

/* 
1.  The Vertex Cache Manager (VCM) gets vertices from *somewhere*
2.  The VCD then DMAs the vertices to Vertex Pipe Memory (VPM)
3.  Your vert shader reads these verts from VPM and transforms them
4.  Your shader writes these verts back to VPM in a certain expected format
5.  The verts are then read directly by the Primitive Tile Binner (PTB) or Primitive Setup Engine (PSE)
*/

/*--------------------------------------------------------------------------}
{						  VPM GENERIC READ BLOCK	 						}
{--------------------------------------------------------------------------*/
typedef union {
	struct {
		unsigned addr : 8;											// @0-7		Location of first vector accessed
		enum {
			VPM_READ_8BIT = 0,
			VPM_READ_16BIT = 1,
			VPM_READ_32BIT = 2,
			VPM_READ_RESERVED = 3,
		} readsize: 2;												// @8-9		0,1,2 8BIT, 16BIT 32 BIT reserved
		unsigned laned : 1;											// @10		0,1  Packed, or laned ignored for 32bit
		unsigned horz : 1;											// @11		0,1  Horizontal or vertical
		unsigned stride : 6;										// @12-17	0-64 stride size
		unsigned unused : 2;										// @18-19
		unsigned numvectors : 4;									// @20-23	0-15 number of vectors to read
		unsigned unused1 : 6;										// @24-29
		unsigned setup_id : 2;										// @30-31	= 0 VPM block setup
	};
	uint32_t Raw32;													// Access whole 32bits at once
} VPM_GENERIC_READ_BLOCK;

/*--------------------------------------------------------------------------}
{						  VPM GENERIC WRITE BLOCK	 						}
{--------------------------------------------------------------------------*/
typedef union {
	struct {
		unsigned addr : 8;											// @0-7		Location of first vector accessed
		enum {
			VPM_WRITE_8BIT = 0,
			VPM_WRITE_16BIT = 1,
			VPM_WRITE_32BIT = 2,
			VPM_WRITE_RESERVED = 3,
		} writesize : 2;											// @8-9		0,1,2 8BIT, 16BIT 32 BIT reserved
		unsigned laned : 1;											// @10		0,1  Packed, or laned ignored for 32bit
		unsigned horz : 1;											// @11		0,1  Horizontal or vertical
		unsigned stride : 6;										// @12-17	0-64 stride size
		unsigned unused : 12;										// @18-29
		unsigned setup_id : 2;										// @30-31	= 0 VPM block setup
	};
	uint32_t Raw32;													// Access whole 32bits at once
} VPM_GENERIC_WRITE_BLOCK;


/*==========================================================================}
{				 	  TEXTURE CONFIG BLOCK STRUCTURES 						}
{==========================================================================*/

/*--------------------------------------------------------------------------}
{						  TEXTURE CONFIG BLOCK	 							}
{--------------------------------------------------------------------------*/
typedef union {
	struct {
		unsigned mip_levels : 4;									// @0-3		number of mipmap levels minus 1
		unsigned texture_type : 4;								    // @4-7		texture type .. TODO: enumerate them 
		unsigned flip_y : 1;										// @8		0,1  top to bottom, bottom to top
		unsigned cubemap : 1;										// @9		0,1  cube map mode off, on
		unsigned cswiz : 2;											// @10-11	cache swizzle
		unsigned base_addr : 20;									// @12-31	Texture base pointer in 4K blocks
	};
	uint32_t Raw32;													// Access whole 32bits at once
} TEXTURE_CONFIG_BLOCK;

/*--------------------------------------------------------------------------}
{						ENUMERATED T & S WRAP MODE							}
{--------------------------------------------------------------------------*/
/* In binary so any error is obvious */
typedef enum {
	WRAPMODE_REPEAT = 0b00,							// 0
	WRAPMODE_CLAMP  = 0b01,							// 1
	WRAPMODE_MIRROR = 0b10,							// 2
	WRAPMODE_BORDER = 0b11,							// 3 
} WRAP_MODE;

/*--------------------------------------------------------------------------}
{					    TEXTURE CONFIG PARAMETERS							}
{--------------------------------------------------------------------------*/
typedef union {
	struct {
		WRAP_MODE wrap_s : 2;										// @0-1		0,1,2,3   S wrap mode  (repeat, clamp mirror, border)
		WRAP_MODE wrap_t : 2;									    // @2-3		0,1,2,3   T wrap mode  (repeat, clamp mirror, border)
		unsigned minfilt : 3;										// @4-6		minification filter
		unsigned magfilt : 1;										// @7		magnification filter
		unsigned width : 11;										// @8-18	Image width (0-2048)
		unsigned etcflip : 1;									    // @19		Flip etc Y
		unsigned height : 11;										// @20-30	Image height (0-2048)
		unsigned texture_type4 : 1;									// @31		Texture data type extended
	};
	uint32_t Raw32;													// Access whole 32bits at once
} TEXTURE_CONFIG_PARAMS;




static GLfloat angle = 0.0f;

void DoRotate(float delta, int screenCentreX, int screenCentreY, struct obj_model_t* model) {
	angle += delta;
	if (angle >= (6.2831852)) angle -= (6.2831852);   // 2 * Pi = 360 degree rotation

	if ((model) && (model->vertexARM) && (model->originalVertexARM)) {
		MATRIX3D a = MultiplyMatrix3D(model->viewMatrix, ZRotationMatrix3D(angle));
		// Vertex Data 1
		struct EmitVertex* v = model->vertexARM;
		struct OriginalVertex* ov = model->originalVertexARM;
		for (unsigned int i = 0; i < model->num_verts; i++) {
			GLfloat dx, dy, dz;
			TransformPoint3D(ov->x, ov->y, ov->z, a, &dx, &dy, &dz);
			v->x = (uint16_t)((int32_t)(dx + screenCentreX) << 4); // X in 12.4 fixed point
			v->y = (uint16_t)((int32_t)(dy + screenCentreY) << 4); // Y in 12.4 fixed point
			v->z = 0.25f; //    0.25f;
			v++;	// Next vertex
			ov++;
		}
	}
}


// Render the model
void RenderModel (struct obj_model_t* model, printhandler prn_handler) {
	//  We allocate/lock some videocore memory
	// I'm just shoving everything in a single buffer because I'm lazy 8Mb, 4k alignment
	// Normally you would do individual allocations but not sure of interface I want yet
	// So lets just define some address in that one single buffer for now 
	// You need to make sure they don't overlap if you expand sample
	#define BUFFER_SHADER_OFFSET	0x80
	#define BUFFER_TILE_STATE		0x200
	#define BUFFER_TILE_DATA		0x6000
	#define BUFFER_RENDER_CONTROL	0x1e200
	#define BUFFER_FRAGMENT_SHADER	0x1fe00
	#define BUFFER_FRAGMENT_UNIFORM	0x1ff00
	if (!model) return;

	// clear caches
	v3d[V3D_L2CACTL] = 4;
	v3d[V3D_SLCACTL] = 0x0F0F0F0F;

	// stop the thread
	v3d[V3D_CT0CS] = 0x20;
	// wait for it to stop
	while (v3d[V3D_CT0CS] & 0x20);

	// Run our control list
	v3d[V3D_BFC] = 1;                         // reset binning frame count
	v3d[V3D_CT0CA] = model->rendererDataVC4;
	v3d[V3D_CT0EA] = model->rendererDataVC4 + model->binningConfigLength;

	// wait for binning to finish
	while (v3d[V3D_BFC] == 0) {}            

	// stop the thread
	v3d[V3D_CT1CS] = 0x20;

	// Wait for thread to stop
	while (v3d[V3D_CT1CS] & 0x20);

	// Run our render
	v3d[V3D_RFC] = 1;						// reset rendering frame count
	v3d[V3D_CT1CA] = model->rendererDataVC4 + BUFFER_RENDER_CONTROL;
	v3d[V3D_CT1EA] = model->rendererDataVC4 + BUFFER_RENDER_CONTROL + model->renderLength;

	// wait for render to finish
	while(v3d[V3D_RFC] == 0) {} 


}


#include <float.h>
static bool ParseWaveFrontMesh(const char* fileName, struct obj_model_t *model, bool secondPass, float desiredMaxSize) {

	int v, t, n;
	char buf[256];
	char buffer[512];
	char* bufptr = NULL;
	char* bufend = NULL;
	uint32_t bytesLeft, bytesRead;
	struct OriginalVertex* ov = model->originalVertexARM;
	struct EmitVertex* ev = model->vertexARM;
	uint16_t *v16p = (uint16_t*)(uintptr_t)model->modelDataARM;		// Transfer to temp
	HANDLE fh = sdCreateFile(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fh == 0) return false;									// If file did not open fail return
	bytesLeft = sdGetFileSize(fh, 0);								// Bytes left starts as file size
	model->minX = FLT_MAX;
	model->minY = FLT_MAX;
	model->minZ = FLT_MAX;
	model->maxX = FLT_MIN;
	model->maxY = FLT_MIN;
	model->maxZ = FLT_MIN;

	do {
		int i;
		bool LFfound = false;
		for (i = 0; i < _countof(buf) && !LFfound; i++) {

			if (bufptr == bufend) {									// Buffer is empty
				int Bw;
				if (_countof(buffer) > bytesLeft)
					Bw = bytesLeft;									// Amount of file left
				else Bw = _countof(buffer);					// Full buffer size
				if (!sdReadFile(fh, &buffer[0], Bw, &bytesRead, 0)   // Read from file
					|| bytesRead != Bw) return false;			// Read failed
				bufptr = &buffer[0];								// Reset BufPtr
				bufend = &buffer[Bw];								// End of buffer
			}
			buf[i] = *bufptr++;
			if (buf[i] == '\n')  LFfound = true;
			bytesLeft--;
		}
		if (i != 0) {
			buf[i] = 0;			// Make asciiz for debugging purposes
			switch (buf[0])
			{
			case 'v':
			{
				if (buf[1] == ' ')
				{
					/* Vertex */
					float x, y, z, w;
					if (sscanf(&buf[2], "%f %f %f %f", &x, &y, &z, &w) != 4)
					{
						if (sscanf(&buf[2], "%f %f %f", &x, &y, &z) != 3)
						{
							printf("Error reading vertex data!\n");
							return false;
						}
						else {
							w = 1.0f;
						}
					}
					if (!secondPass) {
						model->num_verts++;
					}
					else {
						x = x - model->offsX;
						x = x * model->scale;
						y = y - model->offsY;
						y = y * model->scale;
						z = z - model->offsZ;
						z = z * model->scale;
						ov->x = x;
						ov->y = y;
						ov->z = z;
						ov++;

						ev->w = 0.0f;
						ev->red = 0.5f;													// Varying 0 (Red)
						ev->green = 0.5f;												// Varying 1 (Green)
						ev->blue = 0.5f;												// Varying 2 (Blue)
						ev++;
					}
					if (x > model->maxX) model->maxX = x;
					if (x < model->minX) model->minX = x;
					if (y > model->maxY) model->maxY = y;
					if (y < model->minY) model->minY = y;
					if (z > model->maxZ) model->maxZ = z;
					if (z < model->minZ) model->minZ = z;
				}
				else if (buf[1] == 't')
				{
					/* Texture coords. */
					if (!secondPass) model->num_texCoords++;
				}
				else if (buf[1] == 'n')
				{
					/* Normal vector */
					if (!secondPass) model->num_normals++;
				}
				else
				{
					printf("Warning: unknown token \"%s\"! (ignoring)\n", buf);
				}

				break;
			}

			case 'f':
			{
				/* Face */
				if (sscanf(&buf[2], "%d/%d/%d", &v, &n, &t) == 3)
				{
					if (!secondPass) {
						model->num_faces++;
						model->has_texCoords = 1;
						model->has_normals = 1;
					}
				}
				else if (sscanf(&buf[2], "%d//%d", &v, &n) == 2)
				{
					if (!secondPass) {
						model->num_faces++;
						model->has_texCoords = 0;
						model->has_normals = 1;
					}
				}
				else if (sscanf(&buf[2], "%d/%d", &v, &t) == 2)
				{
					if (!secondPass) {
						model->num_faces++;
						model->has_texCoords = 1;
						model->has_normals = 0;
					}
				}
				else if (sscanf(&buf[2], "%d", &v) == 1)
				{
					if (!secondPass) {
						model->num_faces++;
						model->has_texCoords = 0;
						model->has_normals = 0;
					}
				}
				else
				{
					/* Should never be there or the model is very crappy */
					printf("Error: found face with no vertex!\n");
				}


				char* pbuf = &buf[0];
				int num_elems = 0;

				/* Count number of vertices for this face */
				while (*pbuf) {
					if (*pbuf == ' ') num_elems++;
					pbuf++;
				}

				/* Select primitive type */
				if (num_elems < 3 || num_elems > 5)
				{
					printf("Error: faces can only be triangles or quads !\n");
					return 0;
				}
				else if (num_elems == 3)
				{
					// GL_TRIANGLE
					if (!secondPass) model->tri_count++;
				}
				else if (num_elems == 4)
				{
					// GL_QUAD
					if (!secondPass) model->quad_count++;
				}
				else if (num_elems == 5)
				{
					// GL_POLYGON
					if (!secondPass) model->polygon_count++;
				}


				/* Read face data */
				if (secondPass) {
					int vi1, vi2, vi3, v0, v2, v3;
					char* pbuf = &buf[0];
					i = 0;
					for (i = 0; i < num_elems; ++i)
					{
						pbuf = strchr(pbuf, ' ');
						pbuf++; // Skip space

								// Try reading vertices
						if (sscanf(pbuf, "%d/%d/%d",
							&vi1,
							&vi2,
							&vi3) != 3)
						{
							if (sscanf(pbuf, "%d//%d", &vi1,
								&vi3) != 2)
							{
								if (sscanf(pbuf, "%d/%d", &vi1,
									&vi2) != 2)
								{
									sscanf(pbuf, "%d", &vi1);
								}
							}
						}
						// Indices must start at 0
						vi1--;

						//if (mdl->has_texCoords)
						//	pface->uvw_indices[i]--;

						//if (mdl->has_normals)
						//	pface->norm_indices[i]--;
						switch (i) {
						case 0: {
							v0 = vi1;
							if (vi1 > model->MaxIndexVertex) model->MaxIndexVertex = vi1;
							uint16_t t16v = vi1;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, vi1);
							break;
						}
						case 1: {
							//v1 = vi1;
							if (vi1 > model->MaxIndexVertex) model->MaxIndexVertex = vi1;
							uint16_t t16v = vi1;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, vi1);
							break;
						}
						case 2: {
							if (vi1 > model->MaxIndexVertex) model->MaxIndexVertex = vi1;
							v2 = vi1;
							uint16_t t16v = vi1;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, vi1);
							break;
						}
						case 3: {								// We have a quad
							if (vi1 > model->MaxIndexVertex) model->MaxIndexVertex = vi1;
							v3 = vi1;
							uint16_t t16v = v0;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, v0);
							t16v = v2;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, v2);
							t16v = v3;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, v3);
							break;
						}
						case 4: {								// We have a quad	
							if (vi1 > model->MaxIndexVertex) model->MaxIndexVertex = vi1;
							uint16_t t16v = v0;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, v0);
							t16v = v3;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, v3);
							t16v = vi1;
							*v16p = t16v;
							v16p++;
							//emit_uint16_t(&vlist, vi1);
							break;
						}
						}
					}
				}
				break;
			}

			case 'g':
			{
				/* Group */
				/*	fscanf (fp, "%s", buf); */
				break;
			}

			default:
				break;
			}
		}
	} while (bytesLeft > 0);

	sdCloseHandle(fh);
	/* Check if informations are valid */
	if (!secondPass) {
		if ((model->has_texCoords && !model->num_texCoords) ||
			(model->has_normals && !model->num_normals))
		{
			printf("error: contradiction between collected info!\n");
			printf("   * %i texture coords.\n", model->num_texCoords);
			printf("   * %i normal vectors\n", model->num_normals);
			return false;
		}

		if (!model->num_verts)
		{
			printf("error: no vertex found!\n");
			return false;
		}

		model->offsX = (model->maxX + model->minX)/2.0f;
		model->offsY = (model->maxY + model->minY)/2.0f;
		model->offsZ = (model->maxZ + model->minZ)/2.0f;

		float maxSize = model->maxX - model->minX;
		if (model->maxY - model->minY > maxSize)
			maxSize = model->maxY - model->minY;
		if (model->maxZ - model->minZ > maxSize)
			maxSize = model->maxZ - model->minZ;
		model->scale = desiredMaxSize / maxSize;

		printf("first pass results: found\n");
		printf("   * %i vertices\n", (int)model->num_verts);
		printf("   * %i texture coords.\n", model->num_texCoords);
		printf("   * %i normal vectors\n", model->num_normals);
		printf("   * %i faces being %i triangles, %i Quads, %i Polygons\n", model->num_faces, model->tri_count, model->quad_count, model->polygon_count);
		printf("   * has texture coords.: %s\n", model->has_texCoords ? "yes" : "no");
		printf("   * has normals: %s\n", model->has_normals ? "yes" : "no");
		printf("   * minX: %i maxX: %i\n", (int)model->minX, (int)model->maxX);
		printf("   * minY: %i maxY: %i\n", (int)model->minY, (int)model->maxY);
		printf("   * minZ: %i maxZ: %i\n", (int)model->minZ, (int)model->maxZ);
		printf("   * scale: %i\n", (int)model->scale);
	}
	else {
		printf("   * minX: %i maxX: %i\n", (int)model->minX, (int)model->maxX);
		printf("   * minY: %i maxY: %i\n", (int)model->minY, (int)model->maxY);
		printf("   * minZ: %i maxZ: %i\n", (int)model->minZ, (int)model->maxZ);
		printf(" Display starts in 5 seconds\n");
		timer_wait(5000000);
	}
	return true;
}







bool SetupRenderer (struct obj_model_t* model, uint32_t renderWth, uint32_t renderHt, uint32_t renderBufferAddr)
{
	if (model) {
		model->rendererHandle = V3D_mem_alloc(0x800000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
		if (!model->rendererHandle)	return false;

		model->rendererDataVC4 = V3D_mem_lock(model->rendererHandle);
		model->rendererDataARM = GPUaddrToARMaddr(model->rendererDataVC4);

		model->renderWth = renderWth;
		model->renderHt = renderHt;
		model->binWth = (renderWth + 63) / 64;							// Tiles across 
		model->binHt = (renderHt + 63) / 64;							// Tiles down 
		uint8_t *p; // = (uint8_t*)(uintptr_t)model->rendererDataARM;

		// fragment shader
		p = (uint8_t*)(uintptr_t)model->rendererDataARM + BUFFER_FRAGMENT_SHADER;
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
		p = (uint8_t*)(uintptr_t)model->rendererDataARM + BUFFER_RENDER_CONTROL;

		// Clear colors
		emit_uint8_t(&p, GL_CLEAR_COLORS);
		emit_uint32_t(&p, 0xff000000);			// Opaque Black
		emit_uint32_t(&p, 0xff000000);			// 32 bit clear colours need to be repeated twice
		emit_uint32_t(&p, 0);
		emit_uint8_t(&p, 0);

		// Tile Rendering Mode Configuration
		emit_uint8_t(&p, GL_TILE_RENDER_CONFIG);

		emit_uint32_t(&p, renderBufferAddr);	// render address

		emit_uint16_t(&p, model->renderWth);	// width
		emit_uint16_t(&p, model->renderHt);		// height
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

		struct TileListEntry* tp;
		tp = (struct TileListEntry*)(uintptr_t) p;
		// Link all binned lists together
		for (int x = 0; x < model->binWth; x++) {
			for (int y = 0; y < model->binHt; y++) {

				// Tile Coordinates
				tp->coord_cmd = GL_TILE_COORDINATES;
				tp->x = x;
				tp->y = y;

				// Call Tile sublist
				tp->sublist_cmd = GL_BRANCH_TO_SUBLIST;
				tp->sublist_ptr = model->rendererDataVC4 + BUFFER_TILE_DATA + (y * model->binWth + x) * 32;

				// Last tile needs a special store instruction
				if (x == (model->binWth - 1) && (y == model->binHt - 1)) {
					// Store resolved tile color buffer and signal end of frame
					tp->tile_endcmd = GL_STORE_MULTISAMPLE_END;
				}
				else {
					// Store resolved tile color buffer
					tp->tile_endcmd = GL_STORE_MULTISAMPLE;
				}
				tp++;
			}
		}
		model->renderLength = (uintptr_t)tp - (uintptr_t)(model->rendererDataARM + BUFFER_RENDER_CONTROL);

		return true;
	}
	return false;
}

bool DoneRenderer(struct obj_model_t* model) 
{
	if (model) {
		// Release resources
		V3D_mem_unlock(model->rendererHandle);
		V3D_mem_free(model->rendererHandle);
		return true;
	}
	return false;
}


/*--------------------------------------------------------------------------}
{						  NV_SHADER_STATE STRUCTURE	 						}
{--------------------------------------------------------------------------*/
typedef struct __attribute__((__packed__, aligned(1))) tagNVShaderState
{
	uint8_t   flags;												// shader flags 
	uint8_t	  stride;												// stride for VBO
	uint8_t	  numUniforms;											// number of uniforms
	uint8_t   numVaryings;											// number of varyings
	uint32_t  fragment_shader;										// Fragment Shader Code Address
	uint32_t  uniform_data;											// Frag Shader Uniforms Addr
	uint32_t  vertex_data;											// Shaded Vertex Data Address
} NV_SHADER_STATE;

bool CreateVertexData (const char* fileName, struct obj_model_t* model, float desiredMaxSize, printhandler prn_handler) {
	if (prn_handler) prn_handler("Loading %s\n", fileName);
	if (model) {
		model->viewMatrix = MultiplyMatrix3D(XRotationMatrix3D(0.78539815), YRotationMatrix3D(0.78539815));

		if (!ParseWaveFrontMesh(fileName, model, false, desiredMaxSize)) return false;
		
		model->IndexVertexCt = model->tri_count*3;
		model->IndexVertexCt += model->quad_count * 6;
		model->IndexVertexCt += model->polygon_count * 9;

		uint32_t memSize = (model->num_verts * sizeof(struct EmitVertex));
		memSize += (model->num_verts * sizeof(struct OriginalVertex));
		memSize += (model->IndexVertexCt * sizeof(uint16_t));

		model->modelHandle = V3D_mem_alloc (memSize, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
		if (model->modelHandle) {
			model->modelDataVC4 = V3D_mem_lock(model->modelHandle);			// Lock the memory
			model->modelDataARM = GPUaddrToARMaddr(model->modelDataVC4);	// Convert locked memory to ARM address
			
			model->vertexVC4 = (model->modelDataVC4 + (model->IndexVertexCt * sizeof(uint16_t)));  // Create pointer
			model->vertexARM = (struct EmitVertex*)(uintptr_t)(model->modelDataARM + (model->IndexVertexCt * sizeof(uint16_t))); // Create pointer

			model->originalVertexARM = (struct OriginalVertex*)(uintptr_t)(model->modelDataARM + 
				(model->IndexVertexCt * sizeof(uint16_t)) + (model->num_verts * sizeof(struct  EmitVertex)));

			if (!ParseWaveFrontMesh(fileName, model, true, desiredMaxSize)) return false;


			uint8_t *p = (uint8_t*)(uintptr_t)model->rendererDataARM;

			// Configuration stuff
			// Tile Binning Configuration.
			//   Tile state data is 48 bytes per tile, I think it can be thrown away
			//   as soon as binning is finished.
			//   A tile itself is 64 x 64 pixels 
			//   we will need binWth of them across to cover the render width
			//   we will need binHt of them down to cover the render height
			//   we add 63 before the division because any fraction at end must have a bin


			//uint_fast32_t binListSize = (binWth * binHt) * sizeof(struct TileListEntry);

			emit_uint8_t(&p, GL_TILE_BINNING_CONFIG);						// tile binning config control 
			emit_uint32_t(&p, model->rendererDataVC4 + BUFFER_TILE_DATA);	// tile allocation memory address
			emit_uint32_t(&p, 0x10000);										// tile allocation memory size
			emit_uint32_t(&p, model->rendererDataVC4 + BUFFER_TILE_STATE);	// Tile state data address
			emit_uint8_t(&p, model->binWth);								// renderWidth/64
			emit_uint8_t(&p, model->binHt);									// renderHt/64
			emit_uint8_t(&p, 0x04);											// config

			// Start tile binning.
			emit_uint8_t(&p, GL_START_TILE_BINNING);						// Start binning command

			// Primitive type
			emit_uint8_t(&p, GL_PRIMITIVE_LIST_FORMAT);
			emit_uint8_t(&p, 0x12);		/* was 0x32 ???? */					// 16 bit triangle

			// Clip Window
			emit_uint8_t(&p, GL_CLIP_WINDOW);								// Clip window 
			emit_uint16_t(&p, 0);											// 0
			emit_uint16_t(&p, 0);											// 0
			emit_uint16_t(&p, model->renderWth);							// width
			emit_uint16_t(&p, model->renderHt);								// height

			// GL State
			emit_uint8_t(&p, GL_CONFIG_STATE);
			emit_uint8_t(&p, 0x03);											// enable both foward and back facing polygons
			emit_uint8_t(&p, 0x00);											// depth testing disabled
			emit_uint8_t(&p, 0x0 /*0x02*/);											// enable early depth write

			// Viewport offset
			emit_uint8_t(&p, GL_VIEWPORT_OFFSET);							// Viewport offset
			emit_uint16_t(&p, 0);											// 0
			emit_uint16_t(&p, 0);											// 0

			// The model
			// No Vertex Shader state (takes pre-transformed vertexes so we don't have to supply a working coordinate shader.)
			emit_uint8_t(&p, GL_NV_SHADER_STATE);
			emit_uint32_t(&p, model->rendererDataVC4 + BUFFER_SHADER_OFFSET);	// Shader Record

			// primitive index list
			#define INDEX_TYPE_8  0x00   //  Indexed_Primitive_List: Index Type = 8 - Bit
			#define INDEX_TYPE_16 0x10   //  Indexed_Primitive_List: Index Type = 16 - Bit
			emit_uint8_t(&p, GL_INDEXED_PRIMITIVE_LIST);					// Indexed primitive list command
			emit_uint8_t(&p, PRIM_TRIANGLE | INDEX_TYPE_16);				// 16bit index, triangles
			emit_uint32_t(&p, model->IndexVertexCt);						// Length
			emit_uint32_t(&p, model->modelDataVC4);							// address of vertex data
			emit_uint32_t(&p, model->MaxIndexVertex + 2);						// Maximum index

			// End of bin list
			// So Flush
			emit_uint8_t(&p, GL_FLUSH_ALL_STATE);
			// Nop
			emit_uint8_t(&p, GL_NOP);
			// Halt
			emit_uint8_t(&p, GL_HALT);

			model->binningConfigLength = (uintptr_t)p - (uintptr_t)model->rendererDataARM;
			
			// Okay now we need Shader Record to buffer
			NV_SHADER_STATE* sp = (NV_SHADER_STATE*)(uintptr_t)(model->rendererDataARM + BUFFER_SHADER_OFFSET);
			sp->flags =  0x01;										// flags
			sp->stride = sizeof(struct EmitVertex);					// stride for VBO
			sp->numUniforms = 0x0; // 0xcc;									// num uniforms (not used)
			sp->numVaryings = 3;									// num varyings
			sp->fragment_shader = (uintptr_t)(model->rendererDataVC4 + BUFFER_FRAGMENT_SHADER); // Fragment shader code
			sp->uniform_data = (uintptr_t)(model->rendererDataVC4 + BUFFER_FRAGMENT_UNIFORM); // Fragment shader uniforms
			sp->vertex_data = model->vertexVC4;						// Vertex with VC4 address

			return true;
		}
		if (prn_handler) prn_handler("Error: Unable to allocate vertex data memory");
	}
	return false;
}


