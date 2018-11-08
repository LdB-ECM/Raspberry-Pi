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

/* check if GLfloat is defined */
#ifndef GLfloat
#define GLfloat float
#endif

/* check if GLFLOAT_ZERO is defined if not assign it as (GLfloat)0 */
#ifndef GLFLOAT_ZERO
#define GLFLOAT_ZERO ((GLfloat) 0)
#endif

/* check if GLFLOAT_ONE is defined if not assign it as (GLfloat)1 */
#ifndef GLFLOAT_ONE
#define GLFLOAT_ONE ((GLfloat) 1)
#endif


typedef struct vec3 {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} VEC3;

typedef union {
	// Normal C format array which will be ROW MAJOR
	float coef[4][4];
	// OPENGL etc want COLUMN MAJOR so define it so we can transpose when needed
	struct __attribute__((__packed__, aligned(1))) {
		float m00, m01, m02, m03;
		float m10, m11, m12, m13;
		float m20, m21, m22, m23;
		float m30, m31, m32, m33;
	} cm;
} MATRIX3D, *PMATRIX3D;

/* OBJ model structure */
struct obj_model_t
{
	uint32_t rendererHandle;					// Renderer memory handle
	uint32_t rendererDataVC4;					// Renderer data VC4 locked address
	uint32_t rendererDataARM;					// Renderer data ARM locked address
	uint32_t renderWth;							// Render width
	uint32_t renderHt;							// Render height
	uint32_t binWth;							// Bin width
	uint32_t binHt;								// Bin height
	uint32_t binningConfigLength;				// Binning config length
	uint32_t renderLength;						// Renderer data length

	uint32_t modelHandle;						// Model memory handle
	uint32_t modelDataVC4;						// Model data VC4 locked address
	uint32_t modelDataARM;						// Model data ARM locked address

	uint32_t IndexVertexCt;						// Index vertex count
	uint32_t MaxIndexVertex;					// Maximum Index vertex referenced

	uint32_t vertexVC4;							// Physical Vertexes VC4 address
	struct EmitVertex* vertexARM;				// Physical Vertexes ARM address

	struct OriginalVertex* originalVertexARM;	// Original Vertexes ARM address   ... Needed until we sort Shader out


	MATRIX3D viewMatrix;

	uint32_t num_verts;                 /* number of vertices */
	int num_texCoords;                 /* number of texture coords. */
	int num_normals;                   /* number of normal vectors */
	int num_faces;                     /* number of polygons */

	int has_texCoords;                 /* has texture coordinates? */
	int has_normals;                   /* has normal vectors? */

	int tri_count;					   /* triangle face count */
	int quad_count;					   /* quad face count */
	int polygon_count;				   /* polygon face count */

	float minX;
	float maxX;
	float minY;
	float maxY;
	float minZ;
	float maxZ;
	float offsX;
	float offsY;
	float offsZ;
	float scale;
};

bool InitV3D (void);
uint32_t V3D_mem_alloc (uint32_t size, uint32_t align, uint32_t flags);
bool V3D_mem_free (uint32_t handle);
uint32_t V3D_mem_lock (uint32_t handle);
bool V3D_mem_unlock (uint32_t handle);
bool V3D_execute_code (uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);
bool V3D_execute_qpu (int32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);

#define GL_FRAGMENT_SHADER	35632
#define GL_VERTEX_SHADER	35633

void DoRotate(float delta, int screenCentreX, int screenCentreY, struct obj_model_t* model);
// Render a single triangle to memory.
void RenderModel (struct obj_model_t* model, printhandler prn_handler);

bool SetupRenderer(struct obj_model_t* model, uint32_t renderWth, uint32_t renderHt, uint32_t renderBufferAddr);
bool DoneRenderer (struct obj_model_t* model);
bool CreateVertexData (const char* fileName, struct obj_model_t* model, float desiredMaxSize, printhandler prn_handler);


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif