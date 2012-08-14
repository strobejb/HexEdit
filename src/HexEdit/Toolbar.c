//
//  Toolbar.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "HexEdit.h"
#include "..\DockLib\DockLib.h"
#include "resource.h"
#include "..\HexView\HexView.h"

//
//	Compare / NextDiff / PrevDiff
//	32bit/16bit/8bit   Hex/Dec/Oct/Bin
//
//	AutoFit
//	HighlighterV
//
//

typedef struct
{
	UINT	nIcon;
	UINT	nCommand;
	TCHAR  *text;
} TOOLBAR_BUTTON;

TCHAR szToolbarText[] = TEXT("New\0Open\0Save\0Cut\0Copy\0Paste\0Undo\0Redo\0Find\0Types\0Marks\0Spawn");

TOOLBAR_BUTTON mainToolbar[] = 
{
	{ STD_FILENEW,	IDM_FILE_NEW,			TEXT("New") },
	{ STD_FILEOPEN,	IDM_FILE_OPEN,			TEXT("Open") },		
	{ STD_FILESAVE,	IDM_FILE_SAVE,			TEXT("Save") },		
	{ STD_CUT,		IDM_EDIT_CUT,			TEXT("Cut") },		
	{ STD_COPY,		IDM_EDIT_COPY,			TEXT("Copy") },		
	{ STD_PASTE,	IDM_EDIT_PASTE,			TEXT("Paste") }, 
	{ STD_UNDO,		IDM_EDIT_UNDO,			TEXT("Undo") },
	{ STD_REDOW,	IDM_EDIT_REDO,			TEXT("Redo") },
	{ STD_FIND,		IDM_SEARCH_FIND,		TEXT("Find") },

};

TBBUTTON tbb[] = 
{
	{	0/*STD_FILENEW*/,	IDM_FILE_NEW,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,  {0},0, 0 },
	{	1/*STD_FILEOPEN*/,	IDM_FILE_OPEN,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,  {0},0, 1 },
	{	2/*STD_FILESAVE*/,	IDM_FILE_SAVE,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,  {0},0, 2 }, 
	{	0,				0,						TBSTATE_ENABLED,	TBSTYLE_SEP,	  {0},0, 0 },
	{	3/*STD_CUT*/,		IDM_EDIT_CUT,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,  {0},0, 3 },
	{	4/*STD_COPY*/,		IDM_EDIT_COPY,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,  {0},0, 4 },
	{	5/*STD_PASTE*/,		IDM_EDIT_PASTE,		TBSTATE_ENABLED,	TBSTYLE_BUTTON/*|TBSTYLE_DROPDOWN*/,  {0},0, 5 },
	{	0,				0,						TBSTATE_ENABLED,	TBSTYLE_SEP,	  {0},0, 0 },
	{	6/*STD_UNDO*/,		IDM_EDIT_UNDO,		TBSTATE_ENABLED, TBSTYLE_BUTTON,  {0},0, 6 },
	{	7/*STD_REDOW*/,		IDM_EDIT_REDO,		TBSTATE_ENABLED, TBSTYLE_BUTTON,  {0},0, 7 },
	{	8/*STD_FIND*/,		IDM_SEARCH_FIND,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,  {0},0, 8 },
	{	0,				0,						TBSTATE_ENABLED,	TBSTYLE_SEP,	  {0},0, 0 },
	{	10/*STD_FIND*/,		IDM_TOOLS_TYPEVIEW,	TBSTATE_ENABLED,	TBSTYLE_CHECK,  {0},0, 9 },
	{	11/*STD_FIND*/,		IDM_HIGHLIGHT_MANAGE,	TBSTATE_ENABLED, TBSTYLE_CHECK,  {0},0, 10 },

};

DWORD TOOLBAR_STYLE =WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT |
						 WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | 
						CCS_NOPARENTALIGN | 
						 CCS_NORESIZE|
						 CCS_NODIVIDER;
						//|	CCS_ADJUSTABLE ;
						//| TBSTYLE_ALTDRAG| CCS_ADJUSTABLE;


#define Toolbar_EnableButton(hwndTB, idButton, fEnable) \
	SendMessage(hwndTB, TB_ENABLEBUTTON, (WPARAM)(idButton), MAKELPARAM((BOOL)(fEnable), 0))

void UpdateToolbarState(HWND hwndTB, HWND hwndHV)
{
	// save button
	hwndTB = DockWnd_GetContents(g_hwndMain, DWID_TOOLBAR);

	Toolbar_EnableButton(hwndTB, IDM_FILE_SAVE, HexView_CanUndo(hwndHV));
	Toolbar_EnableButton(hwndTB, IDM_EDIT_UNDO, HexView_CanUndo(hwndHV));
	Toolbar_EnableButton(hwndTB, IDM_EDIT_REDO, HexView_CanRedo(hwndHV));
	Toolbar_EnableButton(hwndTB, IDM_EDIT_COPY, HexView_GetSelSize(hwndHV, 0));
	
	Toolbar_EnableButton(hwndTB, IDM_EDIT_CUT,  HexView_GetSelSize(hwndHV, 0) && 
		HexView_GetEditMode(hwndHV) == HVMODE_INSERT);
	
	Toolbar_EnableButton(hwndTB, IDM_EDIT_PASTE, IsClipboardFormatAvailable(CF_TEXT) 
												&& HexView_GetEditMode(hwndHV) != HVMODE_READONLY);
}

HWND CreateToolbar(HWND hwndParent, DWORD dwExtraStyle)
{
	return CreateToolbarEx (hwndParent,
			TOOLBAR_STYLE|dwExtraStyle,
			1, 0,
			HINST_COMMCTRL,
			IDB_STD_SMALL_COLOR,
			NULL,
			0,
			55, 36, 36, 38,
			sizeof(TBBUTTON) );

	return CreateWindowEx(0, TOOLBARCLASSNAME, 0, WS_VISIBLE|WS_CHILD|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT |
		CCS_NORESIZE|CCS_NODIVIDER, 0,0,0,0,hwndParent, 0,0,0);
}

HWND CreateEmptyToolbar(HWND hwndParent, int nBitmapIdx, int nBitmapWidth, int id, DWORD dwExtraStyle);
int ResizeToolbar(HWND hwndTB);

HWND InitToolbar(HWND hwndParent)
{
	HWND hwndTB;
	DWORD sz;
	int x;

	hwndTB = CreateEmptyToolbar(hwndParent, IDB_BITMAP5, 16, 2, 0);

	SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);// |TBSTYLE_EX_MIXEDBUTTONS);
	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(hwndTB, TB_SETPADDING, 0, MAKELPARAM(3, 0));

//	SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS );

//	hImgList = ImageList_LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP5), 16, 0, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	//hImgList = ImageList_LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP5), 16, 0, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

	//SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)hImgList);
	SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)szToolbarText);


//	for(int j = 0; j < 11; j++)
	{
		//if(tbb[j].iString == butstr[i])
		//{
	/*	TBBUTTON tbb = 
		{ 
			mainToolbar[j].nIcon, 
			mainToolbar[j].nCommand,
			TBSTATE_ENABLED,
			TBSTYLE_LIST|TBSTYLE_FLAT|TBSTYLE_BUTTON,
			
			0, 0, 0, j
		};*/

		TBBUTTONINFO tbi = { sizeof(TBBUTTONINFO) };

		tbi.dwMask =//TBIF_BYINDEX|
			TBIF_TEXT|TBIF_SIZE|TBIF_COMMAND|TBIF_STYLE;
		tbi.pszText		= TEXT("TypeView");
		tbi.idCommand	= IDM_TOOLS_TYPEVIEW;
		tbi.fsStyle		= BTNS_SHOWTEXT|TBSTYLE_CHECK;
		tbi.cx			= 84;

		//SendMessage(hwndTB, TB_ADDBUTTONS, 1, (LPARAM) &tbb);
		SendMessage(hwndTB, TB_ADDBUTTONS, sizeof(tbb)/sizeof(tbb[0]), (LPARAM) &tbb);
		SendMessage(hwndTB, TB_SETBUTTONINFO, IDM_TOOLS_TYPEVIEW, (LPARAM)&tbi);

		tbi.pszText		= TEXT("BookMarks");
		tbi.idCommand	= IDM_HIGHLIGHT_MANAGE;
		SendMessage(hwndTB, TB_SETBUTTONINFO, IDM_HIGHLIGHT_MANAGE, (LPARAM)&tbi);
		//	break;
		//}
	}

	sz = (DWORD)SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);
	x = max(LOWORD(sz), HIWORD(sz)) + 10;
	SendMessage(hwndTB, TB_SETBUTTONSIZE, 0, MAKELPARAM(x+2,x-2));
	 sz = (DWORD)SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);

	ResizeToolbar(hwndTB);

	// initialize initial toolbar button state
	//SendMessage(hwndTB, TB_SETSTATE, IDM_TOOLS_TYPEVIEW, DockWnd_IsVisible(g_hWndMain, DWID_TYPEVIEW) ? TBSTATE_CHECKED|TBSTATE_ENABLED : TBSTATE_ENABLED);
	//SendMessage(hwndTB, TB_SETSTATE, IDM_TOOLS_TYPEVIEW, DockWnd_IsVisible(hwnd, DWID_TYPEVIEW) ? TBSTATE_CHECKED|TBSTATE_ENABLED : TBSTATE_ENABLED);
	//SendMessage(hwndTB, TB_SETSTATE, IDM_TOOLS_TYPEVIEW, DockWnd_IsVisible(hwnd, DWID_TYPEVIEW) ? TBSTATE_CHECKED|TBSTATE_ENABLED : TBSTATE_ENABLED);


	return hwndTB;
}