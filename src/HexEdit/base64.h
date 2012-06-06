//
//  base64.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef BASE64_INCLUDED
#define BASE64_INCLUDED

size_t base64_encode(BYTE *inbuf, size_t inlen, char *outbuf);
size_t base64_decode(char *inbuf, size_t inlen, BYTE *outbuf);
size_t uu_encode(BYTE *inbuf, size_t inlen, char *outbuf);
size_t uu_decode(char *inbuf, size_t inlen, BYTE *outbuf);

#endif