//
//  Statusbar.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define _WIN32_WINNT 0x501
#include <windows.h>
#include <tchar.h>
#include <trace.h>
#include <commctrl.h>
#include "HexEdit.h"
#include "HexUtils.h"
#include "..\HexView\HexView.h"
#include "resource.h"

#include "..\TypeLib\parser.h"

DWORD dwStatusBarStyles = WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | CCS_NOPARENTALIGN | 
						CCS_NOMOVEY | SBT_NOBORDERS  | SBT_NOBORDERS |
						WS_CLIPCHILDREN | WS_CLIPSIBLINGS | /*CCS_BOTTOM | */SBARS_SIZEGRIP
						;//| CCS_NORESIZE;

extern HINSTANCE hInstance;

#define MaxStatusParts 5
BOOL g_fStatusValueFromSelection = TRUE;

TCHAR		*g_szEditMode[] = { TEXT("READ"), TEXT("INS"), TEXT("OVR") };

static HWND hwndStatusTBList[5];

void AddButton(HWND hwndTB, UINT uCmdId, UINT uImageIdx, UINT uStyle, TCHAR *szText);

void SetStatusBarParts(HWND hwndSB)
{
	RECT r;
	HWND hwndParent = GetParent(hwndSB);
	int lpParts[MaxStatusParts];
	int parentwidth;

	GetClientRect(hwndParent, &r);

	parentwidth = r.right;
	if(parentwidth < 400) parentwidth = 400;

	// added 2
	lpParts[0] = parentwidth - 780;
	lpParts[1] = parentwidth - 480;		//left side of cursor offset
	lpParts[2] = parentwidth - 280;		//left side of value
	lpParts[3] = parentwidth - 100;		//left side of insert mode
	lpParts[4] = r.right-0;//16;

	// Tell the status bar to create the window parts. 
    SendMessage(hwndSB, SB_SETPARTS, MaxStatusParts, (LPARAM)lpParts); 
}

int Toolbar_IndexToCommand(HWND hwndTB, int pos)
{
	TBBUTTON tbb = { 0 };
	SendMessage(hwndTB, TB_GETBUTTON, pos, (LPARAM)&tbb);
	return tbb.idCommand;
}

void SetStatusBarIcon(HWND hwndSB, int part, int iconIdx)
{
	if(hwndStatusTBList[part] == 0)
	{
		//
	}
	else
	{
		TBBUTTONINFO tbbi = { sizeof(tbbi) };
		HWND hwndTB = hwndStatusTBList[part];
		int  cmdId  = Toolbar_IndexToCommand(hwndTB, 0);

		tbbi.dwMask = TBIF_IMAGE;
		tbbi.iImage = iconIdx;
		SendMessage(hwndTB, TB_SETBUTTONINFO, cmdId, (LPARAM)&tbbi);
	}
}

void SetStatusBarText(HWND hwndSB, int part, unsigned style, TCHAR *fmt, ...)
{
	TCHAR tmpbuf[128];
	va_list argp;
	
	if(part < 0 || part >= MaxStatusParts)
		return;
	
	va_start(argp, fmt);
	
	_vstprintf(tmpbuf, fmt, argp);
	va_end(argp);

	if(style == 1)
		style = SBT_NOBORDERS;
	else 
		style = 0;

	if(hwndStatusTBList[part] == 0)
	{
		SendMessage(hwndSB, SB_SETTEXT, (WPARAM)part | style, (LPARAM)(LPSTR)tmpbuf);
	}
	else
	{
		TBBUTTONINFO tbbi = { sizeof(tbbi) };
		HWND hwndTB = hwndStatusTBList[part];
		int cmdId = Toolbar_IndexToCommand(hwndTB, 0);


		tbbi.dwMask = TBIF_TEXT;
		tbbi.pszText = tmpbuf;

		SendMessage(hwndTB, 
			TB_SETBUTTONINFO, cmdId, (LPARAM)&tbbi);

		//SendMessage(hwndStatusTBList[part], 
			//SB_SETTEXT, (WPARAM)part | style, (LPARAM)(LPSTR)tmpbuf);
	}

}

//-------------------------------------------------------------
int StatusBarMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT fuFlags = (UINT) HIWORD (wParam);
	HMENU hMainMenu = NULL;
	int iMenu = 0;

	//Handle non-system popup menu descriptions
	/*if((fuFlags & MF_POPUP) && (!(fuFlags & MF_SYSMENU)))
	{
		for(iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if((HMENU) lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break;
			}
		}
	}*/

	//Display helpful text in status bar
	//MenuHelp (WM_MENUSELECT, wParam, lParam, hMainMenu, hInstance,
	//		hwnd, (UINT *)&popstr[iMenu]);

	return 0;
}

DWORD FmtData(TCHAR *buf, BYTE *HexData, TOKEN baseType, int displaySigned, int displayHex, int bigEndian);


int StatusType(int nMenuId)
{
	switch(nMenuId)
	{
	case IDM_VALUE_BYTE: return TOK_BYTE; 
	case IDM_VALUE_WORD: return TOK_WORD; 
	case IDM_VALUE_DWORD: return TOK_DWORD; 
	case IDM_VALUE_QWORD: return TOK_QWORD; 
	case IDM_VALUE_FLOAT: return TOK_FLOAT; 
	case IDM_VALUE_DOUBLE: return TOK_DOUBLE; 
	default: return TOK_BYTE;
	}
}


void UpdateStatusBarText(HWND hwndStatusBar, HWND hwndHexView)
{
	UINT  nMode = HexView_GetEditMode(hwndHexView) & 0x3;

	BYTE  buf[8]  = { 0 };
	TCHAR val[20] = TEXT("0x");

	TOKEN ty = StatusType(g_nStatusValueType);

	UINT64 pos = 0;
	UINT64 sellen = 8;

	HexView_GetSelSize(hwndHexView, &sellen);
	
	// take value from selection?
	if(g_fStatusValueFromSelection && sellen > 0)
	{
		HexView_GetSelStart(hwndHexView, &pos);
		sellen = min(sellen, 8);
	}
	else
	{
		HexView_GetCurPos(hwndHexView, &pos);
		sellen = 8;
	}

	HexView_GetData(hwndHexView, pos, buf, (ULONG)sellen);
	FmtData(val+ g_fStatusHexValue*2, buf, ty, g_fStatusSignedValue, g_fStatusHexValue, g_fStatusBigEndian);

	SetStatusBarText(hwndStatusBar, 2, 0, TEXT(" Value: %s"), val);
	SetStatusBarText(hwndStatusBar, 4, 0, g_szEditMode[nMode]);

	if(HexView_GetSelSize(hwndHexView, 0) == 0 || HexView_IsDragLoop(hwndHexView))
	{
		UINT64 len;
		
		HexView_GetFileSize(hwndHexView, &len);

		if(g_fStatusHexCursor)
			SetStatusBarText(hwndStatusBar, 1, 0, TEXT(" Cursor: %08I64X"), pos );
		else
			SetStatusBarText(hwndStatusBar, 1, 0, TEXT(" Cursor: %I64u"), pos );

		if(g_fStatusHexSize)
			SetStatusBarText(hwndStatusBar, 3, 0, TEXT(" 0x%I64X bytes"), len );
		else
			SetStatusBarText(hwndStatusBar, 3, 0, TEXT(" %I64u bytes"), len);

		SetStatusBarIcon(hwndStatusBar, 1, 0);
	}
	else
	{
		UINT64 start;
		UINT64 end;  
		UINT64 len;  

		HexView_GetSelStart(hwndHexView, &start);
		HexView_GetSelEnd(hwndHexView,   &end);
		HexView_GetSelSize(hwndHexView,  &len);

		// show selection size
		if(g_fStatusHexCursor)
			SetStatusBarText(hwndStatusBar, 1, 0, TEXT(" Selection: %08I64X - %08I64X"), start, end);
		else
			SetStatusBarText(hwndStatusBar, 1, 0, TEXT(" Selection: %I64u - %I64u"), start, end);

		if(g_fStatusHexSize)
			SetStatusBarText(hwndStatusBar, 3, 0, TEXT(" 0x%I64X bytes"), len);
		else			
			SetStatusBarText(hwndStatusBar, 3, 0, TEXT(" %I64u bytes"), len);

		SetStatusBarIcon(hwndStatusBar, 1, 1);
	}
}

void FitToolbar(HWND hwndTB, RECT *rect)
{
	DWORD size;
	int  width, height;
	RECT  tmprect;

	SendMessage(hwndTB, TB_SETPADDING, 0, MAKELONG(3, 0));


	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);

	// get current button size
	size = (DWORD)SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);
	width  = LOWORD(size);
	height = HIWORD(size);
	GetClientRect(hwndTB, &tmprect);
	
	// resize so that
	SendMessage(hwndTB, TB_SETBUTTONSIZE, 0, 
		MAKELONG(
			width - ((rect->right-rect->left) - (tmprect.right - tmprect.left)),
			height - ((rect->bottom-rect->top) - (tmprect.bottom - tmprect.top))
			)
			);
			
			
	SetWindowPos(hwndTB, 0, rect->left, rect->top,
		rect->right-rect->left, rect->bottom-rect->top, SWP_NOZORDER|SWP_NOACTIVATE);


	//SendMessage(hwndStatusTB, TB_GETITEMRECT, 0, (LPARAM)&rect);

	//SendMessage(hwndStatusTB, TB_GETBUTTONSIZE, 0, MAKELONG(32, 16));

	//SendMessage(hwndStatusTB, TB_GETPADDING, 0, 0);
	//SendMessage(hwnd, TB_GETITEMRECT, 0, (LPARAM)&rect);

	//UpdateStatusbar(hwndSB);

}

void FitToolbarButton(HWND hwndTB, int idx, RECT *rect)
{
	RECT rc2;
	DWORD sz;
	int width, height;
	int totalWidth  = rect->right  - rect->left;
	int totalHeight = rect->bottom - rect->top;
	int butwidth, butheight;

	SendMessage(hwndTB, TB_GETITEMRECT, idx, (LPARAM)&rc2);
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
	
	// get current size
	SendMessage(hwndTB, TB_GETITEMRECT, 0, (LPARAM)&rc2);
	sz = (DWORD)SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);
	width = LOWORD(sz);
	height = HIWORD(sz);

	// must be sent *after* inserting buttons!!!
	butwidth  = totalWidth - (rc2.right - width);
	butheight = totalHeight - (rc2.bottom - height);
	SendMessage(hwndTB, TB_SETBUTTONSIZE, 0, MAKELONG(butwidth, butheight));
		
	//SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
	sz = (DWORD)SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);
	width = LOWORD(sz);
	height = HIWORD(sz);

	// do it again!!!
	butwidth -= (width-butwidth);
	butheight -= (height-butheight);
	SendMessage(hwndTB, TB_SETBUTTONSIZE, 0, MAKELONG(butwidth, butheight));
	sz = (DWORD)SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);
	width = LOWORD(sz);
	height = HIWORD(sz);

//	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);

	SendMessage(hwndTB, TB_GETITEMRECT, 0, (LPARAM)&rc2);

	SetWindowPos(hwndTB, 0, rect->left,rect->top,totalWidth,totalHeight, 0);
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);


}

//
//	Reposition the toolbars above each statusbar panel
//
void UpdateStatusbar(HWND hwndSB)
{
	RECT rect;
	int  i;

	SendMessage(hwndSB, SB_SIMPLE, FALSE, 0);

	// position each toolbar in turn
	for(i = 0; i < SendMessage(hwndSB, SB_GETPARTS, 0, 0); i++)
	{
		SendMessage(hwndSB, SB_GETRECT, i, (LPARAM)&rect);
		//TRACEA("top=%d\n", rect.top);
		
		// make sure height is always multiple of 2
		rect.top += 1;
		rect.bottom--;
	//	rect.bottom -= (rect.bottom - rect.top) % 2;

		rect.left += 1;
		rect.right -= 2;

		FitToolbarButton(hwndStatusTBList[i], i, &rect);
	}
}

HWND CreateToolbarPart(HWND hwndStatusBar, int nCtrlId, int nBitmapResId, int nBmpIdx)
{
	HWND hwndStatusTB;
	DWORD flags;

	hwndStatusTB = CreateWindowEx(
		WS_EX_TOOLWINDOW, 
		TOOLBARCLASSNAME,
		0,
		WS_VISIBLE|WS_CHILD|TBSTYLE_FLAT|TBSTYLE_LIST|
		TBSTYLE_TRANSPARENT |CCS_NORESIZE|CCS_NODIVIDER,
		0,0,0,0,
		hwndStatusBar,(HMENU)(UINT_PTR)nCtrlId,
		0,0
		);

	SendMessage(hwndStatusTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);// |TBSTYLE_EX_MIXEDBUTTONS);

	SendMessage(hwndStatusTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(hwndStatusTB, TB_SETPADDING, 0, MAKELONG(3, 0));

	if(nBitmapResId != 0)
	{
		HIMAGELIST hImgList;

		SendMessage(hwndStatusTB, TB_SETBITMAPSIZE, 0, MAKELONG(14,14));

		hImgList = ImageList_LoadImage(
			GetModuleHandle(0), 
			MAKEINTRESOURCE(nBitmapResId), 
			13, 14, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

		SendMessage(hwndStatusTB, TB_SETIMAGELIST, 0, (LPARAM)hImgList);
	}


	AddButton(hwndStatusTB, nCtrlId, nBmpIdx,//-1,//0,   
		//TBSTYLE_AUTOSIZE|
		TBSTYLE_BUTTON|TBSTYLE_DROPDOWN ,//|BTNS_SHOWTEXT, 
		TEXT("Value: 0x100"));

	flags = DT_SINGLELINE|DT_VCENTER|DT_LEFT;
	SendMessage(hwndStatusTB, TB_SETDRAWTEXTFLAGS, -1, flags);//I_IMAGENONE

	return hwndStatusTB;

}


//
//
//
HWND CreateStatusBar (HWND hwndParent)
{
	HWND hwndSB;
	
	hwndSB = CreateStatusWindow (dwStatusBarStyles, TEXT("32"), hwndParent, 2);

	SetStatusBarParts(hwndSB);

	SetStatusBarText(hwndSB, 0, 1, TEXT(""));
	SetStatusBarText(hwndSB, 1, 0, TEXT("Cursor: 00000000"));
	SetStatusBarText(hwndSB, 2, 0, TEXT("Value: 00"));
	SetStatusBarText(hwndSB, 3, 0, TEXT("1234 bytes"));
	SetStatusBarText(hwndSB, 4, 0, TEXT(" OVR"));


	//****
	hwndStatusTBList[1] = CreateToolbarPart(hwndSB, IDM_STATUSBAR_CURSORPOS, IDB_BITMAP14, 1);
	hwndStatusTBList[2] = CreateToolbarPart(hwndSB, IDM_STATUSBAR_VALUE, 0, I_IMAGENONE);
	hwndStatusTBList[3] = CreateToolbarPart(hwndSB, IDM_STATUSBAR_SIZE, 0, I_IMAGENONE);
	hwndStatusTBList[4] = CreateToolbarPart(hwndSB, IDM_STATUSBAR_MODE, 0, I_IMAGENONE);

	/*hwndStatusTB2 = CreateEmptyToolbar(hwndSB, 0, 0, 0, TBSTYLE_LIST);
	SendMessage(hwndStatusTB2, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
	AddButton(hwndStatusTB2, IDM_FILE_OPEN, 8,  TBSTYLE_AUTOSIZE|TBSTYLE_BUTTON|TBSTYLE_DROPDOWN|BTNS_WHOLEDROPDOWN , 0);//TEXT("Ascii"));*/

	SendMessage(hwndStatusTBList[1], TB_SETPARENT, (WPARAM)g_hwndMain, 0);
	SendMessage(hwndStatusTBList[2], TB_SETPARENT, (WPARAM)g_hwndMain, 0);
	SendMessage(hwndStatusTBList[3], TB_SETPARENT, (WPARAM)g_hwndMain, 0);
	SendMessage(hwndStatusTBList[4], TB_SETPARENT, (WPARAM)g_hwndMain, 0);

	UpdateStatusbar(hwndSB);

	return hwndSB ;
}
