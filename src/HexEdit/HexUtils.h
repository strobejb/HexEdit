//
//  HexUtils.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef HEXUTIL_INCLUDED
#define HEXUTIL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "..\HexView\seqbase.h"

UINT HexErrorBox(TCHAR *fmt, ...);
UINT HexErrorBoxId(UINT uErrorStrId, ...);
UINT HexErrorBoxIdExt(UINT uMsgBoxId, UINT uErrorStrId, ...);
UINT HexWinErrorBox(DWORD dwError, TCHAR *fmt, ...);

//converts a HVE_xx code into a string ID and displays the message...
UINT WINAPI HexViewErrorBox(HWND hwnd, UINT code);

UINT HexInfoBox(TCHAR *fmt, ...);


size_w GetDlgItemBaseInt(HWND hwnd, UINT nCtrlID, int base);
void   SetDlgItemBaseInt(HWND hwnd, UINT nCtrlID, size_w nNumber, int base, BOOL fZeroPad);

BOOL EnableDlgItem (HWND hwnd, UINT nIDDlgItem, BOOL bEnable);
BOOL ShowDlgItem   (HWND hwnd, UINT nIDDlgItem, UINT nCmdShow);

int  wfprintf(HANDLE file, const TCHAR *fmt, ...);
int  wfputs(HANDLE file, const TCHAR *str);

void MenuCheckMark(HMENU hmenu, int id, BOOL bCheck);
BOOL ToggleMenuItem(HMENU hmenu, UINT menuid);
BOOL EnableMenuCmdItem(HMENU hmenu, UINT uCmd, BOOL fEnable);

UINT HandleContextHelp(HWND hwnd, LPARAM lParam, UINT uDialogId);

TCHAR *lstrrchr(const TCHAR *str, int c);
TCHAR *lstrchr(const TCHAR *str, int c);

// Set the font every control in specified dialog
void SetDialogFont(HWND hdlg, HFONT hfont);

//load a string from resource, replace all tabs with NULLS
BOOL LoadTabbedString(UINT uId, TCHAR *dest, DWORD dwLength);

void SetWindowIcon(HWND hwnd, int nIconResourceId);


UINT AddStyle(HWND hwnd, UINT style);
UINT AddDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style);
UINT DelStyle(HWND hwnd, UINT style);
UINT DelDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style);
BOOL EnableDlgItem(HWND hwnd, UINT nCtrlId, BOOL fEnabled);
//BOOL ShowDlgItem(HWND hwnd, UINT nCtrlId, DWORD dwShowCmd);

int WINAPI GetRectHeight(RECT *rect);
int WINAPI GetRectWidth(RECT *rect);

VOID  CenterWindow(HWND hwnd);
PVOID CenterRelative(HWND hwnd, HWND hwndRelative, HDWP hdwp);

int   GetWindowWidth(HWND hwnd);
int   GetWindowHeight(HWND hwnd);
void  GetWindowPos(HWND hwnd, POINT *pt);
PVOID SetWindowWidth(HWND hwnd, int width, HDWP hdwp);
PVOID SetWindowHeight(HWND hwnd, int height, HDWP hdwp);
PVOID SetWindowSize(HWND hwnd, int width, int height, HDWP hdwp);
PVOID SetWindowXY(HWND hwnd, int x, int y, HDWP hdwp);


#define ALIGN_LEFT    0
#define ALIGN_RIGHT   1
#define ALIGN_TOP     2
#define ALIGN_BOTTOM  3

void AlignWindow(HWND hwnd, HWND hwndRelative, int method);


void SetStatusBarText(HWND hwndSB, int part, unsigned style, TCHAR *fmt, ...);
HFONT CreateBoldFontFromHwnd(HWND hwnd);
BOOL FontExists(HWND hWnd, LPCTSTR szFontName);
void ForceVisibleDisplay(HWND hwnd);

BOOL TouchFile(TCHAR *szFileName);

void SetWindowFileName(HWND hwnd, TCHAR *szFileName, BOOL fModified, BOOL fReadOnly);
BOOL ResolveShortcut(TCHAR *pszShortcut, TCHAR *pszFilePath, int nPathLen);

TCHAR *GetVersionString(TCHAR *szFileName, TCHAR *szValue, TCHAR *szBuffer, ULONG nLength);

void AddComboStringList(HWND hwndCombo, TCHAR **szList, int iInitial);
void AddListStringList(HWND hwndList, TCHAR **szList, int iInitial);
HWND EnumHexView(HWND hwndMain, int idx);

UINT ComboBox_GetDlgSelData(HWND hwndDlg, UINT nComboId);
UINT ComboBox_GetSelData(HWND hwndCombo);
UINT ListBox_GetSelData(HWND hwndList);
UINT ListBox_GetDlgSelData(HWND hwndDlg, UINT nListId);

DWORD SaveFileData(const TCHAR *szPath, const TCHAR *szStreamName, PVOID data, DWORD len);
DWORD LoadFileData(const TCHAR *szPath, const TCHAR *szStreamName, PVOID data, DWORD len);

HWND SetDlgItemFocus(HWND hwndDialog, UINT uDlgItemId);

#ifdef __cplusplus
}
#endif

#endif