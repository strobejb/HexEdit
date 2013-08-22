//
//  base64.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	base64/uuencode routines modified from Bob Trower's sources
//  at http://base64.sourceforge.net/
//

#include <windows.h>
#include "base64.h"

// base64 alphabet
static const char b64table[64] =	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
									"abcdefghijklmnopqrstuvwxyz"
									"0123456789"
									"+/" ;

// base64 7bit reverse-lookup
static const char b64rev[] =
{	
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,		// '+/'
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,		// '0-9' & '='
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,	10, 11, 12, 13, 14,		// 'A-Z'
	15, 16, 17, 18, 19,	20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,		
	-1,	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,		// 'a-z' 
	41, 42, 43, 44, 45,	46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1 
};

// uuencode alphabet. 1st character (`) should strictly be
// a space (32) character but '`' is common too
static const char uuetable[64] =	" !\"#$%&\'()*+,-./"
									"0123456789:;<=>?"
									"@ABCDEFGHIJKLMNO"
									"PQRSTUVWXYZ[\\]^_" ;

// uuencode 7bit reverse-lookup 
static const char uuerev[128] =
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,	10, 11, 12, 13, 14, 15,		
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
	32, 33, 34, 35,	36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // make the grave '`' a zero too
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
};

//
//	dual 8bit->6bit bit-slicing routine
//	encodes every 3x bytes as 4x 6bit values, usuable
//	for both base64 and uuencode algorithms
//
//	table    - lookup encoding table for base64/uuencode
//	deadchar - '=' for base64, '`' for uuencode
//
//
//	returns number of characters stored in outbuf, or if outbuf is NULL,
//	returns number of characters required to store result
//
static size_t encode(BYTE *inbuf, size_t inlen, char *outbuf, const char *table, const int deadchar)
{
	size_t	i;
	size_t	outlen = 0;

	while(inlen)
	{
		// process input buffer in blocks of 3
		size_t  len		= min(inlen, 3);
		BYTE	in[3]	= { 0 };

		// take a copy of (up to) 3 input bytes, with any trailing bytes zeroed
		for(i = 0; i < len; i++)
			in[i] = *inbuf++;
			
		if(outbuf != NULL)
		{
			// encode as 4 output characters having values 0-63
			// use the supplied lookup table to put the values into the correct alphabet
			outbuf[0] = table[ in[0] >> 2 ];
			outbuf[1] = table[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
			outbuf[2] = table[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ];
			outbuf[3] = table[ in[2] & 0x3f ];
			
			// take care of non-multiples of 3 
			switch(len)
			{
			case 1: outbuf[2] = deadchar;  // ('=' for base64, '`' for uuencode)
			case 2: outbuf[3] = deadchar;  
			}

			outbuf += 4;
		}

		outlen += 4;
		inlen  -= len;
	}

	return outlen;
}

//
//	dual 8bit->6bit bit-slicing routine
//	decodes every 3x bytes as 4x 6bit values, usuable
//	for both base64 and uuencode algorithms
//
//	table    - lookup encoding table for base64/uuencode
//	deadchar - '=' for base64, '`' for uuencode
//
//	returns number of bytes stored in outbuf, or if outbuf is NULL,
//	returns number of bytes required to store result
//
static size_t decode(char *inbuf, size_t inlen, BYTE *outbuf, const char *table, const int deadchar)
{
    size_t  i;
	size_t  its = 0;
	size_t  outlen = 0;

	char  * orig_inbuf = inbuf;
	size_t  orig_inlen = inlen;

    while(inlen)
	{
		its++;

		/*if(inlen > 0x100)
		{
			//DebugBreak();
		}*/
		if(*inbuf == '\r' || *inbuf == '\n')
		{
			inbuf++;
			inlen--;
		}
		else if(*inbuf == '\0')
		{
			break;
		}
		else if(inlen > 4)
		{
			BYTE in[4] = { 0 };
			int  v = 0;

			for(i = 0; i < 4 && *inbuf != deadchar; i++, inbuf++)
			{
				v = *inbuf;

				if(v > 127 || table[v] == -1)
					return outlen;//return 0;

				in[i] = table[v];
			}

			if(i > 0)
			{
				outbuf[0] = (BYTE) (in[0] << 2 | in[1] >> 4);
			    outbuf[1] = (BYTE) (in[1] << 4 | in[2] >> 2);
				outbuf[2] = (BYTE) (((in[2] << 6) & 0xc0) | in[3]);
			
				outbuf += (i-1);
				outlen += (i-1);
				inlen  -= i;
			}
			else
			{
				break;
			}
		}	
		else
		{
			break;
		}
    }

	return outlen;
}

size_t base64_encode(BYTE *inbuf, size_t inlen, char *outbuf)
{
	return encode(inbuf, inlen, outbuf, b64table, '=');
}

size_t base64_decode(char *inbuf, size_t inlen, BYTE *outbuf)
{
	return decode(inbuf, inlen, outbuf, b64rev, '=');
}

size_t uu_encode(BYTE *inbuf, size_t inlen, char *outbuf)
{
	// first character represents the length of this line
	if(outbuf)
		*outbuf++ = uuetable[inlen];
		
	return encode(inbuf, inlen, outbuf, uuetable, '`') + 1;
}

size_t uu_decode(char *inbuf, size_t inlen, BYTE *outbuf)
{
	int linelen = *inbuf;

	if(inlen <= 1 || linelen > 127 || uuerev[linelen] == -1 )	
		return 0;
		
	// get length of this line
	linelen = uuerev[linelen];

	// decode the line and make sure the lengths match
	if((ULONG)linelen == decode(inbuf+1, inlen-1, outbuf, uuerev, '`'))
	{
		return linelen;
	}
	else
	{
		return 0;
	}
}


