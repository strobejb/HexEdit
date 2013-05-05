//
//  HexCommands.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <commctrl.h>
#include "resource.h"
#include "HexEdit.h"
#include "HexFile.h"

#include "..\HexView\HexView.h"
#include "..\DockLib\DockLib.h"
#include "..\TypeView\TypeView.h"

extern HWND		 g_hwndMain;
extern HWND		 g_hwndHexView;
extern HWND		 g_hwndSearchBar;
extern HWND		 g_hwndToolbar;
extern HINSTANCE g_hInstance;

extern TCHAR g_szFileName[];
extern TCHAR g_szFileTitle[];
extern BOOL g_fFileChanged;

BOOL ShowFileOpenDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName, DWORD *flags);
BOOL ShowFileSaveDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName);
BOOL ShowFindDialog(HWND hwndOwner, int idx);
void ShowAboutDlg(HWND hwndParent);
BOOL ShowExportDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName);
BOOL ShowImportDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName);
BOOL ShowGotoDialog(HWND hwndOwner);
void ShowOptions(HWND hwndParent);
BOOL ShowChecksumDlg(HWND hwnd);
BOOL ShowStringsDlg(HWND hwnd);
void UpdateToolbarState(HWND hwndTB, HWND hwndHV);
int HighlightDlg(HWND hwndHV, HBOOKMARK hBookMark);
void SetWindowFileName(HWND hwnd, TCHAR *szFileName, BOOL fModified, BOOL fReadOnly);
int ShowSelectRangeDlg(HWND hwnd);
BOOL ShowModifyDlg(HWND hwnd);
BOOL ShowInsertDlg(HWND hwnd);
BOOL ShowReverseDlg(HWND hwnd);
BOOL ShowCompareDlg(HWND hwnd);
BOOL FileProperties(HWND hwndParent);

void Goto(HWND hwndHexView, size_w offset, int nGotoRelative);
BOOL FindNext();
void RepeatGoto(HWND hwndHexView);

BOOL GetRecentFile(int idx, TCHAR *szName, UINT nLength);
void ToggleEditorMode(HWND hwndHV);

void ShowHighlightTextDlg(HWND hwndParent)
{
	POINT pt = {100,10};
	//ClientToScreen(hwndParent, &pt);
	GetCursorPos(&pt);

	/*CreateWindowEx(
		WS_EX_TOOLWINDOW,
		TEXT("EDIT"),
		TEXT("Hello World"),
		WS_OVERLAPPEDWINDOW|WS_POPUP|WS_VISIBLE|ES_MULTILINE|WS_VSCROLL,
		pt.x,pt.y,200,100,hwndParent, 0, 0, 0);*/

	HighlightDlg(hwndParent, NULL);
	
}

BOOL PasteSpecial(HWND hwndDlg, UINT uFormat, UINT nTransformIdx);

LONG HexEdit_OnCommand(HWND hwnd, UINT nCommandId, UINT nNotify, HWND hwndControl)
{
	DWORD	val;
	TCHAR	buf[MAX_PATH];
	TCHAR	path[MAX_PATH], title[MAX_PATH];
	DWORD   flags;

	// which HexView are we operating on?
	HWND hwndHV = g_hwndHexView;
	MAINWND *mainWnd = (MAINWND *)GetWindowLongPtr(hwnd, 0);

	// react to menu messages
	switch(nCommandId)
	{
	//case IDM_VALUE_SHOW:
	//	return 0;

	case IDM_STATUSBAR_CURSORPOS:
		g_fStatusHexCursor = !g_fStatusHexCursor;
		UpdateStatusBarText(g_hwndStatusBar, hwndHV);
		return 0;

	case IDM_STATUSBAR_VALUE:
		return 0;

	case IDM_STATUSBAR_SIZE:
		g_fStatusHexSize = !g_fStatusHexSize;
		UpdateStatusBarText(g_hwndStatusBar, hwndHV);
		return 0;

	case IDM_STATUSBAR_MODE:
		ToggleEditorMode(hwndHV);
		UpdateStatusBarText(g_hwndStatusBar, hwndHV);
		return 0;

	case IDM_FILE_NEW:
		HexCreateNewFile(mainWnd);
		return 0;

	case IDM_FILE_OPEN:

		// get a filename to open
		if(ShowFileOpenDlg(hwnd, path, title, &flags))
		{
			if(HexOpenFile(hwnd, path, flags))
			{
				_tcscpy(g_szFileName, path);
				_tcscpy(g_szFileTitle, title);
			}
		}
		
		

		//HexFileOpenDlg(hwnd, szFileName, szFileTitle);
		return 0;

	case IDM_FILE_REVERT:
		if(HexView_CanUndo(hwndHV) || HexView_CanRedo(hwndHV))
		{
			HexView_Revert(hwndHV);
		}

		UpdateTypeView();

		return 0;

	case IDM_FILE_PROPERTIES:
		FileProperties(hwnd);
		return 0;

	case IDM_FILE_SAVE:
		//PasteSpecial(hwnd, CF_TEXT, 8);

		if(g_szFileName[0] == '\0')
		{
			// do a 'saveas'
			PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_FILE_SAVEAS, nNotify), 0);
		}
		else
		{
			HexSaveCurrent(hwnd);
		}
		return 0;

	case IDM_FILE_SAVEAS:

		if(ShowFileSaveDlg(hwnd, path, title))
		{
			if(HexSaveAs(hwnd, path))
			{
				_tcscpy(g_szFileName, path);
				_tcscpy(g_szFileTitle, title);
			}
		}

		return 0;

	case IDM_FILE_EXPORT:
		ShowExportDlg(hwnd, 0, 0);
		return 0;

	case IDM_FILE_IMPORT:
		ShowImportDlg(hwnd, 0, 0);
		return 0;

	case IDM_RECENT1: case IDM_RECENT2:case IDM_RECENT3:case IDM_RECENT4:
	case IDM_RECENT5: case IDM_RECENT6:case IDM_RECENT7:case IDM_RECENT8:

		if(GetRecentFile(nCommandId-IDM_RECENT1, buf, MAX_PATH))
		{
			HexOpenFile(hwnd, buf, DefaultFileMode());
		}

		return 0;

	case IDM_FILE_EXIT:
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return 0;

	case IDM_FILE_CLOSE:
		if(HexFileCloseNotify(hwnd, hwndHV) == IDCANCEL)
			return 0;

		HexCloseFile(mainWnd, TabCtrl_GetCurSel(mainWnd->hwndTabView));
		return 0;

	case IDM_EDIT_UNDO:
		HexView_Undo(hwndHV);
		UpdateToolbarState(g_hwndToolbar, hwndHV);
		return 0;

	case IDM_EDIT_REDO:
		HexView_Redo(hwndHV);
		UpdateToolbarState(g_hwndToolbar, hwndHV);
		return 0;

	case IDM_EDIT_CUT:
		HexView_Cut(hwndHV);
		return 0;

	case IDM_EDIT_COPY:
		HexView_Copy(hwndHV);
		return 0;

	case IDM_EDIT_PASTE:
		HexView_Paste(hwndHV);
		return 0;

	case IDM_EDIT_DELETE:
		HexView_Delete(hwndHV);
		return 0;

	case IDM_HIGHLIGHT_REMOVEALL:
		HexView_ClearBookmarks(hwndHV);
		InvalidateRect(hwndHV, 0, 0);
		return 0;

	case IDM_HIGHLIGHT_NEW:
		ShowHighlightTextDlg(hwndHV);
		return 0;

	case IDM_EDIT_COPYAS:
		CopyAsDlg(hwnd);
		return 0;

	case IDM_EDIT_FILL:
		ShowInsertDlg(hwnd);
		return 0;

	case IDM_EDIT_REVERSE:
		ShowReverseDlg(hwnd);
		return 0;

	case IDM_TOOLS_TRANSFORM:
		ShowModifyDlg(hwnd);
		return 0;

	case IDM_TOOLS_OPTIONS:
		ShowOptions(hwnd);
		return 0;

	case IDM_VIEW_TOOLBAR:
		DockWnd_Show(hwnd, DWID_TOOLBAR, !DockWnd_IsOpen(hwnd, DWID_TOOLBAR));
		return 0;

	case IDM_VIEW_FITTOWINDOW:
		g_fFitToWindow = !g_fFitToWindow;
		return 0;

	case IDM_TOOLS_SEARCHBAR:
		DockWnd_Show(hwnd, DWID_SEARCHBAR, !DockWnd_IsOpen(hwnd, DWID_SEARCHBAR));
		return 0;

	case IDM_HIGHLIGHT_MANAGE:
		DockWnd_Show(hwnd, DWID_HIGHLIGHT, !DockWnd_IsOpen(hwnd, DWID_HIGHLIGHT));
		return 0;

	case IDM_TYPEVIEW_ALLTYPES:
		DockWnd_Show(hwnd, DWID_ALLTYPES, !DockWnd_IsOpen(hwnd, DWID_ALLTYPES));
		return 0;

	case IDM_TYPEVIEW_DLG:
	case IDM_TOOLS_TYPEVIEW:
		
		if(DockWnd_IsOpen(hwnd, DWID_TYPEVIEW))
			val = TBSTATE_ENABLED;
		else
			val = TBSTATE_ENABLED | TBSTATE_CHECKED;
			
		//SendMessage(DockWnd_GetContents(hwnd, DWID_TOOLBAR), TB_GETSTATE, IDM_TOOLS_TYPEVIEW, 0);

		DockWnd_ShowGroup(hwnd, DWID_TYPEVIEW, (val & TBSTATE_CHECKED) ? TRUE : FALSE);
		
		
		SendMessage(DockWnd_GetContents(hwnd, DWID_TOOLBAR), TB_SETSTATE,
			IDM_TOOLS_TYPEVIEW, val);
		return 0;

	case IDM_EDIT_PASTESPECIAL:
		HexPasteSpecialDlg(hwnd);
		return 0;

	case IDM_EDIT_SELECTALL:
		HexView_SelectAll(hwndHV);
		return 0;

	case IDM_EDIT_SELECTRANGE:
		ShowSelectRangeDlg(hwnd);
		return 0;
		
	case IDM_HELP_ABOUT:
		ShowAboutDlg(hwnd);
		return 0;

	case IDM_SEARCH_FIND:
		ShowFindDialog(hwnd, -1);
		return 0;

	case IDM_SEARCH_NEXT:
		FindNext();
		return 0;

	case IDM_SEARCH_REPLACE:
		ShowFindDialog(hwnd, 3);
		return 0;

	case IDM_SEARCH_GOTO:
		ShowGotoDialog(hwnd);
		return 0;

	case IDM_SEARCH_REPEAT:
		RepeatGoto(hwndHV);
		return 0;
		
	case IDM_VIEW_HEX:
		HexView_SetStyle(hwndHV, HVS_FORMAT_MASK, HVS_FORMAT_HEX);
		return 0;
		
	case IDM_VIEW_OCT:
		HexView_SetStyle(hwndHV, HVS_FORMAT_MASK, HVS_FORMAT_OCT);
		return 0;
		
	case IDM_VIEW_DEC:
		HexView_SetStyle(hwndHV, HVS_FORMAT_MASK, HVS_FORMAT_DEC);
		return 0;
		
	case IDM_VIEW_BIN:
		HexView_SetStyle(hwndHV, HVS_FORMAT_MASK, HVS_FORMAT_BIN);
		return 0;
		
	case IDM_GROUP_BYTE:
		HexView_SetGrouping(hwndHV, 1);
		return 0;
		
	case IDM_GROUP_WORD:
		HexView_SetGrouping(hwndHV, 2);
		return 0;
		
	case IDM_GROUP_DWORD:
		HexView_SetGrouping(hwndHV, 4);
		return 0;
		
		/*	case IDM_GROUP_QWORD:
		HexView_SetGrouping(hwndHexView, 8);
		return 0;*/
		
	case IDM_NEXT_WINDOW:
		
		DockWnd_NextWindow(hwnd);

		//if(DockWnd_IsOpen(hwnd, DWID_TYPEVIEW))
		return 0;

	case IDM_TOOLS_CHECKSUM:
		ShowChecksumDlg(hwnd);
		return 0;

	case IDM_TOOLS_STRINGS:
		ShowStringsDlg(hwnd);
		return 0;

	case IDM_TOOLS_COMPARE:
		ShowCompareDlg(hwnd);
		return 0;

	}
	
	return 0;
}