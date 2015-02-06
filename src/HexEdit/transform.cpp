//
//  transform.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>

static void reverse(BYTE *buf, int len)
{
	int i;
	for(i = 0; i < len/2; i++)
	{
		BYTE b = buf[len-i-1];
		buf[len-i-1] = buf[i];
		buf[i] = b;
	}
}


template <typename type>
static void transform( type * buf, size_t len, int operation, type operand, bool swapendian)
{
	size_t i;

	for(i = 0; i < len; i++)
	{
		if(swapendian)
			reverse((BYTE *)&buf[i], sizeof(buf[i]));

		switch(operation)
		{
		case 0: break;
		case 1: buf[i]  = ~buf[i]; break;
		case 2: buf[i]  = buf[i] >> operand; break;
		case 3: buf[i]  = buf[i] << operand; break;	
		case 4: buf[i]  = buf[i] +  operand; break;
		case 5: buf[i]  = buf[i] -  operand; break;
		case 6: buf[i]  = buf[i] *  operand; break;
		case 7: buf[i]  = buf[i] /  operand; break;
		case 8: buf[i]  = buf[i] %  operand; break;
		case 9: buf[i]  = buf[i] &  operand; break;
		case 10: buf[i] = buf[i] |  operand; break;
		case 11: buf[i] = buf[i] ^  operand; break;
		}

		if(swapendian)
			reverse((BYTE *)&buf[i], sizeof(buf[i]));
	}
}

template <> 
static void transform( float * buf, size_t len, int operation, float operand, bool swapendian )
{
	size_t i;

	for(i = 0; i < len; i++)
	{
		if(swapendian)
			reverse((BYTE *)&buf[i], sizeof(buf[i]));

		switch(operation)
		{
		case 4: buf[i] = buf[i] + operand; break;
		case 5: buf[i] = buf[i] - operand; break;
		case 6: buf[i] = buf[i] * operand; break;
		case 7: buf[i] = buf[i] / operand; break;
		default: break;
		}

		if(swapendian)
			reverse((BYTE *)&buf[i], sizeof(buf[i]));
	}
}

template <> 
static void transform( double * buf, size_t len, int operation, double operand, bool swapendian )
{
	size_t i;

	for(i = 0; i < len; i++)
	{
		if(swapendian)
			reverse((BYTE *)&buf[i], sizeof(buf[i]));

		switch(operation)
		{
		case 4: buf[i] = buf[i] + operand; break;
		case 5: buf[i] = buf[i] - operand; break;
		case 6: buf[i] = buf[i] * operand; break;
		case 7: buf[i] = buf[i] / operand; break;
		default: break;
		}

		if(swapendian)
			reverse((BYTE *)&buf[i], sizeof(buf[i]));
	}
}

extern "C"
void transform( BYTE * buf, size_t len, int operation, BYTE * operand, int basetype, int endian )
{
	bool swapendian = endian ? true : false;

	switch(basetype)
	{
	case 0:	transform <UINT8>	( (UINT8  *)buf, len / sizeof(UINT8),  operation, *((UINT8  *)operand), swapendian); break;
	case 1:	transform <UINT16>	( (UINT16 *)buf, len / sizeof(UINT16), operation, *((UINT16 *)operand), swapendian); break;
	case 2:	transform <UINT32>	( (UINT32 *)buf, len / sizeof(UINT32), operation, *((UINT32 *)operand), swapendian); break;
	case 3:	transform <UINT64>	( (UINT64 *)buf, len / sizeof(UINT64), operation, *((UINT64 *)operand), swapendian); break;
	case 4:	transform <INT8>	( (INT8   *)buf, len / sizeof(INT8),   operation, *((INT8   *)operand), swapendian); break;
	case 5:	transform <INT16>	( (INT16  *)buf, len / sizeof(INT16),  operation, *((INT16  *)operand), swapendian); break;
	case 6:	transform <INT32>	( (INT32  *)buf, len / sizeof(INT32),  operation, *((INT32  *)operand), swapendian); break;
	case 8:	transform <float>	( (float  *)buf, len / sizeof(float),  operation, *((float  *)operand), swapendian); break;
	case 9:	transform <double>	( (double *)buf, len / sizeof(double), operation, *((double *)operand), swapendian); break;
	}
}