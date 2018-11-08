#include <stdbool.h>			// Standard C library needed for bool
#include <stdint.h>				// Standard C library needed for uint8_t, uint32_t etc
#include <stdarg.h>				// Standard C library needed for varadic arguments
#include <float.h>				// Standard C library needed for DBL_EPSILON
#include <string.h>				// Standard C librray needed for strnlen used
#include <stdlib.h>				// Standard C library needed for strtol
#include <math.h>				// Standard C library needed for modf
#include <stddef.h>				// Standard C library needed for ptrdiff_t
#include <ctype.h>				// Standard C library needed for toupper, isdigit etc
#include <wchar.h>				// Standard C library needed for wchar_t
#ifndef _WIN32					// Testing for this unit is done in Windows console need to exclude SmartStart
#include "rpi-SmartStart.h"     // SmartStart system when compiling for embedded system
#endif
#include "emb-stdio.h"			// This units header

/*--------------------------------------------------------------------------}
{ EMBEDDED SYSTEMS DO NOT HAVE M/SOFT BUFFER OVERRUN SAFE STRING FUNCTIONS  }
{    ALL WE CAN DO IS MACRO THE SAFE CALLS TO NORMAL C STANDARD CALLS		} 
{--------------------------------------------------------------------------*/
#ifndef _WIN32					
#define	strncpy_s(a,b,c,d) strncpy(a,c,d)
#define strcpy_s(a,b,c) strcpy(a,c)
#define mbstowcs_s(a,b,c,d,e) mbstowcs(b,d,e)
#define wcscpy_s(a,b,c) wcscpy(a,c)
#endif

/***************************************************************************}
{		 		    PRIVATE VARIABLE DEFINITIONS				            }
****************************************************************************/

/*--------------------------------------------------------------------------}
{				   CONSOLE WRITE CHARACTER HANDLER							}
{--------------------------------------------------------------------------*/
static CHAR_OUTPUT_HANDLER Console_WriteChar = 0;


struct cvt_struct 
{
	unsigned int ndigits : 7;         // Gives max 128 characters
	unsigned int decpt : 7;           // Gives max 128 decimals
	unsigned int sign : 1;            // Sign 0 = positive   1 = negative
	unsigned int eflag : 1;			  // Exponent flag
};

static char *cvt(double arg, struct cvt_struct* cvs, char *buf, int CVTBUFSIZE) {
	int r2;
	double fi, fj;
	char *p, *p1;

	if (CVTBUFSIZE > 128) CVTBUFSIZE = 128;
	r2 = 0;
	cvs->sign = false;
	p = &buf[0];
	if (arg < 0) {
		cvs->sign = true;
		arg = -arg;
	}
	arg = modf(arg, &fi);
	p1 = &buf[CVTBUFSIZE];

	if (fi != 0) {
		p1 = &buf[CVTBUFSIZE];
		while (fi != 0) {
			fj = modf(fi / 10, &fi);
			*--p1 = (int)((fj + .03) * 10) + '0';
			r2++;
			cvs->ndigits++;
		}
		while (p1 < &buf[CVTBUFSIZE]) *p++ = *p1++;
	}
	else if (arg > 0) {
		while ((fj = arg * 10) < 1) {
			arg = fj;
			r2--;
		}
	}
	p1 = &buf[CVTBUFSIZE - 2];
	if (cvs->eflag == 0) p1 += r2;
	cvs->decpt = r2;
	if (p1 < &buf[0]) {
		buf[0] = '\0';
		return buf;
	}
	while (p <= p1 && p < &buf[CVTBUFSIZE] && arg > DBL_EPSILON) {
		arg *= 10;
		arg = modf(arg, &fj);
		*p++ = (int)fj + '0';
		cvs->ndigits++;
	}
	/*p = p1;
	*p1 += 5;
	while (*p1 > '9') {
		*p1 = '0';
		if (p1 > buf) {
			++*--p1;
		}
		else {
			*p1 = '1';
			(cvs->decpt)++;
			if (cvs->eflag == 0) {
				if (p > buf) *p = '0';
				p++;
			}
		}
	}*/
	*p = '\0';
	return buf;
}

/*--------------------------------------------------------------------------}
. This formats a number and returns a string pointed to by txt
. 13Oct2017 LdB
.--------------------------------------------------------------------------*/
static int NumStr (CHAR_OUTPUT_HANDLER out, double value)
{
	uint_fast8_t count = 0;
	struct cvt_struct cvs = { 0 };
	char Ns[256];

	if (out == 0 ) return (0);										// Handler error
	cvt(value, &cvs, &Ns[0], sizeof(Ns));							// Create number string of given dp
	if (cvs.sign) {
		out('-');													// We have a negative number
		count++;													// Increment count
	}
	if (cvs.decpt == 0) {
		out('0');													// We must be "0.xxxxx
		count++;													// Increment count
	} else {
		for (uint_fast8_t i = 0; i < cvs.decpt; i++)
		{
			out(Ns[i]);												// Copy the whole number
			count++;												// Increment count
		}
	}
	if (cvs.decpt > 0) {											// We have decimal point
		out('.');													// Add decimal point
		count++;													// Increment count
		for (uint_fast8_t i = cvs.decpt; i < cvs.ndigits; i++)
		{
			out(Ns[i]);												// Move decimal number
			count++;												// Increment count
		}
	}
	out('\0');														// Make asciiz
	return (count);													// String end returned
}


struct icvt_flags
{
	unsigned zeropad : 1;		/* pad with zero */
	unsigned isSigned : 1;		/* Signed number format so will need to display neg/pos */
	unsigned plus : 1;			/* show plus sign */
	unsigned space : 1;			/* space if plus */
	unsigned left : 1;			/* left justified */
	unsigned small : 1;			/* Must be 32 == 0x20 */
	unsigned special : 1;		/* 0x */
	unsigned dbl_ls : 1;		/* ll or hh specifier */
	unsigned base : 5;			/* Base 2, 8, 10, 16 etc */
	unsigned _reserved : 3;		/* reserved */
	signed field_width : 8;		/* maximum character size for display */
	signed precision : 8;		/* precision */
};


/* we are called with base 2, 8, 10 or 16 */
static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

static int cvt_int (CHAR_OUTPUT_HANDLER out, int num, struct icvt_flags flags)
{
	char tmp[36];													// This is only an integer so assume binary display as worse case 0bxxxxxx		
	char locase, padchar;
	int maxoutsize;
	int count = 0;		
	char sign = 0;


	if (flags.left) flags.zeropad = 0;								// If left align then you can't zero pad
	if ( flags.base != 2 && flags.base != 8 &&
		 flags.base != 16 ) flags.base = 10;						// If base isn't 2, 8 or 16 then assume its base 10
	locase = (flags.small) ? 0x20 : 0x0;							// We OR this onto each letter to make lower case .. A=0x41 a =0x61 etc	
	padchar = (flags.zeropad) ? '0' : ' ';							// Pad character is either zero or space
	maxoutsize = flags.field_width;									// Transfer max size

	if (flags.isSigned) 
	{																// Display is of a signed integer
		if (num < 0) 
		{															// Number is negative										
			sign = '-';												// Will need to output negative sign character
			maxoutsize--;											// One character used if width specified
			num = -num;												// Convert number from negative to positive																			
		}
		else if (flags.plus) {										// Display plus sign flag set
			sign = '+';												// Output positive sign character
			maxoutsize--;											// One character used if width specified
		}
		else if (flags.space) {										// Space flag set
			sign  = ' ';											// Output space character
			maxoutsize--;											// One character used if width specified
		}
	}

	int i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else
		while (num != 0)
		{
			int j = num % flags.base;
			num /= flags.base;
			tmp[i++] = (digits[j] | locase);
		}



	if (i > flags.precision) flags.precision = i;
	maxoutsize -= flags.precision;
	if (!(flags.zeropad + flags.left))
		while (maxoutsize-- > 0) {
			out(' ');
			count++;
		}


	if (!flags.left && !flags.zeropad)								// If not left align or zero padded
		while (maxoutsize-- > 0) {									// Field width is set to a specific value
			out(padchar);											// Output the pad character 
			count++;												// Inc character output count
		}

	while (i < flags.precision--) {									// While precision count greater than created string
		out('0');													// Output a "0" character as pad
		count++;													// Inc character output count
	}
	if (sign) {														// If display signed character is a non zero character
		out(sign);													// Output the sign character that has been set
		count++;													// Inc character output count
	}
	if (flags.special) {											// Special flag set so we will do Binary (0b), Hex (0x) or Octal (0)
		switch (flags.base)
		{
		case 2:														// Binary display 0b??????????
		{
			out('0');												// Output the zero character
			count++;												// Inc character output count											
			out('B' | locase);										// Output the 'b' or 'B' character
			count++;												// Inc character output count
			break;
		}
		case 8:														// Octal display 0????????
		{
			out('0');												// Output the zero character
			count++;												// Inc character output count
			break;
		}
		case 16:													// Hex display 0x?????????
		{
			out('0');												// Output the zero character
			count++;												// Inc character output count												
			out('X' | locase);										// Output the 'x' or 'X' character
			count++;												// Inc character output count
			break;
		}
		}
	}
	while (i-- > 0) {												// For each character in display string (remember string is backwards)
		out(tmp[i]);												// Output the character												
		count++;													// Inc character output count
	}
	while (maxoutsize-- > 0) {										// Field width is set to a specific value
		out(' ');													// Output a space character
		count++;													// Inc character output count
	}
	return count;													// Retunr the character output count
}



static int number (CHAR_OUTPUT_HANDLER out, long num, struct icvt_flags flags)
{
	char tmp[128];													// This is a long so assume binary display as worse case 0bxxxxxx		
	char locase, padchar;
	int maxoutsize;
	int count = 0;
	char sign = 0;

	if (flags.left) flags.zeropad = 0;								// If left align then you can't zero pad
	if (flags.base != 2 && flags.base != 8 &&
		flags.base != 16) flags.base = 10;						// If base isn't 2, 8 or 16 then assume its base 10
	locase = (flags.small) ? 0x20 : 0x0;							// We OR this onto each letter to make lower case .. A=0x41 a =0x61 etc	
	padchar = (flags.zeropad) ? '0' : ' ';							// Pad character is either zero or space
	maxoutsize = flags.field_width;									// Transfer max size

	if (flags.isSigned)
	{																// Display is of a signed integer
		if (num < 0)
		{															// Number is negative										
			sign = '-';												// Will need to output negative sign character
			maxoutsize--;											// One character used if width specified
			num = -num;												// Convert number from negative to positive																			
		}
		else if (flags.plus) {										// Display plus sign flag set
			sign = '+';												// Output positive sign character
			maxoutsize--;											// One character used if width specified
		}
		else if (flags.space) {										// Space flag set
			sign = ' ';											// Output space character
			maxoutsize--;											// One character used if width specified
		}
	}

	int i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else
		while (num != 0)
		{
			int j = num % flags.base;
			num /= flags.base;
			tmp[i++] = (digits[j] | locase);
		}



	if (i > flags.precision) flags.precision = i;
	maxoutsize -= flags.precision;
	if (!(flags.zeropad + flags.left))
		while (maxoutsize-- > 0) {
			out(' ');
			count++;
		}


	if (!(flags.left))
		while (maxoutsize-- > 0) {
			out(padchar);
			count++;
		}


	while (i < flags.precision--) {
		out('0');
		count++;
	}

	if (sign)
	{
		out(sign);
		count++;
	}

	if (flags.special) {											// Special flag set so we will do Hex (0x) or Octal (0) char
		switch (flags.base)
		{
		case 2:														// Binary display 0b??????????
		{
			out('0');												// Output the zero character
			count++;												// One character output												
			out('B' | locase);										// Output the 'b' or 'B' character
			count++;												// One character output
			break;
		}
		case 8:														// Octal display 0????????
		{
			out('0');												// Output the zero character
			count++;												// One character output
			break;
		}
		case 16:													// Hex display 0x?????????
		{
			out('0');												// Output the zero character
			count++;												// One character output												
			out('X' | locase);										// Output the 'x' or 'X' character
			count++;												// One character output
			break;
		}
		}
	}



	while (i-- > 0) {
		out(tmp[i]);
		count++;
	}

	while (maxoutsize-- > 0) {
		out(' ');
		count++;
	}
	return count;
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{						 	PRIVATE ROUTINES							    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ INTERNAL: Process_Out ]------------------------------------------------}
. Provides most of the output for all the routines in this unit. It is built
. around the function pointer of type CHAR_OUTPUT_HANDLER. This could be a 
. routine that outputs to a console, a buffer, a UART or a file as just some
. possibilities. The function knows no specifics of what is beyond the call
. other than once it has a character to output that is what it shoudl call.
.
. RETURN:
.	SUCCESS: Positive number of characters written to the provided handler
.	   FAIL: 0 no characters were output to the provided handler
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
static size_t Process_Out (CHAR_OUTPUT_HANDLER out, const char *fmt, size_t fmtSize, va_list args)
{
	int count = 0;													// Preset output count to zero
	size_t cntfmt = 0;												// Preset count format processed to zero
	if (out == 0 || fmt == 0 || fmtSize == 0)  return 0;			// Immediate fail if no handler or valid format string
	while (fmt[cntfmt] != '\0' && cntfmt < fmtSize)					// While we have stuff in format string to process
	{									
		bool processed = false;										

		if (fmt[cntfmt] != '%')										// If fmt is not a format command
		{
			out(fmt[cntfmt]);										// Output the character directly
			count++;												// One character output											
		}
		else {														// Okay we have format command "%"
			struct icvt_flags flags = { 0 };						// flags to number()
			flags.field_width = -1;									// Preset width of output field to -1
			flags.precision = -1;									// Preset precision field to -1
			flags.base = 10;										// Preset base to 10
			int qualifier = -1;										// Qualifier can be 'h', 'l', 'j', 'z', 't' or 'L' but starts -1
			bool procFlag = true;
			while (procFlag == true) 
			{
				cntfmt++;											// Next format character
				if (cntfmt >= fmtSize) return(count);				// Immediate exit if format string abnormal terminated
				switch (fmt[cntfmt]) {								// Look at that next character
				case '-':
					flags.left = 1;									// Set left flag
					break;
				case '+':
					flags.plus = 1;									// Set plus flag
					break;
				case ' ':
					flags.space = 1;								// Set space flag
					break;
				case '#':
					flags.special = 1;								// Set special flag
					break;
				case '0':
					flags.zeropad = 1;								// Set zero flag
					break;
				default:
					procFlag = false;								// Any other character exits
					break;
				}
			}
			if (fmt[cntfmt] == '*') 								// Width is next argument
			{							
				cntfmt++;											// Next format character
				if (cntfmt >= fmtSize) return(count);				// Immediate exit if format string abnormal terminated
				flags.field_width = va_arg(args, int);				// Read the next argument as the field width
				if (flags.field_width < 0) {						// If the value is negative
					flags.field_width = -flags.field_width;			// Set field width to positive value
					flags.left = 1;									// But set the left flag
				}
			}
			else {													// Check for number width following													
				while (isdigit(fmt[cntfmt])) {						// If it is a digit its a field width specifier
					if (flags.field_width < 0) flags.field_width = 0;// Zero field width remember it starts -1
					flags.field_width *= 10;						// Multiply current field width value by 10
					flags.field_width += (fmt[cntfmt] - '0');		// Add the digit to the value
					cntfmt++;										// Next format character
					if (cntfmt >= fmtSize) return(count);			// Immediate exit if format string abnormal terminated
				}
			}
			if (fmt[cntfmt] == '.')									// Precision is next argument
			{
				cntfmt++;											// Next format character
				if (cntfmt >= fmtSize) return(count);				// Immediate exit if format string abnormal terminated
				if (fmt[cntfmt] == '*') 							// Precision is next argument
				{
					cntfmt++;										// Next format character
					if (cntfmt >= fmtSize) return(count);			// Immediate exit if format string abnormal terminated
					flags.precision = va_arg(args, int);			// Read the next argument as the precision
					if (flags.precision < 0) flags.precision = 0;	// Cant have negatives
				}
				else {												// Check for number width following		
					while (isdigit(fmt[cntfmt])) {					// If it is a digit its a field width specifier
						if (flags.precision < 0) flags.precision = 0;// Zero precision remember it starts -1
						flags.precision *= 10;						// Multiply current field width value by 10
						flags.precision += (fmt[cntfmt] - '0');		// Add the digit to the value
						cntfmt++;									// Next format character
						if (cntfmt >= fmtSize) return(count);		// Immediate exit if format string abnormal terminated
					}
				}
			}
			if ((fmt[cntfmt] == 'h') || (fmt[cntfmt] == 'l') ||
				(fmt[cntfmt] == 'j') || (fmt[cntfmt] == 'z') ||
				(fmt[cntfmt] == 't') || (fmt[cntfmt] == 'L'))		// Special conversion qualifier found
			{					
				qualifier = fmt[cntfmt];							// Hold conversion qualifier
				if ( ((fmt[cntfmt] == 'h') || (fmt[cntfmt] == 'l'))	// We  need to check for special cases hh and ll
					&& (cntfmt < fmtSize-1)	)						// We need to make sure there is a next fmt character			
				{				
					if (fmt[cntfmt+1] == fmt[cntfmt]) 				// Peek at next character for match ... 'hh' or 'll'
					{
						flags.dbl_ls = 1;							// Set the double LS flag
						cntfmt++;									// Next format character
						if (cntfmt >= fmtSize) return(count);		// Immediate exit if format string abnormal terminated
					}
				}
				cntfmt++;											// Next format character
				if (cntfmt >= fmtSize) return(count);				// Immediate exit if format string abnormal terminated
			}

			switch (fmt[cntfmt])									// Process the format specifier type
			{
			case 'c':												// Format specifier is character
			{
				if (!(flags.left))									// Left pad flag set
				{
					if (flags.field_width > 0) flags.field_width--;	// One field will be character
					while (flags.field_width > 0) {					// While field width greater than zero
						out(' ');									// Output space character directly
						count++;									// One character output
						flags.field_width--;						// One field position filled
					}
				}
				if (qualifier == 'l')								// Length specifier 'l' 
				{
					wchar_t wc = va_arg(args, wchar_t);				// Read the next argument as a wide character
					out((char)wctob(wc));							// Narrow to ascii and output
					count++;										// One character output
				}
				else {
					char ac = (char)va_arg(args, int);				// Read the next argument as a character
					out(ac);										// Output the character
					count++;										// One character output
				}
				if (flags.field_width > 0) flags.field_width--;		// Character occupies 1 character
				while (flags.field_width > 0) {						// If the field width is greater than zero
					out(' ');										// Output space character directly
					count++;										// One character output
					flags.field_width--;									// One field position filled
				}
				processed = true;									// All processing of specifier complete
				break;
			}														// end case 'c'
			case 's':												// Format specifier is string
			{
				const char *s = va_arg(args, char *);				// Read the string pointer address
				int len = strnlen(s, flags.precision);				// Read the length of the provided string
				if (!(flags.left))									// Left pad flag set
				{
					if (len < flags.field_width) flags.field_width -= len;// Remove length of string
						else flags.field_width = 0;					// String takes all the field
					while (flags.field_width > 0) {					// While field width greater than zero
						out(' ');									// Output space character directly
						count++;									// One character output
						flags.field_width--;						// One field position filled
					}
				}
				for (int i = 0; i < len; i++)						// For each character in string
				{
					out(s[i]);										// Output the string character directly
					count++;										// One character output
				}
				if (flags.field_width > 0 && len < flags.field_width)
					flags.field_width -= len;						// Remove length of string
				while (flags.field_width > 0) {						// If the field width is greater than zero
					out(' ');										// Output space character directly
					count++;										// One character output
					flags.field_width--;							// One field position filled
				}
				processed = true;									// All processing of specifier complete
				break;
			}														// end case 's'
			case '%':												//  Format specifier is "%%" means we want the actual % sign character
			{
				out('%');											// Output % character directly
				count++;											// One character output
				processed = true;									// All processing of specifier complete
				break;
			}														// end case '%'
			case 'o':												// Format specifier is unsigned octal 
			{
				flags.base = 8;										// Change base to 8 for octal obviously
				break;
			}														// end case 'o'
			case 'x':												// Format specifier is unsigned hex
				flags.small = 1;									// Set small flag and drop thru to 'X'
			case 'X':												// Format specifier is unsigned hex
			{
				flags.base = 16;									// HEX so base 16 obviously
				break;
			}														// End of case 'x'

			case 'd':												// Format specifier is Decimal
			case 'i':												// Format specifier is Integer
				flags.isSigned = 1;									// Set isSigned flag and drop thru to 'u'
			case 'u':
				break;												// Just exit
			case 'p':												// Format specifier is pointer
			{
				if (flags.field_width == -1) {						// No specific field width
					flags.field_width = 2 * sizeof(void *);			// Width will be twice size of pointer 2chars per hex character
					flags.zeropad = 1;								// Set zero pad flag
				}
				flags.base = 16;									// Base 16
				count += number(out, 
					(unsigned long)va_arg(args, void *), flags);	// Output pointer
				processed = true;									// All processing of specifier complete
				break;
			}														// end of case 'p'
			case 'n':												// Format specifier is store number of characters written to pointed location
			{
				switch (qualifier) 
				{
				case 'h':
				{
					if (flags.dbl_ls)								// qualifier is 'hh'
					{
						signed char* ip = va_arg(args, signed char*);// Pointer to a signed char
						*ip = count;								// store count there
					}
					else {
						short* ip = va_arg(args, short*);			// Pointer to a short
						*ip = count;								// store count there
					}
					break;
				}													// end of case 'h'
				case 'l':
				{
					if (flags.dbl_ls)								// qualifier is 'll'
					{
						long long *ip = va_arg(args, long long*);	// Pointer to a long long int
						*ip = count;								// Store count there
					}
					else {
						long *ip = va_arg(args, long*);				// Pointer to a long int
						*ip = count;								// stopr count there
					}
					break;
				}													// end of case 'l'
				case 'j':
				{
					intmax_t* ip = va_arg(args, intmax_t*);			// Pointer to a intmax_t
					*ip = count;									// Store count there
					break;
				}													// end of case 'j'
				case 'z':
				{
					size_t* ip = va_arg(args, size_t*);				// Pointer to a size_t
					*ip = count;									// Store count there
					break;
				}													// end of case 'z'
				case 't':
				{
					ptrdiff_t* ip = va_arg(args, ptrdiff_t*);		// Pointer to a ptrdiff_t
					*ip = count;									// Store count there
					break;
				}													// end of case 't'
				case 'L':
				{
					long double* ip = va_arg(args, long double*);	// Pointer to a long double
					*ip = count;									// Store count there
					break;
				}													// end of case 'L'
				default:
				{
					int *ip = va_arg(args, int *);					// Pointer to an int
					*ip = count;									// Store count there								
					break;
				}													// end of default case
				}													// end of switch qualifier
				processed = true;									// All processing of specifier complete
				break;
			}														// end of case 'n'
			case 'a':												// Format specifier is Hexadecimal floating point, lowercase
			case 'A':												// Format specifier is Hexadecimal floating point, uppercase
			case 'e':												// Format specifier is Scientific notation (mantissa/exponent), lowercase
			case 'E':												// Format specifier is Scientific notation (mantissa/exponent), uppercase
			case 'f':												// Format specifier is Decimal floating point, lowercase
			case 'F':												// Format specifier is Decimal floating point, uppercase
			case 'g':												// Format specifier is Use the shortest representation: %e or %f
			case 'G':												// Format specifier is Use the shortest representation: %E or %F
			{
				if (qualifier == 'L')
				{
					long double ld = va_arg(args, long double);		// Load the long value
					count += NumStr(out, ld);						// Output the long double
				} else 
				{
					double d = (float)va_arg(args, double);			// Load double value
					count += NumStr(out, d);						// Output the double
				}
				processed = true;									// Set processed flag if float/double/long double handled
				break;
			}														// end of case 'a','A',e','E','f','F','g','G'
			}														// End switch fmt[cntfmt]

			if (processed == false)									// If we havent yet completed processed
			{
				switch (qualifier)
				{
				case 'h':
				{
					if (flags.dbl_ls)								// qualifier is 'hh'
					{
						signed char ip = va_arg(args, int);			// Load signed char
						count += cvt_int(out, ip, flags);			// Output signed character
					}
					else {
						short ip = va_arg(args, int);				// Load signed short
						count += cvt_int(out, ip, flags);			// Output short
					}
					break;
				}													// end of case 'h'
				case 'l':
				{
					if (flags.dbl_ls)								// qualifier is 'll'
					{
						long long ip = va_arg(args, long long);		// Load a long long int
						count += number(out, ip, flags);			// Output long long
					}
					else {
						long ip = va_arg(args, long);				// Load a long int
						count += number(out, ip, flags);			// Output long
					}
					break;
				}													// end of case 'l'
				case 'j':
				{
					intmax_t ip = va_arg(args, intmax_t);			// Load a intmax_t
					count += cvt_int(out, ip, flags);				// Output intmax_t
					break;
				}													// end of case 'j'
				case 'z':
				{
					size_t ip = va_arg(args, size_t);				// Load a size_t
					count += number(out, ip, flags);				// Output size_t
					break;
				}													// end of case 'z'
				case 't':
				{
					ptrdiff_t ip = va_arg(args, ptrdiff_t);			// Load a ptrdiff_t
					count += number(out, ip, flags);				// Output ptrdiff_t
					break;
				}													// end of case 't'
				default:
				{
					int ip = va_arg(args, int);						// Load an int
					count += cvt_int(out, ip, flags);				// Output int	
					break;
				}													// end of default case
				}													// end of switch qualifier

			}

		}
		cntfmt++;													// Next format character	
	}
	return count;
}



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{				 	PUBLIC FORMATTED OUTPUT ROUTINES					    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ vsscanf ]--------------------------------------------------------------}
{ Reads data from buf and stores them according to parameter format in the  }
{ locations pointed by the elements in the variable argument list arg.		}
{ Internally, the function retrieves arguments from the list identified as  }
{ arg as if va_arg was used on it, and thus the state of arg is likely to   }
{ be altered by the call. In any case, arg should have been initialized by  }
{ va_start at some point before the call, and it is expected to be released }
{ by va_end at some point after the call.									}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
/* Built around: http://www.cplusplus.com/reference/Cstdio/scanf/           }
{  Format specifier takes form:   "%[*][width][length]specifier"            }
{--------------------------------------------------------------------------*/
int	vsscanf (const char *str, const char *fmt, va_list arg) 
{
	int count = 0;													// Count of specifiers matched
	int base = 10;													// Specifier base
	int width = 0;													// Specifier width
	const char* tp = str;											// Temp pointer starts at str
	const char* spos;												// Start specifier position
	char specifier, lenSpecifier;									// Specifier characters
	char* endPtr;													// Temp end ptr
	char tmp[256];													// Temp buffer
	bool dontStore = false;											// Dont store starts false
	bool notSet = false;											// Not in set starts false
	bool doubleLS = false;											// Double letter of length specifier

	if ( (!fmt) || (!str)) return (0);								// One of the pointers invalid
	while (*tp != '\0' && *fmt != '\0') {
		while (isspace(*fmt)) fmt++;								// find next format specifier start
		if (*fmt == '%')											// Format is a specifier otherwise its a literal
		{
			fmt++;													// Advance format pointer
			if (*fmt == '*') {										// Check for dont store flag
				dontStore = true;									// Set dont store flag
				tp++;												// Next character
			}
			if (isdigit(*fmt)) {									// Look for width specifier
				int cc;
				const char* wspos = fmt;						    // Width specifier start pos
				while (isdigit(*fmt)) fmt++;						// Find end of width specifier
				cc = fmt - wspos;									// Character count for width
				strncpy_s(&tmp[0], _countof(tmp), wspos, cc);		// Copy width specifier
				tmp[cc] = '\0';										// Terminate string
				width = strtol(wspos, &endPtr, 10);					// Calculate specifier width
			}
			else width = 256;
			if ((*fmt == 'h') || (*fmt == 'l') ||
				(*fmt == 'j') || (*fmt == 'z') ||
				(*fmt == 't') || (*fmt == 'L')) {					// Special length specifier
				lenSpecifier = *fmt;								// Hold length specifier
				if ((*fmt == 'h') || (*fmt == 'l')) {				// Check for hh and ll
					if (fmt[1] == *fmt) 							// Peek at next character for match ... 'hh' or 'll'
					{
						doubleLS = true;							// Set the double LS flag
						fmt++;										// Next format character
					}
				}
				fmt++;												// Next format character
			} else lenSpecifier = '\0';								// No length specifier
			specifier = *fmt++;										// Load specifier					

			/* FIND START OF SPECIFIER DATA  */
			switch (specifier) {
			case '\0':												// Terminator found on format string
			{
				return (count);										// Format string ran out of characters
			}

			case '[':												// We have a Set marker
			{
				if (*fmt == '^') {									// Not in set requested
					notSet = true;									// Set notSet flag
					fmt++;											// Move to next character
				}
				break;
			}

			/* string or character find start */
			case 'c':
			case 's':
			{
				// We are looking for non white space to start
				while (isspace(*tp)) tp++;
				break;
			}

			/* Integer/decimal/unsigned/hex/octal and pointer find start */
			case 'i':
			case 'd':
			{
				// We are looking for  -, + or digit to start number
				while (!(isdigit(*tp) || (*tp == '-') || (*tp == '+')) && (*tp != '\0'))  tp++;
				break;
			}

			/* unsigned/hex/octal and pointer find start */
			case 'u':
			case 'x':
			case 'X':
			case 'o':
			case 'p':
			{
				// We are looking for  digit to start number
				while (!isdigit(*tp) && (*tp != '\0'))  tp++;
				break;
			}


			/* Floats/double/double double find start */
			case 'a':
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			{
				// We are looking for  -,'.' or digit to start number
				while (!(isdigit(*tp) || (*tp == '.') || (*tp == '-') || (*tp == '+')) && (*tp != '\0'))  tp++;
				break;
			}

			}  /* end switch */

			/* HOLD MATCH STARTED POSITION */
			if (specifier != '[') {									// Not a creating set characters
				if (*tp == '\0') return(count);						// End of string reached

				spos = tp;											// String match starts here
				tp++;												// Move to next character

				if (*tp == '\0') return(count);						// End of string reached
			}
			else {
				if (*fmt == '\0') return(count);					// End of format reached

				spos = fmt;											// String match starts here
				fmt++;												// Move to next format character

				if (*fmt == '\0') return(count);					// End of format reached
			}

			/* FIND END OF SPECIFIER DATA */
			switch (specifier) {


			/* character find end */
			case 'c':
			{
				break;
			}

			/* set find end */
			case '[':
			{
				while (*fmt != ']' && *fmt != '\0') fmt++;			// Keep reading until set end
				break;
			}

			/* string find end */
			case 's':
			{
				// We are looking for white space to end
				while (!isspace(*tp) && (*tp != '\0') && width != 0) {
					tp++;
					width--;
				}
				break;
			}

			/* decimal and unsigned integer find end */
			case 'd':
			case 'u':
			{
				// We are looking for non digit to end number
				while (isdigit(*tp))  tp++;
				break;
			}

			/* octal find end */
			case 'o':
			{
				base = 8;													// Set base to 8
				while (*tp >= '0' && *tp <= '7') tp++;						// Keep reading while characters are 0-7
				break;
			}

			/* octal find end */
			case 'x':
			case 'X':
			{
				base = 16;													// Set base to 16
				while (isdigit(*tp) || (*tp >= 'a' && *tp <= 'f')
					|| (*tp >= 'A' && *tp <= 'F'))  tp++;					// keep advancing so long as 0-9, a-f, A-F
				break;
			}

			/* integer and pointer find end ... allows 0x  0b 0 at start for binary,hex and octal and ends when no digits */
			case 'i':
			case 'p':
			{
				if (*spos == '0') {
					switch (*tp) {
					case 'x': {												// Hex entry
						base = 16;											// Set base to 16
						tp++;												// Advance tp
						while (isdigit(*tp) || (*tp >= 'a' && *tp <= 'f')
							|| (*tp >= 'A' && *tp <= 'F'))  tp++;			// keep advancing so long as 0-9, a-f, A-F
						break;
					}
					case 'b': {												// Binary entry
						base = 2;											// Set base to 2
						tp++;												// Advance tp
						while (*tp == '0' || *tp == '1') tp++;				// Keep scanning so long as we have 0 or 1's
						break;
					}
					default:												// Octal entry
						base = 8;											// Set base to 8
						tp++;												// Advance tp
						while (*tp >= '0' && *tp <= '7') tp++;				// Keep reading while characters are 0-7
						break;
					} /* switch */
				}
				else {
					// We are looking for any non decimal digit to end number
					while (isdigit(*tp))  tp++;
				}
				break;
			}

			/* Floats/double/double double find end */
			case 'a':
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			{
				bool exitflag = false;
				bool decflag = false;
				bool eflag = false;
				if (*spos == '.') decflag = true;
				do {
					if (!decflag && *tp == '.') {
						decflag = true;
						tp++;
					}
					if (!eflag && (*tp == 'e' || *tp == 'E')) {
						eflag = true;
						tp++;
						/* look for negative index  E-10 etc*/
						if (*tp == '-') tp++;
					}
					if (isdigit(*tp)) tp++;
					else exitflag = true;
				} while (!exitflag);
				break;
			}

			}  /* switch end */


			/* COPY DATA FROM MATCH START TO END TO TEMP BUFFER */
			if (specifier != 'c') {
				int cc;
				if (specifier != '[') cc = tp - spos;				// Not set specifier is from data
					else cc = fmt - spos;							// Set specifier is from format
				strncpy_s(&tmp[0], _countof(tmp), spos, cc);		// Copy to temp buffer
				tmp[cc] = '\0';										// Terminate string
			}

			/* PROCESS THE FOUND SPECIFIER */
			switch (specifier) {

			case 'u':												// Unsigned integer ... base = 10
			case 'o':												// Octal .. same as 'u' but base = 8
			case 'x':												// Hex .. same as 'u' but base = 16
			case 'X':												// Hex .. same as 'u' but base = 16
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					unsigned int i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned integer	
					*va_arg(arg, unsigned int *) = i;				// Store the value
					break;
				}
				case 'h':
				{
					if (doubleLS) {									// Length specifier was "hh"
						unsigned char i;
						i = (unsigned char) strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned char *) = i;			// Store the value
						break;
					}
					else {											// Length specifier was "h"
						unsigned short i;
						i = (unsigned short) strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned short *) = i;			// Store the value
						break;
					}
				}
				case 'l':
				{
					if (doubleLS) {									// Length specifier was "ll"
						unsigned long long i;
						i = strtoull(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned long long *) = i;		// Store the value
						break;
					}
					else {											// Length specifier was "l"
						unsigned long i;
						i = strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned long *) = i;			// Store the value
						break;
					}
				}
				case 'j':											// Length specifier was 'j'
				{
					uintmax_t i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned char
					*va_arg(arg, uintmax_t *) = i;					// Store the value
					break;
				}
				case 'z':											// Length specifier was 'z'
				{
					size_t i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned char
					*va_arg(arg, size_t *) = i;						// Store the value
					break;
				}
				case 't':											// Length specifier was 't'
				{
					ptrdiff_t i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned char
					*va_arg(arg, ptrdiff_t *) = i;					// Store the value
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}
			case 'd':												// Decimal integer .. base = 10
			case 'i':												// Integer .. same as d but base got from string
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					int i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, int *) = i;						// Store the value 
					break;
				}
				case 'h':
				{
					if (doubleLS) {									// Length specifier was "hh"
						signed char i;
						i = (signed char) strtol(&tmp[0], &endPtr, base); // Convert integer
						*va_arg(arg, signed char *) = i;			// Store the value 
						break;
					}
					else {											// Length specifier was "h"
						short i;
						i = (short) strtol(&tmp[0], &endPtr, base);	// Convert integer
						*va_arg(arg, short *) = i;					// Store the value 
						break;
					}
				}
				case 'l':
				{
					if (doubleLS) {									// Length specifier was "ll"
						long long i;
						i = strtoll(&tmp[0], &endPtr, base);		// Convert integer
						*va_arg(arg, long long *) = i;				// Store the value 
						break;
					}
					else {											// Length specifier was "l"
						long i;
						i = strtol(&tmp[0], &endPtr, base);			// Convert integer
						*va_arg(arg, long *) = i;					// Store the value 
						break;
					}
				}
				case 'j':											// Length specifier was 'j' 
				{
					intmax_t i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, intmax_t *) = i;					// Store the value 
					break;
				}
				case 'z':											// Length specifier was 'z' 
				{
					size_t i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, size_t *) = i;						// Store the value 
					break;
				}
				case 't':											// Length specifier was 't' 
				{
					ptrdiff_t i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, ptrdiff_t *) = i;					// Store the value 
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}
			case 'e':
			case 'g':
			case 'E':
			case 'G':
			case 'f':
			case 'F':												// Float
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					float f;
					f = strtof(&tmp[0], &endPtr);					// Convert float
					*va_arg(arg, float *) = f;						// Store the value
					break;
				}
				case 'l':
				{
					double d;
					d = strtod(&tmp[0], &endPtr);					// Convert double
					*va_arg(arg, double *) = d;						// Store the value
					break;
				}
				case 'L':
				{
					long double ld;
					ld = strtold(&tmp[0], &endPtr);					// Convert long double
					*va_arg(arg, long double *) = ld;				// Store the value
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}


			case 'p':												// Pointer  -- special no length specifier possible
			{
				void* p;
				p = (void*)(uintptr_t)strtol(&tmp[0], &endPtr, 16);	// Convert hexadecimal
				if (!dontStore) *va_arg(arg, void **) = p;			// If dontstore flag false then Store the value
				break;
			}


			case 's':												// String
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					strcpy_s(va_arg(arg, char *), _countof(tmp), &tmp[0]);// If dontstore flag false then Store the value
					break;
				}
				case 'l':											// Length specifier 'l' 
				{
					wchar_t wc[256];								// Temp buffer
					size_t cSize = strlen(&tmp[0]) + 1;				// Size of text scanned
                    #ifdef _WIN32
					size_t oSize;
					#endif
					mbstowcs_s(&oSize, &wc[0], cSize, &tmp[0], cSize); // Convert to wide character
					wcscpy_s(va_arg(arg, wchar_t *), _countof(wc), &wc[0]); // Store the wide string
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}

			case 'c':												// Character
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					char c = *spos;
					*va_arg(arg, char *) = c;						// Store the value
					break;
				}
				case 'l':											// Length specifier 'l' 
				{
					wchar_t wc;
					#ifdef _WIN32
					size_t oSize;
					#endif
					mbstowcs_s(&oSize, &wc, 1, spos, 1);			// Convert to wide character
					*va_arg(arg, wchar_t *) = wc;					// Store the value
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}

			case '[':
			{
				if (notSet) {										// Not in set request
					while (!strchr(&tmp[0], *tp) && width != 0) {	// While not matching set advance
						tp++;										// Next chracter
						width--;									// Decrease width
					}
				}
				else {
					while (strchr(&tmp[0], *tp) && width != 0) {	// While matching set advance
						tp++;										// Next chracter
						width--;									// Decrease width
					}
				}
				break;
			}
			}
			if (specifier != '[') count++;							// If not create set the we have matched a specifier
		}
		else {														// Literal character match
			if (*tp != *fmt) return(count);							// literal match fail time to exit
			fmt++;													// Next format character
			tp++;													// Next string character
		}
	}
	return (count);													// Return specifier match count
}

/*-[ sscanf ]---------------------------------------------------------------}
{ Reads data from str and stores them according to parameter format in the  }
{ locations given by the additional arguments, as if scanf was used, but    }
{ reading from buf instead of the standard input (stdin).					}
{ The additional arguments should point to already allocated variables of   }
{ the type specified by their corresponding specifier in the format string. }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
int	sscanf (const char *str, const char *fmt, ...)
{
	va_list args;
	int i = 0;
	if (str) {														// Check buffer is valid
		va_start(args, fmt);										// Start variadic argument
		i = vsscanf(str, fmt, args);								// Run vsscanf
		va_end(args);												// End of variadic arguments
	}
	return (i);														// Return characters place in buffer
}



static char* buffer = 0;
static int bufsize;
static int bufcnt;
void Buffer_WriteChar(char ch)
{
	if ((bufsize == -1 || (bufcnt < bufsize)) && buffer != 0)
	{
		buffer[bufcnt] = ch;
		bufcnt++;
	}
}

static void SetBufferOutput(char* buf, int size)
{
	bufcnt = 0;
	buffer = buf;
	bufsize = size;
}

static void DoneBufferOutput(char* buf)
{
	Buffer_WriteChar('\0');
	if (buf == buffer) buffer = 0;
}


/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*-[Init_EmbStdio]----------------------------------------------------------}
. Initialises the EmbStdio by setting the handler that will be called for
. Each character to be output to the standard console. That routine could be 
. a function that puts the character to a screen or something like a UART.
. Until this function is called with a valid handler output will not occur.
. RETURN: Will be the current handler that was active before the new handler 
. was set. For the first ever handler it will be NULL.
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
CHAR_OUTPUT_HANDLER Init_EmbStdio (CHAR_OUTPUT_HANDLER handler)
{
	CHAR_OUTPUT_HANDLER ret = Console_WriteChar;					// Hold current handler for return
	Console_WriteChar = handler;									// Set new handler function
	return (ret);													// Return the old handler													
}


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
int printf (const char *fmt, ...)
{
	va_list args;													// Argument list
	int count;														// Number of characters printed
	va_start(args, fmt);											// Create argument list
	count = Process_Out(Console_WriteChar, fmt, strlen(fmt), args); // Output print direct to console handler
	va_end(args);													// Done with argument list
	return count;													// Return number of characters printed
}

/*-[ sprintf ]--------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf.
.
. DEPRECATED:
. Using sprintf, there is no way to limit the number of characters written,
. which means the function is susceptible to buffer overruns. The suggested
. replacement is snprintf.
.
. RETURN:
.	SUCCESS: Positive number of characters written to the provided buffer.
.	   FAIL: -1
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int sprintf (char* buf, const char* fmt, ...)
{
	va_list args;													// Argument list
	int count;														// Number of characters printed
	SetBufferOutput(buf, -1);										// Set the very dangerous no size limit buffer
	va_start(args, fmt);											// Create argument list
	count = Process_Out(Buffer_WriteChar, fmt, strlen(fmt), args);	// Create string to print in buffer
	va_end(args);													// Done with argument list
	DoneBufferOutput(buf);											// Done with buffer release it
	return count;													// Return characters added to buffer
}

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
int snprintf (char *buf, size_t bufSize, const char *fmt, ...)
{
	va_list args;													// Argument list
	int count;														// Number of characters printed
	SetBufferOutput(buf, bufSize);									// Set the buffer to given size
	va_start(args, fmt);											// Create argument list
	count = Process_Out(Buffer_WriteChar, fmt, strlen(fmt), args);	// Create string to print in buffer
	va_end(args);													// Done with argument list
	DoneBufferOutput(buf);											// Done with buffer release it
	return count;													// Return characters added to buffer
}

/*-[ vprintf ]--------------------------------------------------------------}
. Writes the C string formatted by fmt to the standard console, replacing 
. any format specifier in the same way as printf, but using the elements in 
. variadic argument list identified by arg instead of additional variadics.
.
. RETURN:
.	SUCCESS: Positive number of characters written to the standard console.
.	   FAIL: -1
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vprintf (const char* fmt, va_list arg)
{
	int count;														// Number of characters printed
	count = Process_Out(Console_WriteChar, fmt, strlen(fmt), arg);	// Output print direct to console handler
	return count;													// Return number of characters printed
}

/*-[ vsprintf ]-------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf.
.
. DEPRECATED:
. Using vsprintf, there is no way to limit the number of characters written,
. which means the function is susceptible to buffer overruns. The suggested
. replacement is vsnprintf.
.
. RETURN:
.	SUCCESS: Positive number of characters written to the provided buffer.
.	   FAIL: -1
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vsprintf (char *buf, const char *fmt, va_list args)
{
	int count;														// Number of characters printed
	SetBufferOutput(buf, -1);										// Set the very dangerous no size limit buffer
	count = Process_Out(Buffer_WriteChar, fmt, strlen(fmt), args);	// Create string to print in buffer
	DoneBufferOutput(buf);											// Done with buffer release it
	return count;													// Return number of characters printed
}

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
int vsnprintf (char *buf, size_t bufSize, const char *fmt, va_list args)
{
	int count;														// Number of characters printed
	SetBufferOutput(buf, bufSize);									// Set the buffer to given size
	count = Process_Out(Buffer_WriteChar, fmt, strlen(fmt), args);	// Create string to buffer
	DoneBufferOutput(buf);											// Done with buffer release it
	return count;													// Return number of characters printed
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{						 	INTERNAL TEST BENCH							    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

bool check_buffer (int* testnumber, const char* testbuf, const char* answer)
{
	const char *p = testbuf;
	const char *q = answer;
	(*testnumber)++;
	while (*p != 0 && *q != 0)
	{
		if (*p != *q) 
		{
			printf("Fail %i, Got: %s Expected: %s\n", *testnumber, testbuf, answer);
			return false;
		}
		p++;
		q++;
	}
	return true;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{						 	PUBLIC TEST BENCH							    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static const char* answers[20] = { "  0", "123456789", "-10 ", "'00010'", "'  +10'" , "<0xc>", "<012345>", "<    12>", 
                             "Hello     AliceBob       END", "computer", "14:05",  "percentage: 99%", "88", "0xa" };

void BENCHTEST_snprintf(void)
{
	int test = 0;
	int success = 0;
	char temp[512];

	snprintf(temp, _countof(temp), "%3d", 0);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%3d", 123456789);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%-3d", -10);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "'%05d'", 10);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "'%+5d'", 10);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "<%#x>", 12);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "<%#.5o>", 012345);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "<%6s>", "12");
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%s%10s%-10sEND\n", "Hello", "Alice", "Bob");
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%.8s", "computerhope");
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%d:%02d", 14, 5);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "percentage: %i%%\n", 99);
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%hhd\n", 'X');
	if (check_buffer(&test, temp, answers[test])) success++;
	snprintf(temp, _countof(temp), "%#x", 10);
	if (check_buffer(&test, temp, answers[test])) success++;

	printf("--------------------\n");
	printf("OVERALL TEST SUMMARY\n");
	printf("--------------------\n");
	printf("TESTED: %i\n", test);
	printf("PASSED: %i\n", success);
	printf("FAILED: %i\n", test-success);
}