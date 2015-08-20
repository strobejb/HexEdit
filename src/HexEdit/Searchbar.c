//
//  Searchbar.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include "trace.h"
#include "HexEdit.h"
#include "HexUtils.h"
#include "resource.h"
#include "..\HexView\HexView.h"

#include "ToolPanel.h"

//HWND SearchBarProc

HWND CreateToolbar(HWND hwndParent,DWORD);
void DrawGripper(HDC hdc, int x, int y, int height, int numgrips);
HDWP StretchWindowToRight(HWND hwndCtrl, HWND hwndParent, int margin, HDWP hdwp);


static DWORD TOOLBAR_STYLE =WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT |
						 WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | 
						CCS_NOPARENTALIGN | 
						// CCS_NORESIZE|
						 CCS_NODIVIDER;
						//|	CCS_ADJUSTABLE ;
						//| TBSTYLE_ALTDRAG| CCS_ADJUSTABLE;


/*void AddBlankSpace(HWND hwndTB, int width, RECT *rect)
{
	TBBUTTON tbb = { 0, 0, 0, TBSTYLE_SEP, 0, 0,0, 0 };

	RECT item;
	SetRect(rect, 0, 0, 0, 0);

	int i = SendMessage(hwndTB, TB_BUTTONCOUNT, 0, 0);

	//for(int j = 0; j < 11; j++)
	for(; rect->right - rect->left < width; i++)
	{
		SendMessage(hwndTB, TB_ADDBUTTONS,  1, (LPARAM) &tbb);
		SendMessage(hwndTB, TB_GETITEMRECT, i, (LPARAM) &item);

		UnionRect(rect, rect, &item);
	}	
}*/

void AddBlankSpace(HWND hwndTB, int width)
{
	TBBUTTON tbb = { width, 0, TBSTATE_ENABLED, TBSTYLE_SEP, {0},0, 0 };
	SendMessage(hwndTB, TB_ADDBUTTONS,  1, (LPARAM) &tbb);
}


void AddButton(HWND hwndTB, UINT uCmdId, UINT uImageIdx, UINT uStyle, TCHAR *szText)
{
	TBBUTTON tbb = 
	{ 
		uImageIdx, 
		uCmdId, 
		TBSTATE_ENABLED, 
		uStyle| BTNS_SHOWTEXT, 
		{0}, 0, 
		(INT_PTR)szText 
	};
	
	//SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)"Hello\0");
	SendMessage(hwndTB, TB_ADDBUTTONS, 1, (LPARAM) &tbb);
}

int GetToolbarRect(HWND hwndTB, RECT *rect)
{
	int  i;
	int  count = (int)SendMessage(hwndTB, TB_BUTTONCOUNT, 0, 0);
	RECT tmp;

	SetRect(rect, 0, 0, 0, 0);
	
	for(i = 0; i < count; i++)
	{
		SendMessage(hwndTB, TB_GETITEMRECT, i, (LPARAM)&tmp);
		UnionRect(rect, rect, &tmp);
	}

	return count;
}

int ToolbarWidth(HWND hwndTB)
{
	RECT rect;
	GetToolbarRect(hwndTB, &rect);
	return rect.right-rect.left;
}

int ToolbarHeight(HWND hwndTB)
{
	RECT rect;
	GetToolbarRect(hwndTB, &rect);
	return rect.bottom-rect.top;
}

HWND CreateEmptyToolbar(HWND hwndParent, int nBitmapIdx, int nBitmapWidth, int nCtrlId, DWORD dwExtraStyle)
{
	HWND	   hwndTB;
	HIMAGELIST hImgList;
	
	hwndTB = CreateToolbarEx (hwndParent,
			TOOLBAR_STYLE|dwExtraStyle,
			nCtrlId, 0,
			0,
			0,
			NULL,
			0,
			0, 0, 0, 0,
			sizeof(TBBUTTON) );

	//hImgList = ImageList_LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(nBitmapIdx), 
	//								nBitmapWidth, 16, RGB(255,0,255));
	hImgList = ImageList_LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(nBitmapIdx), 
									nBitmapWidth, 16, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

	SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)hImgList);

	return hwndTB;

	return CreateWindowEx(0, TOOLBARCLASSNAME,0,WS_VISIBLE|WS_CHILD|TBSTYLE_FLAT|
		TBSTYLE_TRANSPARENT |CCS_NORESIZE|CCS_NODIVIDER,
		0,0,0,0,hwndParent,0,0,0);

  //SendMessage(hToolBar, TB_SETSTYLE, 0, SendMessage(hToolBar, TB_GETSTYLE, 0,0 )
//& ~ TBSTYLE_TRANSPARENT);
}

HWND CreateChild(DWORD dwStyleEx, DWORD dwStyle, TCHAR *szClass, HWND hwndParent, UINT nCommandId)
{
	static HFONT hFont;
	
	//GetStockObject(DEFAULT_GUI_FONT)
	HWND hwnd;
	
	hwnd = CreateWindowEx(dwStyleEx, szClass, 0, dwStyle, 0, 0, 0,0, hwndParent, (HMENU)(UINT_PTR)nCommandId, 0, 0);

	if(hFont == 0)
		hFont = CreateFont(-16,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0, TEXT("Segoe UI"));
	
	SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0);
	return hwnd;
}

int ResizeToolbar(HWND hwndTB)
{
	RECT rect;
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
	GetToolbarRect(hwndTB, &rect);
	SetWindowSize(hwndTB, rect.right-rect.left, rect.bottom-rect.top, NULL);

	return rect.bottom-rect.top;
}


typedef PVOID (__fastcall * VALIDATEHWND)(HWND hwnd);

PVOID ValidateHwnd(HWND hwnd)
{
    DWORD_PTR ptr  =  (DWORD_PTR  )memchr(GetWindowRect, 0xE8, 100);
    DWORD_PTR addr = *(DWORD_PTR *)(ptr+1) + ptr + 5;

    VALIDATEHWND _ValidateHwnd = (VALIDATEHWND)addr;
	//PVOID (__fastcall * _ValidateHwnd)(HWND hwnd) = ((PVOID)(__fastcall *)(HWND))addr;

    return _ValidateHwnd(hwnd);
}

LRESULT CALLBACK oof(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NMHDR *hdr = (NMHDR *)lParam;

	switch(msg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case 6666:
			if(HIWORD(wParam) == CBN_EDITCHANGE)
			{
				TCHAR wstr[64];
				BYTE buf[64];
				int len;
				GetWindowText((HWND)lParam, wstr, 64);
				len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, (char *)buf, 64, 0, 0);
				
				HexView_SetSearchPattern(g_hwndHexView, buf, len - 1);
				InvalidateRect(g_hwndHexView, 0, 0);
				return 0;
			}
			return 0;
		}
		return 0;

	case WM_NOTIFY:
		if(hdr->code == TBN_DROPDOWN)
		{
			RECT rect;
			HMENU hMenu;
			int cmd;
			TCHAR buf[20];
			TBBUTTONINFO tbbi = { sizeof(tbbi) };
			
			hMenu = LoadMenu(0, MAKEINTRESOURCE(IDR_SEARCHBAR_FINDTYPE));
			hMenu = GetSubMenu(hMenu, 0);
			SendMessage(hdr->hwndFrom, TB_GETITEMRECT, 0, (LPARAM)&rect);
			MapWindowPoints(hdr->hwndFrom, 0, (POINT *)&rect, 2);

			cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY, rect.left, rect.bottom, 0, hwnd, 0);

			if(cmd != 0)
			{
				GetMenuString(hMenu, cmd, buf, 20, MF_BYCOMMAND);
				tbbi.dwMask = TBIF_COMMAND|TBIF_TEXT;
				tbbi.idCommand = IDM_FILE_OPEN;
				tbbi.pszText   = buf;
				SendMessage(hdr->hwndFrom, TB_SETBUTTONINFO, IDM_FILE_OPEN, (LPARAM)&tbbi);
			}

			return TBDDRET_DEFAULT;
		}
		break;
	}
	
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND CreateSearchBar(HWND hwndParent)
{
	HWND hwndPanel;
	HWND hwndTB1;
	HWND hwndTB2;
	HWND hwndTB3;
	HWND hwndTB4;
	HWND hwndCombo1;
	HWND hwndCombo2;

	int tbheight;

	HIMAGELIST hImgList;
	COMBOBOXEXITEM cbxi = { CBEIF_IMAGE|CBEIF_SELECTEDIMAGE|CBEIF_TEXT };

	//
	//	Create the base tool panel
	//
	hwndPanel = ToolPanel_Create(hwndParent, oof);
	
//ToolPanel_AddGripper(hwndPanel);

	//
	//	Create the 3rd toolbar (search buttons)
	//
	hwndTB3   = CreateEmptyToolbar(hwndPanel, IDB_BITMAP3, 15, 668, TBSTYLE_LIST|TBSTYLE_TRANSPARENT);
	SendMessage(hwndTB3, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS );
	SendMessage(hwndTB3, TB_SETBUTTONSIZE, 0, MAKELPARAM(32, 42));
	//AddButton(hwndTB3, IDM_FILE_OPEN, 8,   TBSTYLE_BUTTON|TBSTYLE_DROPDOWN , TEXT("Ascii"));
	AddButton(hwndTB3, IDM_FILE_NEW, 5,  TBSTYLE_BUTTON, TEXT("Prev"));	
	AddButton(hwndTB3, IDM_FILE_SAVE, 2,   TBSTYLE_BUTTON , TEXT("Next"));// »"));
	ResizeToolbar(hwndTB3);

	ToolPanel_AddItem(hwndPanel, hwndTB3, 0);

	//
	//	Create the search combobox
	//
	hwndCombo2 = CreateChild(0, WS_TABSTOP|WS_CHILD|WS_VISIBLE|CBS_DROPDOWN, TEXT("ComboBoxEx32"), hwndPanel, 6666);
	
	// meh
	hImgList = ImageList_LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDB_BITMAP7), 16, 10, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
	//SendMessage(hwndCombo2, CBEM_SETIMAGELIST, 0, (LPARAM)hImgList);

	cbxi.iImage = 8;
	cbxi.iSelectedImage = 8;
	cbxi.iItem = 0;
	cbxi.pszText = TEXT("Hello World");
	cbxi.cchTextMax = 10;

	//iSelectedImage
	//SendMessage(hwndCombo2, CBEM_SETITEM, 0, (LPARAM)&cbxi);
	//SendMessage(hwndCombo2, CBEM_INSERTITEM, 0, (LPARAM)&cbxi);



	SetWindowSize(hwndCombo2, 200, 200, NULL);
	ToolPanel_AddItem(hwndPanel, hwndCombo2, 0);

	//
	//	Create the 'type' button
	//
	hwndTB4 = CreateEmptyToolbar(hwndPanel, IDB_BITMAP4, 16, 668, TBSTYLE_LIST);
	SendMessage(hwndTB4, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS );
	AddButton(hwndTB4, IDM_FILE_OPEN, 8,   TBSTYLE_BUTTON|TBSTYLE_DROPDOWN , TEXT("Ascii"));
	tbheight = ResizeToolbar(hwndTB4);
	ToolPanel_AddItem(hwndPanel, hwndTB4, 0);



	//
	//	Create the 1st toolbar (the "Goto" button)
	//
	ToolPanel_AddGripper(hwndPanel);

	hwndTB1   = CreateEmptyToolbar(hwndPanel, IDB_BITMAP3, 15, 666, TBSTYLE_LIST);
	SendMessage(hwndTB1, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS);

	GetWindowWidth(hwndTB1);
	AddButton(hwndTB1, -0, 3, TBSTYLE_BUTTON, TEXT("Goto »"));
	GetWindowWidth(hwndTB1);
	ResizeToolbar(hwndTB1);
	GetWindowWidth(hwndTB1);
	ToolPanel_AddItem(hwndPanel, hwndTB1, 0);

	//
	//	Create the goto-combo box
	//

	//hwndCombo1 = CreateChild(0, WS_CHILD|WS_VISIBLE|CBS_DROPDOWN, "COMBOBOX", hwndPanel);
	hwndCombo1 = CreateChild(0, WS_TABSTOP|WS_CHILD|WS_VISIBLE|CBS_DROPDOWN, TEXT("ComboBoxEx32"), hwndPanel, 0);
	SetWindowSize(hwndCombo1, 100, 200, NULL);
	ToolPanel_AddItem(hwndPanel, hwndCombo1, 0);

	//
	//	Create the 2nd toolbar (bookmark buttons)
	//
	hwndTB2   = CreateEmptyToolbar(hwndPanel, IDB_BITMAP2, 15, 667, TBSTYLE_LIST);
	SendMessage(hwndTB2, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS);
	//SendMessage(hwndTB1, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
	SendMessage(hwndTB2, TB_SETBUTTONSIZE, 0, MAKELPARAM(32, 32));
	AddButton(hwndTB2, IDM_SEARCH_REPLACE, 7, TBSTYLE_DROPDOWN|TBSTYLE_BUTTON|TBSTYLE_CHECK, 0);//TEXT("Highlight"));
	ResizeToolbar(hwndTB2);
	AddButton(hwndTB2, 0, 1, TBSTYLE_BUTTON, TEXT("Bookmark"));
	

	ToolPanel_AddItem(hwndPanel, hwndTB2, 0);
	ToolPanel_AddNewLine(hwndPanel, 0);

	ToolPanel_AutoSize(hwndPanel);

	ShowWindow(hwndPanel, SW_SHOW);

	return hwndPanel;
}
