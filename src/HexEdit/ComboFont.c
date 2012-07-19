//
//  ComboFont.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//


#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _WIN32_WINNT 0x501
#define STRICT

#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <tchar.h>
#include "resource.h"
#include "ComboUtil.h"

HFONT g_hBoldFont;
HFONT g_hNormalFont;
HICON g_hIcon2;
HICON g_hIcon3;


BOOL WINAPI FontCombo_DrawItem(HTHEME hTheme, DRAWITEMSTRUCT *dis, BOOL fDrawBackground);
#define MSG_UPDATE_PREVIEW (WM_USER+1)

//
//	Font-enumeration callback
//
int CALLBACK EnumFontNames(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	HWND hwndCombo = (HWND)lParam;
	TCHAR *pszName  = lpelfe->elfLogFont.lfFaceName;

		if(pszName[0] == '@')
			return 1;
	
	// make sure font doesn't already exist in our list
	if(SendMessage(hwndCombo, CB_FINDSTRING, 0, (LPARAM)pszName) == CB_ERR)
	{
		int		idx;
		BOOL	fFixed;
		int		fTrueType;		// 0 = normal, 1 = TrueType, 2 = OpenType

		// add the font
		idx = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)pszName);

		// record the font's attributes (Fixedwidth and Truetype)
		fFixed		= (lpelfe->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ? TRUE : FALSE;
		fTrueType	= (lpelfe->elfLogFont.lfOutPrecision == OUT_STROKE_PRECIS) ? TRUE : FALSE;
		fTrueType	= (lpntme->ntmTm.ntmFlags & NTM_TT_OPENTYPE) ? 2 : fTrueType;
			
		// store this information in the list-item's userdata area
		SendMessage(hwndCombo, CB_SETITEMDATA, idx, MAKELPARAM(fFixed, fTrueType));

		/*{
			COMBOBOXEXITEM cbxi = { CBEIF_TEXT, 0, pszName };
			SendMessage(GetDlgItem(GetParent(hwndCombo), IDC_COMBOBOXEX1), CBEM_INSERTITEM, 0, &cbxi);
		}*/
	}

	return 1;
}

//
//	Initialize the font-list by enumeration all system fonts
//
void FillFontComboList(HWND hwndCombo, TCHAR *szInitialFont)
{
	HDC		hdc = GetDC(hwndCombo);
	LOGFONT lf;
	int		idx = 0;

	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
//	SendMessage(hwndCombo, WM_SETFONT, (WPARAM)g_hNormalFont, 0);

	lf.lfCharSet			= ANSI_CHARSET;	// DEFAULT_CHARSET;
	lf.lfFaceName[0]		= '\0';			// all fonts
	lf.lfPitchAndFamily		= 0;

	// store the list of fonts in the combo
	EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontNames, (LPARAM)hwndCombo, 0);

	ReleaseDC(hwndCombo, hdc);

	// select current font in list
	if(szInitialFont != 0)
		idx = (int)SendMessage(hwndCombo, CB_FINDSTRING, 0, (LPARAM)szInitialFont);

	SendMessage(hwndCombo, CB_SETCURSEL, idx, 0);
}



//
//	Fontlist owner-draw 
//
BOOL WINAPI FontCombo_DrawItem(CTRLITEM *cip, DRAWITEMSTRUCT *dis, BOOL fDrawBackground)
{
	TCHAR		szText[100];
	
	BOOL		fFixed		= LOWORD(dis->itemData);
	BOOL		fTrueType	= HIWORD(dis->itemData);

	TEXTMETRIC	tm;
	int			xpos, ypos;
	HANDLE		hOldFont;

	if(dis->itemAction & ODA_FOCUS && !(dis->itemState & ODS_NOFOCUSRECT))
	{
		DrawFocusRect(dis->hDC, &dis->rcItem);
		return TRUE;
	}

	/*{
		HTHEME hTheme = 	OpenThemeShim(hwnd, L"combobox");
		RECT rc;
		HDC hdc=GetDC(GetParent(hwnd));
		CopyRect(&rc, &dis->rcItem);
		InflateRect(&rc, 3, 3);
		//GetClientRect(hwnd, &rc);
		//rc.bottom = rc.top + 22;

		//DrawThemeBackground(
		//	hTheme, 
		//	dis->hDC, 
		//	4,//CP_DROPDOWNBUTTON, 
		//	CBXS_HOT,//CBXS_NORMAL, 
		//	&rc, 
		//	&rc);

		CloseThemeData(hTheme);
		ReleaseDC(GetParent(hwnd),hdc);
		return TRUE;
	}*/

	//
	//	Set text colour and background based on current state
	//
	DrawItem_DefaultColours(dis);
	
	if(dis->itemState & ODS_COMBOBOXEDIT)
	{
		RECT rect;
		GetClientRect(dis->hwndItem, &rect);
		fDrawBackground = FALSE;
		SelectClipRgn(dis->hDC, NULL);
		DrawThemedComboBackground(cip, dis->hwndItem, dis->hDC, &rect);

		SetTextColor(dis->hDC, GetSysColor(COLOR_WINDOWTEXT));
		//InvalidateRect(dis->hwndItem, 0, 0);
		//return 0;
	}

	//
	//	Get the item text
	//
	if(dis->itemID == -1)
		SendMessage(dis->hwndItem, WM_GETTEXT, 0, (LPARAM)szText);
	else
		SendMessage(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)szText);
	


	// set the font: BOLD for fixed-width, NORMAL for 'normal'
	hOldFont = SelectObject(dis->hDC, fFixed ? g_hBoldFont : g_hNormalFont);
	GetTextMetrics(dis->hDC, &tm);

	ypos = dis->rcItem.top  + (dis->rcItem.bottom-dis->rcItem.top-tm.tmHeight)/2;
	xpos = dis->rcItem.left + 20;
	
	// draw the text
	SetBkMode(dis->hDC, TRANSPARENT);
	ExtTextOut(dis->hDC, xpos, ypos,
		ETO_CLIPPED|(fDrawBackground?ETO_OPAQUE:0), &dis->rcItem, szText, (UINT)_tcslen(szText), 0);

	// draw a 'TT' icon if the font is TRUETYPE
	if(fTrueType)
	{
		DrawIconEx(dis->hDC, dis->rcItem.left+2, dis->rcItem.top +
		(dis->rcItem.bottom-dis->rcItem.top-16)/2, 
		g_hIcon2, 16, 16, 0, 0, DI_NORMAL);
	}
	//else if(fTrueType == 2)
	//	DrawIconEx(dis->hDC, dis->rcItem.left+2, dis->rcItem.top, g_hIcon3,16, 16, 0, 0, DI_NORMAL);

	SelectObject(dis->hDC, hOldFont);

	// draw the focus rectangle
	if((dis->itemState & ODS_FOCUS) && !(dis->itemState & ODS_NOFOCUSRECT))
	{
		DrawFocusRect(dis->hDC, &dis->rcItem);
	}

	return TRUE;
}






void MakeFontCombo(HWND hwndDlg, HWND hwndCombo)
{
	CTRLITEM *cip;
	HFONT hDlgFont;
	LOGFONT lf;

	//
	//	Load the TrueType icon for the font-list
	//
	g_hIcon2 = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON4), IMAGE_ICON, 16, 16, 0);
	g_hIcon3 = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON4), IMAGE_ICON, 16, 16, 0);
	
	//
	//	Create two fonts (normal+bold) based on current dialog's font settings
	//
	hDlgFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
	GetObject(hDlgFont, sizeof(lf), &lf);
		
	g_hNormalFont = CreateFontIndirect(&lf);
	lf.lfWeight   = FW_BOLD;
	g_hBoldFont   = CreateFontIndirect(&lf);


	SetComboItemHeight(hwndCombo, 16);

	cip = SubclassCombo(hwndCombo, FontCombo_DrawItem);

	// fill the combo with the list of all fonts
	FillFontComboList(hwndCombo, TEXT("Arial"));//g_szFontName);
	
	AutoOwnerDraw(cip->hTheme, hwndCombo, FontCombo_DrawItem, 0);
}
