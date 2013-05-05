//
//  Settings.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "HexEdit.h"
#include "HexUtils.h"
#include "RecentFile.h"
#include "RegLib.h"
#include "..\HexView\HexView.h"

HFONT g_hHexViewFont = 0;

TCHAR g_szHexViewFontName[100];
BOOL  g_fFirstTimeExecution = TRUE;
BOOL  g_fFitToWindow = FALSE;

void FirstTimeOptions(HWND hwndMain)
{
	TCHAR szMsg[] = TEXT("Welcome to HexEdit!\n\n")
		TEXT("Would you like integrate HexEdit into the Explorer shell menu?\n")
		TEXT("This option is available through the main Options dialog");
	MessageBox(hwndMain, szMsg, TEXT(""), MB_ICONQUESTION | MB_YESNO);

}

void LoadSettings0(HKEY hKey)
{
	GetSettingStr(hKey, TEXT("HexViewFontName"), g_szHexViewFontName, 100, TEXT(""));
	GetSettingInt(hKey, TEXT("FirstTimeExecution"), &g_fFirstTimeExecution, TRUE);

	if(g_szHexViewFontName[0] == 0)
	{
		lstrcpy(g_szHexViewFontName, TEXT("Consolas"));
	
		if(FontExists(0, g_szHexViewFontName) == FALSE)
			lstrcpy(g_szHexViewFontName, TEXT("Courier"));
	}
}

void RealiseSettings()
{
	DeleteObject(g_hHexViewFont);

	g_hHexViewFont = CreateFont(-14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, g_szHexViewFontName);

	
}

void SaveSettings0(HKEY hKey)
{
	WriteSettingStr(hKey, TEXT("HexViewFontName"), g_szHexViewFontName);
	WriteSettingInt(hKey, TEXT("FirstTimeExecution"), FALSE);
}

void LoadSettings()
{
	HKEY hKey;
	
	if(RegOpenKeyEx(HKEY_CURRENT_USER, REGBASE, 0, KEY_READ, &hKey) == S_OK)
	{
		LoadRecentFileList(hKey);
		LoadSettings0(hKey);
		RealiseSettings();
		RegCloseKey(hKey);
	}
	else
	{
		// make some defaults!!
	}
}

void SaveSettings()
{
	HKEY hKey;

	if(RegCreateKeyEx(HKEY_CURRENT_USER, REGBASE, 0, 0, 0, KEY_READ|KEY_WRITE, 0, &hKey, 0) == S_OK)
	{
		SaveRecentFileList(hKey);
		SaveSettings0(hKey);
		RegCloseKey(hKey);
	}
	else
	{
		// make some defaults!!
	}

}
