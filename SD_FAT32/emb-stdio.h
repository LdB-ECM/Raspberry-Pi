#ifndef _EMB_STDIO_
#define _EMB_STDIO_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: emd_stdio.h												}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 1.01														}
{																			}		
{***************[ THIS CODE IS FREEWARE UNDER CC Attribution]***************}
{																            }
{      The SOURCE CODE is distributed "AS IS" WITHOUT WARRANTIES AS TO      }
{   PERFORMANCE OF MERCHANTABILITY WHETHER EXPRESSED OR IMPLIED.            }
{   Redistributions of source code must retain the copyright notices to     }
{   maintain the author credit (attribution) .								}
{																			}
{***************************************************************************}
{                                                                           }
{      On embedded system there is rarely a file system or console output   }
{   like that on a desktop system. This file creates the functionality of   }
{   of the C standards library stdio.h but for embedded systems.            }  
{																            }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdarg.h>			// Varadic arguments

int vsprintf (char *buf, const char *fmt, va_list arg);
int sprintf (char *buf, const char *fmt, ...);
int printf (const char *fmt, ...);


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
