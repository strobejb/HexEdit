//
//  Trace.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	Use: TRACE  for TCHAR-strings
//		 TRACEA for char-strings
//		 TRACEW for WCHAR-strings
//

#ifndef TRACE_INCLUDED
#define TRACE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void TraceA(LPCSTR szFmt, ...);
void TraceW(LPCWSTR szFmt, ...);
void TraceErr(DWORD err);

#ifdef _DEBUG

#define TRACEERR(err) TraceErr(err)

#ifdef UNICODE
#define TRACE	TraceW
#else
#define TRACE	TraceA
#endif

#define TRACEA	TraceA
#define TRACEW	TraceW

#else	//ifndef DEBUG

#define TRACEERR(err) 

#ifdef UNICODE
#define TRACE	//1 ? ((void)0) : TraceW
#else
#define TRACE	//1 ? ((void)0) : TraceA
#endif

#define TRACEA	//1 ? ((void)0) : TraceA
#define TRACEW	//1 ? ((void)0) : TraceW

#endif	//DEBUG

#ifdef UNICODE
#define ODB	TraceW
#else
#define ODB	TraceA
#endif

#define ODBW	TraceW
#define ODBA	TraceA

#ifdef __cplusplus
}
#endif

#endif