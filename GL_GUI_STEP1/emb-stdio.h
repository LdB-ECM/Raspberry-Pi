#ifndef _EMB_STDIO_
#define _EMB_STDIO_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{																			}			
{       Filename: emd_stdio.h												}
{       Copyright(c): Leon de Boer(LdB) 2017								}
{       Version: 1.02														}
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
{   of the C standards library stdio.h but for embedded systems. It allows  }
{   easy retargetting of console output to external screen or UART routine  }
{																            }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <stdlib.h>				// Define size_t
#include <stdarg.h>				// Standard C library needed for varadic arguments

/***************************************************************************}
{                         PUBLIC TYPE DEFINITIONS							}
****************************************************************************/

/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*-[Init_EmbStdio]----------------------------------------------------------}
. Initialises the EmbStdio by setting the handler that will be called for
. Each character to be output to the standard console. That routine could be
. a function that puts the character to a screen or something like a UART.
. Until this function is called with a valid handler output will not occur.
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
void Init_EmbStdio ( void (*handler)(char ch));


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{				 	PUBLIC FORMATTED OUTPUT ROUTINES					    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ printf ]---------------------------------------------------------------}
. Writes the C string pointed by fmt to the standard console, replacing any
. format specifier with the value from the provided variadic list. The format
. of a format specifier is %[flags][width][.precision][length]specifier
.
. RETURN:
.	SUCCESS: Positive number of characters written to the standard console.
.	   FAIL: -1
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int printf (const char* fmt, ...);

/*-[ sprintf ]--------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf.
.
. DEPRECATED:
. Using sprintf, there is no way to limit the number of characters written,
. which means the function is susceptible to buffer overruns. It is suggested
. the new function sprintf_s should be used as a replacement. For at least
. some safety the call is limited to writing a maximum of 256 characters.
.
. RETURN:
.	SUCCESS: Positive number of characters written to the provided buffer.
.	   FAIL: -1
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int sprintf (char* buf, const char* fmt, ...);

/*-[ snprintf ]-------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf. This function has protection
. for output buffer size but not for the format buffer. Care should be taken
. to make user provided buffers are not used for format strings which would
. allow users to exploit buffer overruns on the format string.
.
. RETURN:
. Number of characters that are written in the buffer array, not counting the
. ending null character. Excess characters to the buffer size are discarded.
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int snprintf (char *buf, size_t bufSize, const char *fmt, ...);

/*-[ vprintf ]--------------------------------------------------------------}
. Writes the C string formatted by fmt to the standard console, replacing
. any format specifier in the same way as printf, but using the elements in
. variadic argument list identified by arg instead of additional variadics.
.
. RETURN: 
. The number of characters written to the standard console function
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vprintf (const char* fmt, va_list arg);

/*-[ vsprintf ]-------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf.
.
. DEPRECATED:
. Using vsprintf, there is no way to limit the number of characters written,
. which means the function is susceptible to buffer overruns. It is suggested
. the new function vsprintf_s should be used as a replacement. For at least
. some safety the call is limited to writing a maximum of 256 characters.
.
. RETURN:
. The number of characters written to the provided buffer
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vsprintf (char* buf, const char* fmt, va_list arg);

/*-[ vsnprintf ]------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf. This function has protection
. for output buffer size but not for the format buffer. Care should be taken
. to make user provided buffers are not used for format strings which would
. allow users to exploit buffer overruns.
.
. RETURN:
. Number of characters that are written in the buffer array, not counting the
. ending null character. Excess characters to the buffer size are discarded.
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vsnprintf (char* buf, size_t bufSize, const char* fmt, va_list arg);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
