//
//  ConfigLib.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef CONFIG_LIB_INCLUDED
#define CONFIG_LIB_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_LIB
typedef void * HCONFIG;
#endif


HCONFIG  OpenConfig(LPCTSTR pszConfigFile);
BOOL     SaveConfig(LPCTSTR pszConfigFile, HCONFIG config);
HCONFIG  CreateConfig();
VOID	 CloseConfig(HCONFIG config);
void	 DeleteConfigSection(HCONFIG config);

//HCONFIG GetConfigSection(HCONFIG config,  LPCTSTR szSectionName);
HCONFIG EnumConfigSection(HCONFIG config, LPCTSTR szSectionName, int idx);
HCONFIG OpenConfigSection(HCONFIG config,  LPCTSTR szSectionName);
HCONFIG CreateConfigSection(HCONFIG config,  LPCTSTR szSectionName);

BOOL	 GetConfigI32(HCONFIG config, LPCTSTR szKeyName, UINT32 *pnValue, UINT32 nDefault);
BOOL	 GetConfigH32(HCONFIG config, LPCTSTR szKeyName, UINT32 *pnValue, UINT32 nDefault);
BOOL	 GetConfigI64(HCONFIG config, LPCTSTR szKeyName, UINT64 *pnValue, UINT64 nDefault);
BOOL	 GetConfigH64(HCONFIG config, LPCTSTR szKeyName, UINT64 *pnValue, UINT64 nDefault);
BOOL	 GetConfigStr(HCONFIG config, LPCTSTR szKeyName, LPTSTR  pszValue, DWORD nLength, LPCTSTR szDefault);

BOOL	 SetConfigI32(HCONFIG config, LPCTSTR szKeyName, UINT32  nValue);
BOOL	 SetConfigH32(HCONFIG config, LPCTSTR szKeyName, UINT32  nValue);
BOOL	 SetConfigI64(HCONFIG config, LPCTSTR szKeyName, UINT64  nValue);
BOOL	 SetConfigH64(HCONFIG config, LPCTSTR szKeyName, UINT64  nValue);
BOOL	 SetConfigStr(HCONFIG config, LPCTSTR szKeyName, LPCTSTR szValue);
BOOL	 SetConfigBin(HCONFIG config, LPCTSTR szKeyName, PVOID data, DWORD len);

#ifdef __cplusplus
}
#endif

#endif