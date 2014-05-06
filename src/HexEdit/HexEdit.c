//
//  HexEdit.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_DEPRECATE
#define _WIN32_WINNT 0x501
#define STRICT


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "HexEdit.h"
#include "HexUtils.h"
#include "HexFile.h"
#include "TabView.h"
#include "RecentFile.h"
#include "FileChange.h"
#include "..\DockLib\DockLib.h"
#include "..\ConfigLib\ConfigLib.h"
#include "..\TypeView\TypeView.h"
#include "trace.h"


/* Generated by HexEdit */
/* c:\src\test.bin */
BYTE hexData[0x3e9] = 
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 
  0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 
  0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 
  0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 
  0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 
  0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 
  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 
  0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 
  0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 
  0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 
  0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 
  0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 
  0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 
  0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 
  0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 
  0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
  0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 
  0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 
  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 
  0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 
  0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 
  0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 
  0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 
  0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 
  0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 
  0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 
  0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 
  0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 
  0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 
  0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 
  0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 
  0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 
  0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 
  0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 
  0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
  0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 
  0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 
  0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 
  0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 
  0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 
  0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 
  0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 
  0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 
  0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 
  0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0x00, 0x01, 
  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 
  0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 
  0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 
  0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
  0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 
  0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 
  0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 
  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 
  0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 
  0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 
  0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 
  0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 
  0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 
  0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 
  0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 
};


HWND		g_hwndMain;
HWND		g_hwndHexView;
HWND		g_hwndStatusBar;
HWND		g_hwndTabView;
HWND		g_hwndToolbar;
HWND		g_hwndSearchBar;
HINSTANCE	g_hInstance;
HWND		g_hwndTypeView;
HWND		g_hwndGoto;


extern HWND g_hwndSearch;
extern BOOL g_fDisplayHex;
extern BOOL g_fDisplayBigEndian;

extern HFONT g_hHexViewFont;

#define HEXVIEW_DEFAULT_STYLE (HVS_RESIZEBAR|HVS_ALWAYSVSCROLL|HVS_SHOWMODS|HVS_FORCE_FIXEDCOLS)

TCHAR		g_szFileName[MAX_PATH];
TCHAR		g_szAppName[] = APPNAME;
TCHAR		g_szFileTitle[MAX_PATH];
BOOL		g_fFileChanged		= FALSE;
BOOL		g_fStatusHexCursor	= TRUE;
BOOL		g_fStatusHexSize	= FALSE;

int			g_nStatusValueType  = IDM_VALUE_BYTE;
BOOL		g_fStatusSignedValue = FALSE;
BOOL		g_fStatusHexValue    = TRUE;
BOOL		g_fStatusBigEndian   = FALSE;

BOOL		g_fQuickLoad		= TRUE;
BOOL		g_fRestoreWinPos	= TRUE;


LONG HexEdit_OnCommand(HWND hwnd, UINT nCommandId, UINT nNotify, HWND hwndControl);
BOOL ShowFindDialog(HWND hwndOwner);
HWND InitToolbar(HWND hwndParent);
HWND CreateStatusBar (HWND hwndParent);
void SetStatusBarParts(HWND hwndSB);
HWND CreateSearchBar(HWND hwndParent);
HWND CreateTypeView(HWND hwndParent, HKEY hKey, BOOL fAllTypes);
HWND CreateHighlightView(HWND hwndParent);
HWND CreateStringsView(HWND hwndParent);
void UpdateToolbarState(HWND hwndTB, HWND hwndHV);
BOOL SaveHighlights(HWND hwndHexView);
void SaveSettings();
void LoadSettings();
void FirstTimeOptions(HWND hwndMain);
BOOL UpdateHighlights(BOOL fAlways);
int HexPasteSpecialDlg2(HWND hwnd);
void InitTypeLibrary();

#pragma comment(lib, "comctl32.lib")

/*	DWORD	dwStyle;		//styles..

	int		xpos;			//coordinates of FRAME when floating
	int		ypos;

	int		cxFloating;		//size of CONTENTS when floating 
	int		cyFloating;

	int		nDockedSize;	//width/height of window when docked..
	BOOL	fDocked;		//docked/floating?
	UINT	uDockedState;	//left/right/top/bottom when docked?

	RECT	rcBorderDock;	//border to place around each side of contents
	RECT	rcBorderFloat;	//border to place around each side of contents

	DWORD	dwUser;			//whatever..
*/

//DockWnd g_DockBar[30]; 
/*{
	WS_CHILD|WS_VISIBLE, 100, 100, 300, 32, 32, TRUE, 0
};*/

//
//	Set the main window filename
//
void SetWindowFileName(HWND hwnd, TCHAR *szFileName, BOOL fModified, BOOL fReadOnly)
{
	TCHAR ach[MAX_PATH + sizeof(g_szAppName) + 4];
	TCHAR mod[4]   = TEXT("");
	TCHAR read[14] = TEXT("");

	if(fModified)
		lstrcpy(mod, TEXT(" *"));

	if(fReadOnly)
		lstrcpy(read, TEXT(" (readonly)"));

	wsprintf(ach, TEXT("%s - %s%s%s"), szFileName, g_szAppName, mod, read);
	SetWindowText(hwnd, ach);
}

HWND GetActiveHexView(HWND hwndMain)
{
	return g_hwndHexView;

	/*MAINWND *mainWnd;
	TCITEM tci = { TCIF_PARAM };

	if((mainWnd = (MAINWND *)GetWindowLongPtr(hwndMain, 0)) == 0)
		return 0;
	
	TabCtrl_GetItem(mainWnd->hwndTabView, TabCtrl_GetCurSel(mainWnd->hwndTabView), &tci);
	return (HWND)tci.lParam;*/
}

HWND CreateToolTip(HWND hwndTarget)
{
	HWND hwnd;
	TOOLINFO ti = { 0 };
	RECT rect;
    
	hwnd = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        TEXT("WTF"),
        WS_VISIBLE|WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP |TTS_BALLOON,		
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwndTarget,
        NULL,
        g_hInstance,
        NULL
        );

	GetClientRect(hwndTarget, &rect);

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS|TTF_IDISHWND;
    ti.hwnd = hwndTarget;
    ti.hinst = g_hInstance;
    ti.uId = (UINT)hwndTarget;
    ti.lpszText = TEXT("Ballooooon");
        // ToolTip control will cover the whole window
    ti.rect.left = rect.left;    
    ti.rect.top = rect.top;
    ti.rect.right = rect.right;
    ti.rect.bottom = rect.bottom;
    
    /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
    SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

	SendMessage(hwnd, TTM_SETTITLE, 1, (LPARAM)TEXT("Bookmark"));
	return hwnd;
}

void MainWndSize(MAINWND *mainWnd, int width, int height)
{	
	int statusheight;
	static int tabheight = 0;
	int toolheight;
	HDWP hdwp;
	RECT rect;
	RECT rcClient;

	HWND hwndHV = GetActiveHexView(mainWnd->hwndMain);

	hdwp = BeginDeferWindowPos(0);

	GetWindowRect(mainWnd->hwndStatusBar, &rect);
	statusheight = rect.bottom-rect.top;

	// dock out where docking windows are allowed
	SetRect(&rcClient, 0, 0, width, height-statusheight);
	//TRACEA("1: %d %d %d %d\n", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	// Position the specified dock windows. rect will be modified to contain the
	// "inner" client rectangle, where we can position an MDI client,
	// view window, whatever
//	DockWnd_Position(hwnd, hdwp, &g_DockBar[0], 3, &rcClient);

	DockWnd_DeferPanelPos(hdwp, mainWnd->hwndMain, &rcClient);

	//TRACEA("2: %d %d %d %d\n", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	if(rcClient.top > 0)//DockWnd_IsOpen(hwnd, DWID_TOOLBAR))
		rcClient.top += 1;

	// make a resize bar
	//rcClient.top += 2;

	height  = rcClient.bottom-rcClient.top;
	width = rcClient.right-rcClient.left;

	//TRACEA("3: %d %d %d %d\n", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	if(TabCtrl_GetItemCount(mainWnd->hwndTabView) > 1)
	{
		tabheight = 32;
	}
	else
	{
		TCHAR  szText[20];
		TCITEM tci = { TCIF_TEXT|TCIF_PARAM, 0, 0, szText, 20 };
		
		TabCtrl_GetItem(mainWnd->hwndTabView, 0, &tci);
		if(lstrcmp(szText, TEXT("(Untitled)")) == 0)
			tabheight = 0;//max(tabheight, 0);
		else
			tabheight =max(tabheight, 0);
	}
		//mainWnd->hwndTabView ? 32 : 0;

	//TRACEA("4: %d %d %d %d\n", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);


	toolheight = 43;

	//DeferWindowPos(hdwp, g_hwndToolbar, hwnd, 0, 0, width, toolheight, SWP_NOACTIVATE|SWP_NOZORDER);
	//DeferWindowPos(hdwp, g_hwndDock[0], hwnd, 0, 0, width, toolheight, SWP_NOACTIVATE|SWP_NOZORDER);
		
//		// resize editbox to fit in main window
	//TRACEA("%d %d %d %d\n", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
	DeferWindowPos(hdwp, hwndHV, mainWnd->hwndMain, rcClient.left, rcClient.top, width, height-tabheight,SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);

	DeferWindowPos(hdwp, mainWnd->hwndTabView,  mainWnd->hwndMain, rcClient.left, rcClient.top+height-tabheight, width, tabheight, SWP_NOACTIVATE|SWP_NOZORDER);
	
	GetClientRect(mainWnd->hwndMain, &rect);
	DeferWindowPos(hdwp, mainWnd->hwndStatusBar, mainWnd->hwndMain, 0, rect.bottom-statusheight, rect.right, statusheight,SWP_NOACTIVATE|SWP_NOZORDER);

	EndDeferWindowPos(hdwp);

	SetStatusBarParts(mainWnd->hwndStatusBar);
}

void InitDockingBars(HWND hwnd)
{
	HDWP hdwp = 0;

	DockWnd_Initialize(hwnd, REGLOC);

	// load DOCKWNDs from the registry
	DockWnd_LoadSettings(hwnd, g_fRestoreWinPos);

	//hdwp = DockWnd_BeginDefer(hwnd, 10);
	DockWnd_DeferShowPopups(hwnd);
	
	// 
	if(DockWnd_Undefined(hwnd, DWID_TOOLBAR))//, TEXT("Toolbar")))
	{
		DWORD dwStyle = DWS_FORCEDOCK
						|DWS_DRAWGRIPPER
						|DWS_DOCKED_TOP
						|DWS_THEMED_BACKGROUND
						|DWS_NOSETFOCUS
						|DWS_FIXED_VERT
						|DWS_FIXED_HORZ
						;

		DockWnd_Register(hwnd, DWID_TOOLBAR, TEXT("Toolbar"));

		//DockWnd_SetStyle(hwnd, DWID_TOOLBAR, DWS_DOCKED|DWS_FORCEDOCK|DWS_FIXED_SIZE, DWS_DOCKED|DWS_FORCEDOCK|DWS_FIXED_SIZE);
		DockWnd_SetStyle(hwnd, DWID_TOOLBAR, dwStyle, dwStyle);

		DockWnd_Dock(hwnd, DWID_TOOLBAR);
		DockWnd_Show(hwnd, DWID_TOOLBAR, TRUE);
		CenterRelative(DockWnd_GetWindow(hwnd, DWID_TOOLBAR), hwnd, hdwp);

	}
	
	//
	if(0&&DockWnd_Undefined(hwnd, DWID_SEARCHBAR))//, TEXT("SearchBar")))
	{
		DWORD dwStyle = DWS_DRAWGRIPPER | DWS_FIXED_VERT | DWS_DOCKED_BOTTOM;

		DockWnd_Register(hwnd, DWID_SEARCHBAR, TEXT("SearchBar"));

		DockWnd_SetStyle(hwnd, DWID_SEARCHBAR, dwStyle, dwStyle);

		//DockWnd_Dock(hwnd, DWID_SEARCHBAR);
		DockWnd_Show(hwnd, DWID_SEARCHBAR, TRUE);
		//CenterWindow(DockWnd_GetWindow(hwnd, DWID_SEARCHBAR));
	}
	
	if(DockWnd_Undefined(hwnd, DWID_TYPEVIEW))//, TEXT("TypeView")))
	{
		DWORD dwStyle = DWS_SPLITTER |
						DWS_TABSTRIP |
						DWS_DOCKED_BOTTOM //|
						//DWS_DOCKED_TITLEBAR
						;// | DWS_DRAWGRIPPER;

		DockWnd_Register(hwnd, DWID_TYPEVIEW, TEXT("TypeView"));
		DockWnd_SetGroupId(hwnd, DWID_TYPEVIEW, DWID_TYPEVIEW);

		DockWnd_SetStyle(hwnd, DWID_TYPEVIEW, dwStyle, dwStyle);

		DockWnd_Dock(hwnd, DWID_TYPEVIEW);
		//DockWnd_Show(hwnd, DWID_TYPEVIEW, TRUE);
		CenterRelative(DockWnd_GetWindow(hwnd, DWID_TYPEVIEW), hwnd, hdwp);
	}

	if(DockWnd_Undefined(hwnd, 	DWID_ALLTYPES))//, TEXT("TypeView")))
	{
		DWORD dwStyle = DWS_SPLITTER|DWS_TABSTRIP| DWS_DOCKED_BOTTOM;// | DWS_DRAWGRIPPER;

		DockWnd_RegisterEx(hwnd, DWID_ALLTYPES, DWID_TYPEVIEW, TEXT("All Types"));
		DockWnd_SetGroupId(hwnd, DWID_ALLTYPES, DWID_TYPEVIEW);

		DockWnd_SetStyle(hwnd, DWID_ALLTYPES, dwStyle, dwStyle);

		DockWnd_Dock(hwnd, DWID_ALLTYPES);
		//DockWnd_Show(hwnd, DWID_ALLTYPES, TRUE);
		CenterRelative(DockWnd_GetWindow(hwnd, DWID_ALLTYPES), hwnd, hdwp);
	}


	if(DockWnd_Undefined(hwnd, DWID_HIGHLIGHT))
	{
		DWORD dwStyle = DWS_SPLITTER|DWS_TABSTRIP| DWS_DOCKED_RIGHT;// | DWS_DRAWGRIPPER;

		DockWnd_Register(hwnd, DWID_HIGHLIGHT, TEXT("Bookmarks"));
		DockWnd_SetStyle(hwnd, DWID_HIGHLIGHT, dwStyle, dwStyle);

		DockWnd_Dock(hwnd, DWID_HIGHLIGHT);
		//DockWnd_Show(hwnd, DWID_HIGHLIGHT, TRUE);
		CenterRelative(DockWnd_GetWindow(hwnd, DWID_HIGHLIGHT), hwnd, hdwp);
	}

	if(DockWnd_Undefined(hwnd, DWID_STRINGS))
	{
		DWORD dwStyle = DWS_SPLITTER| DWS_DOCKED_RIGHT;// | DWS_DRAWGRIPPER;

		DockWnd_Register(hwnd, DWID_STRINGS, TEXT("Strings"));
		DockWnd_SetStyle(hwnd, DWID_STRINGS, dwStyle, dwStyle);

		//DockWnd_Dock(hwnd, DWID_HIGHLIGHT);
		//DockWnd_Show(hwnd, DWID_STRINGS, TRUE);
		CenterRelative(DockWnd_GetWindow(hwnd, DWID_STRINGS), hwnd, hdwp);
	}
	

//	DockWnd_ShowGui(hwnd);
	DockWnd_Update(hwnd);

	//DockWnd_EndDefer(hwnd);
}

BOOL UpdateProgress(MAINWND *mainWnd, BOOL fVisible, size_w pos, size_w len)
{
	MSG msg;
	BOOL fAbort = FALSE;

	int newpos = len > 0 ? (int)((UINT64)pos*100/len) : 0;//(unsigned __int64)256) / len);
	DWORD dwRate;
	
	static HWND hwndProgress;
	static DWORD startTicks;
	static DWORD lastTick;
	static size_w oldpos;
	static size_w startpos;

	TRACEA("update progress: %x/%x\n", (DWORD)(pos), (DWORD)(len));


	if(fVisible == FALSE)
	{
		DestroyWindow(hwndProgress);
		SetStatusBarText(mainWnd->hwndStatusBar, 0, 1, TEXT(""));
		return FALSE;
	}
	

	// create the progress window if it's not been done already
	if(hwndProgress == 0)
	{
		RECT rect;

		SendMessage(mainWnd->hwndStatusBar, SB_GETRECT, 0, (LPARAM)&rect);

		hwndProgress = CreateWindowEx(0, PROGRESS_CLASS, 0, WS_CHILD|WS_VISIBLE,
			rect.left+220,rect.top+2,
			min(rect.right-rect.left-220,159),rect.bottom-rect.top-5,
			mainWnd->hwndStatusBar, 0, 0, 0);

		SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		startTicks = GetTickCount();
		oldpos = 0;
		startpos = pos;
	}

	if(GetTickCount() - lastTick > 1000)
	{
		lastTick = GetTickCount(); 
		dwRate = (lastTick - startTicks) / 1000;
		
		//TRACEA("search: searched=%d elapsed=%d (%u/%u -> %d)\n", pos-oldpos, dwRate, pos, len, newpos);
		
		dwRate = (DWORD)(dwRate ? ((pos-startpos)/(1024*1024)) / dwRate : 0);
		//dwRate = dwRate / (1024 * 1024);
		
		SetStatusBarText(mainWnd->hwndStatusBar, 0, 1, TEXT("%s: %d Mb/s"), TEXT("Searching"), dwRate);
	}
		
	SendMessage(hwndProgress, PBM_SETPOS, newpos, 0);

	// dispatch any messages 
	while(!fAbort && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		
		switch(msg.message)
		{
		case WM_QUIT:
			PostQuitMessage(0);
			fAbort = TRUE;
			break;

		case WM_KEYDOWN:
			if(msg.wParam == VK_ESCAPE)
			{
				fAbort = TRUE;
				//fAbort = QueryAbortDlg();
				continue;
			}
			break;

		case WM_LBUTTONDOWN : case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
			//fAbort = QueryAbortDlg();
			fAbort = TRUE;
			continue;
		}
			
		DispatchMessage(&msg);
	}

	oldpos = pos;
	return fAbort;
}

void HexView_CursorChanged(HWND hwndMain, HWND hwndHV)
{
	MAINWND *mainWnd = (MAINWND *)GetWindowLongPtr(hwndMain, 0);

	UpdateStatusBarText(mainWnd->hwndStatusBar, hwndHV);
	UpdateToolbarState(mainWnd->hwndStatusBar, hwndHV);
}

LONG HexView_Changed(MAINWND *mainWnd, NMHVCHANGED *hvc)
{
	HWND hwndHV = GetActiveHexView(mainWnd->hwndMain);//g_hwndHexView;

	TCHAR *szMethod[] = {
		TEXT("??"), TEXT("HVMETHOD_OVERWRITE"), TEXT("HVMETHOD_INSERT"), TEXT("HVMETHOD_DELETE"), 
	};

	TRACE(TEXT("Changed: %08I64x %I64x %s\n"), hvc->offset, hvc->length, szMethod[hvc->method]);

	mainWnd->fChanged = TRUE;
	UpdateToolbarState(mainWnd->hwndToolbar, hwndHV);

	if(g_szFileTitle[0])
	{
		BOOL fModified = HexView_CanUndo(hwndHV);

		if(fModified != g_fFileChanged)
		{
			SetWindowFileName(mainWnd->hwndMain, g_szFileTitle, fModified, FALSE);
			g_fFileChanged = fModified;
		}
	}

	UpdateTypeView();
	return 0;
}

LONG_PTR HexViewNotifyHandler(MAINWND *mainWnd, HWND hwnd, NMHDR *hdr)
{
	HWND hwndHV = GetActiveHexView(hwnd);//g_hwndHexView;
	NMHVPROGRESS *hvp;
	NMHVCHANGED  *hvc;

	switch(hdr->code)
	{
	/*case HVN_CHANGING:
		hvc = (NMHVCHANGED *)hdr;
		if(hvc->data)
			hexData[hvc->offset] = hvc->data[0];

		return -1;
		*/
	case HVN_CHANGED:

		hvc = (NMHVCHANGED *)hdr;
		HexView_Changed(mainWnd, hvc);
		return 0;

	case HVN_EDITMODE_CHANGE:

		// show the unicode value under the cursor
		//SetStatusBarText(g_hwndStatusbar, 1, 0, TEXT(" U+%04X"), 
		//	TextView_GetCurChar(g_hwndTextView) );

		UpdateStatusBarText(mainWnd->hwndStatusBar, hwndHV);

		//SetStatusBarText(mainWnd->hwndStatusBar, 4, 0, 
		//	g_szEditMode[HexView_GetEditMode(hwndHV)] );

		return 0;

	case HVN_CURSOR_CHANGE:

		UpdateStatusBarText(mainWnd->hwndStatusBar, hwndHV);
		UpdateToolbarState(mainWnd->hwndStatusBar, hwndHV);

		UpdateTypeView();

		return 0;

	case HVN_PROGRESS:
		hvp = (NMHVPROGRESS *)hdr;
		return UpdateProgress(mainWnd, TRUE, hvp->pos, hvp->len);
		//return 0;

	case HVN_BOOKCLOSE:
		//HexView_DelBookmark(((NMHVBOOKMARK *)hdr)->hdr.hwndFrom, 0);
		//RemoveBookmark((NMHVBOOKMARK *)hdr);
		return 0;

	default:
		return 0;
	}
}

void ToggleEditorMode(HWND hwndHV)
{
	UINT nMode;

	nMode   = HexView_GetEditMode(hwndHV);
	//nMode	= (nMode + 1) % 2;//3;

	if(nMode == HVMODE_INSERT)
		nMode = HVMODE_OVERWRITE;
	else
		nMode = HVMODE_INSERT;
		
	
	HexView_SetEditMode(hwndHV, nMode);
	nMode   = HexView_GetEditMode(hwndHV);
}

LONG StatusBar_DropDownTB(MAINWND *mainWnd, HWND hwndHV, NMTOOLBAR *nmtb)
{
	RECT rect = nmtb->rcButton;
	HMENU hMenu, hPopup;
	UINT mode, cmdId;

	MapWindowPoints(nmtb->hdr.hwndFrom, 0, (POINT *)&rect, 2);

		switch(nmtb->hdr.idFrom)
		{
		case IDM_STATUSBAR_CURSORPOS:
			hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU3));
			hPopup = GetSubMenu(hMenu, 0);	

			MenuCheckMark(hPopup, IDM_STATUS_HEX, g_fStatusHexCursor);
			MenuCheckMark(hPopup, IDM_STATUS_DEC, !g_fStatusHexCursor);

			break;

		case IDM_STATUSBAR_VALUE:
			hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU2));
			hPopup = GetSubMenu(hMenu, 0);	
		
			MenuCheckMark(hPopup, IDM_VALUE_SIGNED, g_fStatusSignedValue);
			MenuCheckMark(hPopup, IDM_VALUE_ENDIAN, g_fStatusBigEndian);
			MenuCheckMark(hPopup, IDM_VALUE_HEX,    g_fStatusHexValue);
			CheckMenuRadioItem(hPopup, IDM_VALUE_BYTE, IDM_VALUE_DOUBLE, g_nStatusValueType, MF_BYCOMMAND);

			break;

		case IDM_STATUSBAR_SIZE:
			hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU3));
			hPopup = GetSubMenu(hMenu, 0);	

			MenuCheckMark(hPopup, IDM_STATUS_HEX, g_fStatusHexSize);
			MenuCheckMark(hPopup, IDM_STATUS_DEC, !g_fStatusHexSize);

			break;

		case IDM_STATUSBAR_MODE:
			hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU4));
			hPopup = GetSubMenu(hMenu, 0);	

			mode = HexView_GetEditMode(hwndHV);

			MenuCheckMark(hPopup, IDM_STATUSBAR_INSERT,    mode == HVMODE_INSERT);
			MenuCheckMark(hPopup, IDM_STATUSBAR_OVERWRITE, mode == HVMODE_OVERWRITE);
			MenuCheckMark(hPopup, IDM_STATUSBAR_READONLY,  mode == HVMODE_READONLY);
			
			break;
		}

		//CheckMenuItem(hMenu, IDM_

		
		cmdId = TrackPopupMenu(hPopup, TPM_RETURNCMD|TPM_RIGHTALIGN|TPM_BOTTOMALIGN, rect.right, rect.top, 0, g_hwndMain, 0);

		switch(cmdId)
		{
		case IDM_STATUSBAR_INSERT: 
			HexView_SetEditMode(hwndHV, HVMODE_INSERT);
			break;

		case IDM_STATUSBAR_OVERWRITE: 
			HexView_SetEditMode(hwndHV, HVMODE_OVERWRITE);
			break;

		case IDM_STATUSBAR_READONLY: 
			HexView_SetEditMode(hwndHV, HVMODE_READONLY);
			break;

		case IDM_STATUS_HEX:
			if(nmtb->hdr.idFrom == IDM_STATUSBAR_SIZE)
				g_fStatusHexSize = TRUE;
			else
				g_fStatusHexCursor = TRUE;
			break;

		case IDM_STATUS_DEC:
			if(nmtb->hdr.idFrom == IDM_STATUSBAR_SIZE)
				g_fStatusHexSize = FALSE;
			else
				g_fStatusHexCursor = FALSE;
			break;

		case IDM_VALUE_HEX:
			g_fStatusHexValue = !g_fStatusHexValue;
			break;

		case IDM_VALUE_ENDIAN:
			g_fStatusBigEndian = !g_fStatusBigEndian;
			break;

		case IDM_VALUE_SIGNED:
			g_fStatusSignedValue = !g_fStatusSignedValue;
			break;
			
		case IDM_VALUE_BYTE:  case IDM_VALUE_WORD:  case IDM_VALUE_DWORD: 
		case IDM_VALUE_QWORD: case IDM_VALUE_FLOAT: case IDM_VALUE_DOUBLE:
			g_nStatusValueType = cmdId;

			break;
		}

		DestroyMenu(hMenu);
			
		UpdateStatusBarText(mainWnd->hwndStatusBar, hwndHV);
			
		if(nmtb->hdr.idFrom == IDM_VALUE_SHOW)
		{
	
		}

	return 0;
}

BOOL OnFileChange(MAINWND *mainWnd)
{
	UpdateTypeView();
	return TRUE;
}

LRESULT HexEdit_OnNotify(MAINWND *mainWnd, HWND hwnd, UINT idCtrl, NMHDR *hdr)
{
	HWND hwndHV = GetActiveHexView(hwnd);

	if(hdr->hwndFrom == mainWnd->hwndTabView)
	{
		TCITEM tci = { TCIF_PARAM };
		
		TabCtrl_GetItem(mainWnd->hwndTabView, TabCtrl_GetCurSel(mainWnd->hwndTabView), &tci);

		// has the user clicked a file-tab?
		if(hdr->code == TCN_SELCHANGE)
		{
			HexSetCurFile(hwnd, TabCtrl_GetCurSel(mainWnd->hwndTabView), TRUE);

			OnFileChange(mainWnd);
			return 0;
		}
		else if(hdr->code == TCN_CLOSING)
		{
			// prompt close if
			if(HexFileCloseNotify(hwnd, hwndHV) == IDCANCEL)
				return TRUE;

			return 0;
		}
		else if(hdr->code == TCN_CLOSE)
		{
			// ask user if they want to save changes
			if(HexFileCloseNotify(hwnd, hwndHV) == IDCANCEL)
				return 0;

			//SetCurFile(hwnd, TabCtrl_GetCurSel(mainWnd->hwndTabView));
			//DestroyWindow((HWND)tci.lParam);
			HexCloseFile(mainWnd, TabCtrl_GetCurSel(mainWnd->hwndTabView));
			return 0;
		}
	}

	// double-click in a statusbar pane?
	if(hdr->hwndFrom == mainWnd->hwndStatusBar && hdr->code == NM_DBLCLK)
	{
		NMMOUSE *nmmouse;

		// statusbar is the only window at present which sends double-clicks
		nmmouse = (NMMOUSE *)hdr;

		// toggle the Readonly/Insert/Overwrite mode
		if(nmmouse->dwItemSpec == 4)
		{
			ToggleEditorMode(hwndHV);
			UpdateStatusBarText(mainWnd->hwndStatusBar, hwndHV);
		}

		return 0;
	}

	if(hdr->code == TBN_DROPDOWN)
	{
		if(GetParent(hdr->hwndFrom) == mainWnd->hwndStatusBar)
		{
			StatusBar_DropDownTB(mainWnd, hwndHV, (NMTOOLBAR *)hdr);
		}

		if(hdr->hwndFrom == mainWnd->hwndToolbar)
		{
			HexPasteSpecialDlg2(hwnd);
		}

		return 0;
	}

/*	if(hdr->code == DWN_ISDOCKABLE)
	{
		RECT rc1, rc2;
		
		// Get main window "outer" rectangle
		GetWindowRect(hwnd, &rc1);
		
		// Get main window "inner" rectangle
		GetClientRect(hwnd, &rc2);
		MapWindowPoints(hwnd, 0, (POINT *)&rc2, 2);
		InflateRect(&rc2, -2, -2);
		
		return DockWnd_GetDockSide(hwnd, (NMDOCKWNDQUERY *)hdr, &rc1, &rc2);
	}*/

	if(hdr->code == DWN_SAVESETTINGS)
	{
		NMDOCKWNDCREATE *nmdw = (NMDOCKWNDCREATE *)hdr;

		TRACEA("  DWN_SAVESETTINGS: %d\n", nmdw->uId);

		switch(nmdw->hdr.idFrom)
		{
		case DWID_TYPEVIEW: case DWID_ALLTYPES:
			SaveTypeView(nmdw->hwndClient, nmdw->hKey);
			break;
		}

		return 0;
	}

	if(hdr->code == DWN_UPDATE_CONTENT)
	{
		NMDOCKWND *nmdw = (NMDOCKWND *)hdr;

		HWND hwndHV = GetActiveHexView(hwnd);

		switch(nmdw->hdr.idFrom)
		{
		case DWID_HIGHLIGHT:
			UpdateHighlights(TRUE);
			break;
		}

		return 0;
	}

	if (hdr->code == FCN_FILECHANGE)
	{
		InitTypeLibrary();
		UpdateTypeView();
		return 0;
	}

	if(hdr->code == DWN_CREATE_CONTENT)
	{
		NMDOCKWNDCREATE *nmdw = (NMDOCKWNDCREATE *)hdr;

		TRACEA("DWN_CREATE_CONTENT: %d\n", hdr->idFrom);

		switch(nmdw->hdr.idFrom)
		{
		case DWID_TOOLBAR:
			mainWnd->hwndToolbar = InitToolbar(hdr->hwndFrom);
			return (LONG)mainWnd->hwndToolbar;

		case DWID_SEARCHBAR:
			return (LONG)CreateSearchBar(hdr->hwndFrom);

		case DWID_ALLTYPES:
			//SendMessage(hwndTB, TB_SETSTATE, IDM_TOOLS_TYPEVIEW, DockWnd_IsOpen(g_hwndMain, DWID_TYPEVIEW) ? TBSTATE_CHECKED|TBSTATE_ENABLED : TBSTATE_ENABLED);
			return (LONG)CreateTypeView(hdr->hwndFrom, nmdw->hKey, TRUE);//TEXT("struct ALL"));

		case DWID_TYPEVIEW:
			//SendMessage(hwndTB, TB_SETSTATE, IDM_TOOLS_TYPEVIEW, DockWnd_IsOpen(g_hwndMain, DWID_TYPEVIEW) ? TBSTATE_CHECKED|TBSTATE_ENABLED : TBSTATE_ENABLED);
			return (LONG)CreateTypeView(hdr->hwndFrom, nmdw->hKey, 0);

		case DWID_HIGHLIGHT:
			return (LONG)CreateHighlightView(hdr->hwndFrom);

		case DWID_STRINGS:
			return (LONG)CreateStringsView(hdr->hwndFrom);
		}
	}
	else if(hdr->code == DWN_DOCKCHANGE)
	{
		NMDOCKWND *nmdw = (NMDOCKWND *)hdr;

		switch(nmdw->hdr.idFrom)
		{
		case DWID_TOOLBAR:
			SendMessage(DockWnd_GetContents(mainWnd->hwndMain, DWID_TOOLBAR),
				TB_SETPARENT, (WPARAM)hdr->hwndFrom, 0);

			return 0;
		}
	}
	else if(hdr->code == DWN_CLOSING)
	{
		NMDOCKWND *nmdw = (NMDOCKWND *)hdr;

		switch(nmdw->hdr.idFrom)
		{
		case DWID_TYPEVIEW:
			break;
		}
		return 0;
	}
	
	return DefWindowProc(hwnd, WM_NOTIFY, idCtrl, (LONG)hdr);
}



HWND CreateHexViewCtrl(HWND hwndParent)
{
	HWND hwndHV = CreateHexView(hwndParent);

	HMENU hMenu   = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_HEXCONTEXT));
	DWORD dwStyle = HEXVIEW_DEFAULT_STYLE;// | HVS_FITTOWINDOW;

	// set the right-click context menu
	HexView_SetContextMenu(hwndHV, GetSubMenu(hMenu, 0));

	// set the default styles	
	HexView_SetStyle(hwndHV, -1, dwStyle);//|HVS_ASCII_INVISIBLE);//|HVS_FITTOWINDOW);//|HVS_ASCII_INVISIBLE );
	HexView_SetDataShift(hwndHV, 0);


	// set the font0
	SendMessage(hwndHV, WM_SETFONT, (WPARAM)g_hHexViewFont, 0);
	HexView_SetFontSpacing(hwndHV, 2, 0);
	return hwndHV;
}

//
//	Main Window message handler
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NMHDR *hdr;
	POINT pt;
	RECT  rect;
	HMENU hMenu;
	HWND hwndHV = GetActiveHexView(hwnd);//g_hwndHexView;
	int i;
	TCITEM tci;

	MAINWND *mainWnd = (MAINWND *)GetWindowLongPtr(hwnd, 0);

	switch(msg)
	{
	case WM_NCCREATE:
		
		if((mainWnd = malloc(sizeof(MAINWND))) == 0)
			return FALSE;
		
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)mainWnd);
		ZeroMemory(mainWnd, sizeof(MAINWND));
		return TRUE;

	case WM_NCDESTROY:
		free(mainWnd);
		return 0;

	case WM_CREATE:

		g_hwndMain = hwnd;

		SetWindowIcon(hwnd, IDI_APP);

		// create a child-window EDIT control
		//g_hwndHexView	= CreateHexViewCtrl(hwnd);
		g_hwndTabView	= CreateWindow(WC_TABVIEW, TEXT(""), WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd, 0, g_hInstance, 0);
		g_hwndStatusBar = CreateStatusBar(hwnd);

		SendMessage(g_hwndTabView, TCM_SETITEMSIZE, 0, MAKELPARAM(150, 0));

		SetStatusBarParts(g_hwndStatusBar);

		hwndHV = g_hwndHexView;

		mainWnd->hwndMain		= hwnd;
		mainWnd->hwndStatusBar	= g_hwndStatusBar;
		mainWnd->hwndTabView	= g_hwndTabView;

		CreateToolTip(g_hwndHexView);

//		g_hwndDock[0] = CreateDockWnd(&dock, hwnd, TEXT("Toolbar"));



		//g_hwndToolbar   = InitToolbar(hwnd);
		//g_hwndSearchBar = CreateSearchBar(hwnd);
		//g_hwndTypeView  = CreateTypeView(hwnd);


		SetFocus(hwndHV);


		// tell windows that we can handle drag+drop'd files
		DragAcceptFiles(hwnd, TRUE);

		UpdateRecentMenu(GetSubMenu(GetMenu(hwnd), 0));

		SetTimer(hwnd, 0xdeadbeef, 1000, 0);
		
		return TRUE;

	case WM_TIMER:
		if(wParam == 0xdeadbeef)
		{
			KillTimer(hwnd, wParam);
			//FirstTimeOptions(hwnd);
		}
		return 0;

	case WM_DROPFILES:

		// get the screen coordinates of the drop-location
		if(DragQueryPoint((HDROP)wParam, &pt))
			ClientToScreen(hwnd, &pt);
		
		GetWindowRect(hwndHV, &rect);
		
		// drop anywhere *except* the hexview, as that does D&D itself
		if(!PtInRect(&rect, pt))
		{
			HandleDropFiles(hwnd, (HDROP)wParam);
		}

		//CreateToolTip(mainWnd->hwndTabView);
		return 0;

	case WM_ENABLE:
		EnableWindow(g_hwndSearch, (BOOL)wParam);
		EnableWindow(g_hwndGoto, (BOOL)wParam);
		return 0;

	case WM_CONTEXTMENU:
		if((HWND)wParam == DockWnd_GetWindow(hwnd, DWID_TYPEVIEW))
		{
			HMENU hMenu = GetSubMenu(LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_TYPECONTEXT)), 0);
			UINT  u;

			MenuCheckMark(hMenu, IDM_TYPEVIEW_HEX, g_fDisplayHex);
			MenuCheckMark(hMenu, IDM_TYPEVIEW_BIGENDIAN, g_fDisplayBigEndian);
			u = TrackPopupMenu(hMenu, TPM_RETURNCMD, (short)LOWORD(lParam), (short)HIWORD(lParam), 0, hwnd, 0);

			SendMessage(DockWnd_GetContents(hwnd, DWID_TYPEVIEW), WM_COMMAND, u, 0);
		}

		break;

	case WM_COMMAND:
		return HexEdit_OnCommand(hwnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

	case WM_NOTIFY:		
		hdr = (NMHDR *)lParam;
		if(hdr->hwndFrom == hwndHV)
			return HexViewNotifyHandler(mainWnd, hwnd, hdr);
		else
			return HexEdit_OnNotify(mainWnd, hwnd, (UINT)wParam, (NMHDR *)lParam);

	case WM_CLOSE:

		tci.mask = TCIF_PARAM;

		for(i = 0; (hwndHV = EnumHexView(hwnd, i)) != NULL; )
		{
			UINT uAnswer = HexFileCloseNotify(hwnd, hwndHV);
			
			if(uAnswer == IDCANCEL)
			{
				return 0;
			}
			else if(uAnswer == IDNO)
			{
				SaveHighlights(hwndHV);
				TabCtrl_DeleteItem(mainWnd->hwndTabView, i);
			}
			else
			{
				i++;
			}
		}

		// save settings *before* we destroy anything!
		DockWnd_SaveSettings(hwnd);

		// shut program down
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		DestroyWindow(hwndHV);

		// 
		PostQuitMessage(0);
		return 0;

	case WM_SETFOCUS:
		SetFocus(hwndHV);
		return 0;

	case WM_SIZE:

		MainWndSize(mainWnd, LOWORD(lParam), HIWORD(lParam));
		UpdateStatusbar(mainWnd->hwndStatusBar);

		return 0;

	case WM_INITMENUPOPUP:
		hMenu = (HMENU)wParam;//GetMenu(hwnd);
		
		MenuCheckMark(hMenu, IDM_VIEW_FITTOWINDOW,  g_fFitToWindow);
		MenuCheckMark(hMenu, IDM_VIEW_TOOLBAR,		DockWnd_IsOpen(hwnd, DWID_TOOLBAR));
		MenuCheckMark(hMenu, IDM_TOOLS_TYPEVIEW,	DockWnd_IsOpen(hwnd, DWID_TYPEVIEW));
		MenuCheckMark(hMenu, IDM_TOOLS_SEARCHBAR,	DockWnd_IsOpen(hwnd, DWID_SEARCHBAR));

		CheckMenuRadioItem(hMenu, IDM_VIEW_HEX, IDM_VIEW_BIN, 
			IDM_VIEW_HEX + (HexView_GetStyle(hwndHV) & HVS_FORMAT_MASK),
			MF_BYCOMMAND);

		{int look[32] = { 0, 0, 1, 0, 2 };
		CheckMenuRadioItem(hMenu, IDM_GROUP_BYTE, IDM_GROUP_DWORD, 
			IDM_GROUP_BYTE + look[HexView_GetGrouping(hwndHV)],
			MF_BYCOMMAND);
		}

		{
		size_w selsize; 
		UINT   edmode  = HexView_GetEditMode(hwndHV);
		BOOL   cftext  = IsClipboardFormatAvailable(CF_TEXT);
		BOOL   canundo = HexView_CanUndo(hwndHV);
		BOOL   canredo = HexView_CanRedo(hwndHV);

		HexView_GetSelSize(hwndHV, &selsize);

		//hMenu = GetSubMenu(GetMenu(hwnd), 1);

		EnableMenuCmdItem(hMenu, IDM_EDIT_UNDO,  canundo);
		EnableMenuCmdItem(hMenu, IDM_EDIT_REDO,  canredo);
		EnableMenuCmdItem(hMenu, IDM_EDIT_CUT,  selsize > 0 && edmode == HVMODE_INSERT);
		EnableMenuCmdItem(hMenu, IDM_EDIT_COPY, selsize > 0);
		EnableMenuCmdItem(hMenu, IDM_EDIT_COPYAS, selsize > 0);
		EnableMenuCmdItem(hMenu, IDM_EDIT_PASTE, cftext && edmode != HVMODE_READONLY  );
		EnableMenuCmdItem(hMenu, IDM_EDIT_PASTESPECIAL, edmode != HVMODE_READONLY  );
		EnableMenuCmdItem(hMenu, IDM_EDIT_DELETE, selsize > 0 && edmode != HVMODE_READONLY );

		EnableMenuCmdItem(hMenu, IDM_EDIT_REVERSE, selsize > 0 && edmode != HVMODE_READONLY );
		EnableMenuCmdItem(hMenu, IDM_TOOLS_TRANSFORM, selsize > 0 && edmode != HVMODE_READONLY );

		EnableMenuCmdItem(hMenu, IDM_FILE_REVERT, canundo || canredo);
		}

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitMainWnd()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.lpfnWndProc   = WndProc;
	wc.lpszClassName = APPNAME;
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
	wc.hInstance     = g_hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wc.cbWndExtra	 = sizeof(MAINWND*);

	RegisterClassEx(&wc);
}

void CreateMainWnd()
{
	CreateWindowEx(0, APPNAME, APPNAME, 
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 
		CW_USEDEFAULT, CW_USEDEFAULT, 1050, 300, 0,0,
		g_hInstance, 0);
}

TCHAR **GetArgvCommandLine(int *argc)
{
#ifdef UNICODE
	return CommandLineToArgvW(GetCommandLineW(), argc);
#else
	*argc = __argc;
	return __argv;
#endif
}

TCHAR * GetArg(TCHAR *ptr, TCHAR *buf, int len)
{
	int i  = 0;
	int ch;

	// make sure there's something to parse
	if(ptr == 0 || *ptr == '\0')
	{
		*buf = '\0';
		return 0;
	}

	ch = *ptr++;

	// skip leading whitespace
	while(ch == ' ' || ch == '\t')
		ch = *ptr++;

	// quoted filenames
	if(ch == '\"')
	{
		ch = *ptr++;
		while(i < len - 1 && ch && ch != '\"')
		{
			buf[i++] = ch;
			if(ch = *ptr) *ptr++;
		}
	}
	// grab a token
	else
	{
		while(i < len - 1 && ch && ch != ' ' && ch != '\t')
		{
			buf[i++] = ch;
			if(ch = *ptr) *ptr++;
		}
	}

	buf[i] = '\0';

	// skip trailing whitespace
	while(*ptr == ' ' || *ptr == '\t')
		ptr++;

	return ptr;
}

//
//	http://randomascii.wordpress.com/2012/07/05/when-even-crashing-doesnt-work/
//
void EnableCrashingOnCrashes() 
{ 
    typedef BOOL (WINAPI *tGetPolicy)(LPDWORD lpFlags); 
    typedef BOOL (WINAPI *tSetPolicy)(DWORD dwFlags); 
    const DWORD EXCEPTION_SWALLOWING = 0x1;

    HMODULE kernel32 = LoadLibraryA("kernel32.dll"); 

    tGetPolicy pGetPolicy = (tGetPolicy)GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy"); 
    tSetPolicy pSetPolicy = (tSetPolicy)GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy"); 
    
	if (pGetPolicy && pSetPolicy) 
    { 
        DWORD dwFlags; 
        if (pGetPolicy(&dwFlags)) 
        { 
            // Turn off the filter 
            pSetPolicy(dwFlags & ~EXCEPTION_SWALLOWING); 
        } 
    } 
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd)
{
	MSG		msg;
	HACCEL	hAccel;
	TCHAR	arg[MAX_PATH];
	TCHAR *	pszCmdline;
	//char buf[] = "hello world";

	EnableCrashingOnCrashes();

	//INITCOMMONCONTROLSEX icx = { sizeof(icx), ICC_
	g_hInstance = hInst;

	// This program requires COM
	OleInitialize(0);

	//
	//	Commandline processing
	//
	
	pszCmdline = GetArg(GetCommandLineW(), arg, MAX_PATH);

	if(pszCmdline && *pszCmdline == '-')
	{
		pszCmdline = GetArg(pszCmdline, arg, MAX_PATH);

		// 'touch' the specified file!
		if(lstrcmpi(arg, TEXT("-touch")) == 0)
		{
			// check to see if it's a quoted filename
			if(*pszCmdline == '\"')
			{
				GetArg(pszCmdline, arg, MAX_PATH);
				pszCmdline = arg;
			}

			if(!TouchFile(pszCmdline))
			{
				HexWinErrorBox(GetLastError(), 0);
				return 1;
			}
			return 0;
		}
		else
		{
			pszCmdline = GetArg(pszCmdline, arg, MAX_PATH);
		}
	}


	{
		INITCOMMONCONTROLSEX icex = { sizeof(icex), -1 };
		InitCommonControlsEx(&icex);
	}
	
	/*g_Config = OpenConfig(TEXT("mapsize.exe.129082349834.bookmarks"));
	smeg(g_Config, TEXT("bookmarks.bookmark."), TEXT("mapsize.exe"));
	SaveConfig(TEXT("oof.config"), g_Config);*/
	LoadSettings();

	RegisterTabView();
	InitMainWnd();
	CreateMainWnd();

	// open file specified on commmand line?
	if(pszCmdline && *pszCmdline)
	{
		// check to see if it's a quoted filename
		if(*pszCmdline == '\"')
		{
			GetArg(pszCmdline, arg, MAX_PATH);
			pszCmdline = arg;
		}

		if(!HexeditOpenFile(g_hwndMain, pszCmdline, DefaultFileMode()))//HVOF_DEFAULT))
		{
			SendMessage(g_hwndMain, WM_COMMAND, IDM_FILE_NEW, 0);
		}
	}
	// automatically create new document if none specified
	else
	{
		SendMessage(g_hwndMain, WM_COMMAND, IDM_FILE_NEW, 0);
	}

	InitDockingBars(g_hwndMain);		

	if(g_fRestoreWinPos)
		nShowCmd = SW_SHOW;

	ShowWindow(g_hwndMain, nShowCmd);
	
	// force any floating popups (i.e. the DOCKWNDs) to 
	// become visible at the same time
	//ShowOwnedPopups(g_hwndMain, TRUE);
	DockWnd_ShowDeferredPopups(g_hwndMain);

#ifdef _DEBUG
//	HexView_InitBufShared(g_hwndHexView, hexData, sizeof(hexData));
#endif


	//
	// load keyboard accelerator table
	//
	hAccel = LoadAccelerators(GetModuleHandle(0)/*g_hResourceModule*/, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	//HexView_ScrollTop(g_hwndHexView, 9);

	//
	// message-pump
	//
	while(GetMessage(&msg, 0, 0, 0) > 0)
	{
		if(!TranslateAccelerator(g_hwndMain, hAccel, &msg))
		{
			if( !IsDialogMessage(g_hwndSearch, &msg) && 
				!IsDialogMessage(g_hwndGoto, &msg)   && 
				!DockWnd_IsDialogMessage(g_hwndMain, DWID_TYPEVIEW, &msg) &&
				!DockWnd_IsDialogMessage(g_hwndMain, DWID_SEARCHBAR, &msg) &&
				!DockWnd_IsDialogMessage(g_hwndMain, DWID_HIGHLIGHT, &msg)
			  )
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	SaveSettings();

	// Shutdown COM
	OleUninitialize();

	return 0;
}
