//
//  endian.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef ENDIAN_INCLUDED
#define ENDIAN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#ifdef _X86_
#define LITTLE_ENDIAN
#undef  BIG_ENDIAN
#else
#undef  LITTLE_ENDIAN
#define BIG_ENDIAN
#endif

size_t reverse(BYTE *buf, size_t len);
UINT16 reverse16(UINT16 n16);
UINT32 reverse32(UINT32 n32);
UINT64 reverse64(UINT64 n64);

//
//	LITTLE ENDIAN SYSTEMS
//
#if defined(LITTLE_ENDIAN)

#define NATIVE_TO_BE reverse
#define NATIVE_TO_LE
#define BE_TO_NATIVE reverse
#define LE_TO_NATIVE

#define ENDIAN_TO_NATIVE16(bigend, n16) ((bigend) ? reverse16((n16)) : (n16))
#define ENDIAN_TO_NATIVE32(bigend, n32) ((bigend) ? reverse32((n32)) : (n32))
#define ENDIAN_TO_NATIVE64(bigend, n64) ((bigend) ? reverse64((n64)) : (n64))

#define ENDIAN_TO_NATIVE(bigend, buf, len) ((bigend) ? reverse(buf,len) : 0)

#define LITTLEENDIAN_TO_NATIVE16(n16) ((n16))
#define LITTLEENDIAN_TO_NATIVE32(n32) ((n32))
#define LITTLEENDIAN_TO_NATIVE64(n64) ((n64))

#define BIGENDIAN_TO_NATIVE16(n16) (reverse16((n16))
#define BIGENDIAN_TO_NATIVE32(n32) (reverse32((n32))
#define BIGENDIAN_TO_NATIVE64(n64) (reverse64((n64))



//
//	BIG ENDIAN SYSTEMS
//
#elif defined(BIG_ENDIAN)

#define ENDIAN_TO_NATIVE16(bigend, n16) ((bigend) ? (n16) : reverse16((n16)))
#define ENDIAN_TO_NATIVE32(bigend, n32) ((bigend) ? (n32) : reverse32((n32)))
#define ENDIAN_TO_NATIVE64(bigend, n64) ((bigend) ? (n64) : reverse64((n64)))

#define LITTLEENDIAN_TO_NATIVE16(n16) (reverse16((n16))
#define LITTLEENDIAN_TO_NATIVE32(n32) (reverse32((n32))
#define LITTLEENDIAN_TO_NATIVE64(n64) (reverse64((n64))

#define ENDIAN_TO_NATIVE(bigend, buf, len) ((bigend) ? 0 : reverse(buf,len))

#define BIGENDIAN_TO_NATIVE16(n16) ((n16))
#define BIGENDIAN_TO_NATIVE32(n32) ((n32))
#define BIGENDIAN_TO_NATIVE64(n64) ((n64))


#endif







#ifdef __cplusplus
}
#endif

#endif