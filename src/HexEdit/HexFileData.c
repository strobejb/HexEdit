//
//  HexFileData.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include "HexUtils.h"

#define HEX_FILESPEC _T("%s:HexEdit.FileInfo")

//
//	Save window-position for the specified file
//
DWORD SaveFileData(const TCHAR *szPath, const TCHAR *szStreamName, PVOID data, DWORD len)
{
	WINDOWPLACEMENT wp = { sizeof(wp) };

	TCHAR		szStream[MAX_PATH];
	HANDLE		hFile;
	DWORD		numwritten;
	BOOL		restoretime = FALSE;
	FILETIME	ctm, atm, wtm;

	if(szPath == 0 || szPath[0] == 0)
		return 0;

	wsprintf(szStream, TEXT("%s:%s"), szPath, szStreamName);

	//
	//	Get the file time-stamp. Try the stream first - if that doesn't exist 
	//	get the time from the 'base' file
	//
	if((hFile = CreateFile(szStream, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE ||
	   (hFile = CreateFile(szPath,   GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
	{
		if(GetFileTime(hFile, &ctm, &atm, &wtm))
			restoretime = TRUE;

		CloseHandle(hFile);
	}

	//
	//	Now open the stream for writing
	//
	if((hFile = CreateFile(szStream, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE)
		return 0;

	WriteFile(hFile, data, len, &numwritten, 0);
	
	//
	//	Restore timestamp if necessary
	//
	if(restoretime == TRUE)
		SetFileTime(hFile, &ctm, &atm, &wtm);

	CloseHandle(hFile);

	return numwritten;
}

//
//	Restore the last window-position for the specified file
//
DWORD LoadFileData(const TCHAR *szPath, const TCHAR *szStreamName, PVOID data, DWORD len)
{
	TCHAR		szStream[MAX_PATH];
	HANDLE		hFile;
	DWORD		numread;

	if(szPath == 0 || szPath[0] == 0)
		return 0;

	wsprintf(szStream, TEXT("%s:%s"), szPath, szStreamName);

	//
	//	Can only set the window-position if the alternate-data-stream exists
	//
	if((hFile = CreateFile(szStream, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		return 0;

	ReadFile(hFile, data, len, &numread, 0);
	CloseHandle(hFile);
	return numread;
}