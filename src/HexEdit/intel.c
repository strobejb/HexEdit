//
//  intel.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>

static int fmtbyte(char *s, unsigned val, unsigned *checksum)
{
	static const char hex[] = "0123456789ABCDEF";
	
	s[0] = hex[(val >> 4) & 0x0f];
	s[1] = hex[(val & 15) & 0x0f];

	if(checksum)
		*checksum += val;

	return 2;
}

char * getbyte(char *s, unsigned *val)
{
	static const char rev[] =// "0123456789xxxxxxxABCDEFxxxxxxxxxxxxxxxxxxxxxxyyyyyyabcdefg";
	{
		0,   1,   2,   3,   4,   5,   6, 7, 8, 9,
		0,   0,   0,   0,   0,   0,   0,
		0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 
		0,   0,   0,   0,   0,   0,
		0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 
	};

	if(!isxdigit(s[0]) || !isxdigit(s[1]))
		return 0;

	*val = rev[s[1] - '0'] | (rev[s[0] - '0'] << 4);
	return s+2;
}

//
//	Create an Intel Hex-record
//  'hrec' is a string-buffer to hold the result, must be at least 80 chars long
//
//	record-types:
//		0 - Data record 
//		1 - End of file record 
//		2 - Extended segment address record 
//		3 - Start segment address record 
//		4 - Extended linear address record 
//		5 - Start linear address record 
//
size_t intel_frame(char *hrec, int type, size_t count, unsigned long addr, BYTE *data)
{
	char *h = hrec;
	size_t i;
	unsigned checksum = 0;

	// start
	*h++ = ':';
	
	// record-length
	h += fmtbyte(h, (unsigned)count, &checksum);

	// address
	h += fmtbyte(h, addr >> 8, &checksum);
	h += fmtbyte(h, addr >> 0, &checksum);

	// record-type
	h += fmtbyte(h, type, &checksum);

	// data bytes
	for(i = 0; i < count; i++)
	{
		h += fmtbyte(h, data[i], &checksum);
	}

	// two's complement checksum
	h += fmtbyte(h, ~checksum + 1, 0);

	return h - hrec;
}

//
//	Convert Intel Hex-Record to binary
//
size_t intel_to_bin(char *hrec, int *type, int *count, unsigned long *addr, BYTE *data)
{
	char *h = hrec;
	unsigned val;
	int i;

	// records must begin with ':'
	if(*h++ != ':')
		return 0;

	// record length
	if((h = getbyte(h, count)) == 0)
		return 0;

	// 16bit address
	if((h = getbyte(h, &val)) == 0)
		return 0;

	*addr = val << 8;
	
	if((h = getbyte(h, &val)) == 0)
		return 0;

	*addr |= val;

	// record type
	if((h = getbyte(h, type)) == 0)
		return 0;

	// data
	for(i = 0; i < *count; i++)
	{
		if((h = getbyte(h, &val)) == 0)
			return 0;

		data[i] = (BYTE )val;
	}

	// checksum
	if((h = getbyte(h, &val)) == 0)
		return 0;

	return h - hrec;
}
