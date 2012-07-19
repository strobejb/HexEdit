//
//  HexUtils.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <windows.h>
#include <tchar.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <shellapi.h>
#include <stdarg.h>

#include "HexEdit.h"
#include "HexUtils.h"

typedef HMONITOR (WINAPI * MFR_PROC)(LPCRECT, DWORD);

UINT HexWinErrorBox(DWORD dwError, TCHAR *fmt, ...)
{
	TCHAR  *lpMsgBuf;
	TCHAR  *buf, *ptr;
	va_list varg;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);

	if(fmt)
	{
		buf = malloc((lstrlen(lpMsgBuf) + 1000) * sizeof(TCHAR));

		ptr = buf + _stprintf(buf, TEXT("%s\n"), lpMsgBuf);

		va_start(varg, fmt);
		_vstprintf(ptr, fmt, varg);
		va_end(varg);

		MessageBox( GetActiveWindow(), buf, APPNAME, MB_OK | MB_ICONEXCLAMATION);
		free(buf);
	}
	else
	{
		MessageBox( GetActiveWindow(), lpMsgBuf, APPNAME, MB_OK | MB_ICONEXCLAMATION);
	}
		
	LocalFree( lpMsgBuf );
	return 0;
}

UINT HexErrorBox(TCHAR *fmt, ...)
{
	static TCHAR tmpbuf[MAX_PATH+256];

	va_list argp;
	va_start(argp, fmt);
	
	wvsprintf(tmpbuf, fmt, argp);
	va_end(argp);

	return MessageBox(GetActiveWindow(), tmpbuf, APPNAME, MB_OK | MB_ICONEXCLAMATION);
}

UINT HexErrorBoxIdExt(UINT uMsgBoxId, UINT uErrorStrId, ...)
{
	static TCHAR szTmp[MAX_PATH];
	static TCHAR szFmt[MAX_PATH];

	va_list argp;
	va_start(argp, uErrorStrId);

	LoadString(GetModuleHandle(0), uErrorStrId, szFmt, MAX_PATH);

	wvsprintf(szTmp, szFmt, argp);
	va_end(argp);

	return MessageBox(GetActiveWindow(), szTmp, APPNAME, uMsgBoxId);
}

UINT HexErrorBoxId(UINT uErrorStrId, ...)
{
	static TCHAR szTmp[MAX_PATH];
	static TCHAR szFmt[MAX_PATH];

	va_list argp;
	va_start(argp, uErrorStrId);
	
	LoadString(GetModuleHandle(0), uErrorStrId, szFmt, MAX_PATH);

	wvsprintf(szTmp, szFmt, argp);
	va_end(argp);

	return MessageBox(GetActiveWindow(), szTmp, APPNAME, MB_OK | MB_ICONEXCLAMATION);
}

UINT HexInfoBox(TCHAR *fmt, ...)
{
	static TCHAR tmpbuf[MAX_PATH+256];

	va_list argp;
	va_start(argp, fmt);
	
	wvsprintf(tmpbuf, fmt, argp);
	va_end(argp);

	return MessageBox(GetActiveWindow(), tmpbuf, APPNAME, MB_OK);
}


BOOL ShowDlgItem(HWND hwnd, UINT nIDDlgItem, UINT nCmdShow)
{
	return ShowWindow(GetDlgItem(hwnd, nIDDlgItem), nCmdShow);
}

size_w GetDlgItemBaseInt(HWND hwnd, UINT ctrlID, int base)
{
	TCHAR buf[40];
	UINT64 num = 0;

	GetDlgItemText(hwnd, ctrlID, buf, 40);

	if(memcmp(buf, TEXT("0x"), sizeof(TCHAR)*2) == 0)
		base = 16;

	switch(base)
	{
	case 0: case 10: _stscanf(buf, TEXT("%I64u"), &num); break;
	case 1: case 16: _stscanf(buf, TEXT("%I64x"), &num); break;
	}

	return (size_w)num;
}

void SetDlgItemBaseInt(HWND hwnd, UINT ctrlID, size_w num, int base, BOOL fZeroPad)
{
	TCHAR ach[256] = TEXT("0");
	UINT64 num64 = num;

	switch(base)
	{
	case 0: case 10:		//decimal
		if(fZeroPad)	_stprintf(ach, TEXT("%I64u"), num64);
		else			_stprintf(ach, TEXT("%I64u"), num64);
		break;

	case 1: case 16:		//hex
		if(fZeroPad)	_stprintf(ach, TEXT("%08I64X"), num64);
		else			_stprintf(ach, TEXT("%I64X"), num64);
		break;
	}

	SetDlgItemText(hwnd, ctrlID, ach);
	//SendDlgItemMessage(hwnd, ctrlID, WM_SETTEXT, 0, (LPARAM)ach);
	//SetWindowText(GetDlgItem(hwnd, ctrlID), ach);
}

int wfprintf(HANDLE file, const TCHAR *fmt, ...)
{
	//variable argument
	TCHAR tmpbuf[256];
	va_list argp;
	int len, numwritten;

	if(file == 0) return 0;

	va_start(argp, fmt);
	
	len = wvsprintf(tmpbuf, fmt, argp) * sizeof(TCHAR);
	va_end(argp);

	WriteFile(file, tmpbuf, len, (LPDWORD)&numwritten, NULL);
	
	return len;
}

int wfputs(HANDLE file, const TCHAR *str)
{
	int len, numwritten;

	len = lstrlen(str) * sizeof(TCHAR);
	WriteFile(file, str, len, (DWORD *)&numwritten, 0);
	
	return len;
}

void MenuCheckMark(HMENU hmenu, int id, BOOL bCheck)
{
	int iState;
	iState = (bCheck) ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem (hmenu, id, iState);
}

BOOL ToggleMenuItem(HMENU hmenu, UINT menuid)
{	
	if(MF_CHECKED & GetMenuState(hmenu, menuid, MF_BYCOMMAND))
	{
		CheckMenuItem(hmenu, menuid, MF_UNCHECKED | MF_BYCOMMAND);
		return FALSE;
	}
	else
	{
		CheckMenuItem(hmenu, menuid, MF_CHECKED | MF_BYCOMMAND);
		return TRUE;
	}
}

BOOL EnableMenuCmdItem(HMENU hmenu, UINT uCmd, BOOL fEnable)
{
	if(fEnable)
	{
		EnableMenuItem(hmenu, uCmd, MF_ENABLED | MF_BYCOMMAND);
		return TRUE;
	}
	else
	{
		EnableMenuItem(hmenu, uCmd, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
		return FALSE;
	}
}

UINT HandleContextHelp(HWND hwnd, LPARAM lParam, UINT uDialogId)
{
	HELPINFO *hi = (HELPINFO *)lParam;
	HWND hwndNext;

	// Needed for XP - sometimes we get sent
	// a WM_HELP when clicking on a check-box, and lParam
	// is an invalid HELPINFO pointer
	if(IsBadReadPtr(hi, sizeof(HELPINFO)))
		return FALSE;

	//if we have a static control then find the ID of the next control
	//in the tab order (hopefully the one next to the label we clicked)
	if(hi->iCtrlId == -1 && uDialogId != 0)
	{
		//if we clicked on a groupbox, ignore this request
		if(GetWindowLongPtr((HWND)hi->hItemHandle, GWL_STYLE) & BS_GROUPBOX)
			return 0;

		//
		// Construct the help context ID using the following formula:
		//
		//   ContextId = 0x80000000 + (DialogId << 16) + ControlId
		//

		hwndNext = GetNextDlgTabItem(hwnd, (HWND)hi->hItemHandle, FALSE);
	
		hi->dwContextId = (uDialogId << 16) + GetWindowLongPtr(hwndNext, GWL_ID);
		
		if(hi->dwContextId != 0)
			hi->dwContextId |= 0x80000000;  
	}

	if(hi->dwContextId != 0)
	{
	//	WinHelp(hwnd, szHelpPath, HELP_CONTEXTPOPUP, hi->dwContextId);
	}
	
	return TRUE;
}
/*
//
//	Find last occurance of 'c' in str
//
TCHAR *lstrrchr(const TCHAR *str, int c)
{
	const TCHAR *ptr = str;
	str = 0;
	
	//strcpy:
	//
	//	while(*dest++ = *src++);
	//
	while(*ptr)
	{
		if(*ptr == c) str = ptr;
		ptr++;
	}

	return (TCHAR *)(void *)str;
}

//
//
//
//
//	Find first occurance of 'c' in str
//
TCHAR *lstrchr(const TCHAR *str, int c)
{
	//do the search
	while(*str && *str != c) 
		str++;
	//while(*str++ != c);

	//--str;

	if(*str == TEXT('\0'))	return (TCHAR *)(void *)0;
	else					return (TCHAR *)(void *)(str);

	return 0;
}
*/
BOOL CALLBACK EnumDialogFontProc(HWND hwnd, LPARAM lParam)
{
	SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, 0);
	return TRUE;
}

void SetWindowIcon(HWND hwnd, int nIconResourceId)
{
	HICON hIcon;
	
	hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(nIconResourceId), IMAGE_ICON, 16, 16, 0);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(nIconResourceId), IMAGE_ICON, 32, 32, 0);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

void SetDialogFont(HWND hdlg, HFONT hfont)
{
/*	LOGFONT lf;
	static bool fInit;
	static HFONT hf;

	if(fInit == false)
	{
		HDC hdc = GetDC(0);
		ZeroMemory(&lf, sizeof(lf));
		lf.lfHeight = MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		lf.lfCharSet = ANSI_CHARSET;
		lstrcpy(lf.lfFaceName, "Tahoma");

		hf = CreateFontIndirect(&lf);

		ReleaseDC(0, hdc);

		fInit = true;
	}

	if(hfont == 0) hfont = hf;
	EnumChildWindows(hdlg, 	EnumDialogFontProc, (LPARAM)hfont);*/
}

/*
UINT WINAPI HexViewErrorBox(HWND hwnd, UINT code)
{
	TCHAR szString[MAX_PATH];
	UINT uResID;

	switch(code)
	{
	case HVE_NOERROR:				return 0;
	case HVE_OUT_OF_MEMORY:			uResID = IDS_ERROR_OUTOFMEMORY;			break;
	case HVE_ALREADY_OPEN:			uResID = IDS_ERROR_ALREADYOPEN;			break;
	case HVE_SHARING_VIOLATION:		uResID = IDS_ERROR_SHARINGVIOLATION;	break;
	case HVE_ACCESS_DENIED:			uResID = IDS_ERROR_ACCESSDENIED;		break;
	case HVE_MEMMAP:				return 0;

	case HVE_FILE_TOO_BIG:			uResID = IDS_ERROR_FILETOOBIG;			break;
	case HVE_FILE_TOO_SMALL:		uResID = IDS_ERROR_FILETOOSMALL;		break;
	case HVE_INVALID_OFFSET:		uResID = IDS_ERROR_INVALIDARG;			break;
	case HVE_INVALID_LENGTH:		uResID = IDS_ERROR_INVALIDARG;			break;
	case HVE_INVALID_ARG:			uResID = IDS_ERROR_INVALIDARG;			break;
	case HVE_MAX_LINKED_FILES:		uResID = IDS_ERROR_MAXLINKED;			break;
	case HVE_FILE_ERROR:			uResID = IDS_ERROR_FILE;				break;
	case HVE_FAILED_TO_FIND:		uResID = IDS_ERROR_FINDFAILED;			break;
	case HVE_USER_ABORT:			uResID = IDS_ERROR_ABORTED;				break;
	case HVE_FILE_READONLY:			uResID = IDS_ERROR_READONLY;			break;
	case HVE_DISKSPACE:				uResID = IDS_ERROR_DISKSPACE;			break;
	case HVE_FILE_QUICKSAVE:		uResID = IDS_ERROR_QUICKSAVE;			break;
	case HVE_FILE_NOLINK_CURRENT:	uResID = IDS_ERROR_INSERTCURRENT;		break;
	case HVE_FILE_NOINSERT_CURRENT:	uResID = IDS_ERROR_INSERTCURRENT;		break;
	case HVE_FILE_EMPTY:			uResID = IDS_ERROR_FILEEMPTY;			break;
	case HVE_FILE_INSERT_FAILED:	uResID = IDS_ERROR_INSERTFAILED;		break;
	case HVE_NOSELECTION:			uResID = IDS_ERROR_NOSELECTION;			break;
	case HVE_INSERTMODE:			uResID = IDS_ERROR_INSERTMODE;			break;

	case HVE_FILE_OPENFAIL:			uResID = IDS_ERROR_OPENFAIL;			break;
	case HVE_FILE_SAVEFAIL:			uResID = IDS_ERROR_SAVEFAIL;			break;
	case HVE_FILE_CREATEFAIL:		uResID = IDS_ERROR_CREATEFAIL;			break;
	case HVE_FILE_WRITEFAIL:		uResID = IDS_ERROR_WRITEFAIL;			break;
	}

	LoadString(GetModuleHandle(0), uResID, szString, sizeof(szString)); 
	HexErrorBox(szString);

	return 0;
}*/

BOOL LoadTabbedString(UINT uId, TCHAR *dest, DWORD dwLength)
{
	int i;

	LoadString(GetModuleHandle(0), uId, dest, dwLength);

	for(i = 0; dest[i] != TEXT('\0'); i++)
	{
		if(dest[i] == TEXT('\t'))
			dest[i++] = TEXT('\0');
	}

	return TRUE;
}

//
// Style helper functions
//
UINT AddStyle(HWND hwnd, UINT style)
{
	UINT oldstyle = GetWindowLong(hwnd, GWL_STYLE);
	SetWindowLongPtr(hwnd, GWL_STYLE,  oldstyle | style);
	return oldstyle;
}

UINT AddDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style)
{
	return AddStyle(GetDlgItem(hwnd, nCtrlId), style);
}

UINT DelStyle(HWND hwnd, UINT style)
{
	UINT oldstyle = GetWindowLong(hwnd, GWL_STYLE);
	SetWindowLongPtr(hwnd, GWL_STYLE, oldstyle & ~style);
	return oldstyle;
}

UINT DelDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style)
{
	return DelStyle(GetDlgItem(hwnd, nCtrlId), style);
}

BOOL EnableDlgItem(HWND hwnd, UINT nCtrlId, BOOL fEnabled)
{
	return EnableWindow(GetDlgItem(hwnd, nCtrlId), fEnabled);
}

int WINAPI GetRectHeight(RECT *rect)
{
	return rect->bottom - rect->top;
}

int WINAPI GetRectWidth(RECT *rect)
{
	return rect->right - rect->left;
}

int GetWindowWidth(HWND hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	return rect.right - rect.left;
}

int GetWindowHeight(HWND hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	return rect.bottom - rect.top;
}



void GetWindowPos(HWND hwnd, POINT *pt)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	pt->x = rect.left;
	pt->y = rect.top;
}


PVOID SetWindowXY(HWND hwnd, int x, int y, HDWP hdwp)
{
	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwnd, 0, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);	
	}
	else
	{
		SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);	
		return NULL;	
	}
}

PVOID SetWindowWidth(HWND hwnd, int width, HDWP hdwp)
{
	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwnd, 0, 0, 0, width, GetWindowHeight(hwnd), SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(hwnd, 0, 0, 0, width, GetWindowHeight(hwnd), SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);	
		return NULL;
	}
}

PVOID SetWindowHeight(HWND hwnd, int height, HDWP hdwp)
{
	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwnd, 0, 0, 0, GetWindowWidth(hwnd), height, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);	
	}
	else
	{
		SetWindowPos(hwnd, 0, 0, 0, GetWindowWidth(hwnd), height, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);	
		return NULL;
	}
}

PVOID SetWindowSize(HWND hwnd, int width, int height, HDWP hdwp)
{
	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwnd, 0, 0, 0, width, height, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);	
	}
	else
	{
		SetWindowPos(hwnd, 0, 0, 0, width, height, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);	
		return NULL;
	}
}

PVOID ShowHideWindow(HWND hwnd, UINT uShowCmd, HDWP hdwp)
{
	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|
			(uShowCmd == SW_SHOW ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
	}
	else
	{
		ShowWindow(hwnd, uShowCmd);
		return NULL;
	}
}

PVOID CenterRelative(HWND hwnd, HWND hwndRelative, HDWP hdwp)
{
	RECT rect;
	RECT rectP;

	int width, height;		
	int screenwidth, screenheight;
	int x, y;

	screenwidth  = GetSystemMetrics(SM_CXSCREEN);
	screenheight = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(hwnd,		  &rect);

	if(hwndRelative == 0)
		SetRect(&rectP, 0,0,screenwidth-1, screenheight-1);
	else
		GetWindowRect(hwndRelative, &rectP);
		
	width  = rect.right  - rect.left;
	height = rect.bottom - rect.top;

	x = ((rectP.right-rectP.left) -  width) / 2 + rectP.left;
	y = ((rectP.bottom-rectP.top) - height) / 2 + rectP.top;
	
	//make sure that the dialog box never moves outside of
	//the screen
//	if(x < 0) x = 0;
//	if(y < 0) y = 0;
//	if(x + width  > screenwidth)  x = screenwidth  - width;
//	if(y + height > screenheight) y = screenheight - height;

	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwnd, 0, x, y, width, height, SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
	}
	else
	{
		SetWindowPos(hwnd, 0, x, y, width, height, SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
		return NULL;
	}
}

// Center a window relative to its parent
VOID CenterWindow(HWND hwnd)
{
	(VOID)CenterRelative(hwnd, GetParent(hwnd), NULL);
}

//
//	Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

// 
typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

//
//	Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
	HMODULE hUXTheme;
	ETDTProc fnEnableThemeDialogTexture;

	hUXTheme = LoadLibrary(TEXT("uxtheme.dll"));

	if(hUXTheme)
	{
		fnEnableThemeDialogTexture = 
			(ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

		if(fnEnableThemeDialogTexture)
		{
			fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);
			
			FreeLibrary(hUXTheme);
			return TRUE;
		}
		else
		{
			// Failed to locate API!
			FreeLibrary(hUXTheme);
			return FALSE;
		}
	}
	else
	{
		// Not running under XP? Just fail gracefully
		return FALSE;
	}
}

//
//	Return a BOLD version of whatever font the 
//	specified window is using
//
HFONT CreateBoldFontFromHwnd(HWND hwnd)
{
	HFONT	hFont;
	LOGFONT logfont;

	// get the font information
	hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
	GetObject(hFont, sizeof(logfont), &logfont);
		
	// create it with the 'bold' attribute
	logfont.lfWeight = FW_BOLD;
	return CreateFontIndirect(&logfont);
}

//
//	Ensure that the specified window is on a visible monitor
//
void ForceVisibleDisplay(HWND hwnd)
{
	RECT		rect;

	GetWindowRect(hwnd, &rect);

	// check if the specified window-rectangle is visible on any display
	if(NULL == MonitorFromRect(&rect, MONITOR_DEFAULTTONULL))
	{
		HMONITOR hMonitor;
		MONITORINFO mi = { sizeof(mi) };
			
		// find the nearest display to the rectangle
		hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);

		GetMonitorInfo(hMonitor, &mi);

		// center window rectangle
		rect.left = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - (rect.right-rect.left)) / 2;
		rect.top  = mi.rcWork.top  + ((mi.rcWork.bottom - mi.rcWork.top) - (rect.bottom-rect.top)) / 2;

		SetWindowPos(hwnd, 0, rect.left, rect.top, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
	}
}

//
//	Resolve a ShellLink (i.e. c:\path\shortcut.lnk) to a real path
//	Refactored from MFC source
//
BOOL ResolveShortcut(TCHAR *pszShortcut, TCHAR *pszFilePath, int nPathLen)
{
	IShellLink * psl;
	SHFILEINFO   info = { 0 };
	IPersistFile *ppf;

	*pszFilePath = 0;   // assume failure

	// retrieve file's shell-attributes
	if((SHGetFileInfo(pszShortcut, 0, &info, sizeof(info), SHGFI_ATTRIBUTES) == 0))
		return FALSE;

	// not a shortcut?
	if(!(info.dwAttributes & SFGAO_LINK))
	{
		lstrcpyn(pszFilePath, pszShortcut, nPathLen);
		return TRUE;
	}

	// obtain the IShellLink interface
	if(FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl)))
		return FALSE;

	if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf)))
	{
		if (SUCCEEDED(ppf->lpVtbl->Load(ppf, pszShortcut, STGM_READ)))
		{
			// Resolve the link, this may post UI to find the link
			if (SUCCEEDED(psl->lpVtbl->Resolve(psl, 0, SLR_NO_UI )))
			{
				psl->lpVtbl->GetPath(psl, pszFilePath, nPathLen, NULL, 0);
				ppf->lpVtbl->Release(ppf);
				psl->lpVtbl->Release(psl);
				return TRUE;
			}
		}

		ppf->lpVtbl->Release(ppf);
	}

	psl->lpVtbl->Release(psl);
	return FALSE;
}


#pragma comment(lib, "version.lib")

//
//	Get the specified file-version information string from a file
//	
//	szItem	- version item string, e.g:
//		"FileDescription", "FileVersion", "InternalName", 
//		"ProductName", "ProductVersion", etc  (see MSDN for others)
//
TCHAR *GetVersionString(TCHAR *szFileName, TCHAR *szValue, TCHAR *szBuffer, ULONG nLength)
{
	DWORD  len;
	PVOID  ver;	
	DWORD  *codepage;
	TCHAR  fmt[0x40];
	PVOID  ptr = 0;
	BOOL   result = FALSE;
	
	szBuffer[0] = '\0';

	len = GetFileVersionInfoSize(szFileName, 0);

	if(len == 0 || (ver = malloc(len)) == 0)
		return NULL;

	if(GetFileVersionInfo(szFileName, 0, len, ver))
	{
		if(VerQueryValue(ver, TEXT("\\VarFileInfo\\Translation"), &codepage, &len))
		{
			wsprintf(fmt, TEXT("\\StringFileInfo\\%04x%04x\\%s"), (*codepage) & 0xFFFF, 
					(*codepage) >> 16, szValue);
			
			if(VerQueryValue(ver, fmt, &ptr, &len))
			{
				lstrcpyn(szBuffer, (TCHAR*)ptr, min(nLength, len));
				result = TRUE;
			}
		}
	}

	free(ver);
	return result ? szBuffer : NULL;
}

#define ALIGN_LEFT    0
#define ALIGN_RIGHT   1
#define ALIGN_TOP     2
#define ALIGN_BOTTOM  3

void AlignWindow(HWND hwnd, HWND hwndRelative, int method)
{
	RECT rect1;
	RECT rect2;

	GetWindowRect(hwnd, &rect1);
	MapWindowPoints(NULL, GetParent(hwnd), (POINT *)&rect1, 2);

	GetWindowRect(hwndRelative, &rect2);
	MapWindowPoints(NULL, GetParent(hwnd), (POINT *)&rect2, 2);

	switch(method)
	{
	case ALIGN_LEFT:	
		SetWindowXY(hwnd, rect2.left, rect1.top, NULL);
		break;

	case ALIGN_RIGHT:	
		SetWindowXY(hwnd, rect2.right - (rect1.right-rect1.left), rect1.top, NULL);
		break;

	case ALIGN_TOP:	
		SetWindowXY(hwnd, rect1.left, rect2.top, NULL);
		break;

	case ALIGN_BOTTOM:	
		SetWindowXY(hwnd, rect1.left, rect2.bottom - (rect1.bottom-rect1.top), NULL);
		break;
	}
}

void AddComboStringList(HWND hwndCombo, TCHAR **szList, int iInitial)
{
	int i;

	for(i = 0; szList[i]; i++)
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)szList[i]);

	SendMessage(hwndCombo, CB_SETCURSEL, iInitial, 0);
}

void AddListStringList(HWND hwndList, TCHAR **szList, int iInitial)
{
	int i;

	for(i = 0; szList[i]; i++)
		SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)szList[i]);

	SendMessage(hwndList, LB_SETCURSEL, iInitial, 0);
}

UINT ComboBox_GetSelData(HWND hwndCombo)
{
	int idx = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
	return (UINT)SendMessage(hwndCombo, CB_GETITEMDATA, idx, 0);
}

UINT ComboBox_GetDlgSelData(HWND hwndDlg, UINT nComboId)
{
	return ComboBox_GetSelData(GetDlgItem(hwndDlg, nComboId));
}

UINT ListBox_GetSelData(HWND hwndList)
{
	int idx = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	return (UINT)SendMessage(hwndList, LB_GETITEMDATA, idx, 0);
}

UINT ListBox_GetDlgSelData(HWND hwndDlg, UINT nListId)
{
	return ListBox_GetSelData(GetDlgItem(hwndDlg, nListId));
}

static int CALLBACK EnumFontProc(ENUMLOGFONTEX *elfx, NEWTEXTMETRICEX *ntmx, DWORD dwFontType, LPARAM lParam) 
{ 
	*(BOOL *)lParam = TRUE; 
	return 0; 
} 

BOOL FontExists(HWND hWnd, LPCTSTR szFontName) 
{ 
	BOOL fExists = FALSE; 
	HDC hDC = GetDC(hWnd); 

	LOGFONT lf = { 0 }; 
	lf.lfCharSet = DEFAULT_CHARSET; 
	lstrcpy(lf.lfFaceName, szFontName); 

	EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)EnumFontProc, (LPARAM)&fExists, 0); 
	ReleaseDC(hWnd, hDC); 
	return fExists; 
} 


HWND SetDlgItemFocus(HWND hwndDialog, UINT uDlgItemId)
{
	return SetFocus(GetDlgItem(hwndDialog, uDlgItemId));
}