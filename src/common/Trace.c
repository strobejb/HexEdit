//
//  Trace.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	MODULE:		Trace.c
//
//	PURPOSE:	Provides TRACE macros (ala MFC) which wrap the 
//				OutputDebugString API call - removing these
//				calls in release build.
//
//	USAGE:		TRACE(...)  for TCHAR-strings
//				TRACEA(...) for char-strings
//				TRACEW(...) for WCHAR-strings
//
//	HISTORY:	v1.0 16/04/2002 J Brown		- Initial version
//

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "Trace.h"

#ifdef _DEBUG

#ifdef _CONSOLE
#define OUTPUTA(s) printf("%s", s)
#define OUTPUTW(s) wprintf(L"%s", s)
#else
#define OUTPUTA(s) OutputDebugStringA(s)
#define OUTPUTW(s) OutputDebugStringW(s)
#endif

//
//	Wide-character (UNICODE) version
//
void TraceW(LPCWSTR szFmt, ...)
{
	wchar_t szBuf[0x400];

	va_list arg;

	if(szFmt == 0) return;

	va_start(arg, szFmt);

	_vsnwprintf(szBuf, 0x400, szFmt, arg);
	OUTPUTW(szBuf);
	
	va_end(arg);
}

//
//	Single-character (ANSI) version
//
void TraceA(LPCSTR szFmt, ...)
{
	char szBuf[0x400];

	va_list arg;

	if(szFmt == 0) return;

	va_start(arg, szFmt);

	_vsnprintf(szBuf, 0x400, szFmt, arg);
	OUTPUTA(szBuf);
	
	va_end(arg);
}

void TraceErr(DWORD err)
{
	LPVOID lpMsgBuf;
	FormatMessageA( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPSTR) &lpMsgBuf,
		0,
		NULL 
		);

	TraceA( "[%x] %s\n", err, (lpMsgBuf ? lpMsgBuf : "unknown"));
	LocalFree( lpMsgBuf );	
}

#endif