//
//  GridViewScroll.cpp
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

bool IsKeyPressed(UINT nVirtKey);


bool IsKeyPressed(UINT nVirtKey)
{
	return GetKeyState(nVirtKey) < 0 ? true : false;
}

//
//	Set scrollbar positions and range
//
void GridView::SetupScrollbars()
{
	RECT rect;
	GetClientRect(m_hWnd, &rect);

	//
	//	Set the GridView scrollbars
	//
	SCROLLINFO si = { sizeof(SCROLLINFO) };

	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin  = 0;
    si.nMax  = m_gvData.VisibleRows() - 1;
    si.nPage = m_nWindowLines;
    si.nPos  = m_nVScrollPos;

	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	//
	//	Horizontal scrollbar
	//
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS ;
	si.nPos  = m_nHScrollPos;		// scrollbar thumb position
	si.nPage = m_nHorzScrollUnits;	// number of 'scroll units' in window
	si.nMin  = 0;
	si.nMax  = m_nTotalWidth - 1;	// total number of lines in file

	SetScrollInfo(m_hWnd, SB_HORZ, &si, TRUE);
	MoveWindow(m_hWndHeader, -m_nHScrollPos, 0, rect.right-rect.left + m_nHScrollPos, m_nHeaderHeight-1, TRUE);

	// adjust our interpretation of the max scrollbar range to make
	// range-checking easier. The scrollbars don't use these values, they
	// are for our own use.
	m_nVScrollMax = m_gvData.VisibleRows()   - m_nWindowLines;
	m_nHScrollMax = m_nTotalWidth - m_nHorzScrollUnits;
}

//
//	Ensure that we never scroll off the end of the document
//
bool GridView::PinToBottomCorner()
{
	bool repos = false;

	if(m_nHScrollPos + m_nHorzScrollUnits > m_nTotalWidth)
	{
		m_nHScrollPos = m_nTotalWidth - m_nHorzScrollUnits;
		repos = true;
	}

	if(m_nVScrollPos + m_nWindowLines > m_gvData.VisibleRows())
	{
		m_nVScrollPos = m_gvData.VisibleRows() - m_nWindowLines;
		repos = true;
	}

	return repos;
}

//
//	The window has changed size - update the scrollbars
//
LRESULT GridView::OnSize(int width, int height)
{
	RECT rect;
	WINDOWPOS wp;
	HDLAYOUT hdlayout = { &rect, &wp };
	int i;
	
	//
	//	Resize the header control
	//
	GetClientRect(m_hWnd, &rect);
	Header_Layout(m_hWndHeader, &hdlayout);

	for(i = 0, m_nTotalWidth = 0; i < Header_GetItemCount(m_hWndHeader); i++)
	{
		HDITEM hditem = { HDI_WIDTH };
		Header_GetItem(m_hWndHeader, i, &hditem);
		m_nTotalWidth += hditem.cxy;
	}
	
	m_nHeaderHeight = wp.cy+1;
	//MoveWindow(m_hWndHeader, -m_nHScrollPos, 0, width + m_nHScrollPos, m_nHeaderHeight, TRUE);

	//
	//	Update the scrollbars
	//
 	m_nWindowLines = min((height - m_nHeaderHeight) / m_nLineHeight, m_gvData.VisibleRows()); 
	m_nHorzScrollUnits = min(width, m_nTotalWidth);// / 20;

	if(PinToBottomCorner())
	{
		RefreshWindow();
	}

	SetupScrollbars();
	return 0;
}

void GridView::UpdateMetrics()
{
	RECT rect;
	
	GetClientRect(m_hWnd, &rect);
	
	OnSize(rect.right, rect.bottom);
	RefreshWindow();
}

void GridView::Scroll(int dx, int dy)
{
	ScrollRgn(dx, dy, false);
}

HRGN GridView::ScrollRgn(int dx, int dy, bool fReturnUpdateRgn)
{
	RECT clip;

	GetClientRect(m_hWnd, &clip);

	// scroll up
	if(dy < 0)
	{
		dy = -(int)min((ULONG)-dy, m_nVScrollPos);
		clip.top = -dy * m_nLineHeight;
	}
	// scroll down
	else if(dy > 0)
	{
		dy = min((ULONG)dy, m_nVScrollMax-m_nVScrollPos);
		clip.bottom = (m_nWindowLines -dy) * m_nLineHeight;
	}

	// scroll left
	if(dx < 0)
	{
		dx = -(int)min(-dx, m_nHScrollPos);
		//clip.left = -dx;//m_nFontWidth * 4;
	}
	// scroll right
	else if(dx > 0)
	{
		dx = min((unsigned)dx, (unsigned)m_nHScrollMax-m_nHScrollPos);
		//clip.right = (m_nHorzScrollUnits - dx );//m_nFontWidth ;
	}

	clip.top += m_nHeaderHeight;

	// adjust the scrollbar thumb position
	m_nHScrollPos += dx;
	m_nVScrollPos += dy;

	if(fReturnUpdateRgn == false)
		GetClientRect(m_hWnd, &clip);

	// perform the scroll
	if(dx != 0 || dy != 0)
	{
		// do the scroll!
		ScrollWindowEx(
			m_hWnd, 
			-dx ,					// scale up to pixel coords
			-dy * m_nLineHeight,
			NULL,								// scroll entire window
			&clip,								// clip the non-scrolling part
			0, 
			0, 
			SW_INVALIDATE
			);

		SetupScrollbars();

		if(fReturnUpdateRgn)
		{
			RECT client;

			GetClientRect(m_hWnd, &client);

			//clip.left -= LeftMarginWidth();

			HRGN hrgnClient  = CreateRectRgnIndirect(&client);
			HRGN hrgnUpdate  = CreateRectRgnIndirect(&clip);

			// create a region that represents the area outside the
			// clipping rectangle (i.e. the part that is never scrolled)
			CombineRgn(hrgnUpdate, hrgnClient, hrgnUpdate, RGN_XOR);

			DeleteObject(hrgnClient);

			return hrgnUpdate;
		}
	}

	return NULL;
}

void GridView::ScrollToLine(ULONG lineNo)
{
	//m_nCurrentLine = lineNo;

	if(lineNo < m_nVScrollPos)
		Scroll(0, (lineNo-m_nVScrollPos));
	else if(lineNo >= m_nVScrollPos + m_nWindowLines)
		Scroll(0, lineNo-(m_nVScrollPos+m_nWindowLines-1));
		
}

LONG GetTrackPos32(HWND hwnd, int nBar)
{
	SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
	GetScrollInfo(hwnd, nBar, &si);
	return si.nTrackPos;
}

LRESULT GridView::OnVScroll(UINT nSBCode, UINT uPos)
{
	ULONG oldpos = m_nVScrollPos;

	switch(nSBCode)
	{
	case SB_TOP:
		m_nVScrollPos = 0;
		RefreshWindow();
		break;

	case SB_BOTTOM:
		m_nVScrollPos = m_nVScrollMax;
		RefreshWindow();
		break;

	case SB_LINEUP:
		Scroll(0, -1);
		break;

	case SB_LINEDOWN:
		Scroll(0, 1);
		break;

	case SB_PAGEDOWN:
		Scroll(0, m_nWindowLines);
		break;

	case SB_PAGEUP:
		Scroll(0, -m_nWindowLines);
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:

		m_nVScrollPos = GetTrackPos32(m_hWnd, SB_VERT);
		RefreshWindow();

		break;
	}

	if(oldpos != m_nVScrollPos)
	{
		SetupScrollbars();
		//RepositionCaret();
	}

	return 0;
}

//
//	Horizontal scrollbar support
//
LRESULT GridView::OnHScroll(UINT nSBCode, UINT nPos)
{
	int oldpos = m_nHScrollPos;

	switch(nSBCode)
	{
	case SB_LEFT:
		m_nHScrollPos = 0;
		RefreshWindow();
		break;

	case SB_RIGHT:
		m_nHScrollPos = m_nHScrollMax;
		RefreshWindow();
		break;

	case SB_LINELEFT:
		Scroll(-10, 0);
		break;

	case SB_LINERIGHT:
		Scroll(10, 0);
		break;

	case SB_PAGELEFT:
		Scroll(-m_nHorzScrollUnits, 0);
		break;

	case SB_PAGERIGHT:
		Scroll(m_nHorzScrollUnits, 0);
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:

		m_nHScrollPos = GetTrackPos32(m_hWnd, SB_HORZ);
		RefreshWindow();
		break;
	}

	if(oldpos != m_nHScrollPos)
	{
		SetupScrollbars();
	//	RepositionCaret();
	}

	return 0;
}


LRESULT GridView::OnMouseWheel(int nDelta)
{
#ifndef	SPI_GETWHEELSCROLLLINES	
#define SPI_GETWHEELSCROLLLINES   104
#endif

	if(!IsKeyPressed(VK_SHIFT))
	{
		int nScrollLines;

		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);

		if(nScrollLines <= 1)
			nScrollLines = 3;

		int nScrollAmount = nDelta + m_nScrollMouseRemainder;
		m_nScrollMouseRemainder = nScrollAmount % (120 / nScrollLines);
		Scroll(0, -nScrollAmount * nScrollLines / 120);
		//RepositionCaret();
	}
	
	return 0;
}
