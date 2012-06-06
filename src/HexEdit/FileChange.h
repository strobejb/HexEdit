//
//  FileChange.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef FILECHANGE_INCLUDED
#define FILECHANGE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	TCHAR		szFile[MAX_PATH];
	HWND		hwndNotify;
	HANDLE		hQuitEvent;
	UINT		uMsg;
	FILETIME	lastChange;
	//ULONG64		lastChange;

} NOTIFY_DATA;

typedef struct
{
	NMHDR hdr;
	TCHAR *pszFile;
} NMFILECHANGE, *PNMFILECHANGE;

#define FCN_FILECHANGE 2000

BOOL NotifyFileChange(TCHAR *szPathName, HWND hwndNotify, HANDLE hQuitEvent);

#ifdef __cplusplus
}
#endif

#endif