//
//  ColourCombo.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#define STRICT

#include <windows.h>
#include "ComboUtil.h"

static void PaintFrameRect(HDC hdc, RECT *rect, COLORREF border, COLORREF fill)
{
	HBRUSH   hbrFill	= CreateSolidBrush(fill);
	HBRUSH   hbrBorder	= CreateSolidBrush(border);

	FrameRect(hdc, rect, hbrBorder);
	InflateRect(rect, -1, -1);
	FillRect(hdc, rect,  hbrFill);
	InflateRect(rect, 1, 1);

	DeleteObject(hbrFill);
	DeleteObject(hbrBorder);
}

//
//	Combobox must have the CBS_HASSTRINGS style set!!
//	
BOOL WINAPI ColourCombo_DrawItem(CTRLITEM *cip, DRAWITEMSTRUCT *dis, BOOL fDrawBackground)//BOOL fSelectImage)
{
	RECT		rect	= dis->rcItem;
	int			boxsize = (dis->rcItem.bottom - dis->rcItem.top) - 4;			
	int			xpos;
	int			ypos;
	TEXTMETRIC	tm;
	TCHAR		szText[80];
	HANDLE		hOldFont;
	HANDLE		hCurFont;
	
	//if(!fSelectImage)
	rect.left += boxsize + 4;

	if(dis->itemAction & ODA_FOCUS && !(dis->itemState & ODS_NOFOCUSRECT))
	{
		DrawFocusRect(dis->hDC, &rect);
		return TRUE;
	}

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
	}
	
	//
	//	Get the item text
	//
	if(dis->itemID == -1)
		SendMessage(dis->hwndItem, WM_GETTEXT, 0, (LPARAM)szText);
	else
		SendMessage(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)szText);
	
	//
	//	Draw the text (centered vertically)
	//	
	hCurFont = (HANDLE)SendMessage(dis->hwndItem, WM_GETFONT, 0, 0);
	hOldFont = SelectObject(dis->hDC, hCurFont);//g_hNormalFont);

	GetTextMetrics(dis->hDC, &tm);
	ypos = dis->rcItem.top  + (dis->rcItem.bottom - dis->rcItem.top - tm.tmHeight) / 2;
	xpos = dis->rcItem.left + boxsize + 4 + 4;
	
	//if(fDrawBackground == FALSE)
		SetBkMode(dis->hDC, TRANSPARENT);

	ExtTextOut(dis->hDC, xpos, ypos, 
		ETO_CLIPPED|(fDrawBackground ? ETO_OPAQUE : 0), &rect, szText, lstrlen(szText), 0);
	
	if((dis->itemState & ODS_FOCUS) && !(dis->itemState & ODS_NOFOCUSRECT))
	{
		DrawFocusRect(dis->hDC, &rect);
	}
	
	// 
	//	Paint the colour rectangle
	//	
	rect = dis->rcItem;
	InflateRect(&rect, -2, -2);
	rect.right = rect.left + boxsize;
	
	if(dis->itemState & ODS_DISABLED)
		PaintFrameRect(dis->hDC, &rect, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DFACE));
	else
		PaintFrameRect(dis->hDC, &rect, RGB(0,0,0), (COLORREF)dis->itemData);//REALIZE_SYSCOL(dis->itemData));
	
	
	return TRUE;
}

void AddColourComboItem(HWND hwndCombo, COLORREF col, TCHAR *szName)
{
	int idx = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)szName);
	SendMessage(hwndCombo, CB_SETITEMDATA, idx, col);
}

COLORREF GetColourComboRGB(HWND hwndCombo)
{
	int idx = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
	return (COLORREF)SendMessage(hwndCombo, CB_GETITEMDATA, idx, 0);
}

BOOL MakeColourCombo(HWND hwndCombo, COLORREF crList[], TCHAR *szTextList[], int nCount)
{
	int i;
	CTRLITEM *cip;
	
	if((cip = SubclassCombo(hwndCombo, ColourCombo_DrawItem)) == 0)
		return FALSE;

	AutoOwnerDraw(cip->hTheme, hwndCombo, ColourCombo_DrawItem, 0);

	for(i = 0; i < nCount; i++)
		AddColourComboItem(hwndCombo, crList[i], szTextList[i]);
	
	SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);
	return TRUE;
}
