//
//  HexFile.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef HEXFILE_INCLUDED
#define HEXFILE_INCLUDED

UINT HexFileCloseNotify(HWND hwndMain, HWND hwndHV);
BOOL HexSetCurFile(HWND hwndMain, int iItem, BOOL fSetFocus);
BOOL HexCreateNewFile(MAINWND *mainWnd);
BOOL HexCloseFile(MAINWND *mainWnd, int iItem);
void UpdateCurFileName(HWND hwndMain, HWND hwndHV, LPCTSTR szFileName, BOOL fChanged);

BOOL HexOpenFile(HWND hwndMain, LPCTSTR szFileName, DWORD fHexViewFlags);
BOOL HexeditOpenFile(HWND hwnd, LPCTSTR szFile, DWORD openFlags);
void HandleDropFiles(HWND hwnd, HDROP hDrop);
BOOL HexSaveAs(HWND hwndMain, LPCTSTR szFileName);
BOOL HexSaveCurrent(HWND hwndMain);
HWND EnumHexView(HWND hwndMain, int idx);

#endif