#include <stdbool.h>		// Needed for bool
#include <stdint.h>			// Needed for uint8_t etc
#include <stdarg.h>			// Varadic arguments
#include <string.h>			// strnlen used
#include <stdlib.h>			// strtol
#include <ctype.h>			// Needed for toupper, isdigit etc
#include <wchar.h>			// Needed for wchar_t
#include "rpi-smartstart.h" // Smartstart header
#include "emb-stdio.h"		// This units header

/* If you are compiling under POSIX it won't have strnlen in library string.h so provide it as a weak backup */
size_t __attribute__((weak)) strnlen(const char *s, size_t max_len)
{
	size_t i = 0;
	for (; (i < max_len) && s[i]; ++i);
	return i;
}

static int checkdigit (char c) {
	if ((c >= '0') && (c <= '9')) return 1;
	return 0;
}

static int skip_atoi(const char **s)
{
	int i = 0;

	while (checkdigit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SMALL	32		/* Must be 32 == 0x20 */
#define SPECIAL	64		/* 0x */

#define __do_div(n, base) ({ \
int __res; \
__res = ((unsigned long) n) % (unsigned) base; \
n = ((unsigned long) n) / (unsigned) base; \
__res; })

static char *number(char *str, long num, int base, int size, int precision,
	int type)
{
	/* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
	static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

	char tmp[66];
	char c, sign, locase;
	int i;

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	* produces same digits or (maybe lowercased) letters */
	locase = (type & SMALL);
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 16)
		return NULL;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		}
		else if (type & PLUS) {
			sign = '+';
			size--;
		}
		else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else
		while (num != 0)
			tmp[i++] = (digits[__do_div(num, base)] | locase);
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while (size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL) {
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = ('X' | locase);
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}





int emb_vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	unsigned long num;
	int i, base;
	char *str;
	const char *s;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
						number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str = buf; *fmt; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
	repeat:
		++fmt;		/* this also skips first '%' */
		switch (*fmt) {
		case '-':
			flags |= LEFT;
			goto repeat;
		case '+':
			flags |= PLUS;
			goto repeat;
		case ' ':
			flags |= SPACE;
			goto repeat;
		case '#':
			flags |= SPECIAL;
			goto repeat;
		case '0':
			flags |= ZEROPAD;
			goto repeat;
		}

		/* get field width */
		field_width = -1;
		if (checkdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (checkdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char)va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			continue;

		case 's':
			s = va_arg(args, char *);
			len = strnlen(s, precision);

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			continue;

		case 'p':
			if (field_width == -1) {
				field_width = 2 * sizeof(void *);
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long)va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;

		case 'n':
			if (qualifier == 'l') {
				long *ip = va_arg(args, long *);
				*ip = (str - buf);
			}
			else {
				int *ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

		case '%':
			*str++ = '%';
			continue;

			/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'x':
			flags |= SMALL;
		case 'X':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

		default:
			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}
		if (qualifier == 'l')
			num = va_arg(args, unsigned long);
		else if (qualifier == 'h') {
			num = (unsigned short)va_arg(args, int);
			if (flags & SIGN)
				num = (short)num;
		}
		else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
		str = number(str, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str - buf;
}

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
int	vsscanf (const char *str, const char *format, va_list arg) 
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

	if (!format || !str) return (0);								// One of the pointers invalid
	while (*tp != 0 && *format != 0) {
		while (isspace(*format)) format++;							// find next format specifier start
		if (*format == '%')											// Format is a specifier otherwise its a literal
		{
			format++;												// Advance format pointer
			if (*format == '*') {									// Check for dont store flag
				dontStore = true;									// Set dont store flag
				tp++;												// Next character
			}
			if (isdigit(*format)) {									// Look for width specifier
				int cc;
				const char* wspos = format;						    // Width specifier start pos
				while (isdigit(*format)) format++;					// Find end of width specifier
				cc = format - wspos;								// Character count for width
				strncpy(&tmp[0], wspos, cc);						// Copy width specifier
				tmp[cc] = '\0';										// Terminate string
				width = strtol(wspos, &endPtr, 10);					// Calculate specifier width
			}
			else width = 256;
			if ((*format == 'h') || (*format == 'l') ||
				(*format == 'j') || (*format == 'z') ||
				(*format == 't') || (*format == 'L')) {				// Special length specifier
				lenSpecifier = *format;								// Hold length specifier
				if ((*format == 'h') || (*format == 'l')) {			// Check for hh and ll
					if (format[1] == *format) 						// Peek at next character for match ... 'hh' or 'll'
					{
						doubleLS = true;							// Set the double LS flag
						format++;									// Next format character
					}
				}
				format++;											// Next format character
			} else lenSpecifier = '\0';								// No length specifier
			specifier = *format++;									// Load specifier					

			/* FIND START OF SPECIFIER DATA  */
			switch (specifier) {
			case '\0':												// Terminator found on format string
			{
				return (count);										// Format string ran out of characters
			}

			case '[':												// We have a Set marker
			{
				if (*format == '^') {								// Not in set requested
					notSet = true;									// Set notSet flag
					format++;										// Move to next character
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
				if (*format == '\0') return(count);					// End of format reached

				spos = format;										// String match starts here
				format++;											// Move to next format character

				if (*format == '\0') return(count);					// End of format reached
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
				while (*format != ']' && *format != '\0') format++;			// Keep reading until set end
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
					else cc = format - spos;						// Set specifier is from format
				strncpy(&tmp[0], spos, cc);							// Copy to temp buffer
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
						i = strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned char *) = i;			// Store the value
						break;
					}
					else {											// Length specifier was "h"
						unsigned short i;
						i = strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
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
						i = strtol(&tmp[0], &endPtr, base);			// Convert integer
						*va_arg(arg, signed char *) = i;			// Store the value 
						break;
					}
					else {											// Length specifier was "h"
						short i;
						i = strtol(&tmp[0], &endPtr, base);			// Convert integer
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
					strcpy(va_arg(arg, char *), &tmp[0]);			// If dontstore flag false then Store the value
					break;
				}
				case 'l':											// Length specifier 'l' 
				{
					wchar_t wc[256];								// Temp buffer
					size_t cSize = strlen(&tmp[0]) + 1;				// Size of text scanned
					mbstowcs(&wc[0], &tmp[0], cSize);				// Convert to wide character
					wcscpy(va_arg(arg, wchar_t *), &wc[0]);			// Store the wide string
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
					mbstowcs(&wc, spos, 1);							// Convert to wide character
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
			if (*tp != *format) return(count);						// literal match fail time to exit
			format++;												// Next format character
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
int	sscanf (const char *str, const char *format, ...)
{
	va_list args;
	int i = 0;
	if (str) {														// Check buffer is valid
		va_start(args, format);										// Start variadic argument
		i = vsscanf(str, format, args);								// Run vsscanf
		va_end(args);												// End of variadic arguments
	}
	return (i);														// Return characters place in buffer
}


int emb_sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = emb_vsprintf(buf, fmt, args);
	va_end(args);
	return i;
}

int printf (const char *fmt, ...)
{
	char printf_buf[512];
	va_list args;
	int printed = 0;

	va_start(args, fmt);
	printed = emb_vsprintf(printf_buf, fmt, args);
	va_end(args);
	for (int i = 0; i < printed; i++)
		Embedded_Console_WriteChar(printf_buf[i]);

	return printed;
}


