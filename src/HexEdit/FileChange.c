//
//  FileChange.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#include <windows.h>
#include <tchar.h>
#include "trace.h"
#include "FileChange.h"

void GetLastWriteTime(TCHAR *szFile, FILETIME *wt)
{
	HANDLE hFile = CreateFile(szFile, 0, 0, 0, OPEN_EXISTING, 0, 0);
	GetFileTime(hFile, 0, 0, wt);
	CloseHandle(hFile);
}

DWORD WINAPI ChangeNotifyThread(NOTIFY_DATA *pnd)
{
	HANDLE hChange;
	DWORD  dwResult;
	TCHAR  szDirectory[MAX_PATH];

	//FILE_NOTIFY_INFORMATION notifyinfo;
	//HANDLE hDirectory;

	//OVERLAPPED overlapped = { 0 };

	//hChange = CreateEvent(0, FALSE, FALSE, 0);
	//overlapped.hEvent = hChange;


	lstrcpy(szDirectory, pnd->szFile);

	// get the directory name from filename
	if(GetFileAttributes(szDirectory) != FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR *slash = _tcsrchr(szDirectory, _T('\\'));
		if(slash) *slash = '\0';
	}

	//hDirectory = CreateFile(szDirectory, FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_DELETE,
	//	NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED , NULL);

	//ReadDirectoryChangesW(hDirectory, &notifyinfo, sizeof(notifyinfo), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, 0, &overlapped, 0);
	
	// watch the specified directory for changes
	hChange = FindFirstChangeNotification(szDirectory, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	do
	{
		HANDLE hEventList[2] = { hChange, pnd->hQuitEvent };
		
		if((dwResult = WaitForMultipleObjects(1, hEventList, FALSE, INFINITE)) == WAIT_OBJECT_0)
		{
			NMFILECHANGE nmfc = { { pnd->hwndNotify, 0, FCN_FILECHANGE }, pnd->szFile };
			FILETIME wt;
			ULONG64 n1, n2;
			//PostMessage(pnd->hwndNotify, WM_NOTIFY, 0, (LPARAM)pnd);

			GetLastWriteTime(pnd->szFile, &wt);

			//GetSystemTimeAsFileTime(&ft);


			n1 = (((ULONG64)wt.dwHighDateTime) << 32) + wt.dwLowDateTime;
			n2 = (((ULONG64)pnd->lastChange.dwHighDateTime) << 32) + pnd->lastChange.dwLowDateTime;

			TRACEA("%I64x %I64x (%d)\n", n1, n2, GetCurrentThreadId());

			//if(*(ULONG64 *)&ft > *(ULONG64 *)&pnd->lastChange + (10000000 * 5))
			if(n1 > n2 + (10000000 * 3))
			{
				PostMessage(pnd->hwndNotify, WM_NOTIFY, 0, (LPARAM)&nmfc);
			}

			pnd->lastChange = wt;
		}

		FindNextChangeNotification(hChange);
	} 
	while(dwResult == WAIT_OBJECT_0);

	// cleanup
	FindCloseChangeNotification(hChange);
	free(pnd);

	return 0;
}

BOOL NotifyFileChange(TCHAR *szPathName, HWND hwndNotify, HANDLE hQuitEvent)
{
	NOTIFY_DATA *pnd = malloc(sizeof(NOTIFY_DATA));

	ZeroMemory(pnd, sizeof(NOTIFY_DATA));

	pnd->hQuitEvent = 0;
	pnd->hwndNotify = hwndNotify;
	pnd->uMsg		= WM_USER;
	lstrcpy(pnd->szFile, szPathName);

	GetLastWriteTime(pnd->szFile, &pnd->lastChange);

	CreateThread(0, 0, ChangeNotifyThread, pnd, 0, 0);

	return TRUE;
}
