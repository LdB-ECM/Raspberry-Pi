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

int __errno;

//int vsprintf (char *buf, const char *fmt, va_list arg);
//int sprintf (char *buf, const char *fmt, ...);

/*-[ vsscanf ]--------------------------------------------------------------}
{ Reads data from str and stores them according to parameter format in the  }
{ locations pointed by the elements in the variable argument list arg.		}
{ Internally, the function retrieves arguments from the list identified as  }
{ arg as if va_arg was used on it, and thus the state of arg is likely to   }
{ be altered by the call. In any case, arg should have been initialized by  }
{ va_start at some point before the call, and it is expected to be released }
{ by va_end at some point after the call.									}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
int	vsscanf (const char *str, const char *format, va_list arg);


/*-[ sscanf ]---------------------------------------------------------------}
{ Reads data from str and stores them according to parameter format in the  }
{ locations given by the additional arguments, as if scanf was used, but    }
{ reading from buf instead of the standard input (stdin).					}
{ The additional arguments should point to already allocated variables of   }
{ the type specified by their corresponding specifier in the format string. }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
int	sscanf (const char *str, const char *format, ...);

int printf (const char *fmt, ...);



#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
