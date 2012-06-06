//
//  RegLib.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef REGLIB_INCLUDED
#define REGLIB_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

BOOL GetSettingBin(HKEY hkey, TCHAR szKeyName[], PVOID pBuffer, LONG nLength);
BOOL GetSettingInt(HKEY hkey, TCHAR szKeyName[], LONG *pnReturnVal, LONG nDefault);
BOOL GetSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR pszReturnStr[], DWORD nLength, TCHAR szDefault[]);
BOOL WriteSettingBin(HKEY hkey, TCHAR szKeyName[], PVOID pData, ULONG nLength);
BOOL WriteSettingInt(HKEY hkey, TCHAR szKeyName[], LONG nValue);
BOOL WriteSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR szString[]);

#ifdef __cplusplus
}
#endif

#endif