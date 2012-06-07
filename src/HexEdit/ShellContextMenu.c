//
//  ShellContextMenu.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	This is the simplest way to add an 'Open With....' entry to
//  every filetype in the Explorer context menu
//

#include <windows.h>

// HKEY_CU
#define CONTEXT_CMD_LOC TEXT("Software\\Classes\\*\\shell\\Open with HexEdit\\command")
#define CONTEXT_APP_LOC TEXT("Software\\Classes\\*\\shell\\Open with HexEdit")

// from HexEdit.c
TCHAR * GetArg(TCHAR *ptr, TCHAR *buf, int len);

//
//	Add or remove Neatpad from the Explorer context-menu
//
BOOL SetExplorerContextMenu(BOOL fAddToMenu)
{
	HRESULT hr;

	if(fAddToMenu)
	{
		TCHAR szAppPath[MAX_PATH];
		TCHAR szDefaultStr[MAX_PATH];

		GetModuleFileName(0, szAppPath, MAX_PATH);

		wsprintf(szDefaultStr, TEXT("\"%s\" \"%%1\""), szAppPath);

		hr = RegSetValue(HKEY_CURRENT_USER, CONTEXT_CMD_LOC, REG_SZ, szDefaultStr, lstrlen(szDefaultStr) * sizeof(TCHAR));
	}
	else
	{
		hr = RegDeleteKey(HKEY_CURRENT_USER, CONTEXT_CMD_LOC);

		if(hr == ERROR_SUCCESS)
			hr = RegDeleteKey(HKEY_CURRENT_USER, CONTEXT_APP_LOC);
	}

	return (hr == ERROR_SUCCESS) ? TRUE : FALSE;
}

//
//	Check the registry to see if the context-menu extension is still installed
//
BOOL IsContextMenuInstalled()
{
	TCHAR szAppPath[MAX_PATH];
	TCHAR szRegPath[MAX_PATH];
	LONG len = sizeof(szAppPath);
	HRESULT hr;
	
	// get current exe path
	GetModuleFileName(0, szAppPath, MAX_PATH);

	// get the commandline for our shell-item
	hr = RegQueryValue(HKEY_CURRENT_USER, CONTEXT_CMD_LOC, szRegPath, &len); 

	// strip out the program path
	GetArg(szRegPath, szRegPath, MAX_PATH);

	// make sure the two paths match
	return (hr == ERROR_SUCCESS && lstrcmp(szAppPath, szRegPath) == 0) ? TRUE : FALSE;
}
