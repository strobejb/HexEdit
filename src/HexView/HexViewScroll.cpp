//
//  HexViewScroll.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"
#include <limits.h>
#include <float.h>

#define WIN32_SCROLLBAR_MAX 0x7fff

//
//	Return number of leading zero-bits in specified dword
//
static int numlz(size_w num) 
{
	size_w  mask  = ~((size_w)-1 >> 1);
	int		count = 0;

	while((num & mask) == 0)
	{
		count++;
		mask >>= 1;
	}

	return count; 
} 

//
//	Wrapper around SetScrollInfo, performs scaling to allow massive 64bit scroll ranges
//	
BOOL SetScrollInfo64(HWND hwnd, int nBar, int fMask, size_w nMax, size_w nPos, int nPage, BOOL fRedraw)
{
	SCROLLINFO si = { sizeof(si), (UINT)fMask };

	// normal scroll range requires no adjustment
	if(nMax <= WIN32_SCROLLBAR_MAX)
	{
		si.nMin  = (int)0;
		si.nMax  = (int)nMax;
		si.nPage = (int)nPage;
		si.nPos  = (int)nPos;
	}
	// scale the scrollrange down into allowed bounds
	else
	{
		int shift = numlz(nMax);

		si.nMin  = (int)0;
		si.nMax  = (int)WIN32_SCROLLBAR_MAX;
		si.nPage = (int)nPage;
		si.nPos  = (int)((nPos << shift) / ((nMax << shift) / WIN32_SCROLLBAR_MAX));
	}

	SetScrollInfo(hwnd, nBar, &si, fRedraw);
	return TRUE;
}

//
//	Wrapper around GetScrollInfo, returns scrollbar position
//
size_w GetScrollPos64(HWND hwnd, int nBar, int fMask, size_w nMax)
{
	SCROLLINFO si = { sizeof(si), (UINT)fMask | SIF_PAGE};
	size_w     nPos;

	if(!GetScrollInfo(hwnd, nBar, &si))
		return 0;

	nPos = (fMask & SIF_TRACKPOS) ? si.nTrackPos : si.nPos;

	// normal scroll range requires no adjustment
	if(nMax <= WIN32_SCROLLBAR_MAX)
	{
		return nPos;
	}
	// scroll position at the very end
	else if(nPos == WIN32_SCROLLBAR_MAX - si.nPage + 1)
	{
		return nMax - si.nPage + 1;
	}
	// adjust the scroll position to be relative to maximum value
	else
	{
		int shift = numlz(nMax);

		return (((nMax << shift) / WIN32_SCROLLBAR_MAX) * nPos) >> shift;
	}
}

int HexView::CalcTotalWidth()
{
	int width = 0;
	
	width += CheckStyle(HVS_ADDR_INVISIBLE)  ? 0 : m_nAddressWidth;
	width += CheckStyle(HVS_HEX_INVISIBLE)   ? 0 : m_nHexPaddingLeft;
	width += CheckStyle(HVS_HEX_INVISIBLE)   ? 0 : m_nHexWidth;
	width += CheckStyle(HVS_ASCII_INVISIBLE) ? 0 : m_nHexPaddingRight;
	width += CheckStyle(HVS_ASCII_INVISIBLE) ? 0 : m_nBytesPerLine;
	width += 1;

	return width;
}

//
//	Set scrollbar positions and range
//
VOID HexView::SetupScrollbars()
{
	UINT fMask;

	//
	//	Vertical scrollbar
	//
	fMask = CheckStyle(HVS_ALWAYSVSCROLL) ? SIF_DISABLENOSCROLL : 0;
	fMask |= SIF_PAGE | SIF_POS | SIF_RANGE; 

	size_w nMax = NumFileLines(m_pDataSeq->size());
	
	if(nMax > 0)
		nMax -= 1;
	
	SetScrollInfo64(m_hWnd, SB_VERT, fMask, nMax, m_nVScrollPos, m_nWindowLines, TRUE);
	m_nVScrollMax = NumFileLines(m_pDataSeq->size()) - m_nWindowLines;// + 1;

	//
	//	Horizontal scrollbar
	//
	m_nTotalWidth = CalcTotalWidth();

	fMask = CheckStyle(HVS_ALWAYSHSCROLL) ? SIF_DISABLENOSCROLL : 0;
	fMask |= SIF_PAGE | SIF_POS | SIF_RANGE;

	SetScrollInfo64(m_hWnd, SB_HORZ, fMask, m_nTotalWidth - 1, m_nHScrollPos, m_nWindowColumns, TRUE);
	m_nHScrollMax = m_nTotalWidth - m_nWindowColumns;

	UpdateResizeBarPos();
	UpdatePinnedOffset();
}

//
//	Reposition the viewport so the specified offset is at the top of the display
//
BOOL HexView::ScrollTop(size_w offset)
{
	if(offset > m_pDataSeq->size())
		return FALSE;

	PinToOffset(offset);
	m_nVScrollPinned = offset;

	//m_nVScrollPos = min(offset / m_nBytesPerLine, m_nVScrollMax);

	SetupScrollbars();
	RefreshWindow();

	return TRUE;
}

//
//	Scroll to the specified offset - but only if cursor is outside the viewport
//
BOOL HexView::ScrollTo(size_w offset)
{
	bool fRedraw = false;

	if(offset > m_pDataSeq->size())
		return FALSE;

	if(offset / m_nBytesPerLine < m_nVScrollPos)
	{
		m_nVScrollPos = offset / m_nBytesPerLine;
		fRedraw = true;
	}
	else if(offset / m_nBytesPerLine > m_nVScrollPos + m_nWindowLines)
	{
		m_nVScrollPos = offset / m_nBytesPerLine - m_nWindowLines;
		fRedraw = true;
	}

	if(fRedraw)
	{
		SetupScrollbars();
		RefreshWindow();
	}

	return TRUE;
}

//
//	ScrollRgn
//
//	Scrolls the viewport in specified direction. If fReturnUpdateRgn is true, 
//	then a HRGN is returned which holds the client-region that must be redrawn 
//	manually. This region must be deleted by the caller using DeleteObject.
//
//  Otherwise ScrollRgn returns NULL and updates the entire window 
//
HRGN HexView::ScrollRgn(int dx, int dy, bool fReturnUpdateRgn)
{
	RECT clip;

	GetClientRect(m_hWnd, &clip);

	// scroll up
	if(dy < 0)
	{
		dy = -(int)min((size_w)-dy, m_nVScrollPos);
		clip.top = -dy * m_nFontHeight;
	}
	// scroll down
	else if(dy > 0)
	{
		dy = (int)min((size_w)dy, m_nVScrollMax-m_nVScrollPos);
		clip.bottom = (m_nWindowLines - dy) * m_nFontHeight;
	}

	// scroll left
	if(dx < 0)
	{
		dx = -(int)min(-dx, m_nHScrollPos);
		clip.left = -dx * m_nFontWidth * 4;
	}
	// scroll right
	else if(dx > 0)
	{
		dx = (int)min((unsigned)dx, (unsigned)m_nHScrollMax-m_nHScrollPos);
		clip.right = (m_nWindowColumns - dx - 4) * m_nFontWidth;
	}

	// adjust the scroll thumb position
	m_nHScrollPos += dx;
	m_nVScrollPos += dy;

	// ignore clipping rectangle if its a whole-window scroll
	if(fReturnUpdateRgn == false)
		GetClientRect(m_hWnd, &clip);

	// take margin into account
	//clip.left += LeftMarginWidth();

	// extend selection whilst scrolling
	/*if(m_fMouseDown)
	{
		m_nSelectionEnd += dy * m_nBytesPerLine + dx;
		InvalidateRange(m_nCursorOffset, m_nSelectionEnd);
		m_nCursorOffset = m_nSelectionEnd;
	}*/

	// perform the scroll
	if(dx != 0 || dy != 0)
	{
		ScrollWindowEx(
			m_hWnd, 
			-dx * m_nFontWidth, 
			-dy * m_nFontHeight,
			NULL,
			&clip, 
			0, 
			0, 
			SW_INVALIDATE
			);

		SetupScrollbars();

		if(fReturnUpdateRgn)
		{
			RECT client;

			GetClientRect(m_hWnd, &client);

			HRGN hrgnClient  = CreateRectRgnIndirect(&client);
			HRGN hrgnUpdate  = CreateRectRgnIndirect(&clip);

			// create a region that represents the area outside the
			// clipping rectangle (i.e. the part that is never scrolled)
			CombineRgn(hrgnUpdate, hrgnClient, hrgnUpdate, RGN_XOR);

			DeleteObject(hrgnClient);
			return hrgnUpdate;
		}
	}

	/*
	// Update the margin to keep line numbers in sync
	if(dy != 0)
	{
		GetClientRect(m_hWnd, &clip);
		clip.right = LeftMarginWidth();
		InvalidateRect(m_hWnd, &clip, 0);
	}*/	

	return NULL;
}

//
//	Scroll viewport in specified direction
//
VOID HexView::Scroll(int dx, int dy)
{
	// do a "normal" scroll - don't worry about invalid regions,
	// just scroll the whole window 
	ScrollRgn(dx, dy, false);
}

void HexView::UpdatePinnedOffset()
{
	m_nVScrollPinned = m_nVScrollPos * m_nBytesPerLine;
	
	// take the datashift into account!
	m_nVScrollPinned -= m_nDataShift;

}

//
//	Vertical scrollbar support
//
LRESULT HexView::OnVScroll(UINT nSBCode, UINT nPos)
{
	size_w oldpos = m_nVScrollPos;

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

		m_nVScrollPos = GetScrollPos64(m_hWnd, SB_VERT, SIF_TRACKPOS, NumFileLines(m_pDataSeq->size())-1);
		RefreshWindow();

		break;
	}

	if(oldpos != m_nVScrollPos)
	{
		SetupScrollbars();
		RepositionCaret();
	}

	return 0;
}

//
//	Horizontal scrollbar support
//
LRESULT HexView::OnHScroll(UINT nSBCode, UINT nPos)
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
		Scroll(-1, 0);
		break;

	case SB_LINERIGHT:
		Scroll(1, 0);
		break;

	case SB_PAGELEFT:
		Scroll(-m_nWindowColumns, 0);
		break;

	case SB_PAGERIGHT:
		Scroll(m_nWindowColumns, 0);
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:

		m_nHScrollPos = nPos;
		RefreshWindow();
		break;
	}

	if(oldpos != m_nHScrollPos)
	{
		SetupScrollbars();
		RepositionCaret();
	}

	return 0;
}


