//
//  RecentFile.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include "resource.h"

#include "RecentFile.h"
#include "RegLib.h"


#define MAX_RECENT_FILE 8

static TCHAR g_szRecentFileList[MAX_RECENT_FILE][MAX_PATH+4];
static int   g_nRecentFileCount = 0;

void AddRecentFile(LPCTSTR szFileName)
{
	int i, j;

	// search for duplicate
	for(i = 0; i < g_nRecentFileCount; i++)
	{
		// if they match, remove from list
		if(lstrcmpi(g_szRecentFileList[i]+3, szFileName) == 0)
		{
			for(j = i; j < g_nRecentFileCount-1; j++)
				lstrcpy(g_szRecentFileList[j], g_szRecentFileList[j+1]);

			g_nRecentFileCount--;
			break;
		}
	}

	if(g_nRecentFileCount < MAX_RECENT_FILE)
		g_nRecentFileCount++;

	// move any existing files down one position
	for(i = g_nRecentFileCount-1; i > 0; i--)
		lstrcpy(g_szRecentFileList[i], g_szRecentFileList[i-1]);

	// add file to beginning
	wsprintf(g_szRecentFileList[0], TEXT("&1 %s"), szFileName);

	// renumber the files
	for(i = 0; i < g_nRecentFileCount; i++)
		g_szRecentFileList[i][1] = '0' + i + 1;
}

BOOL GetRecentFile(int idx, TCHAR *szName, UINT nLength)
{
	if(idx < 0 || idx >= g_nRecentFileCount)
		return FALSE;

	lstrcpyn(szName, g_szRecentFileList[idx]+3, nLength);
	return TRUE;
}

void UpdateRecentMenu(HMENU hFileMenu)
{
	int i;
	MENUITEMINFO mii = { sizeof(mii), MIIM_SUBMENU  };
	UINT state;
	HMENU hRecent = 0;

	// search for the popup
	for(i = 0; ; i++)
	{
		if((state = GetMenuState(hFileMenu, i, MF_BYPOSITION)) == -1)
			break;

		if(state & MF_POPUP)
		{
			if(g_nRecentFileCount > 0)
				EnableMenuItem(hFileMenu, i, MF_BYPOSITION|MF_ENABLED);

			hRecent = GetSubMenu(hFileMenu, i);
			break;
		}
	}

	if(hRecent)
	{
		// delete all items
		while(DeleteMenu(hRecent, 0, MF_BYPOSITION))
			;
		
		// add in our list
		for(i = 0; i < g_nRecentFileCount; i++)
		{
			AppendMenu(hRecent, MF_ENABLED, IDM_RECENT1+i, g_szRecentFileList[i]);
		}
	}
}

BOOL LoadRecentFileList(HKEY hRoot)
{
	HKEY hRecent;

	g_nRecentFileCount = 0;

	if(RegOpenKey(hRoot, TEXT("RecentList"), &hRecent) == S_OK)
	{
		int i;

		for(i = 0; ; i++)
		{
			TCHAR name[10];
			wsprintf(name, TEXT("%d"), i+1);
			
			if(GetSettingStr(hRecent, name, g_szRecentFileList[i], MAX_PATH+4, TEXT("")))
			{
				g_nRecentFileCount++;
			}
			else
			{
				break;
			}
		}
	}

	return TRUE;
}

BOOL SaveRecentFileList(HKEY hRoot)
{
	HKEY hRecent;

	RegDeleteKey(hRoot, TEXT("RecentList"));
	
	if(RegCreateKey(hRoot, TEXT("RecentList"), &hRecent) == S_OK)
	{
		int i;
		for(i = 0; i < g_nRecentFileCount; i++)
		{
			TCHAR name[10];
			wsprintf(name, TEXT("%d"), i+1);
			WriteSettingStr(hRecent, name, g_szRecentFileList[i]);
		}
	}

	return TRUE;
}