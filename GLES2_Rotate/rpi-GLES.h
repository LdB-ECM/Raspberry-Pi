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


bool InitV3D (void);
uint32_t V3D_mem_alloc (uint32_t size, uint32_t align, uint32_t flags);
bool V3D_mem_free (uint32_t handle);
uint32_t V3D_mem_lock (uint32_t handle);
bool V3D_mem_unlock (uint32_t handle);
bool V3D_execute_code (uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);
bool V3D_execute_qpu (int32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);

#define GL_FRAGMENT_SHADER	35632
#define GL_VERTEX_SHADER	35633

void DoRotate(float delta);
int LoadShader(int shaderType, printhandler prn_handler);
// Render a single triangle to memory.
void testTriangle (uint16_t renderWth, uint16_t renderHt, uint32_t renderBufferAddr, printhandler prn_handler);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif