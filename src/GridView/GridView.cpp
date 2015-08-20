//
//  GridView.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include "GridViewInternal.h"
#include "trace.h"

// delay-load the UXTHEME library
//#pragma comment(lib, "uxtheme.lib")
//#pragma comment(lib, "DelayImp.lib")
//#pragma comment(linker, "/DELAYLOAD:uxtheme.dll")

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

static HWND CreateGVToolTip(HWND hwndParent)
{
	//
	//	Create the tooltip for truncated item info-tips
	//
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		TEXT("WTF"),
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_NOANIMATE ,		
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hwndParent,
		NULL,
		GetModuleHandle(0),
		NULL
		);

	SetWindowPos(hwndTT, HWND_TOPMOST,0,0,0,0,
				SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE);


	TOOLINFO ti = { sizeof(ti) };
	ti.hwnd		= hwndParent;
	ti.uFlags	= TTF_SUBCLASS|TTF_IDISHWND|TTF_TRANSPARENT;//0;//TTF_SUBCLASS ;
	ti.lpszText = LPSTR_TEXTCALLBACK ;
		//_T("hello");//LPSTR_TEXTCALLBACK;//_T("hello");
	ti.uId		= (UINT_PTR)hwndParent;
	ti.hinst	= 0;//GetModuleHandle(0);
	
	//SetRect(&ti.rect, 0, 0, 1600, 600);
	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
	//SendMessage(m_hWndTooltip, TTM_SETMAXTIPWIDTH, 0, 200);
		
	return hwndTT;

}

GridView::GridView(HWND hwnd)
{
	m_hWnd = hwnd;

	//
	//	Create the header-control to display the column headers
	//
	m_hWndHeader = CreateWindowEx(0, WC_HEADER, 0, WS_VISIBLE|WS_CHILD|
		HDS_BUTTONS|HDS_DRAGDROP|HDS_FULLDRAG|HDS_HORZ,
		0, 0, 0, 0, m_hWnd, 0, 0, 0);

	//
	//	Create the edit-control for item-editing
	//
	//m_hwndEdit = CreateWindowEx(0, _T("EDIT"), 0, WS_CHILD|ES_MULTILINE|ES_AUTOHSCROLL,
	//	0, 0, 0, 0, m_hWnd, 0, 0, 0);

	m_hComboTheme = OpenThemeShim(m_hWnd, L"ComboBox");
	m_hGridTheme  = OpenThemeShim(m_hWnd, L"Edit");

	m_nNumColumns = 0;
	m_nNumLines	  = 0;
	m_nVScrollPos = 0;
	m_nHScrollPos = 0;
	m_nLineHeight = 23;
	m_nHeaderHeight = 0;
	m_fMouseDown	= FALSE;
	m_uState		= 0;
	m_nCurrentColumn = 0;
	m_nCurrentLine	= 0;
	m_nScrollTimer  = 0;
	m_nScrollMouseRemainder = 0;
	m_hwndEdit		= 0;
	m_hwndComboLBox = 0;
	m_pTempInsertItem = 0;
	m_fInNotify		  = false;
	m_fEditError		= false;
	m_fRedrawChanges   = true;

	m_hWndTooltip = CreateGVToolTip(m_hWnd);

	m_rgbColourList[GVC_FOREGROUND]		= SYSCOL(COLOR_WINDOWTEXT);
	m_rgbColourList[GVC_BACKGROUND]		= 0xf0f0f0;//SYSCOL(COLOR_WINDOW);
	m_rgbColourList[GVC_BACKGROUND]		= SYSCOL(COLOR_WINDOW);
	m_rgbColourList[GVC_HIGHLIGHTTEXT]	= SYSCOL(COLOR_HIGHLIGHTTEXT);
	m_rgbColourList[GVC_HIGHLIGHT]		= SYSCOL(COLOR_HIGHLIGHT);
	m_rgbColourList[GVC_HIGHLIGHTTEXT2]	= SYSCOL(COLOR_WINDOWTEXT);
	m_rgbColourList[GVC_HIGHLIGHT2]		= SYSCOL(COLOR_3DFACE);

	m_rgbColourList[GVC_TREEBUT]		= SYSCOL(COLOR_WINDOW);
	m_rgbColourList[GVC_TREEBUT_BORDER]	= SYSCOL(COLOR_3DSHADOW);
	m_rgbColourList[GVC_TREEBUT_GLYPH]	= SYSCOL(COLOR_3DDKSHADOW);

	m_rgbColourList[GVC_GRIDLINES]		= SYSCOL(COLOR_3DFACE);
	m_rgbColourList[GVC_GRIDLINES2]		= 0xDDAE81;


	/*
	m_rgbColourList[GVC_FOREGROUND]		= SYSCOL(COLOR_WINDOWTEXT);
	m_rgbColourList[GVC_BACKGROUND]		= 0xf0f0f0;//SYSCOL(COLOR_WINDOW);
	m_rgbColourList[GVC_HIGHLIGHTTEXT]	= SYSCOL(COLOR_HIGHLIGHTTEXT);
	m_rgbColourList[GVC_HIGHLIGHT]		= SYSCOL(COLOR_HIGHLIGHT);
	m_rgbColourList[GVC_HIGHLIGHTTEXT2]	= SYSCOL(COLOR_WINDOWTEXT);
	m_rgbColourList[GVC_HIGHLIGHT2]		= 0xffffff;//0xe0e0e0;
	m_rgbColourList[GVC_GRIDLINES]		= 0xffffff;
*/

//	m_rgbColourList[GVC_HIGHLIGHTTEXT]	= 0xffffff;//SYSCOL(COLOR_HIGHLIGHTTEXT);
//	m_rgbColourList[GVC_HIGHLIGHT]		= 0xDF803D;
//	m_rgbColourList[GVC_GRIDLINES2]		= 0xC1692A;

	OnSetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT), 0);
	UpdateMetrics();
}

GridView::~GridView()
{
	if(m_hComboTheme)
		CloseThemeData(m_hComboTheme);

	if(m_hGridTheme)
		CloseThemeData(m_hGridTheme);
}

LRESULT GridView::OnSetRedraw(BOOL fRedraw)
{
	BOOL oldRedraw = m_fRedrawChanges;

	if(fRedraw)
	{
		m_fRedrawChanges = true;
		UpdateMetrics();
		//ContentChanged();
	}
	else
	{
		m_fRedrawChanges = false;
	}	

	return oldRedraw ? TRUE : FALSE;
}


LRESULT GridView::OnSetFocus()
{
	// force a redraw so that the selection colour changes
	RefreshWindow();
	return 0;
}

LRESULT GridView::OnKillFocus()
{
	if(m_fMouseDown)
	{
		m_fMouseDown = FALSE;
		ReleaseCapture();
	}

	//if(m_hwndEdit)
	//	ExitEditMode();
	
	// force a redraw so that the selection colour changes
	RefreshWindow();
	return 0;
}


UINT GridView::SetStyle(ULONG uMask, ULONG uStyle)
{
	m_uState = (m_uState & ~uMask) | uStyle;
	RefreshWindow();
	return 0;
}


LRESULT GridView::OnSetFont(HFONT hFont, int nFontIdx)
{
	int i;

	if(nFontIdx >= MAX_GRIDVIEW_FONTS)
		return FALSE;

	HDC hdc = GetDC(m_hWnd);

	m_hFont[nFontIdx] = hFont;
	m_nLineHeight = 0;

	for(i = 0; i < MAX_GRIDVIEW_FONTS; i++)
	{
		if(m_hFont[i])
		{
			TEXTMETRIC tm;
			HANDLE hOldFont = SelectObject(hdc, hFont);

			GetTextMetrics(hdc, &tm);
			SelectObject(hdc, hOldFont);

			m_nLineHeight = max((ULONG)tm.tmHeight + 3, m_nLineHeight);
		}
	}
	
	ReleaseDC(m_hWnd, hdc);

	if(m_nLineHeight == 0)
	{

	}
	
	if(nFontIdx == 0)
	{
		SendMessage(m_hWndHeader, WM_SETFONT, (WPARAM)hFont, 0);
		//	SendMessage(m_hWndEdit, WM_SETFONT, (WPARAM)hFont, 0);
	}

	return TRUE;
}

LRESULT GridView::OnHeaderNotify(int nCtrlId, NMHEADER *nmheader)
{
	switch(nmheader->hdr.code)
	{
	case HDN_ENDDRAG:
		RefreshWindow();
		return FALSE;

	case HDN_BEGINTRACK:
		return FALSE;

	case HDN_TRACK: case HDN_ITEMCHANGING:
		UpdateMetrics();
		return FALSE;

	default:
		return 0;
	}
}

LRESULT GridView::OnTooltipNotify(int nCtrlId, NMHDR *hdr)
{
	NMTTDISPINFO *nmdi = (NMTTDISPINFO *)hdr;

	static TCHAR smeg[200];
	POINT pt;
	RECT  rect;
	GVRow *gvrow;
	GVITEM *gvitem;

	switch(hdr->code)
	{
	//case TTN_SHOW:

	case TTN_GETDISPINFO:

		GetCursorPos(&pt);
		ScreenToClient(m_hWnd, &pt);

		gvrow = MouseToItem(pt.x, pt.y, 0, 0, 0, &rect, &gvitem);

		if(gvitem && (gvitem->mask & GVIF_TEXT) && gvitem->pszText && lstrlen(gvitem->pszText) > 10)
		{
			//nmdi->lpszText
			nmdi->lpszText = smeg;
			lstrcpy(smeg, gvitem->pszText);//TEXT("Hello World!!!"));

			pt.x = rect.left;
			pt.y = rect.top;
			ClientToScreen(m_hWnd, &pt);
			SetWindowPos(m_hWndTooltip, 0, pt.x, pt.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
			//SendMessage(m_hWndTooltip, TTM_TRACKPOSITION, 0, MAKELONG(rect.left, rect.top));
		}
		else
		{
			nmdi->lpszText = NULL;
			PostMessage(m_hWndTooltip, TTM_POP, 0, 0);
		}

		return 0;

	default:
		return 0;
	}
}

LRESULT GridView::OnCommand(UINT nCtrlId, UINT uNotifyCode, HWND hwndFrom)
{
	// has the edit control lost focus?
	if(uNotifyCode == EN_KILLFOCUS)
	{
		if(hwndFrom == m_hwndEdit)
		{
			TRACEA("EditLostFocus\n");

			ExitEditMode(FALSE);
			RedrawLine(m_nCurrentLine);
		}
	}

	if(uNotifyCode == CBN_SELENDCANCEL || uNotifyCode == CBN_SELENDOK)
	{
		BOOL fAcceptChange = FALSE;

		if(uNotifyCode == CBN_SELENDOK)
		{
			TCHAR szText[200];
			int idx = (int)SendMessage(hwndFrom, LB_GETCURSEL, 0, 0);
			SendMessage(hwndFrom, LB_GETTEXT, idx, (LPARAM)szText);//(LPARAM)gvip->pszText);
			SetWindowText(m_hwndEdit, szText);

			//GVRow *gvrowp
			/*GVITEM *gvip;
			if(GetRowItem(m_nCurrentLine, m_nCurrentColumn, &gvip))
			{
			SendMessage(hwndFrom, LB_GETTEXT, idx, (LPARAM)gvip->pszText);
				
			}*/

			fAcceptChange = TRUE;
		}

		ExitEditMode(fAcceptChange);
		RedrawLine(m_nCurrentLine);
	}


	if(uNotifyCode == LBN_KILLFOCUS || uNotifyCode == CBN_KILLFOCUS)
	{
	TRACEA("OnCommand %x %x %x\n", nCtrlId, uNotifyCode, hwndFrom);
	}

	return 0;
}

LRESULT GridView::SetImageList(HIMAGELIST hImgList)
{
	m_hImageList = hImgList;
	return 0;
}

BOOL GridView::ExpandItem(GVRow *gvrow, BOOL expand, BOOL recurse)
{
	if(expand)	gvrow->items[0].state |= GVIS_EXPANDED;
	else		gvrow->items[0].state &= ~GVIS_EXPANDED;
		
	UpdateMetrics();
	return TRUE;
}

LRESULT GridView::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	MSG *msgptr;

	switch(msg)
	{
	// Don't paint the background
	case WM_ERASEBKGND:
		return 1;

	case WM_COPY:
		return OnCopy(GetCurRow());

	case WM_CUT:
		return OnCut(GetCurRow());

	case WM_PASTE:
		return OnPaste(GetCurRow(), NULL);

	case WM_CLEAR:
		return OnClear(GetCurRow());

	case WM_SIZE:
		return OnSize((short)LOWORD(lParam), (short)HIWORD(lParam));

	case WM_PAINT:
		return OnPaint();

	case WM_NCPAINT:
		return OnNcPaint((HRGN)wParam);

	case WM_DRAWITEM:
		return SendMessage(GetParent(m_hWnd), msg, wParam, lParam);
		//return OnDrawItem(wParam, (DRAWITEMSTRUCT *)lParam);

	case WM_CTLCOLOREDIT:
		return OnColorEdit((HDC)wParam, (HWND)lParam);


	// header-control notifications
	case WM_NOTIFY:
		
		if(((NMHDR*)lParam)->hwndFrom == m_hWndHeader)
			return OnHeaderNotify((int)wParam, (NMHEADER *)lParam);

		else if(((NMHDR*)lParam)->hwndFrom == m_hWndTooltip)
			return OnTooltipNotify((int)wParam, (NMHDR *)lParam);
		else
			break;

	case WM_COMMAND:
		return OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
	
	case WM_SETFONT:
		return OnSetFont((HFONT)wParam, (int)lParam);

	case WM_SETREDRAW:
		return OnSetRedraw((BOOL)wParam);

	case WM_SETFOCUS:
		return OnSetFocus();

	case WM_KILLFOCUS:
		return OnKillFocus();

	case WM_CANCELMODE:
		// cancelmode - allows anybody else to cancel our item editing
		ExitEditMode(FALSE);
		return 0;

	case WM_MOUSEACTIVATE:
		if(GetFocus() != m_hWnd)
			SetFocus(m_hWnd);

		return MA_ACTIVATE;

	case WM_GETDLGCODE:

		msgptr = (MSG *)lParam;
			
		// we want everything apart from TAB
		if(msgptr && msgptr->message == WM_KEYDOWN && msgptr->wParam == VK_TAB)
			return 0;

		return DLGC_WANTALLKEYS;

	case WM_CHAR:
		return OnChar((UINT)wParam);

	case WM_KEYDOWN:
		return OnKeyDown((UINT)wParam, (UINT)lParam);

	case WM_HSCROLL:
		return OnHScroll(LOWORD(wParam), HIWORD(wParam));

	case WM_VSCROLL:
		return OnVScroll(LOWORD(wParam), HIWORD(wParam));

	case WM_MOUSEWHEEL:
		return OnMouseWheel((short)HIWORD(wParam));

	case WM_LBUTTONDBLCLK:
		return OnLButtonDblClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_LBUTTONDOWN:
		return OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_RBUTTONDOWN:
		return OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_LBUTTONUP:
		return OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_MOUSEMOVE:
		return OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_TIMER:
		return OnTimer((UINT_PTR)wParam);

	case WM_THEMECHANGED:
		if(m_hComboTheme)
			CloseThemeData(m_hComboTheme);

		if(m_hGridTheme)
			CloseThemeData(m_hGridTheme);

		m_hComboTheme = OpenThemeShim(m_hWnd, L"ComboBox");
		m_hGridTheme  = OpenThemeShim(m_hWnd, L"Edit");
		return 0;

	//
	//	User-defined messages!
	//

	//case GVM_SETBKCOLOR:
	//	return gvp->GVSetBkColor(wParam, lParam);

	case GVM_UPDATE:
		UpdateMetrics();
		return 0;

	case GVM_SETSTYLE:
		return SetStyle((UINT)wParam, (UINT)lParam);

	case GVM_INSERTCOLUMN:
		return InsertColumn((int)wParam, (GVCOLUMN *)lParam);

	case GVM_INSERTCHILD:
		return (LRESULT)InsertItem((HGRIDITEM)wParam, GVI_CHILD, (GVITEM *)lParam);

	case GVM_INSERTUNIQUECHILD:
		return (LRESULT)InsertUniqueChild((HGRIDITEM)wParam, (GVITEM *)lParam);

	case GVM_INSERTBEFORE:
		return (LRESULT)InsertItem((HGRIDITEM)wParam, GVI_BEFORE, (GVITEM *)lParam);

	case GVM_INSERTAFTER:
		return (LRESULT)InsertItem((HGRIDITEM)wParam, GVI_AFTER, (GVITEM *)lParam);

	case GVM_SETIMAGELIST:
		return SetImageList((HIMAGELIST)lParam);

	case GVM_SETITEM:
		return m_gvData.SetRowItem((GVRow *)wParam, (GVITEM *)lParam);

	case GVM_GETITEMHANDLE:
		return (LRESULT)m_gvData.GetRow((UINT)wParam);

	case GVM_GETITEM:
		return (LRESULT)m_gvData.GetRowItem((GVRow *)wParam, (GVITEM *)lParam);

	//case GVM_GETITEMTEXT:
	//	return m_gvData.GetItemText(w

	case GVM_GETHEADER:
		return (LRESULT)m_hWndHeader;

	case GVM_GETFIRSTCHILD:

		if(wParam)
		{
			GVRow *gvrow = (GVRow *)wParam;
			return (LRESULT)gvrow->first;
		}
		else
		{
			return (LRESULT)m_gvData.m_gvRoot.first;
		}

	case GVM_GETNEXTSIBLING:

		if(wParam)
		{
			GVRow *gvrow = (GVRow *)wParam;
			return (LRESULT)gvrow->next;
		}
		else
		{
			return (LRESULT)m_gvData.m_gvRoot.first;
		}


	case GVM_GETPARENT:
		
		if(wParam)
		{
			GVRow *gvrow = (GVRow *)wParam;
			
			if(gvrow->parent && gvrow->parent->items)
				return (LRESULT)gvrow->parent;
		}

		return 0;

	case GVM_GETPARENTITEM:
		return (LRESULT)m_gvData.GetRowParentItem((GVRow *)wParam, (GVITEM *)lParam);

	case GVM_DELETECHILDREN:
		return m_gvData.DeleteChildren((GVRow *)wParam);

	case GVM_DELETEITEM:
		return m_gvData.DeleteRow((GVRow *)wParam);

	// find 1st-level child of specified parent
	case GVM_FINDCHILD:
		return (LRESULT)FindChild((GVRow *)wParam, (GVITEM *)lParam, 1);

	// find any child of specified parent
	case GVM_FINDCHILDR:
		return (LRESULT)FindChild((GVRow *)wParam, (GVITEM *)lParam, -1);

	case GVM_EXPANDITEM:
		return (LRESULT)ExpandItem((GVRow *)wParam, LOWORD(lParam), HIWORD(lParam));

	case GVM_GETCURSEL:
		return m_nCurrentLine;

	case GVM_DELETEALL:
		m_nNumLines = 0;
		m_nVScrollPos = 0;
		m_nCurrentLine = 0;
		m_nWindowLines = 0;
		return m_gvData.DeleteAll();

	case GVM_GETCHILDINDEX:
		return m_gvData.GetChildIndex((GVRow *)wParam);

	default:
		break;
	}

	return DefWindowProc(m_hWnd, msg, wParam, lParam);
}

//
//	Window-procedure for the GridView control
//
LRESULT CALLBACK GridViewWndProc32(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GridView *gvp = (GridView *)GetWindowLongPtr(hwnd, 0);

	switch(msg)
	{
	case WM_NCCREATE:

		if((gvp = new GridView(hwnd)) == 0)
			return FALSE;

		SetWindowLongPtr(hwnd, 0, (LONG_PTR)gvp);
		return TRUE;

	case WM_NCDESTROY:
		delete gvp;
		SetWindowLongPtr(hwnd, 0, 0);
		return 0;

	default:
		if(gvp)
			return gvp->WndProc(msg, wParam, lParam);
		else
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}



void InitGridView()
{
	WNDCLASSEX wc;

	INITCOMMONCONTROLSEX icex;

	// classes necessary for header control
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_LISTVIEW_CLASSES;
	
	InitCommonControlsEx(&icex);		//necessary for toolbar, status etc

	//
	// Window class for the client window contained within the parent
	//
	wc.cbSize			= sizeof(wc);
	wc.lpszClassName	= WC_GRIDVIEW;
	wc.hInstance		= GetModuleHandle(0);
	wc.lpfnWndProc	    = GridViewWndProc32;
	wc.hCursor		    = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon			= NULL;
	wc.lpszMenuName	    = NULL;
	wc.hbrBackground	= (HBRUSH)0;
	wc.style			= CS_DBLCLKS;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(GridView *);	//extra for the pointer
	wc.hIconSm		    = NULL;

	RegisterClassEx(&wc);
}

HWND CreateGridView(HWND hwndParent, int id, UINT uStyle, UINT uExStyle)
{
	if(uStyle == 0)
		uStyle = WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_TABSTOP|
		WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL;

	return CreateWindowEx(uExStyle, 
		WC_GRIDVIEW, 0, uStyle, 
		0, 0, 0, 0, hwndParent, (HMENU)(UINT_PTR)id, 0, 0);
}

