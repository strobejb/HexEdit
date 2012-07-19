//
//  motorola.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//#include "motorola.h"
#include <windows.h>


int fmtbyte(char *s, unsigned val, unsigned *checksum)
{
	static const char hex[] = "0123456789ABCDEF";
	
	s[0] = hex[(val >> 4) & 0x0f];
	s[1] = hex[(val & 15) & 0x0f];

	if(checksum)
		*checksum += val;

	return 2;
}

char * getbyte(char *s, unsigned *val);

static int sizelook[] = 
{
	2, 2, 3, 4, 0, 2, 0, 4, 3, 2
};

//
//	Create a Motorola S-record
//  'srec' is a string-buffer to hold the result, must be at least 80 chars long
//
//	type:
//		0 - frame header + comment
//		1 - 16bit data record
//		2 - 24bit data record
//		3 - 32bit data record
//		4 - n/a
//		5 - 16bit unknown
//		6 - n/a
//		7 - 32bit terminator
//		8 - 24bit terminator
//		9 - 16bit terminator
//		
size_t motorola_frame(char *srec, int type, size_t count, unsigned long addr, BYTE *data)
{
	char *s = srec;
	size_t i;
	unsigned checksum = 0;

	if(type < 0 || type > 9)
		return 0;

	// record type
	*s++ = 'S';
	*s++ = '0' + type;

	// record size
	s += fmtbyte(s, sizelook[type] + (unsigned)count + 1, &checksum);

	// 2/3/4 byte address field (depends on type)
	switch(type)
	{
	case 3: case 7:		// 4-byte address
		s += fmtbyte(s, addr >> 24, &checksum);

	case 2: case 8:		// 3-byte address
		s += fmtbyte(s, addr >> 16, &checksum);					

	case 0: case 1: 
	case 5: case 9:		// 2-byte address
		s += fmtbyte(s, addr >> 8, &checksum);
		s += fmtbyte(s, addr >> 0, &checksum);
		break;
	}

	// data
	for(i = 0; i < count; i++)
		s += fmtbyte(s, data[i], &checksum);

	// 1's compliment checksum
	s += fmtbyte(s, ~checksum, 0);

	return s - srec;
}

//
//
//
size_t motorola_to_bin(char *srec, int *type, int *count, unsigned long *addr, BYTE *data)
{
	char *s = srec;
	unsigned val;
	int i;

	// start of record 
	if(*s++ != 'S')
		return 0;

	// record type
	*type = *s++ - '0';
	*addr = 0;

	// record size
	if((s = getbyte(s, &val)) == 0)
		return 0;

	// convert to byte-size
	*count = val - 1 - sizelook[*type];

	// 2/3/4 byte address
	switch(*type)
	{
	case 3: case 7:		// 4-byte address
		
		if((s = getbyte(s, &val)) == 0)
			return 0;

		*addr = val;

	case 2: case 8:		// 3-byte address

		if((s = getbyte(s, &val)) == 0)
			return 0;

		*addr = (*addr << 8) | val;

	case 0: case 1: 
	case 5: case 9:		// 2-byte address

		if((s = getbyte(s, &val)) == 0)
			return 0;

		*addr = (*addr << 8) | val;

		if((s = getbyte(s, &val)) == 0)
			return 0;

		*addr = (*addr << 8) | val;

		break;

	default:			// error, unknown type!
		return 0;
	}

	// data
	if(*type >= 0 && *type <= 3)
	{
		for(i = 0; i < *count; i++)
		{
			if((s = getbyte(s, &val)) == 0) 
				return 0;

			data[i] = (BYTE)val;
		}
	}

	// checksum
	if((s = getbyte(s, &val)) == 0)
		return 0;

	return s - srec;
}