//
//  RegLib.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include "RegLib.h"

#define REGISTRY_SETTINGS
//#define 



// Get a binary buffer from the registry
BOOL GetSettingBin(HKEY hkey, TCHAR szKeyName[], PVOID pBuffer, LONG nLength)
{
#ifdef REGISTRY_SETTINGS
	//	ZeroMemory(pBuffer, nLength);
	return !RegQueryValueEx(hkey, szKeyName, 0, 0, (BYTE *)pBuffer, &nLength);

#else

#endif
}

// Get an integer value from the registry
BOOL GetSettingInt(HKEY hkey, TCHAR szKeyName[], LONG *pnReturnVal, LONG nDefault)
{
#ifdef REGISTRY_SETTINGS

	ULONG len = sizeof(nDefault);

	*pnReturnVal = nDefault;

	return !RegQueryValueEx(hkey, szKeyName, 0, 0, (BYTE *)pnReturnVal, &len);

#else

#endif
}

// Get a string buffer from the registry
BOOL GetSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR pszReturnStr[], DWORD nLength, TCHAR szDefault[])
{
#ifdef REGISTRY_SETTINGS

	ULONG len = nLength * sizeof(TCHAR);

	lstrcpyn(pszReturnStr, szDefault, nLength);

	return !RegQueryValueEx(hkey, szKeyName, 0, 0, (BYTE *)pszReturnStr, &len);
#else

#endif
}

// Write a binary value from the registry
BOOL WriteSettingBin(HKEY hkey, TCHAR szKeyName[], PVOID pData, ULONG nLength)
{
#ifdef REGISTRY_SETTINGS

	return !RegSetValueEx(hkey, szKeyName, 0, REG_BINARY, (BYTE *)pData, nLength);

#else

#endif
}

// Write an integer value from the registry
BOOL WriteSettingInt(HKEY hkey, TCHAR szKeyName[], LONG nValue)
{
#ifdef REGISTRY_SETTINGS

	return !RegSetValueEx(hkey, szKeyName, 0, REG_DWORD, (BYTE *)&nValue, sizeof(nValue));

#else

#endif
}

// Get a string buffer from the registry
BOOL WriteSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR szString[])
{
#ifdef REGISTRY_SETTINGS

	return !RegSetValueEx(hkey, szKeyName, 0, REG_SZ, (BYTE *)szString, (lstrlen(szString) + 1) * sizeof(TCHAR));

#else

#endif
}