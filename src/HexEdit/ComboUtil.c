//
//  ComboUtil.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#define STRICT

#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <tchar.h>

#include "ComboUtil.h"

static HTHEME OpenThemeShim(HWND hwnd, LPCWSTR pszClassList)
{
	__try
	{
		return OpenThemeData(hwnd, pszClassList);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return NULL;
	}
}


LRESULT CALLBACK ItemMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DLGCTRL * dcp;
	WNDPROC   oldProc;
	LONG i;
	
	dcp		= (DLGCTRL *)GetProp(hwnd, _T("AutoOwnerDrawPtr"));
	oldProc = dcp->oldProc;

	switch(msg)
	{
	case WM_DRAWITEM:
		
		for(i = 0; i < dcp->itemCount; i++)
		{
			if(dcp->itemList[i].ctrlId == wParam)
			{
				DrawItemProc drawProc = dcp->itemList[i].pfnDrawItemProc;
				drawProc(&dcp->itemList[i], (DRAWITEMSTRUCT *)lParam, TRUE);
				return TRUE;
			}
		}

		break;

	}

	return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
}


void AutoOwnerDraw(HTHEME hTheme, HWND hwndCtrl, DrawItemProc drawProc, MeasureItemProc measureProc)
{
	HWND hwndParent = GetParent(hwndCtrl);
	DLGCTRL *dcp;
	int idx;

	dcp = (DLGCTRL *)GetProp(hwndParent, _T("AutoOwnerDrawPtr"));

	if(dcp == 0)
	{
		dcp				= malloc(sizeof(DLGCTRL));
		dcp->itemCount	= 0;
		dcp->oldProc	= (WNDPROC)GetWindowLongPtr(hwndParent, GWLP_WNDPROC);
		
		SetProp(hwndParent, _T("AutoOwnerDrawPtr"), (HANDLE)dcp);
		SetWindowLongPtr(hwndParent, GWLP_WNDPROC, (LONG_PTR)ItemMsgProc);
	}

	idx = dcp->itemCount++;

	dcp->itemList[idx].ctrlId			= GetWindowLongPtr(hwndCtrl, GWLP_ID);
	dcp->itemList[idx].pfnDrawItemProc	= drawProc;
	dcp->itemList[idx].hTheme			= hTheme;
	dcp->itemList[idx].fMouseOver		= FALSE;
	dcp->itemList[idx].oldWndProc		= NULL;
}


void SetComboItemHeight(HWND hwndCombo, int nMinHeight)
{
	TEXTMETRIC	tm;
	HDC			hdc	 = GetDC(hwndCombo);
	HANDLE		hold = SelectObject(hdc, 0);//g_hNormalFont);
	
	// item height must fit the font+smallicon height
	GetTextMetrics(hdc, &tm);
	nMinHeight = max(tm.tmHeight, nMinHeight);

	SelectObject(hdc, hold);
	ReleaseDC(hwndCombo, hdc);

	SendMessage(hwndCombo, CB_SETITEMHEIGHT, -1, nMinHeight);
	SendMessage(hwndCombo, CB_SETITEMHEIGHT, 0, nMinHeight);
}

void DrawThemedComboBackground(CTRLITEM *cip, HWND hwndCombo, HDC hdc, RECT *rect)
{
	UINT uState;
	RECT clip;

	if(cip->hTheme == 0)
		return;

	if(IsWindowEnabled(hwndCombo) == FALSE)
		uState = CBXS_DISABLED;
	else if(cip->fMouseOver)
		uState = CBXS_HOT;
	else
		uState = CBXS_NORMAL;

	if(SendMessage(hwndCombo, CB_GETDROPPEDSTATE, 0, 0))
		uState = 3;//CBX_PRESSED;

	//if (IsThemeBackgroundPartiallyTransparent (hTheme, 1, uState))
	DrawThemeParentBackground(hwndCombo, hdc, rect);

	DrawThemeBackground(
		//cip->
		cip->hTheme, 
		hdc, 
		5,//CP_DROPDOWNBUTTON, 
		uState,//uState,//CBXS_NORMAL, 
		rect, 
		0);//&clip);

	//rect.right = rect2.left;
	clip = *rect;
	clip.right = clip.left;
	clip.left = 0;

	rect->left = rect->right - GetSystemMetrics(SM_CXVSCROLL);
	clip = *rect;
	clip.left += 2;

	DrawThemeBackground(
		cip->hTheme, 
		hdc, 
		6,	//CP_DROPDOWNBUTTONRIGHT, 
		uState,//CBXS_NORMAL, 
		rect, 
		&clip);

}

//
//
//
LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	RECT		rect;
	POINT		pt;

	CTRLITEM *cip = (CTRLITEM *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	WNDPROC oldProc = cip->oldWndProc;

	switch(msg)
	{
	case WM_NCDESTROY:
		
		if(cip->hTheme)
			CloseThemeData(cip->hTheme);

		//SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		//free(ccp);
		break;

	case WM_THEMECHANGED:
		
		if(cip->hTheme)
			CloseThemeData(cip->hTheme);

		cip->hTheme = OpenThemeShim(hwnd, L"ComboBox");		
		return 0;

	case WM_LBUTTONDOWN:
		InvalidateRect(hwnd, 0, 0);
		break;

	case WM_MOUSEMOVE:

		if(cip->fMouseOver == FALSE)
			SetTimer(hwnd, 0xdeadbeef, 15, 0);

		break;

	case WM_TIMER:

		if(wParam == 0xdeadbeef)
		{
			GetCursorPos(&pt);
			ScreenToClient(hwnd, &pt);
			GetClientRect(hwnd, &rect);
			
			if(PtInRect(&rect, pt))
			{
				if(!cip->fMouseOver)
				{
					cip->fMouseOver = TRUE;
					InvalidateRect(hwnd, 0, TRUE);
				}
			}
			else
			{
				cip->fMouseOver = FALSE;
				KillTimer(hwnd, 0xdeadbeef);
				InvalidateRect(hwnd, 0, TRUE);
			}
			
			return 0;
		}

		break;

	case WM_KILLFOCUS: case WM_SETFOCUS:
		InvalidateRect(hwnd, 0, 0);
		break;

	case WM_PAINT:
		
		if(cip->hTheme)
		{
			BeginPaint(hwnd, &ps);
			
			GetClientRect(hwnd, &rect);
			DrawThemedComboBackground(cip, hwnd, ps.hdc, &rect);
			
			GetClientRect(hwnd, &rect);
			{
				int idx = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
				DRAWITEMSTRUCT dis = { ODT_COMBOBOX, GetWindowLong(hwnd, GWL_ID),
					idx, ODA_DRAWENTIRE, 0, hwnd, ps.hdc, 0, (int)SendMessage(hwnd, CB_GETITEMDATA, idx, 0) };
				CopyRect(&dis.rcItem, &rect);
				dis.itemData = SendMessage(hwnd, CB_GETITEMDATA, idx, 0);
				
				if(GetFocus() == hwnd)
					dis.itemState |= ODS_FOCUS;
				
				if(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0))
					dis.itemState |= ODS_NOFOCUSRECT;
				
				InflateRect(&dis.rcItem, -3, -3);
				dis.rcItem.right -= GetSystemMetrics(SM_CXVSCROLL);
				cip->pfnDrawItemProc(hwnd, &dis, FALSE);
			}
			
			EndPaint(hwnd, &ps);
			return 0;
		}
		else
		{
			break;
		}
	}

	return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
}


CTRLITEM * SubclassCombo(HWND hwndCombo, DrawItemProc drawProc)
{
	CTRLITEM *cip;

	// subclass the combo so we can handle its WM_PAINT
	if((cip = malloc(sizeof(CTRLITEM))) == 0)
		return 0;

	cip->oldWndProc			= (WNDPROC)GetWindowLongPtr(hwndCombo, GWLP_WNDPROC);
	cip->pfnDrawItemProc	= drawProc;
	cip->fMouseOver			= FALSE;
	cip->hTheme				= OpenThemeShim(hwndCombo, L"ComboBox");

	SetWindowLongPtr(hwndCombo, GWLP_USERDATA, (LONG_PTR)cip);
	SetWindowLongPtr(hwndCombo, GWLP_WNDPROC, (LONG_PTR)ComboProc);
	return cip;
}

void DrawItem_DefaultColours(DRAWITEMSTRUCT *dis)
{
	if(dis->itemState & ODS_DISABLED)
	{
		SetTextColor(dis->hDC, GetSysColor(COLOR_3DSHADOW));
		SetBkColor(dis->hDC,   GetSysColor(COLOR_3DFACE));
	}
	else
	{
		if((dis->itemState & ODS_SELECTED))
		{
			SetTextColor(dis->hDC,  GetSysColor(COLOR_HIGHLIGHTTEXT));
			SetBkColor(dis->hDC,	GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{
			SetTextColor(dis->hDC, GetSysColor(COLOR_WINDOWTEXT));
			SetBkColor(dis->hDC,   GetSysColor(COLOR_WINDOW));
		}
	}
}

