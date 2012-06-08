//
//  Options.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shellapi.h>
#include "resource.h"
#include "HexEdit.h"
#include "HexUtils.h"
#include "RecentFile.h"
#include "..\HexView\HexView.h"

#include "ComboUtil.h"

BOOL SetExplorerContextMenu(BOOL fAddToMenu);
BOOL IsContextMenuInstalled();

void MakeFontCombo(HWND hwndDlg, HWND hwndCombo);
BOOL MakeColourCombo(HWND hwndCombo, COLORREF crList[], TCHAR *szTextList[], int nCount);
void InitSizeList(HWND hwndCombo, TCHAR *szFontName);

COLORREF crDefault[] = 
{
	 RGB(255,255,255),	
	 RGB(0,0,0),		
	 RGB(255,255,255),	
	 RGB(128, 0, 0),	
	 RGB(0, 128,0),	
	 RGB(128,128,0),	
	 RGB(0,0,128),		
	 RGB(128,0,128),	
	 RGB(0,128,128),	
	 RGB(196,196,196),	
	 RGB(128,128,128),	
	 RGB(255,0,0),		
	 RGB(0,255,0),		
	 RGB(255,255,0),	
	 RGB(0,0,255),		
	 RGB(255,0,255),	
	 RGB(0,255,255),	

	

//	{ RGB(255,255,255),	"Custom..." },
};

TCHAR *szTextList[] = 
{
TEXT("Automatic"),
TEXT("Black"),
TEXT("White"),
TEXT("Maroon"),
TEXT("Dark Green"),
TEXT("Olive"),
TEXT("Dark Blue"),
TEXT("Purple"),
TEXT("Aquamarine"),
TEXT("Light Grey"),
TEXT("Dark Grey"),
TEXT("Red"),
TEXT("Green"),
TEXT("Yellow"),
TEXT("Blue"),
TEXT("Magenta"),
TEXT("Cyan"),
};



void AddColourListItem(HWND hwnd, UINT uItem, int fgIdx, int bgIdx, TCHAR *szName)
{
	HWND hwndCtrl = GetDlgItem(hwnd, uItem);
	int idx = (int)SendMessage(hwndCtrl, LB_ADDSTRING, 0, (LONG)szName);
	SendMessage(hwndCtrl, LB_SETITEMDATA, idx, MAKELONG(fgIdx, bgIdx));
}


/*void AddColourComboItem(HWND hwnd, UINT uItem, COLORREF col, TCHAR *szName)
{
	HWND hwndCtrl = GetDlgItem(hwnd, uItem);
	int idx = SendMessage(hwndCtrl, CB_ADDSTRING, 0, (LONG)szName);
	SendMessage(hwndCtrl, CB_SETITEMDATA, idx, col);
}*/

static WNDPROC  oldPreviewProc;
static HFONT	g_hPreviewFont;

static COLORREF g_crPreviewFG = 0;
static COLORREF g_crPreviewBG = RGB(200,130,220);

LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT		rect;
	PAINTSTRUCT ps;
	HANDLE		hold;

	switch(msg)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		
		GetClientRect(hwnd, &rect);

		FrameRect(ps.hdc, &rect, GetSysColorBrush(COLOR_3DSHADOW));
		InflateRect(&rect, -1, -1);

		SetTextColor(ps.hdc, g_crPreviewFG);
		SetBkColor(ps.hdc, g_crPreviewBG);

		ExtTextOut(ps.hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
		hold = SelectObject(ps.hdc, g_hPreviewFont);

		DrawText(ps.hdc, TEXT("Sample Text"), -1, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
		
		SelectObject(ps.hdc, hold);
		EndPaint(hwnd, &ps);
		return 0;
	}

	return CallWindowProc(oldPreviewProc, hwnd, msg, wParam, lParam);
}


INT_PTR CALLBACK DisplayOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND	hwndPreview;

	switch(msg)
	{
	case WM_INITDIALOG:

		MakeFontCombo(hwnd, GetDlgItem(hwnd, IDC_FONTLIST));

		MakeColourCombo(GetDlgItem(hwnd, IDC_FGCOLCOMBO), crDefault, szTextList, 17);
		MakeColourCombo(GetDlgItem(hwnd, IDC_BGCOLCOMBO), crDefault, szTextList, 17);

		SetComboItemHeight(GetDlgItem(hwnd, IDC_FGCOLCOMBO), 14);
		SetComboItemHeight(GetDlgItem(hwnd, IDC_BGCOLCOMBO), 14);

		InitSizeList(GetDlgItem(hwnd, IDC_SIZELIST), TEXT("Courier New"));

	//
	//	Subclass the PREVIEW static control so we can custom-draw it
	//
	hwndPreview = GetDlgItem(hwnd, IDC_PREVIEW);
	oldPreviewProc = (WNDPROC)SetWindowLongPtr(hwndPreview, GWLP_WNDPROC, (LONG)PreviewWndProc);

		AddColourListItem(hwnd, IDC_ITEMLIST, -1,				HVC_BACKGROUND,   TEXT("Background"));	
		AddColourListItem(hwnd, IDC_ITEMLIST, HVC_ADDRESS,		HVC_BACKGROUND,   TEXT("Address Column"));
		AddColourListItem(hwnd, IDC_ITEMLIST, HVC_HEXEVEN,		HVC_BACKGROUND,   TEXT("Hex Odd Columns"));
		AddColourListItem(hwnd, IDC_ITEMLIST, HVC_HEXODD,		HVC_BACKGROUND,   TEXT("Hex Even Columns"));
		AddColourListItem(hwnd, IDC_ITEMLIST, HVC_ASCII,		HVC_BACKGROUND,   TEXT("Ascii Column"));
		AddColourListItem(hwnd, IDC_ITEMLIST, HVC_SELECTION,	HVC_SELECTION,	  TEXT("Selected Data"));
		AddColourListItem(hwnd, IDC_ITEMLIST, HVC_SELECTION,	HVC_SELECTION,	  TEXT("Inactive Selection"));
		//AddColourListItem(hwnd, IDC_ITEMLIST, -1,					HVC_LONGLINE,	  TEXT("Modified Bytes"));
		//AddColourListItem(hwnd, IDC_ITEMLIST, HVC_CURRENTLINETEXT, HVC_CURRENTLINE,  TEXT("Current Line"));

		return TRUE;

	case WM_MEASUREITEM:
		// can't do anything here because we haven't created
		// the fonts yet, so send a manual CB_SETITEMHEIGHT instead
		return FALSE;
	}

	return FALSE;
}

INT_PTR CALLBACK MiscOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL g_fAddToExplorer;
	PSHNOTIFY *pshn;

	switch(msg)
	{
	case WM_INITDIALOG:
		g_fAddToExplorer = IsContextMenuInstalled();
		CheckDlgButton(hwnd, IDC_ADDCONTEXT, g_fAddToExplorer);

		return TRUE;

	case WM_NOTIFY:

		pshn = (PSHNOTIFY *)lParam;

		if(pshn->hdr.code == PSN_APPLY)
		{
			g_fAddToExplorer = IsDlgButtonChecked(hwnd, IDC_ADDCONTEXT);
			SetExplorerContextMenu(g_fAddToExplorer);

		}
	}

	return FALSE;
}

INT_PTR CALLBACK TypeViewOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	}

	return FALSE;
}


TCHAR *szLayout1[] = 
{
	TEXT("Binary"), 
	TEXT("Octal"), 
	TEXT("Decimal"), 
	TEXT("Hexadecimal"), 
	0
};

TCHAR *szLayout2[] = 
{
	TEXT("8 bit Byte"), 
	TEXT("16 bit Word"), 
	TEXT("32 bit Dword"), 
	TEXT("64bit Qword"), 
	0
};

INT_PTR CALLBACK LayoutOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//PSHNOTIFY *pshn;

	switch(msg)
	{
	case WM_INITDIALOG:
		CenterWindow(GetParent(hwnd));

		AddComboStringList(GetDlgItem(hwnd, IDC_COMBO4), szLayout1, 3);
		AddComboStringList(GetDlgItem(hwnd, IDC_COMBO1), szLayout2, 3);
		return TRUE;
	}

	return FALSE;
}

void ShowOptions(HWND hwndParent)
{
	PROPSHEETHEADER psh    = {   sizeof(psh)   };
	PROPSHEETPAGE   psp[2] = {  { sizeof(psp[0]) },  
								{ sizeof(psp[1]) }, 
								//{ sizeof(psp[2]) }, 
						//		{ sizeof(psp[3]) }, 
							};

	int i = 0;

	CoInitialize(0);
	
	// configure property sheet
	psh.dwFlags			= PSH_PROPSHEETPAGE;
	psh.hwndParent		= hwndParent;
	psh.nPages			= sizeof(psp) / sizeof(psp[0]);
	psh.ppsp			= psp;
	psh.pszCaption		= TEXT("Options");

	// configure property sheet page(1)
/*	psp[i].dwFlags		= PSP_USETITLE;
	psp[i].hInstance	= GetModuleHandle(0);
	psp[i].pfnDlgProc	= LayoutOptionsDlgProc;
	psp[i].pszTemplate	= MAKEINTRESOURCE(IDD_PROPERTY_LAYOUT);
	psp[i].pszTitle		= TEXT("Layout");
	i++;*/

	// configure property sheet page(2)
	psp[i].dwFlags		= PSP_USETITLE;
	psp[i].hInstance	= GetModuleHandle(0);
	psp[i].pfnDlgProc	= DisplayOptionsDlgProc;
	psp[i].pszTemplate	= MAKEINTRESOURCE(IDD_PROPERTY_FONT);
	psp[i].pszTitle		= TEXT("Colours");
	i++;

	// configure property sheet page(3)
	psp[i].dwFlags		= PSP_USETITLE;
	psp[i].hInstance	= GetModuleHandle(0);
	psp[i].pfnDlgProc	= MiscOptionsDlgProc;
	psp[i].pszTemplate	= MAKEINTRESOURCE(IDD_PROPERTY_MISC);
	psp[i].pszTitle		= TEXT("Settings");
	i++;

/*	psp[i].dwFlags		= PSP_USETITLE;
	psp[i].hInstance	= GetModuleHandle(0);
	psp[i].pfnDlgProc	= TypeViewOptionsDlgProc;
	psp[i].pszTemplate	= MAKEINTRESOURCE(IDD_PROPERTY_TYPE);
	psp[i].pszTitle		= TEXT("TypeView");
	i++;*/

	if(PropertySheet(&psh))
	{
		//ApplyRegSettings();		
	}

	CoUninitialize();
}

void FirstTimeOptions(HWND hwndMain)
{
	TCHAR szMsg[] = TEXT("Welcome to HexEdit!\n\n")
		TEXT("Would you like integrate HexEdit into the Explorer shell menu?\n")
		TEXT("This option is available through the main Options dialog");
	MessageBox(hwndMain, szMsg, TEXT(""), MB_ICONQUESTION|MB_YESNO);

}

void LoadSettings()
{
	HKEY hKey;
	
	if(RegOpenKeyEx(HKEY_CURRENT_USER, REGBASE, 0, KEY_READ, &hKey) == S_OK)
	{
		LoadRecentFileList(hKey);

		RegCloseKey(hKey);
	}
	else
	{
		// make some defaults!!
	}
}

void SaveSettings()
{
	HKEY hKey;

	if(RegCreateKeyEx(HKEY_CURRENT_USER, REGBASE, 0, 0, 0, KEY_READ|KEY_WRITE, 0, &hKey, 0) == S_OK)
	{
		SaveRecentFileList(hKey);

		RegCloseKey(hKey);
	}
	else
	{
		// make some defaults!!
	}

}
