//
//  endian.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include "endian.h"

size_t reverse(BYTE *buf, size_t len)
{
	size_t i;

	if(buf == 0 || len == 0)
		return 0;

	for(i = 0; i < len/2; i++)
	{
		BYTE b = buf[len-i-1];
		buf[len-i-1] = buf[i];
		buf[i] = b;
	}

	return len;
}

UINT16 reverse16(UINT16 n16)
{
	reverse((BYTE *)&n16, sizeof(n16));
	return n16;
}

UINT32 reverse32(UINT32 n32)
{
	reverse((BYTE *)&n32, sizeof(n32));
	return n32;
}

UINT64 reverse64(UINT64 n64)
{
	reverse((BYTE *)&n64, sizeof(n64));
	return n64;
}

