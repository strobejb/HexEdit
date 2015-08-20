//
//  GridViewMouse.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "GridViewInternal.h"
#include "trace.h"

#define GHT_NONE		0
#define GHT_TEXT		1
#define GHT_ICON		2
#define GHT_COMBO		4

#define GHT_TREELINE	(0x10)
#define GHT_TREEBUTTON	(0x20)
#define GHT_TREE		(GHT_TREELINE|GHT_TREEBUTTON)

LRESULT RelayEvent(HWND hwnd, UINT uMsg, int x, int y)
{
	MSG msg;
	DWORD pos = GetMessagePos();

	msg.hwnd	= hwnd;
	msg.message = uMsg;
	msg.time	= GetMessageTime();
	msg.wParam	= 0;
	msg.lParam	= MAKELPARAM(x, y);//GetMessagePos();
	msg.pt.x	= x;//(short)LOWORD(pos);//msg.lParam);
	msg.pt.y	= y;//(short)HIWORD(pos);//msg.lParam);

	return SendMessage(hwnd, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

LRESULT GridView::NotifyParent(UINT nCode, HGRIDITEM hItem)
{
	LRESULT    res;
	NMGRIDVIEW nmgv =
	{
		{ m_hWnd, (UINT_PTR)GetWindowLongPtr(m_hWnd, GWL_ID), nCode },
		0,
		hItem,
		{0,0},
		m_nCurrentLine,
		m_nCurrentColumn,
		m_hwndComboLBox
	};	

	m_fInNotify = true;
	res = SendMessage(GetParent(m_hWnd), WM_NOTIFY, nmgv.hdr.idFrom, (LPARAM)&nmgv);
	m_fInNotify = false;
	return res;
}

void GridView::GetActiveClientRect(RECT *rect)
{
	GetClientRect(m_hWnd, rect);
		
	rect->top		+= m_nHeaderHeight;
	rect->bottom	-= (rect->bottom - m_nHeaderHeight) % m_nLineHeight;
}

//
//	return direction to scroll (+ve, -ve or 0) based on 
//  distance of mouse from window edge
//
//	note: counter now redundant, we scroll multiple lines at
//  a time (with a slower timer than before) to achieve
//	variable-speed scrolling
//
static int ScrollDir(int counter, int distance)
{
	if(distance > 48)		return 5;
	if(distance > 16)		return 2;
	if(distance > 3)		return 1;
	if(distance > 0)		return counter % 5 == 0 ? 1 : 0;
	
	if(distance < -48)		return -5;
	if(distance < -16)		return -2;
	if(distance < -3)		return -1;
	if(distance < 0)		return counter % 5 == 0 ? -1 : 0;

	return 0;
}

void GridView::RedrawLine(ULONG lineNo)
{
	RECT rect;

	GetClientRect(m_hWnd, &rect);
	rect.top    = (lineNo - m_nVScrollPos) * m_nLineHeight + m_nHeaderHeight;
	rect.bottom = rect.top + m_nLineHeight;

	rect.top -= m_nLineHeight;
	InvalidateRect(m_hWnd, &rect, 0);
}

BOOL GridView::ToggleRow(GVRow *rowptr, ULONG lineNo)
{
	if(rowptr->HasChildren())
	{
		if(rowptr->items[0].state & GVIS_EXPANDED)
		{
			if(rowptr->IsChild(m_gvData.GetRow(m_nCurrentLine)))
				m_nCurrentLine = lineNo;
			
			rowptr->items[0].state &= ~GVIS_EXPANDED;				
		}
		else
		{
			rowptr->items[0].state |=GVIS_EXPANDED;
		}
		
		UpdateMetrics();
	}
	return TRUE;
}

//
//	MouseToItem
//
//	Return the line+column information from the specified mouse coordinates
//
GVRow *GridView::MouseToItem(int mx, int my, ULONG *pline, ULONG *pcol, ULONG *puPortion, RECT *prect, GVITEM **gvitem)
{
	RECT rect;
	ULONG line = 0, col = 0, portion = GHT_NONE;

	GetActiveClientRect(&rect);

	// clip mouse to edge of window
	if(mx < rect.left)		mx = rect.left;
	if(my < rect.top)		my = rect.top;
	if(my >= rect.bottom)	my = rect.bottom - 1;
	if(mx >= rect.right)	mx = rect.right  - 1;

	// take the head-control into account
	my -= m_nHeaderHeight;
	mx += m_nHScrollPos;

	// work out the visual line-number
	line = my / m_nLineHeight + m_nVScrollPos;

	if(line >= m_gvData.VisibleRows())
		line = m_gvData.VisibleRows()-1;

	if(gvitem) 
		*gvitem = 0;

	GVRow *rowptr = m_gvData.GetRow(line);

	// check each column to see which was clicked in
	for(UINT i = 0; rowptr && i < m_nNumColumns; i++)
	{
		RECT  rect;

		ULONG lidx = Header_OrderToIndex(m_hWndHeader, i);
		Header_GetItemRect(m_hWndHeader, lidx, &rect);

		// have we clicked in this item? perform a hittest to see what
		// part of the item was clicked (i.e. tree/icon/text etc)
		if(mx >= rect.left && mx < rect.right)
		{
			int treepos = rect.left;//0;
			int iconpos = rect.left;//0;
			
			if(prect)
				SetRect(prect, rect.left, line * m_nLineHeight + m_nHeaderHeight, rect.right, (line +1) * m_nLineHeight + m_nHeaderHeight - 1);
			
			// if this is column#0 then it contains the tree-hierarchy graphics
			if(lidx == 0)
			{
				treepos += rowptr->TreeIndent() * LEVEL_WIDTH;
				iconpos  = treepos + LEVEL_WIDTH;

				if(rowptr->HasChildren() == false)
					treepos += LEVEL_WIDTH;

			}
			
			// clicked somewhere in the tree?
			if(mx < treepos)
			{
				portion = GHT_TREELINE;
			}
			else if(mx >= treepos && mx < iconpos)
			{
				portion = GHT_TREEBUTTON;
			}
			// clicked on the icon?
			else if(mx >= iconpos && mx < iconpos + 16)
			{
				portion = GHT_ICON;
			}
			// otherwise it's on the item itself
			else
			{
				portion = GHT_TEXT;
			}
			
			if(gvitem)
				*gvitem = &rowptr->items[lidx];
			
			col	= lidx;
			break;
		}
	}

	if(pcol) *pcol = col;
	if(puPortion) *puPortion = portion;
	if(pline) *pline = line;

	return rowptr;
}

//
//	WM_LBUTTONDOWN
//
//	Set the item position from the specified client coordinates
//
LRESULT GridView::OnLButtonDown(int x, int y)
{
	ULONG lineNo, colIdx, portion;

	GVRow *rowptr;
	RECT rect;

//	RelayEvent(m_hWndTooltip, WM_LBUTTONDOWN, x, y);
	
	if((rowptr = MouseToItem(x, y, &lineNo, &colIdx, &portion, &rect)) == 0)
		return 0;

	if(m_hwndEdit)
		ExitEditMode(FALSE);

	if((portion & GHT_TREEBUTTON) && rowptr)
	{
		ToggleRow(rowptr, lineNo);

		NotifyParent(GVN_ITEMEXPANDED, rowptr);
	}
	else
	{
		// have we clicked again in the already selected item?
		if(m_nCurrentColumn == colIdx && m_nCurrentLine == lineNo)
		{
			if(portion & GHT_TEXT)
			{
				EnterEditMode();
				return 0;
			}
		}
		else
		{
			if(m_nCurrentColumn != colIdx)
			{
				RefreshWindow();
				m_nCurrentColumn = colIdx;
			}
			
			if(m_nCurrentLine != lineNo)
			{
				RedrawLine(m_nCurrentLine);
				RedrawLine(lineNo);
				m_nCurrentLine   = lineNo;
			}

			NotifyParent(GVN_SELCHANGED, rowptr);

			// make an exception - always edit if the combo is clicked on!
			if(portion & GHT_COMBO)
				EnterEditMode();
		}
	
		m_fMouseDown = TRUE;
		SetCapture(m_hWnd);
	}
	return 0;
}

LRESULT GridView::OnRButtonDown(int x, int y)
{
	ULONG lineNo, colIdx, portion;
	GVRow *rowptr;
	
	if((rowptr = MouseToItem(x, y, &lineNo, &colIdx, &portion, 0)) == 0)
		return 0;

	if(m_nCurrentColumn != colIdx || m_nCurrentLine != lineNo)
	{
		//RedrawLine(m_nCurrentLine);
		//RedrawLine(lineNo);

		RefreshWindow();
		m_nCurrentColumn = colIdx;
		m_nCurrentLine   = lineNo;
	}
			
	NotifyParent(GVN_SELCHANGED, rowptr);

	return 0;
}
//
//	WM_LBUTTONDBLCLK
//
//	Double-click handler - expand tree nodes
//
LRESULT GridView::OnLButtonDblClick(int x, int y)
{
	ULONG lineNo, colIdx, portion;

	GVRow *rowptr;
	GVITEM *gvitem;
	GVCOLUMN *gvcol;
	
	// make sure we clicked a valid row/item
	if((rowptr = MouseToItem(x, y, &lineNo, &colIdx, &portion, 0, &gvitem)) == 0 || gvitem == 0)
		return 0;

	if((gvcol = GetColumn(colIdx, 0)) == 0)
		return 0;

	// only collapse/expand on double-clicks if the grid is readonly
	if((portion & (GHT_TREE|GHT_ICON)) || 
		(gvitem->state & GVIS_READONLY) || 
		(gvcol->uState & GVCS_READONLY) ||
		(m_uState & GVS_READONLY))
	{
		ToggleRow(rowptr, lineNo);

		if(!(portion & (GHT_TREE|GHT_ICON)))
			NotifyParent(GVN_DBLCLK, rowptr);		
	}
	// otherwise a double-click should edit the cell
	else
	{
		EnterEditMode();
	}

	return 0;
}

//
//	WM_TIMER handler
//
//	Used to handle mouse-scrolling 
//
LRESULT	GridView::OnTimer(UINT_PTR nTimer)
{
	RECT rect;
	POINT pt;
	int dx = 0, dy = 0;
	
	// get the current client-rectangle
	GetActiveClientRect(&rect);

	// get the mouse's client-coordinates
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);

	//
	// scrolling up/down?
	//
	if(pt.y < rect.top)
		dy = ScrollDir(m_nScrollCounter, pt.y - rect.top);

	else if(pt.y >= rect.bottom)
		dy = ScrollDir(m_nScrollCounter, pt.y - rect.bottom);

	//
	// scrolling left / right?
	//
	if(pt.x < rect.left)					
		dx = ScrollDir(m_nScrollCounter, pt.x - rect.left);

	else if(pt.x > rect.right)		
		dx = ScrollDir(m_nScrollCounter, pt.x - rect.right);


	HRGN hrgnUpdate = ScrollRgn(dx * 10, dy, true);

	if(hrgnUpdate != NULL)
	{
		OnMouseMove(pt.x, pt.y);

		InvalidateRgn(m_hWnd, hrgnUpdate, FALSE);
		DeleteObject(hrgnUpdate);
	
		UpdateWindow(m_hWnd);
	}

	m_nScrollCounter++;

	return 0;
}

//
//	WM_MOUSEMOVE
//
//	Handled captured mouse movement
//
LRESULT GridView::OnMouseMove(int x, int y)
{
	ULONG lineNo, colIdx, portion;
	RECT  rect;
	POINT pt = { x, y };

	GVRow *gvrow = MouseToItem(x, y, &lineNo, &colIdx, &portion, 0);

	if(m_fMouseDown)
	{
		//
		//	Mouse-scrolling: if the mouse is outside the client-area then we 
		//	need to scroll
		//
		GetActiveClientRect(&rect);

		// If mouse is within this area, we don't need to scroll
		if(PtInRect(&rect, pt))
		{
			if(m_nScrollTimer != 0)
			{
				KillTimer(m_hWnd, m_nScrollTimer);
				m_nScrollTimer = 0;
			}
		}
		// If mouse is outside window, start a timer in
		// order to generate regular scrolling intervals
		else 
		{
			if(m_nScrollTimer == 0)
			{
				m_nScrollCounter = 0;
				m_nScrollTimer   = SetTimer(m_hWnd, 1, 30, 0);
			}
		}

		bool changed = false;

		if(m_nCurrentColumn != colIdx)
		{
			RefreshWindow();
			m_nCurrentColumn = colIdx;
			changed = true;
		}

		if(m_nCurrentLine != lineNo)
		{
			RedrawLine(m_nCurrentLine);
			RedrawLine(lineNo);
			m_nCurrentLine = lineNo;
			changed = true;
		}

		if(changed)
			NotifyParent(GVN_SELCHANGED, gvrow);
	}
	else if(gvrow)
	{
	//	GVRow *gvrow = MouseToItem(x, y, &lineNo, &colIdx, &portion, 0);

		// does the item under the mouse have a combobox button?
		if(gvrow->items[colIdx].state & (GVIS_DROPLIST|GVIS_DROPDOWN))
		{
			// ok, is the mouse over the button itself?
			TRACEA("mouse over button\n");
		}

	/*	TOOLINFO ti = { sizeof(ti), 0, m_hWnd, 1 };

		// support the tooltip
		GVRow *gvrow = MouseToItem(x, y, &lineNo, &colIdx, &portion, &ti.rect);

		char f[100];
		wsprintf(f, "[%d,%d] %d %d %d %d\n", x, y, ti.rect.left, ti.rect.top, ti.rect.right, ti.rect.bottom);
		OutputDebugString(f);
		
//		SendMessage(m_hWndTooltip, TTM_DELTOOL, 0, (LPARAM)&ti);
//		SendMessage(m_hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);

		ti.hwnd		= m_hWnd;
		ti.uFlags	= 0;//TTF_IDISHWND;//0;
		ti.uId		= 1;//(ULONG)m_hWnd;//1;
		ti.hinst	= 0;
		ti.lpszText	= f;

	//	SendMessage(m_hWndTooltip, TTM_ACTIVATE, TRUE, 0);
		SendMessage(m_hWndTooltip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
		//RelayEvent(m_hWndTooltip, WM_MOUSEMOVE, x, y);

		SendMessage(m_hWndTooltip, TTM_SETTOOLINFO, 0, (LPARAM)&ti);

		TTHITTESTINFO hti = { m_hWnd, { x, y }, { sizeof(TOOLINFO) } };
		if(SendMessage(m_hWndTooltip, TTM_HITTEST, 0, (LPARAM)&hti))
		{
			OutputDebugString("smeg\n");
		}
		else
		{
			OutputDebugString("head\n");
		}

	
		//SendMessage(m_hWndTooltip, TTM_POPUP, 0, 0);
*/
	}

	return 0;
}

//
//	WM_LBUTTONUP
//
LRESULT GridView::OnLButtonUp(int x, int y)
{
//	RelayEvent(m_hWndTooltip, WM_LBUTTONUP, x, y);

	if(m_fMouseDown)
	{
		// cancel the scroll-timer if it is still running
		if(m_nScrollTimer != 0)
		{
			KillTimer(m_hWnd, m_nScrollTimer);
			m_nScrollTimer = 0;
		}

		ReleaseCapture();
		m_fMouseDown = FALSE;
	}

	return 0;
}

BOOL GridView::GetItemRect(ULONG column, ULONG line, RECT *rect)
{
	// get the current position of the column header
	Header_GetItemRect(m_hWndHeader, column, rect);

	// wo
	rect->top = (line - m_nVScrollPos) * m_nLineHeight + m_nHeaderHeight;
	rect->bottom = rect->top + m_nLineHeight;

	return TRUE;
}


