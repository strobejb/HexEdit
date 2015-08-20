//
//  TabView.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_IE    0x0600 
#define _WIN32_WINNT 0x501
//#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <uxtheme.h>
#include <vssym32.h>
#include "resource.h"
#include "TabView.h"
#include "trace.h"

HWND CreateEmptyToolbar(HWND hwndParent, int nBitmapIdx, int nBitmapWidth, int nCtrlId, DWORD dwExtraStyle);
void AddButton(HWND hwndTB, UINT uCmdId, UINT uImageIdx, UINT uStyle, TCHAR *szText);

#define DROPLIST 1
#define MAX_TAB_TEXT	200
#define MAX_TAB_ITEMS	64
#define LEFT_INDENT     24//(4 + (DROPLIST ? 20 : 0))

#define ID_WINLIST_TOOLBAR 123

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

typedef struct
{
	int			nWidth;		// width of the *string*
	LPARAM		lParam;
	int			nImage;
	int			nState;
	TCHAR		szText[MAX_TAB_TEXT];
} TabItem;

typedef struct
{
	int			nNumTabs;
	int			nSelectedIdx;
	int			nCurrentIdx;

	int			nMaxTabWidth;
	int			nMaxTabHeight;

	HTHEME		hTheme;
	HWND		hwnd;
	HWND		hwndToolTip;
	HWND		hwndToolbar;
	HFONT		hFont;
	HFONT		hFontBold;
	HIMAGELIST	hImageList;
	HIMAGELIST	hImageListButton;
	
	BOOL		fMouseDown;
	BOOL		fCloseDown;
	BOOL		fWindowList;
	POINT		ptLastPos;

	TabItem	item[MAX_TAB_ITEMS];

} TabView;

static COLORREF MixRgb(COLORREF col1, COLORREF col2)
{
	return RGB( 
				(GetRValue(col1) + GetRValue(col2)) / 2,
				(GetGValue(col1) + GetGValue(col2)) / 2,
				(GetBValue(col1) + GetBValue(col2)) / 2
			  );
}

static TabView *GetTabViewPtr(HWND hwnd)
{
	return (TabView *)GetWindowLongPtr(hwnd, 0);
}

static void SetTabViewPtr(HWND hwnd, TabView *tv)
{
	SetWindowLongPtr(hwnd, 0, (LONG_PTR)tv);
}

static LRESULT PostTabNotify(TabView *tv, UINT nCode)
{
	NMHDR hdr;

	hdr.code		= nCode;
	hdr.hwndFrom	= tv->hwnd;
	hdr.idFrom		= GetWindowLongPtr(tv->hwnd, GWL_ID);

	return SendMessage(GetParent(tv->hwnd), WM_NOTIFY, hdr.idFrom, (LPARAM)&hdr);
}

static int ShowTabList(TabView *tv, int x, int y)
{
	HMENU hMenu = CreatePopupMenu();
	int   i;

	for(i = 0; i < tv->nNumTabs; i++)
	{
		TCHAR text[200];

		wsprintf(text, TEXT("%d %s"), i+1, tv->item[i].szText);
		AppendMenu(hMenu, MF_ENABLED, i+1, text);
	}

	i = TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RETURNCMD, x, y, 0, tv->hwnd, 0);

	if(i > 0)
	{
		if(PostTabNotify(tv, TCN_SELCHANGING) == TRUE)
			return 0;
			
		tv->nSelectedIdx = i - 1;
		InvalidateRect(tv->hwnd, 0, TRUE);
			
		PostTabNotify(tv, TCN_SELCHANGE);
	}

	return i;
}

static int TabView_GetItemRect(TabView *tv, int iItem, RECT *rect)
{
	int i, pos = LEFT_INDENT;

	for(i = 0; i < tv->nNumTabs; i++)
	{
		int xtra = 12;
		xtra+=20;
		
		// current item gets the close button
		if(i == tv->nSelectedIdx && (tv->item[i].nState & TCIS_NOCLOSE) != TCIS_NOCLOSE)
			xtra += 20;

		// add on space for image+borders
		if(tv->item[i].nImage != -1)
			xtra += 20;

		if(i == iItem)
		{
			rect->left   = pos;
			rect->right  = pos + tv->item[i].nWidth;
			rect->top    = 0;
			rect->bottom = 20;

			if(i == tv->nSelectedIdx)
				rect->right += xtra;
			else
				rect->right += xtra;
				
			return TRUE;
		}

		pos += tv->item[i].nWidth + xtra;
	}
	
	return FALSE;
}

static BOOL TabView_GetCloseRect(TabView *tv, int iItem, RECT *rect)
{
	if(!TabView_GetItemRect(tv, iItem, rect))
		return FALSE;

	rect->right -= 4;
	rect->bottom -= 4;
	rect->top = rect->bottom - 15;
	rect->left = rect->right - 15;

	return TRUE;
}

static BOOL TabView_HitTest(TabView *tv, int x, int y, int *iItem, BOOL *fCloseButton, BOOL *fWindowList)
{
	POINT pt = { x, y };
	RECT  rect;
	int i;

	*fWindowList	= FALSE;
	*fCloseButton	= FALSE;
	*iItem			= -1;

	// check the windowlist dropdown 
	GetClientRect(tv->hwnd, &rect);
	rect.right = LEFT_INDENT;//rect.right - 20;
	rect.top = 3;
	
	if(PtInRect(&rect, pt))
	{
		*fWindowList = TRUE;
		return TRUE;
	}


	// check which TAB is down
	for(i = 0; i < tv->nNumTabs; i++)
	{		
		TabView_GetItemRect(tv, i, &rect);

		if(PtInRect(&rect, pt))
		{
			TabView_GetCloseRect(tv, i, &rect);

			*iItem			= i;
			*fCloseButton	= PtInRect(&rect, pt);
			return TRUE;
		}
	}

	return FALSE;
}

static const TCHAR * ItemText(TabItem *ti)
{
	if((ti->nState & TCIS_FILENAME) == TCIS_FILENAME)
	{
		TCHAR *slash = _tcsrchr(ti->szText, '\\');

		return slash ? slash + 1 : ti->szText;
	}
	else
	{
		return ti->szText;
	}
}

static int PaintTabItem(HDC hdc, TabView *tv, int iItem, RECT *rect, HPEN hPen1, HPEN hPen2, BOOL fHotButton)
{
	int x = rect->left, y=rect->right;
	int tabwidth;
	TEXTMETRIC tm;
	TabItem *ti = &tv->item[iItem];
	
	RECT rc2;

	int y1 = 3;
	int y2 = 26;

	SelectObject(hdc, tv->hFont);
	GetTextMetrics(hdc, &tm);
	y = ((rect->bottom-rect->top) - (tm.tmHeight)) / 2;

	tabwidth	= rect->right-rect->left;//ti->nWidth+ (i == tv->nSelectedIdx?20:2);

	CopyRect(&rc2, rect);
	//SetRect(&rc2, x, rc.top, x + tabwidth, y2);

	if(iItem == tv->nSelectedIdx)
	{
		SelectObject(hdc, tv->hFontBold);
		SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
		SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
		rc2.left ++;
		rc2.right --;
		rc2.top = y1;
		rc2.bottom = y2;

				
		tabwidth -= 20;
		
		// offset current item's text
		y++;
	}
	else
	{
		SelectObject(hdc, tv->hFont);
		SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
		SetTextColor(hdc, GetSysColor(COLOR_3DDKSHADOW));
		rc2.top = y1+1;
		rc2.bottom = y2+1;
	y++;
	}



	//GetTextExtentPoint32(hdc, ptr, len, &sz);
	ExtTextOut(hdc, x + 2+(tabwidth-ti->nWidth)/2, y+2, ETO_OPAQUE, &rc2, 0,0,0);//ti->szText, lstrlen(ti->szText), 0);

	// if the item has a close icon then 
	if(!(ti->nState & TCIS_NOCLOSE) && iItem == tv->nSelectedIdx)
		rc2.right -= 20;

	rc2.left += 2;
	rc2.right -= 2;
	
	DrawText(hdc, 
		ItemText(ti), 
		-1, 
		&rc2, 
		DT_END_ELLIPSIS|DT_CENTER|DT_VCENTER|DT_SINGLELINE
		);
		
	tabwidth	= rect->right-rect->left;

	if(tv->item[iItem].nImage != -1)
		ImageList_Draw(tv->hImageList, ti->nImage, hdc, x+2, y+2, ILD_TRANSPARENT );


	//	return 0;
		if(iItem != tv->nSelectedIdx)
		{
			x += tabwidth;
			LineTo(hdc, x+1, y1);
		}
		else
		{
			RECT r = { x + tabwidth - 20, y+4, 0, 0 };
			POINT pt;
			int idx=1;
			GetCursorPos(&pt);
			ScreenToClient(tv->hwnd, &pt);
			//GetThemeRect(tv->hTheme, WP_SMALLCLOSEBUTTON, CBS_NORMAL, WP_SMALLCLOSEBUTTON/*TMT_RECT*/, &r);
			r.right = r.left + 15;
			r.bottom = r.top+ 15;
			
			if(IsWindowEnabled(tv->hwnd))
			{
				if(PtInRect(&r, pt))
					idx=2;
			
				if(idx==2 && (GetKeyState(VK_LBUTTON)&0x80000000))
					idx=3;
			}
			else
			{
				idx = 0;
			}

			//r.right -= 2;
		//	r.bottom -= 2;
			//DrawThemeBackground(tv->hTheme, hdc, WP_SMALLCLOSEBUTTON, 
			//	iItem == tv->nSelectedIdx?CBS_NORMAL:CBS_DISABLED, &r, 0);			

			//if()
			//HIMAGELIST hl = 
			ImageList_Draw(tv->hImageListButton, idx, hdc, r.left, r.top, ILD_TRANSPARENT );
			//}
			
			MoveToEx(hdc, x, y1+1, 0);
			SelectObject(hdc, hPen2);
			LineTo(hdc, x, y2);
			SelectObject(hdc, hPen1);
			LineTo(hdc, x+tabwidth-1, y2);
			LineTo(hdc, x+tabwidth-1, y1);
			SelectObject(hdc, hPen1);
			x += tabwidth + 1;
		}
	return rect->right-rect->left;
}

static LRESULT PaintTabView(HWND hwnd, TabView *tv)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rc, rc2;

	COLORREF colBackground = //MixRgb(MixRgb(GetSysColor(COLOR_3DFACE), GetSysColor(COLOR_3DHIGHLIGHT)),
							 GetSysColor(COLOR_3DFACE)
							 //)
							 ;
	COLORREF colActiveTab  = GetSysColor(COLOR_WINDOW);//3DFACE);

	HBRUSH	 hbrBackground = CreateSolidBrush(colBackground);
	HBRUSH	 hbrActiveTab  = CreateSolidBrush(colActiveTab);

	int x, i;

	int y1 = 3;
	int y2 = 26;

	//TEXTMETRIC tm;
	//LOGFONT lf;

	HPEN hPen1, hOldPen, hPen2, hPen3;
	
	GetClientRect(hwnd, &rc);

	hdc = BeginPaint(hwnd, &ps);

	hPen1 = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	hPen2 = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNHIGHLIGHT));
	hPen3 = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DDKSHADOW));

	// draw the horizontal line up to the first tab
	SetRect(&rc2, 0, y1+1, LEFT_INDENT, y2+1);
	FillRect(hdc, &rc2, hbrBackground);

	// fill 2-pixel "grey" border across the top
	SetRect(&rc2, 0, 0, rc.right, y1);
	FillRect(hdc, &rc2, hbrActiveTab);

	// Draw the 3d etched line at the bottom
	rc.top = rc2.bottom + 1;
	//DrawEdge(hdc, &rc, EDGE_ETCHED, BF_ADJUST | BF_BOTTOM);

	hOldPen = SelectObject(hdc, hPen1);
	
	// Draw the grey border - first "margin" before tabs begin
	MoveToEx(hdc, 0, y1, 0);
	LineTo(hdc, LEFT_INDENT, y1);

	//SelectObject(hdc, tv->hFont);
	//GetTextMetrics(hdc, &tm);

	x = LEFT_INDENT;
//	y = ((rc.bottom-rc.top) - (tm.tmHeight)) / 2;
	
	SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
	SetTextColor(hdc, GetSysColor(COLOR_3DDKSHADOW));
	
	// Draw each tab in turn
	for(i = 0; i < tv->nNumTabs; i++)
	{
		RECT ri;
		TabView_GetItemRect(tv, i, &ri);

		//SetRect(&rc2, x, rc.top, x + tabwidth, y2);
		ri.top = rc.top;
		ri.bottom = y2;
		x += PaintTabItem(hdc, tv, i, &ri, hPen1, hPen2, 0);
	//	rc.right = x;
	}

/*	SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	if(tv->nSelectedIdx < tv->nNumTabs)
	{
		ptr			= tv->item[i].szText;
		len			= lstrlen(ptr);
		tabwidth	= tv->item[i].nWidth;

		GetTextExtentPoint32(hdc, ptr, len, &sz);

		
		MoveToEx(hdc, x, y1+1, 0);
		SelectObject(hdc, hPen2);
		LineTo(hdc, x, y2);
		SelectObject(hdc, hPen3);
		LineTo(hdc, x+tabwidth, y2);
		LineTo(hdc, x+tabwidth, y1);
		SelectObject(hdc, hPen1);

		TextOut(hdc, x + (tabwidth-sz.cx)/2, y, ptr, len);

		x += tabwidth;
	}
	
	SetTextColor(hdc, GetSysColor(COLOR_3DDKSHADOW));
	for(i = tv->nSelectedIdx + 1; i < tv->nNumTabs; i++)
	{
		ptr			= tv->item[i].szText;
		len			= lstrlen(ptr);
		tabwidth	= tv->item[i].nWidth;

		GetTextExtentPoint32(hdc, ptr, len, &sz);

		TextOut(hdc, x + (tabwidth-sz.cx)/2, y, ptr, len);
		
		x += tabwidth;
		LineTo(hdc, x, y1);
	}*/

	LineTo(hdc, rc.right, y1);

	SetRect(&rc2, x, y1+1, rc.right, y2+1);
	FillRect(hdc, &rc2, hbrBackground);

	SetRect(&rc2, 0, y2+1, rc.right, rc.bottom);
	FillRect(hdc, &rc2, hbrBackground);


	//
	//	Paint the left/right arrows
	//
	{int w = GetSystemMetrics(SM_CXVSCROLL);
	 int h = GetSystemMetrics(SM_CYVSCROLL);
	GetClientRect(hwnd, &rc);
	
	rc.left  = 3;
	rc.right = 23;
	rc.top += 8;
	rc.bottom = rc.top + h;//-= 2;
	//DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLLEFT|DFCS_FLAT);

	ImageList_Draw(tv->hImageListButton, tv->fWindowList ? 9 : 8
	, hdc, rc.left, rc.top, ILD_TRANSPARENT );

	// draw the 'close' button
	GetClientRect(hwnd, &rc);
	rc.top += 8;
	rc.bottom = rc.top + h;//-= 2;

	rc.left = rc.right - 20;
	ImageList_Draw(tv->hImageListButton, 1, hdc, rc.left, rc.top, ILD_TRANSPARENT );


	}

	SelectObject(hdc, hOldPen);
	DeleteObject(hPen1);
	DeleteObject(hPen2);
	DeleteObject(hPen3);

	DeleteObject(hbrBackground);
	DeleteObject(hbrActiveTab);

	EndPaint(hwnd, &ps);

	return 0;
}

static LRESULT WINAPI TabView_LButtonDown(TabView *tv, HWND hwnd, int x, int y)
{
	int i;

	if(TabView_HitTest(tv, x, y, &i, &tv->fCloseDown, &tv->fWindowList))
	{
		tv->ptLastPos.x = x;
		tv->ptLastPos.y = y;
		
		tv->fMouseDown = TRUE;
		
		// windowlist dropdown??
		if(tv->fWindowList)
		{
			InvalidateRect(tv->hwnd, 0, 0);
		}
		else if(i != tv->nSelectedIdx)
		{
			if(PostTabNotify(tv, TCN_SELCHANGING) == TRUE)
				return 0;
			
			tv->nSelectedIdx = i;
			InvalidateRect(hwnd, 0, TRUE);
			
			PostTabNotify(tv, TCN_SELCHANGE);
		}
		else if(i == tv->nSelectedIdx && tv->fCloseDown)
		{
			InvalidateRect(tv->hwnd, 0, 0);
		}
		
		SetCapture(tv->hwnd);
	}

	return 0;
}

static LRESULT WINAPI TabView_MouseMove(TabView *tv, HWND hwnd, int x, int y)
{
	RECT	rect;

	POINT	pt = { x, y };
	int i;
	BOOL a, oldWinList = tv->fWindowList;

	if(TabView_HitTest(tv, x, y, &i, &a, &tv->fWindowList) && i != tv->nCurrentIdx)
	{
		TOOLINFO ti = { sizeof(ti) };
		POINT    pt = { x, y };


		tv->nCurrentIdx = i;

		SendMessage(tv->hwndToolTip, TTM_POP, 0, 0);

		//SendMessage(tv->hwndToolTip, TTM_SETTITLE, 0, (LPARAM)tv->item[i].szText);

		ti.hwnd = tv->hwnd;
		ti.uId  = (UINT_PTR)tv->hwndToolTip;
		SendMessage(tv->hwndToolTip, TTM_GETTOOLINFO, 0, (LPARAM)&ti);

		ti.lpszText = tv->item[i].szText;
		SendMessage(tv->hwndToolTip, TTM_SETTOOLINFO, 0, (LPARAM)&ti);

		//SendMessage(tv->hwndToolTip, TTM_ADJUSTRECT, 0, (LPARAM)&ti);

		GetWindowRect(tv->hwndToolTip, &rect);
		ClientToScreen(tv->hwnd, &pt);

		//SetWindowPos(tv->hwndToolTip, 0, pt.x - (rect.right-rect.left)/2, 
		//	pt.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);


	}

	//TRACEA("winlist: %d\n", tv->fWindowList);

	if(tv->fWindowList != oldWinList)
	{
		InvalidateRect(tv->hwnd, 0, 0);
	}

	if(tv->fMouseDown == FALSE)
		return 0;

	GetClientRect(hwnd, &rect);

//	InvalidateRect(hwnd,0,0);

	//TRACEA("%d %d (%d %d)\n", PtInRect(&rect, pt), PtInRect(&rect, tv->ptLastPos), tv->ptLastPos.x, tv->ptLastPos.y);

	if(tv->fWindowList)
	{
		InvalidateRect(tv->hwnd, 0, 0);
	}

	if(tv->fMouseDown && !tv->fCloseDown)
	{
		if(!PtInRect(&rect, pt) && PtInRect(&rect, tv->ptLastPos))
		{
			PostTabNotify(tv, TCN_MOUSELEAVE);
		}
	}
	
	tv->ptLastPos.x = x;
	tv->ptLastPos.y = y;

	return 0;
}

static LRESULT WINAPI TabView_LButtonUp(TabView *tv, HWND hwnd, int x, int y)
{
	tv->fWindowList = FALSE;

	if(tv->fMouseDown)
	{
		POINT pt = { x, y };
		int i;

		tv->fMouseDown = FALSE;

		if(tv->fCloseDown)
		{
			BOOL fOverCloseButton;
			BOOL fWindowList;
			TabView_HitTest(tv, x, y, &i, &fOverCloseButton, &fWindowList);	

			if(i == tv->nSelectedIdx && fOverCloseButton)
				PostTabNotify(tv, TCN_CLOSE);
		}
		//else if(tv->fWindowList)
		//{
			//PostTabNotify(tv, TCN_DROPDOWN);
			//ShowTabList(tv);
		//}

		ReleaseCapture();
	}

	InvalidateRect(hwnd,0,0);

	return 0;
}

static int TabView_GetItem(TabView *tv, int iItem, TCITEM *tcItem)
{
	if(tcItem == 0 || iItem < 0 || iItem >= tv->nNumTabs)
		return FALSE;

	if(tcItem->mask & TCIF_PARAM)
		tcItem->lParam = tv->item[iItem].lParam;
	
	if(tcItem->mask & TCIF_IMAGE)
		tcItem->iImage = tv->item[iItem].nImage;

	if(tcItem->mask & TCIF_TEXT)
		lstrcpyn(tcItem->pszText, tv->item[iItem].szText, tcItem->cchTextMax);

	return TRUE;
}

static int TabView_SetItem(TabView *tv, int iItem, TCITEM *tcItem)
{
	HDC    hdc;
	HANDLE hold;
	SIZE   sz;
	TabItem *ti;

	if(tcItem == 0 || iItem < 0 || iItem >= tv->nNumTabs)
		return FALSE;

	ti = &tv->item[iItem];

	if(tcItem->mask & TCIF_PARAM)
		ti->lParam = tcItem->lParam;
	
	if(tcItem->mask & TCIF_IMAGE)
		ti->nImage = tcItem->iImage;

	if(tcItem->mask & TCIF_STATE)
		ti->nState = tcItem->dwState;

	if(tcItem->mask & TCIF_TEXT)
		lstrcpyn(ti->szText, tcItem->pszText, MAX_TAB_TEXT);

	// calculate width of the string
	hdc  = GetDC(tv->hwnd);
	hold = SelectObject(hdc, tv->hFontBold);

	GetTextExtentPoint32(hdc, 
		ItemText(ti), lstrlen(ItemText(ti)), &sz);

	if(tv->nMaxTabWidth > 0)
		sz.cx = min(sz.cx, tv->nMaxTabWidth);

	ti->nWidth = sz.cx;

	SelectObject(hdc, hold);
	ReleaseDC(tv->hwnd, hdc);

	InvalidateRect(tv->hwnd, 0, 0);
	return TRUE;
}

static int TabView_InsertItem(TabView *tv, int iItem, TCITEM *tcItem)
{
	int i;

	if(tcItem == 0 || iItem < 0 || iItem > tv->nNumTabs || iItem == MAX_TAB_ITEMS)
		return -1;

	if((tcItem->mask & TCIF_TEXT) == 0)
		return -1;

	for(i = tv->nNumTabs; i > iItem; i--)
		tv->item[i] = tv->item[i-1];

	tv->nNumTabs++;
	tv->item[iItem].lParam		=  0;
	tv->item[iItem].nImage		= -1;
	tv->item[iItem].nWidth		=  0;
	tv->item[iItem].nState		=  0;
	tv->item[iItem].szText[0]	=  0;
	TabView_SetItem(tv, iItem, tcItem);
	return iItem;
}

static int TabView_DeleteItem(TabView *tv, int iItem)
{
	int i;

	if(iItem < 0 || iItem >= tv->nNumTabs)
		return FALSE;
	
	for(i = iItem; i < tv->nNumTabs-1; i++)
		tv->item[i] = tv->item[i+1];

	tv->nNumTabs--;

	if(tv->nSelectedIdx > iItem)
		tv->nSelectedIdx--;

	if(tv->nSelectedIdx >= tv->nNumTabs)
		tv->nSelectedIdx = tv->nNumTabs-1;

	InvalidateRect(tv->hwnd, 0, 0);
	return TRUE;
}

static int TabView_SetCurSel(TabView *tv, int iItem)
{
	int i;

	if(iItem < 0 || iItem >= tv->nNumTabs)
		return -1;

	i = tv->nSelectedIdx;
	tv->nSelectedIdx = iItem;
	InvalidateRect(tv->hwnd, 0, 0);
	return i;
}

static BOOL TabView_SetFont(TabView *tv, HFONT hFont)
{
	LOGFONT lf;

	DeleteObject(tv->hFont);
	DeleteObject(tv->hFontBold);

	// store the specified font
	GetObject(hFont, sizeof(LOGFONT), &lf);
	tv->hFont		= CreateFontIndirect(&lf);

	// make a bold version
	GetObject(tv->hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight		= FW_BOLD;
	tv->hFontBold	= CreateFontIndirect(&lf);

	return TRUE;
}

static BOOL TabView_CreateTooltip(TabView *tv)
{
	HWND hwnd;
	TOOLINFO ti = { 0 };
	RECT rect;
    
	tv->hwndToolTip = hwnd = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        TEXT("WTF"),
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        tv->hwnd,
        NULL,
        GetModuleHandle(0),
        NULL
        );

 //SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

	GetClientRect(tv->hwnd, &rect);

    ti.cbSize = sizeof(TOOLINFO);;//TTTOOLINFOW_V2_SIZE;
    ti.uFlags = TTF_SUBCLASS;//|TTF_TRACK ;
    ti.hwnd = tv->hwnd;
    ti.hinst = GetModuleHandle(0);
    ti.uId = (UINT_PTR)hwnd;
    ti.lpszText = TEXT("Ballooooon");
        // ToolTip control will cover the whole window
    ti.rect.left = 0;//rect.left;    
    ti.rect.top = 0;//rect.top;
    ti.rect.right = 500;//rect.right;
    ti.rect.bottom = 200;//rect.bottom;
    
    /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
    SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

	//SendMessage(hwnd, TTM_SETTITLE, 1, (LPARAM)TEXT("Bookmark"));

	SendMessage(hwnd, TTM_ACTIVATE, TRUE, 0);
	//SendMessage(hwnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
	return TRUE;
}

//
//	Create a toolbar with one button in it, for
//  the pin-button
//
HWND CreateWinListToolbar(HWND hwndDlg, UINT nCtrlId, BOOL fRightAligned)
{
	RECT    rect;
	RECT	rc1, rc2;
	HWND	hwndPin;

#define TOOLBAR_STYLES  (TBSTYLE_FLAT |	WS_CHILD | WS_VISIBLE | \
						CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER)

	
	static TBBUTTON tbbWinList[] = 
	{	
		{	0,	0, TBSTATE_ENABLED, TBSTYLE_BUTTON,  {0}	},
	};	

	tbbWinList[0].idCommand = nCtrlId;

	// Create the toolbar to hold pin bitmap
	hwndPin = CreateToolbarEx(
			hwndDlg,	
			TOOLBAR_STYLES	,					//,
			nCtrlId,						// toolbar ID (don't need)
			1,								// number of button images
			GetModuleHandle(0),					// where the bitmap is
			IDB_DROPLIST_BITMAP,					// bitmap resource name
			tbbWinList,							// TBBUTTON structure
			sizeof(tbbWinList) / sizeof(tbbWinList[0]),
			15,14,15,14,					// 
			sizeof(TBBUTTON) );


	// Find out how big the button is, so we can resize the
	// toolbar to fit perfectly
	SendMessage(hwndPin, TB_GETITEMRECT, 0, (LPARAM)&rect);
	
	SetWindowPos(hwndPin, HWND_TOP, 0,0, 
		rect.right-rect.left, 
		rect.bottom-rect.top, SWP_NOMOVE);

	// Setup the bitmap image
//	SendMessage(hwndPin, TB_CHANGEBITMAP, nCtrlId, (LPARAM)MAKELPARAM(0, 0)); 

	// Checked / Unchecked
//	SendMessage(hwndPin, TB_CHECKBUTTON, nCtrlId, MAKELONG(0, 0));

	GetClientRect(hwndDlg, &rc1);
	GetClientRect(hwndPin, &rc2);
	
	//if(fRightAligned)
	SetWindowPos(hwndPin, 0, 0, 5,0,0,SWP_NOSIZE);
	//else
	//	SetWindowPos(hwndPin, 0, rc1.left+10, rc1.bottom-rc2.bottom-8,0,0,SWP_NOSIZE);

	return hwndPin;
}


static void InitTabView(HWND hwnd, TabView *tv)
{
	int i;
	HFONT hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0,TEXT("Tahoma"));//GetStockObject(DEFAULT_GUI_FONT);

	// create the font to be used
	TabView_SetFont(tv, hFont);
	DeleteObject(hFont);
	
	tv->hwnd     = hwnd;
	tv->nNumTabs = 0;
	tv->nSelectedIdx = 0;
	tv->fMouseDown = 0;

	tv->nMaxTabWidth = -1;
	tv->nMaxTabHeight = -1;

	tv->hImageListButton = ImageList_LoadImage(GetModuleHandle(0), 
		MAKEINTRESOURCE(IDB_BITMAP10), 15, 0, RGB(255,0,255), 
		IMAGE_BITMAP, LR_CREATEDIBSECTION);
	
	tv->hImageList = ImageList_LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP6), 15, 0, RGB(255,0,255));
	
	lstrcpy(tv->item[0].szText, TEXT("hexedit.exe"));
	lstrcpy(tv->item[1].szText, TEXT("boot.ini"));
	lstrcpy(tv->item[2].szText, TEXT("ntuser.dat"));
	lstrcpy(tv->item[3].szText, TEXT("(untitled)"));
	
	for(i = 0; i < tv->nNumTabs; i++)
	{
		tv->item[i].nWidth = 0;// lstrlen(tv->item[i].szText) * 8;
		tv->item[i].nImage = -1;
		tv->item[i].lParam = 0;
		//tv->item[i].nImage = i;
	}
	
	tv->hTheme = OpenThemeShim(hwnd, L"scrollbar");

	TabView_CreateTooltip(tv);

	tv->hwndToolbar = CreateWinListToolbar(tv->hwnd, ID_WINLIST_TOOLBAR, TRUE);
}


static LRESULT WINAPI TabViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TabView *tv = GetTabViewPtr(hwnd);
	NMHDR *hdr;

	switch(msg)
	{
	case WM_CREATE:
		InitTabView(hwnd, tv);
		return TRUE;

	case WM_NCCREATE:

		// Allocate a new TabView structure	
		if((tv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TabView))) == 0)
			return FALSE;

		SetTabViewPtr(hwnd, tv);
		return TRUE;

	case WM_NCDESTROY:
		
		if(tv->hTheme)
			CloseThemeData(tv->hTheme);

		DeleteObject(tv->hFont);
		DeleteObject(tv->hFontBold);

		HeapFree(GetProcessHeap(), 0, tv);
		return 0;

	case WM_THEMECHANGED:
		
		if(tv->hTheme)
			CloseThemeData(tv->hTheme);

		tv->hTheme = OpenThemeShim(hwnd, L"scrollbar");	
		return 0;

	case WM_LBUTTONDOWN:
		return TabView_LButtonDown(tv, hwnd, (short)LOWORD(lParam), (short)HIWORD(lParam));

	case WM_MOUSEMOVE:
		return TabView_MouseMove(tv, hwnd, (short)LOWORD(lParam), (short)HIWORD(lParam));

	case WM_LBUTTONUP:
		return TabView_LButtonUp(tv, hwnd, (short)LOWORD(lParam), (short)HIWORD(lParam));

	//case WM_MOUSEACTIVATE:
		// do nothing
	//	break;

	case WM_ERASEBKGND:
		{
			RECT rc;
			GetClientRect(hwnd, &rc);
			rc.top   = 4;
			rc.right = LEFT_INDENT;
			FillRect((HDC)wParam, &rc, GetSysColorBrush(COLOR_3DFACE));
		}
		return 1;

	case TCM_SETCURSEL:
		return TabView_SetCurSel(tv, (int)wParam);

	case TCM_GETCURSEL:
		return tv->nSelectedIdx;

	case TCM_GETITEMCOUNT:
		return tv->nNumTabs;

	case TCM_SETITEM:
		return TabView_SetItem(tv, (int)wParam, (TCITEM *)lParam);

	case TCM_INSERTITEM:
		return TabView_InsertItem(tv, (int)wParam, (TCITEM *)lParam);

	case TCM_DELETEITEM:
		return TabView_DeleteItem(tv, (int)wParam);

	case TCM_DELETEALLITEMS:
		tv->nNumTabs = 0;
		return TRUE;

	case TCM_SETITEMSIZE:
		tv->nMaxTabWidth  = (int)(short)LOWORD(lParam);
		tv->nMaxTabHeight = (int)(short)HIWORD(lParam);
		return TRUE;

	case TCM_GETITEM:
		return TabView_GetItem(tv, (int)wParam, (TCITEM *)lParam);

	case TCM_GETITEMRECT:
		return TabView_GetItemRect(tv, (int)wParam, (RECT *)lParam);

	case TCM_SETIMAGELIST:
		wParam = (WPARAM)tv->hImageList;
		tv->hImageList = (HIMAGELIST)lParam;
		return wParam;

	case WM_CANCELMODE:
		tv->fMouseDown = FALSE;
		break;

	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *wp = (WINDOWPOS *)lParam;
			RECT rect;

			GetClientRect(hwnd, &rect);

			rect.left = rect.right - 32;
			rect.right += wp->cx;

			//rect.left = rect.right - 32;
			InvalidateRect(hwnd, &rect, 0);
		}
		return 0;

	case WM_PAINT:
		return PaintTabView(hwnd, tv);

	case WM_SETFONT:
		TabView_SetFont(tv, (HFONT)wParam);
		return 0;

	case WM_NOTIFY:
		hdr = (NMHDR *)lParam;

		if(hdr->code == TTN_GETDISPINFO || hdr->code == TTN_SHOW)
		{
			return 0;
		}
		else if(hdr->code == NM_LDOWN)
		{
		}
		//else if(hdr->code == TBN_
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_WINLIST_TOOLBAR:
			{
			HWND hwndTB = (HWND)lParam;
			int id = LOWORD(wParam);
			RECT rect;
			int pos = (int)SendMessage(hwndTB, TB_COMMANDTOINDEX, id, 0);

			UINT oldStyle = (UINT)SendMessage(hwndTB, TB_GETSTYLE, pos, (LPARAM)&rect);
			SendMessage(hwndTB, TB_GETITEMRECT, pos, (LPARAM)&rect);
			SendMessage(hwndTB, TB_CHECKBUTTON, id, MAKELONG(TRUE, 0));

			SendMessage(hwndTB, TB_COMMANDTOINDEX, id, 0);

			//if(SendMessage(hwndTB, TB_ISBUTTONCHECKED, id, 0))
			{
				MapWindowPoints(hwndTB, 0, (POINT *)&rect, 2);
				
				if(ShowTabList(tv, rect.left, rect.bottom) >= 0)
				{
					//
					//PostTabNotify(tv, TCN_DROPDOWN);
					SendMessage(hwndTB, TB_CHECKBUTTON, id, MAKELONG(FALSE, 0));
				}
			}
			

			}


			return 0;
		}

		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}


ATOM WINAPI RegisterTabView()
{
	WNDCLASSEX wcx = { sizeof(wcx) };

	wcx.lpfnWndProc		= TabViewProc;
	wcx.cbWndExtra		= sizeof(TabView *);
	wcx.hInstance		= GetModuleHandle(0);
	wcx.hCursor			= LoadCursor (NULL, IDC_ARROW);
	wcx.lpszClassName	= WC_TABVIEW;

	return RegisterClassEx(&wcx);
}	



