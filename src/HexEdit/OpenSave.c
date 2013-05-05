//
//  OpenSave.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define STRICT
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define _WIN32_WINNT 0x501

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include "HexEdit.h"
#include "HexUtils.h"
#include "HexFile.h"
#include "TabView.h"
#include "RecentFile.h"
#include "resource.h"

#include "..\HexView\HexView.h"
#include "..\TypeView\TypeView.h"

#pragma comment(lib, "shlwapi.lib")

extern BOOL g_fFileChanged;
extern TCHAR g_szFileName[];
extern TCHAR g_szFileTitle[];
extern HWND  g_hwndTabView;
static HHOOK g_hHook;
extern TCHAR g_szAppName[];

static BOOL g_fUserReadOnly;
static BOOL g_fUserQuickSave;
static BOOL g_fUserKeepOnDisk;
static BOOL g_fPreserveModifyTime;

BOOL LoadHighlights(HWND hwndHexView);
BOOL SaveHighlights(HWND hwndHexView);
HWND CreateHexViewCtrl(HWND hwndParent);
BOOL UpdateHighlight(LPCTSTR pszFilePath, TCHAR * pszBookPath, BOOL fAlways);


BOOL UpdateHighlights(BOOL fAlways);//HWND hwndHexView, HWND hwndGridView);

INT_PTR CALLBACK OpenHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	OFNOTIFY *pon;
	UINT openmode;
	static BOOL fFirst = TRUE;

	switch(uiMsg)
	{
	case WM_SHOWWINDOW:

		AlignWindow(GetDlgItem(hdlg, IDC_QUICKSAVE), GetDlgItem(GetParent(hdlg), 0x470), ALIGN_LEFT);
		AlignWindow(GetDlgItem(hdlg, IDC_QUICKLOAD), GetDlgItem(GetParent(hdlg), 0x470), ALIGN_LEFT);

		g_fUserReadOnly = FALSE;
		CenterWindow(GetParent(hdlg));
		
		if(fFirst)
		{
			g_fUserKeepOnDisk = 0;//HexGetStateInt(STATE_ENABLEQUICKLOAD);
			fFirst = FALSE;
		}

		if(g_fUserKeepOnDisk)//HexGetStateInt(STATE_ENABLEQUICKLOAD))
		{
			EnableDlgItem(hdlg, IDC_QUICKSAVE, TRUE);
			CheckDlgButton(hdlg, IDC_QUICKLOAD, TRUE);
		}
		else
		{
			EnableDlgItem(hdlg, IDC_QUICKSAVE, FALSE);
		}

		openmode = 1;//HexGetStateInt(STATE_OPENMODE);
		if(openmode == 0)//HV_READONLY)
		{
			EnableDlgItem(hdlg, IDC_QUICKSAVE, FALSE);
			CheckDlgButton(hdlg, IDC_READONLY, TRUE);
			return TRUE;
		}
		
		if(g_fUserKeepOnDisk )//&& HexGetStateInt(STATE_ENABLEQUICKSAVE))
		{
			CheckDlgButton(hdlg, IDC_QUICKSAVE, TRUE);
			return TRUE;
		}

		
		return -1;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_READONLY:
			if(IsDlgButtonChecked(hdlg, IDC_READONLY))
			{
				EnableDlgItem(hdlg, IDC_QUICKSAVE, FALSE);	//quicksave
				CheckDlgButton(hdlg, IDC_QUICKSAVE, FALSE);
			}
			else if(IsDlgButtonChecked(hdlg, IDC_QUICKLOAD))
			{
				EnableDlgItem(hdlg, IDC_QUICKSAVE, TRUE);
			}
			break;

		case IDC_QUICKLOAD:
			if(IsDlgButtonChecked(hdlg, IDC_QUICKLOAD) && 
				!IsDlgButtonChecked(hdlg, IDC_READONLY))
			{
				EnableDlgItem(hdlg, IDC_QUICKSAVE, TRUE);	//quicksave
			}
			else
			{
				EnableDlgItem(hdlg, IDC_QUICKSAVE, FALSE);
				CheckDlgButton(hdlg, IDC_QUICKSAVE, FALSE);
			}

		}
		
		break;
	
	case WM_NOTIFY:
		
		if(!lParam) return 0;
		pon = (OFNOTIFY *)lParam;
	
		switch(pon->hdr.code)
		{
		case CDN_FILEOK:
			g_fUserKeepOnDisk = IsDlgButtonChecked(hdlg, IDC_QUICKLOAD);
			g_fUserReadOnly   = IsDlgButtonChecked(hdlg, IDC_READONLY);
			g_fUserQuickSave  = IsDlgButtonChecked(hdlg, IDC_QUICKSAVE) && !g_fUserReadOnly;
			break;

		default:
			break;
		}
		return 0;

	//case WM_HELP: 
	//	return HandleContextHelp(hdlg, lParam, IDD_OPENEXTEND);
	}

	return 0;
}

BOOL ShowFileOpenDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName, DWORD *openFlags)
{
	TCHAR *szFilter		= TEXT("All Files (*.*)\0*.*\0\0");

	OPENFILENAME ofn = { sizeof(ofn) };

	ofn.hwndOwner		= hwnd;
	ofn.hInstance		= GetModuleHandle(0);
	ofn.lpstrFilter		= szFilter;
	ofn.lpstrFile		= pszFileName;
	ofn.lpstrFileTitle	= pszTitleName;
	ofn.lpfnHook		= OpenHookProc;
	
	ofn.nFilterIndex	= 0;
	ofn.nMaxFile		= MAX_PATH;
	ofn.nMaxFileTitle	= MAX_PATH;

	// flags to control appearance of open-file dialog
	ofn.Flags			=	OFN_EXPLORER			|
							OFN_ENABLESIZING		|
							OFN_FILEMUSTEXIST		|
							OFN_ENABLEHOOK			|
							OFN_ENABLETEMPLATE		|
							OFN_CREATEPROMPT;

	ofn.lpTemplateName  = MAKEINTRESOURCE(IDD_OPENEXTEND);
	
	// nul-terminate filename
	pszFileName[0] = '\0';

	if(GetOpenFileName(&ofn)) 
	{
		*openFlags = 0;

		// check the standard openfile dialog flags
		if(ofn.Flags & OFN_READONLY)	*openFlags |= HVOF_READONLY;

		// check our user options
		if(g_fUserKeepOnDisk)			*openFlags |= HVOF_QUICKLOAD;
	
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

INT_PTR CALLBACK SaveHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	OFNOTIFY *pon;
	static BOOL fFirst = TRUE;

	switch(uiMsg)
	{
	case WM_SHOWWINDOW:

		AlignWindow(GetDlgItem(hdlg, IDC_PRESERVEMODIFY), GetDlgItem(GetParent(hdlg), 0x470), ALIGN_LEFT);
		CenterWindow(GetParent(hdlg));
				
		return -1;

	case WM_NOTIFY:
		
		if(!lParam) return 0;
		pon = (OFNOTIFY *)lParam;
	
		switch(pon->hdr.code)
		{
		case CDN_FILEOK:
			g_fPreserveModifyTime = IsDlgButtonChecked(hdlg, IDC_PRESERVEMODIFY);
			break;

		default:
			break;
		}
		return 0;

	}

	return 0;
}


BOOL ShowFileSaveDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName)
{
	TCHAR *szFilter		= TEXT("All Files (*.*)\0*.*\0\0");

	OPENFILENAME ofn = { sizeof(ofn) };

	ofn.hwndOwner		= hwnd;
	ofn.hInstance		= GetModuleHandle(0);
	ofn.lpstrFilter		= szFilter;
	ofn.lpstrFile		= pszFileName;
	ofn.lpstrFileTitle	= pszTitleName;
	ofn.lpfnHook		= SaveHookProc;
	
	ofn.nFilterIndex	= 0;
	ofn.nMaxFile		= MAX_PATH;
	ofn.nMaxFileTitle	= MAX_PATH;

	// flags to control appearance of open-file dialog
	ofn.Flags			=	OFN_EXPLORER			|
							OFN_ENABLESIZING		|
							OFN_ENABLEHOOK			|
							OFN_HIDEREADONLY		|  //
							OFN_ENABLETEMPLATE		|
							OFN_OVERWRITEPROMPT		;

	ofn.lpTemplateName  = MAKEINTRESOURCE(IDD_SAVEEXTEND);
	
	// nul-terminate filename
	pszFileName[0] = '\0';

	if(GetSaveFileName(&ofn)) 
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


BOOL TouchFile(TCHAR *szFileName)
{
	HANDLE hFile;
	SYSTEMTIME st;
	FILETIME ft;
	BOOL fSuccess;
	
	if((hFile = CreateFile(szFileName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		return FALSE;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	
	fSuccess = SetFileTime(hFile, NULL, NULL, &ft);
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, szFileName, 0);

	CloseHandle(hFile);
	return fSuccess;
}


UINT FmtErrorMsg(HWND hwnd, DWORD dwMsgBoxType, DWORD dwError, TCHAR *szFmt, ...)
{
	TCHAR *lpMsgBuf;
	TCHAR *ptr;
	UINT   msgboxerr;
	va_list varg;

	va_start(varg, szFmt);

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(LPTSTR) &lpMsgBuf, 0, NULL 
		);

	ptr = LocalAlloc(LPTR, LocalSize(lpMsgBuf) + 1000 * sizeof(TCHAR));
	_vstprintf(ptr, szFmt, varg);
	_tcscat(ptr, lpMsgBuf);

	msgboxerr = MessageBox(hwnd, ptr, APPNAME, dwMsgBoxType);
	
	LocalFree( lpMsgBuf );
	LocalFree( ptr );
	va_end(varg);

	return msgboxerr;
}

void ForceClientResize(HWND hwnd)
{
	RECT   rect;
	GetClientRect(hwnd, &rect);
	SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right,rect.bottom));
}

//
//	yes IDYES/IDNO/IDCANCEL
//
UINT HexFileCloseNotify(HWND hwndMain, HWND hwndHV)
{
	if(HexView_CanUndo(hwndHV))
	{
		TCHAR buf[MAX_PATH+100];
		TCHAR name[MAX_PATH];

		if(HexView_GetFileName(hwndHV, name, MAX_PATH))
			wsprintf(buf, TEXT("Do you want to save changes to\r\n%s?"), name);
		else
			wsprintf(buf, TEXT("Do you want to save changes to Untitled?"));

		return MessageBox(hwndMain, buf, g_szAppName, MB_YESNOCANCEL | MB_ICONQUESTION);
	}
	else
	{
		return IDNO;
	}
}


BOOL HexSetCurFile(HWND hwndMain, int iItem, BOOL fSetFocus)
{
	TCITEM tci = { TCIF_PARAM };
	HWND hwndHV, hwndOld;
	TCHAR szFilePath[MAX_PATH];
	TCHAR *name;
	RECT rect;
		
	// get specied item
	if(!TabCtrl_GetItem(g_hwndTabView, iItem, &tci))
		return FALSE;

	TabCtrl_SetCurSel(g_hwndTabView, iItem);

	hwndHV = (HWND)tci.lParam;
	hwndOld = g_hwndHexView;

	GetWindowRect(hwndOld, &rect);
	MapWindowPoints(0, hwndMain, (POINT *)&rect, 2);
	MoveWindow(hwndHV, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,0);
	
	g_hwndHexView = hwndHV;
			
	ForceClientResize(hwndMain);

	if(fSetFocus)
		SetFocus(hwndHV);

	HexView_GetFileName(hwndHV, szFilePath, MAX_PATH);

	name = _tcsrchr(szFilePath, '\\');
	name = name ? name+1 : TEXT("Untitled");

	SetWindowFileName(hwndMain, name, HexView_CanUndo(hwndHV), HexView_IsReadOnly(hwndHV));
	
	if(hwndOld != hwndHV)
		ShowWindow(hwndOld, SW_HIDE);

	UpdateStatusBarText(g_hwndStatusBar, hwndHV);

	UpdateTypeView();

	return TRUE;
}

BOOL HexCreateNewFile(MAINWND *mainWnd)
{
	// create a new
	int count;

	TCITEM tci = { TCIF_TEXT|TCIF_PARAM, 0, 0, TEXT("(Untitled)"), 0, 0 }; 
	
	tci.lParam = (LPARAM)CreateHexViewCtrl(mainWnd->hwndMain);

	g_fFileChanged   = FALSE;

	// reset to an empty file
	SetWindowFileName(mainWnd->hwndMain, TEXT("Untitled"), FALSE, FALSE);

	g_szFileTitle[0] = '\0';
	g_szFileName[0]  = '\0';
	g_fFileChanged   = FALSE;
	
	// insert at the end of the tab-list
	count = TabCtrl_GetItemCount(mainWnd->hwndTabView);
	TabCtrl_InsertItem(mainWnd->hwndTabView, count, &tci);
	
	HexSetCurFile(mainWnd->hwndMain, count, TRUE);
	return TRUE;
}

BOOL HexCloseFile(MAINWND *mainWnd, int iItem)
{
	TCITEM	tci = { TCIF_PARAM };
	HWND	hwndHV;
	TCHAR	szFilePath[MAX_PATH];

	if(!TabCtrl_GetItem(mainWnd->hwndTabView, iItem, &tci))
		return FALSE;

	hwndHV = (HWND)tci.lParam;

	// is this the last tab left?
	if(TabCtrl_GetItemCount(mainWnd->hwndTabView) == 1)
	{
		HexView_Clear((HWND)tci.lParam);
		
		tci.mask = TCIF_TEXT;
		tci.pszText = TEXT("(Untitled)");
		TabCtrl_SetItem(mainWnd->hwndTabView, 0, &tci);
		tci.lParam = 0;
	}
	else
	{
		// remove the tab
		TabCtrl_DeleteItem(mainWnd->hwndTabView, iItem);
	}

	// set focus to a new tab
	iItem = TabCtrl_GetCurSel(mainWnd->hwndTabView);
	HexSetCurFile(mainWnd->hwndMain, iItem, TRUE);
	
	// lastly destroy the unwanted hexview
	SaveHighlights(hwndHV);
	
	HexView_GetFileName(hwndHV, szFilePath, MAX_PATH);
	
	if(hwndHV == (HWND)tci.lParam)
		DestroyWindow(hwndHV);

	UpdateHighlight(szFilePath, 0, TRUE);
	return TRUE;
}

void UpdateCurFileName(HWND hwndMain, HWND hwndHV, LPCTSTR szFileName, BOOL fChanged)
{
	TCHAR  szFullPath[MAX_PATH];
	TCHAR *pszFileTitle;

	if(GetFullPathName(szFileName, MAX_PATH, szFullPath, &pszFileTitle))
	{
		lstrcpy(g_szFileName, szFullPath);
		lstrcpy(g_szFileTitle, pszFileTitle);

		SetWindowFileName(hwndMain, pszFileTitle, fChanged, HexView_IsReadOnly(hwndHV));
	}

	AddRecentFile(szFileName);
	UpdateRecentMenu(GetSubMenu(GetMenu(hwndMain), 0));
}

HWND HexIsOpen(HWND hwndMain, LPCTSTR szFileName, int *idx)
{
	int i;
	HWND hwndHV;
	
	// is the file already open?!
	for(i = 0; (hwndHV = EnumHexView(hwndMain, i)) != 0; i++)
	{
		TCHAR szPath[MAX_PATH];
		
		if(HexView_GetFileName(hwndHV, szPath, MAX_PATH))
		{
			if(lstrcmpi(szPath, szFileName) == 0)
			{
				if(idx) 
					*idx = i;
				
				return hwndHV;
			}
		}
	}

	return NULL;
}

void UpdateMainTabs(HWND hwndTabView)
{
	int i;
	TCITEM tci = { TCIF_PARAM, 0, 0, NULL, 0, 0, 0 };

	for(i = 0; TabCtrl_GetItem(hwndTabView, i, &tci); i++)
	{
		HWND hwndHV = (HWND)tci.lParam;
		TCHAR szPath[MAX_PATH];

		if(HexView_GetFileName(hwndHV, szPath, MAX_PATH))
		{
			TCITEM tci2 = {0};

			tci.mask     = TCIF_PARAM | TCIF_TEXT | TCIF_STATE;
			tci.dwState  = TCIS_FILENAME;
			tci.lParam   = (LPARAM)hwndHV;
			tci.pszText  = (WCHAR *)szPath;//g_szFileTitle;
			
			TabCtrl_SetItem(hwndTabView, i, &tci);
		}
	}
}


//
//	Open the specified file
//
BOOL HexOpenFile(HWND hwndMain, LPCTSTR szFileName, DWORD fHexViewFlags)
{
	HWND   hwndHV;
	TCITEM tci = { TCIF_PARAM, 0, 0, NULL, 0, 0, 0 };
	BOOL fReuseTab = FALSE;
	int  i;
	
	// is the file already open?!
	if(HexIsOpen(hwndMain, szFileName, &i))
	{
		return HexSetCurFile(hwndMain, i, TRUE);
	}
	
	// shall we use the (untitled) document or create a new window?
	if(TabCtrl_GetItemCount(g_hwndTabView) == 1 && TabCtrl_GetItem(g_hwndTabView, 0, &tci))
	{
		hwndHV = (HWND)tci.lParam;

		// is file (new) and unmodified?
		if(HexView_GetFileName(hwndHV, 0,0) == 0 && HexView_CanUndo(hwndHV) == FALSE && HexView_CanRedo(hwndHV) == FALSE)
		{
			fReuseTab = TRUE;
		}
	}
	
	if(fReuseTab == FALSE)
		hwndHV = CreateHexViewCtrl(hwndMain);

	// 
	if(HexView_OpenFile(hwndHV, szFileName, fHexViewFlags))
	{
		DWORD e = GetLastError();
			
		LoadHighlights(hwndHV);

		UpdateCurFileName(hwndMain, hwndHV, szFileName, FALSE);

		if(fReuseTab == FALSE)
		{
			tci.mask     = TCIF_PARAM | TCIF_TEXT | TCIF_STATE;
			tci.dwState  = TCIS_FILENAME;
			tci.lParam   = (LPARAM)hwndHV;
			tci.pszText  = (WCHAR *)szFileName;//g_szFileTitle;
			
			// insert at the end
			TabCtrl_InsertItem(g_hwndTabView, TabCtrl_GetItemCount(g_hwndTabView), &tci);			
		}

		UpdateMainTabs(g_hwndTabView);
		HexSetCurFile(hwndMain, TabCtrl_GetItemCount(g_hwndTabView)-1, TRUE);

		if(e == ERROR_SHARING_VIOLATION)
		{
			HexInfoBox(TEXT("%s\r\nhas been opened as read-only."), g_szFileName);
			//MessageBox(hwndMain, TEXT("Another "), TEXT("HexEdit"), MB_ICONINFORMATION);
		}

		UpdateHighlight(szFileName, 0, TRUE);
		UpdateTypeView();

		return TRUE;
	}
	else
	{
		DWORD dwError = GetLastError();
		DestroyWindow(hwndHV);
		FmtErrorMsg(hwndMain, MB_OK|MB_ICONWARNING, dwError, TEXT("Error opening \'%s\'\r\n\r\n"), szFileName);
		return FALSE;
	}
}

BOOL HexeditOpenFile(HWND hwnd, LPCTSTR szFile, DWORD openFlags)
{
	TCHAR *name;
	TCHAR *fp;

	GetFullPathName(szFile, MAX_PATH, g_szFileName, &fp);

	name = _tcsrchr(g_szFileName, '\\');
	_tcscpy(g_szFileTitle, name ? name+1 : szFile);

	return HexOpenFile(hwnd, g_szFileName, openFlags);//, g_szFileTitle);
}

//
//	How to process WM_DROPFILES
//
void HandleDropFiles(HWND hwnd, HDROP hDrop)
{
	TCHAR buf[MAX_PATH];
	DROPFILES *df = (DROPFILES *)hDrop;
	
	if(DragQueryFile(hDrop, 0, buf, MAX_PATH))
	{
		TCHAR tmp[MAX_PATH];
		
		if(ResolveShortcut(buf, tmp, MAX_PATH))
			lstrcpy(buf,tmp);

		HexeditOpenFile(hwnd, buf, HVOF_DEFAULT);
	}
	
	DragFinish(hDrop);
}


BOOL HexSaveAs(HWND hwndMain, LPCTSTR szFileName)
{
	HWND hwndHV = GetActiveHexView(hwndMain);

	if(szFileName == 0)
		return FALSE;

	// 
	if(HexView_SaveFile(hwndHV, szFileName, FALSE))
	{
		UpdateCurFileName(hwndMain, hwndHV, szFileName, FALSE);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL HexSaveCurrent(HWND hwndMain)
{
	HWND hwndHV;

	// shall we use the (untitled) document or create a new window?
	hwndHV = GetActiveHexView(hwndMain);

	// 
	if(HexView_SaveFile(hwndHV, 0, NULL))
	{
		TCHAR szFile[MAX_PATH];
		HexView_GetFileName(hwndHV, szFile, MAX_PATH);
		UpdateCurFileName(hwndMain, hwndHV, szFile, FALSE);
		
		return TRUE;
	}

	return FALSE;
}


HWND EnumHexView(HWND hwndMain, int idx)
{
	MAINWND *mainWnd;
	TCITEM tci = { TCIF_PARAM };

	if((mainWnd = (MAINWND *)GetWindowLongPtr(hwndMain, 0)) == 0)
		return 0;
	
	if(TabCtrl_GetItem(mainWnd->hwndTabView, idx, &tci))
	{
		return (HWND)tci.lParam;
	}
	else
	{
		return NULL;
	}
}


UINT DefaultFileMode()
{
	UINT mode = 0;

	if(g_fQuickLoad)
		mode |= HVOF_QUICKLOAD;

	return mode;
}

BOOL HexSetCurFileHwnd(HWND hwndMain, HWND hwndHexView)
{
	int i;
	HWND hwndHV;

	for(i = 0; (hwndHV = EnumHexView(hwndMain, i)) != 0; i++)
	{
		if(hwndHV == hwndHexView)
		{
			HexSetCurFile(hwndMain, i, FALSE);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL HexSetCurFileName(HWND hwndMain, LPCTSTR szFileName)
{
	int idx;
	HWND hwndHV;
	
	if((hwndHV = HexIsOpen(hwndMain, szFileName, &idx)) != 0)
	{
		HexSetCurFile(hwndMain, idx, FALSE);
		return TRUE;
	}

	return FALSE;
}

