//
//  RecentFile.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef RECENT_FILE_INCLUDED
#define RECENT_FILE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

BOOL LoadRecentFileList(HKEY hRoot);
BOOL SaveRecentFileList(HKEY hRoot);

void AddRecentFile(LPCTSTR szFileName);
void UpdateRecentMenu(HMENU hMenu);


#ifdef __cplusplus
}
#endif

#endif