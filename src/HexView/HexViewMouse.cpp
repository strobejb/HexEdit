//
//  HexViewMouse.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define STRICT
#define _WIN32_WINNT 0x500

#include <windows.h>
#include <tchar.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"

HRESULT CreateDropSource(IDropSource **ppDropSource);
HRESULT CreateDataObject(int max, IDataObject **ppDataObject);

void CloseButton(HDC hdc, RECT *rect, RECT *out, BOOL fHover);

#define DRAGBORDER_SIZE 16

bool __inline inrange(size_w offset, size_w start, size_w finish)
{
	return offset >= start && offset < finish ||
		   offset >= finish && offset < start;
}

int HexView::GetLogicalX(int x, int *pane, int *subitem /*= 0*/)
{
	if(pane) *pane = CheckStyle(HVS_HEX_INVISIBLE) ? 1 : 0;
	if(subitem) *subitem = 0;

	// take into account the horizontal scroll position
	x += m_nHScrollPos * m_nFontWidth;

	// round down to mid-character position
	x = (x + (m_nFontWidth * 7) / 16) / m_nFontWidth;

	//TRACEA("xchar: %d: ", x);

	// clicked in address column?
	if(CheckStyle(HVS_ADDR_INVISIBLE) == false)
	{
		if(x < m_nAddressWidth + m_nHexPaddingLeft)
		{
			return 0;
		}
	
		x -= m_nAddressWidth;
	}

	if(CheckStyle(HVS_HEX_INVISIBLE) == false)
	{
		x -= m_nHexPaddingLeft;
	
		// clicked in hex column?
		if(x < m_nHexWidth)
		{
			int unitwidth = UnitWidth();

			int col    = x / (m_nBytesPerColumn * unitwidth + 1);
			int coloff = min(m_nBytesPerColumn-1,(x - col * (m_nBytesPerColumn * unitwidth + 1)) / unitwidth);

			col *= m_nBytesPerColumn;
			col += coloff;

			if(subitem)
				//*subitem = 0;
				*subitem = (x % (m_nBytesPerColumn * unitwidth + 1)) % unitwidth;

			return col;
		}

		x -= m_nHexWidth;// + 1;			// errrm
	
		// clicked in the 1st half of hex-padding-right?
		if(x < m_nHexPaddingRight / 2)
		{
			//TRACEA("%d %d\n", pane?*pane:-1, m_nBytesPerLine);
			return m_nBytesPerLine;
		}
	}
	
	if(CheckStyle(HVS_ASCII_INVISIBLE) == false)
	{
		x -= m_nHexPaddingRight;// - 1;  // errrm

		if(pane) *pane = 1;

		// clicked in ascii column?
		if(x > m_nBytesPerLine)// && !CheckStyle(HVS_ASCII_INVISIBLE))
			x = m_nBytesPerLine;
	}
	else
	{
		x = m_nBytesPerLine;
	}
	
	if(x < 0) 
		x = 0;

	//TRACEA("%d %d\n", pane ? *pane : -1, x);
	return x;
}

int HexView::GetLogicalY(int y)
{
	if(y < 0) y = 0;

	y = y / m_nFontHeight;

	return y;
}

void HexView::CaretPosFromOffset(size_w offset, int *x, int *y)
{
	// take into account any offset/shift in the datasource
	//offset -= m_nDataShift;
	offset += m_nDataShift;

	if(m_fCursorAdjustment && offset > 0)//>= m_nBytesPerLine)
	{
		*y = (int)(offset / m_nBytesPerLine - m_nVScrollPos - 1);
		*x = (int)(m_nBytesPerLine);

		if(*y < 0 && m_nVScrollPos == 0)
			*y = 0;
	}
	else
	{
		*y = (int)(offset / m_nBytesPerLine - m_nVScrollPos);
		*x = (int)(offset % m_nBytesPerLine);
	}

	// if the cursor is well outside the viewport, then hide it
	// because rounding errors could cause it to wrap around
	if(offset / m_nBytesPerLine < m_nVScrollPos)
	{
		*y = -1;
	}
	else if(offset / m_nBytesPerLine - m_nVScrollPos > (unsigned)m_nWindowLines+1)
	{
		*y = m_nWindowLines+1;
	}
}

int HexView::LogToPhyXCoord(int x, int pane)
{
	int offset	  = 0;
	int xpos	  = x;
	int unitwidth = UnitWidth();
	
	switch(pane)
	{
	case 0:		// hex
		
		// if at the end of line need to adjust
		// but only if we are also at end of a "block"
		if((x == m_nBytesPerLine) && (m_nBytesPerLine % m_nBytesPerColumn) == 0)
			offset = -1;

		x = (x * unitwidth) + (x / m_nBytesPerColumn);
		
		x -= m_nHScrollPos;
		x += CheckStyle(HVS_ADDR_INVISIBLE) ? 0 : m_nAddressWidth;
		x += m_nHexPaddingLeft;
		x += offset;
		
		if(m_nSelectionStart < m_nSelectionEnd && offset == 0 &&
			(xpos % m_nBytesPerColumn) == 0 && xpos > 0)
			x--;
		
		return x * m_nFontWidth;
		
	case 1:		// asc
		
		x -= m_nHScrollPos;
		x += CheckStyle(HVS_ADDR_INVISIBLE) ? 0 : m_nAddressWidth;
		x += CheckStyle(HVS_HEX_INVISIBLE) ? 0 : m_nHexPaddingLeft;
		x += CheckStyle(HVS_HEX_INVISIBLE) ? 0 : m_nHexWidth;
		
		x += CheckStyle(HVS_ASCII_INVISIBLE) ? 0 : m_nHexPaddingRight;

		x -= CheckStyle(HVS_ASCII_INVISIBLE) ? xpos : 0;
		
		return x * m_nFontWidth;
		
	default:
		return 0;
	}
}

size_w HexView::OffsetFromPhysCoord(int mx, int my, int *pane, int *lx, int *ly, int *subitem)
{
	size_w offset;

	int x = GetLogicalX(mx, pane, subitem);
	int y = GetLogicalY(my);

	if(y >= m_nWindowLines)   y = m_nWindowLines -1;
	if(x >= m_nWindowColumns) x = m_nWindowColumns -1;
	if(x < 0) x = 0;
	if(y < 0) y = 0;

	size_w adjdocsize = m_pDataSeq->size() + m_nDataShift;

	// need to do this in two stages to avoid overflow
	offset = /*x+*/(y + m_nVScrollPos) * m_nBytesPerLine;
	//offset -= m_nDataShift;
	offset += min((size_w)x, adjdocsize - offset);

	if(offset < m_nDataShift)
	{
		// did we click in the deadspace at start of the file?
		x = m_nDataShift;
	}

//	TRACEA("SMEG: %d\n", offset);

	if(lx) *lx = x;
	if(ly) *ly = y;
	
	// take into account any offset/shift in the datasource
	//int shft = min(m_nDataShift, offset);
	///offset -= min(m_nDataShift, offset);


//	offset -= min(m_nDataShift, offset);

	if(offset >= adjdocsize)
	{
		offset = adjdocsize;
		m_fCursorAdjustment = (offset % m_nBytesPerLine == 0);

		if(lx && ly)
		{
			if(offset == 0)
				*lx = *ly = 0;
			else
				CaretPosFromOffset(offset - m_nDataShift, lx, ly);
		}
	}
	else
	{
		//offset - min(m_nDataShift, offset);
	}

	offset -= min(m_nDataShift, offset);
	//return offset + m_nDataShift;
	return offset ;//- min(m_nDataShift, offset);
}

void HexView::PositionCaret(int x, int y, int pane)
{
	if(m_nLastEditOffset != m_nCursorOffset)
		m_fCursorMoved = true;

	m_fCursorAdjustment = (x == m_nBytesPerLine);

	int physx = LogToPhyXCoord(x, pane);
	int physy = y * m_nFontHeight;

	physx += m_nSubItem * m_nFontWidth;

	SetCaretPos(physx, physy);
	ShowCaret(m_hWnd);
}

void HexView::RepositionCaret()
{
	int x, y;

	CaretPosFromOffset(m_nCursorOffset, &x, &y);
	PositionCaret(x, y, m_nWhichPane);
}

bool HexView::IsOverResizeBar(int x)
{
	const int BARWIDTH = 8;
	return (x / BARWIDTH) == (m_nResizeBarPos / BARWIDTH);
}

UINT HexView::HitTest(int x, int y, RECT *phirc, BOOKNODE **pbnp)// = 0)
{
	const int BARWIDTH = 8;

	if(pbnp)
		*pbnp = 0;
		
	// mouse within the resize bar?
	if(!CheckStyle(HVS_FITTOWINDOW) && CheckStyle(HVS_RESIZEBAR))
	{
				int pos1 = (m_nAddressWidth - m_nHScrollPos) * m_nFontWidth + 
					(m_nHexPaddingLeft * m_nFontWidth) / 2;

		if((x / BARWIDTH) == (m_nResizeBarPos / BARWIDTH))
		{
			return HVHT_RESIZE;
		}
		else if((x / BARWIDTH) == (pos1/BARWIDTH))
		{
			return HVHT_RESIZE0;
		}
	}

	x -= m_nHScrollPos;

	// mouse within the main hex display?
	if(x < m_nWindowColumns * m_nFontWidth + BOOKMARK_XOFFSET - 3)
	{
		size_w curoff = OffsetFromPhysCoord(x, y);
		
		if(inrange(curoff, m_nSelectionStart, m_nSelectionEnd))
			return HVHT_SELECTION;
		else
			return HVHT_MAIN;
	}
	// otherwise it's in the bookmark column
	else
	{
		// ok we clicked to the right of the ascii column, where
		// the bookmarks sometimes live. Did we click on a bookmark,
		// or on deadspace?
		BOOKNODE *bptr;
	
		// what offset?
		size_w offset = (GetLogicalY(y) + m_nVScrollPos) * m_nBytesPerLine;
				
		// find a bookmark that we overlap
		for(bptr = m_BookHead->next; bptr != m_BookTail; bptr = bptr->next)
		{
			BOOKMARK *bookmark = &bptr->bookmark;

			RECT rect;

			// get the bounding rectangle of specified highlight
			if(BookmarkRect(bookmark, &rect))
			{
				POINT pt = { x, y };

				if(phirc)
					*phirc = rect;

				if(pbnp)
					*pbnp = bptr;
				
				if(PtInRect(&rect, pt))
				{
					//TRACEA("Wee %d,%d - %d,%d\n", rect.left,rect.top, rect.right,rect.bottom);

					if(pt.x < rect.left + BOOKMARK_GRIPWIDTH)
						return HVHT_BOOKGRIP;

					if(pt.x >= rect.right-16)
					{
						if(pt.y >= rect.bottom-16)
							return HVHT_BOOKSIZE;

						if(pt.x < rect.right - 3 &&
							pt.y >= rect.top + 4 && pt.y < rect.top + 16)
						{
							return HVHT_BOOKCLOSE;
						}
					}

					rect.left  += BOOKMARK_GRIPWIDTH;
					rect.right -= 16;
					InflateRect(&rect, -3, -3);

					if(PtInRect(&rect, pt))
					{
						if(pt.y < rect.top + 12)
						{
							if(pt.x > rect.right - 150)
								return HVHT_BOOKEDIT3;
							else
								return HVHT_BOOKEDIT1;
						}
						else
						{
							return HVHT_BOOKEDIT2;
						}
					}


					return HVHT_BOOKMARK;
				}
			}

			// does this highlight's starting offset intersect the line the mouse is over?
			if(bookmark->offset >= offset && bookmark->offset + bookmark->length < offset + m_nBytesPerLine*3)
			{
				int width  = 300;
				int height = 75;

				//HiglightRect(


				
			}
		}
	}

	return HVHT_NONE;
}

//
//	WM_MOUSEACTIVATE
//
//	Grab the keyboard input focus 
//	
LRESULT HexView::OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage)
{
	//TRACEA("activate\n");
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);

	if(HitTest(pt.x, pt.y, 0, 0) & HVHT_BOOKMARK)
		return MA_ACTIVATE;

	//if(GetFocus() == m_hwndEdit)
		
	else if(GetFocus() != m_hWnd) 
		SetFocus(m_hWnd);
	
	return MA_ACTIVATE;

//	SetFocus(m_hWnd);
//	return MA_ACTIVATE;
}

LRESULT HexView::OnLButtonDblClick(UINT nFlags, int x, int y)
{
	size_t offset;
	size_w start;
	size_t len, i;

	BYTE buf[256];
	RECT hir;
	UINT ht = HitTest(x, y, &hir, &m_HighlightCurrent);
	m_HighlightHot = m_HighlightCurrent;
	m_HitTestHot   = ht;
	m_HitTestCurrent = ht;

	// if clicked on the resize bar
	if(ht & HVHT_BOOKMARK)//!CheckStyle(HVS_FITTOWINDOW) && CheckStyle(HVS_RESIZEBAR) && IsOverResizeBar(x))
	{
		InvalidateRect(m_hWnd, &hir, FALSE);
		UpdateWindow(m_hWnd);
		return 0;
	}

	if(m_hwndEdit)
		return 0;

	offset = (size_t)min(128, m_nCursorOffset);

	//start  = min(m_nCursorOffset, 128);

	start = 0;
	
	len = m_pDataSeq->render(m_nCursorOffset - offset, buf, 256);

	BYTE under = buf[offset];

	// search forwards through the buffer
	for(i = offset; i < len; i++)
	{
		if(!isalnum(buf[i]))
			break;
	}

	m_nSelectionEnd = m_nCursorOffset - offset + i;

	// search backwards
	for(i = 0; i < offset; i++)
	{
		if(!isalnum(buf[offset - i - 1]))
			break;
	}

	m_nSelectionStart = m_nCursorOffset - i;
	m_nCursorOffset   = m_nSelectionEnd;
	
	NotifyParent(HVN_SELECTION_CHANGE);
	NotifyParent(HVN_CURSOR_CHANGE);	

	RefreshWindow();
	ScrollToCaret();
	
	return 0;
}


static WNDPROC g_oldEditProc;

LRESULT HexView::EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	BOOKNODE *bptr = (BOOKNODE *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg)
	{
	case WM_MOUSEACTIVATE:
		if(GetFocus() != hwnd) 
			SetFocus(hwnd);
	
		return MA_ACTIVATE;

	case WM_SETTEXT:
	case WM_PASTE:
	case WM_KEYDOWN:
	case WM_CHAR:
		ret = CallWindowProc(g_oldEditProc, hwnd, msg, wParam, lParam);

		GetWindowText(hwnd, bptr->bookmark.pszText, 32);

		InvalidateRect(GetParent(hwnd),0,0);
		return ret;

	case WM_MOUSEWHEEL:
		return SendMessage(GetParent(hwnd), msg, wParam, lParam);
	}

	return CallWindowProc(g_oldEditProc, hwnd, msg, wParam, lParam);
}

BOOL HexView::EditBookmark(BOOKNODE *bptr, RECT *hir, bool fEditTitle)
{
	RECT rect = *hir;
	DWORD dwStyle;
	TCHAR *szText;

	HFONT hFont;

	if(fEditTitle)
	{
		dwStyle = WS_CHILD|ES_AUTOHSCROLL;
		szText  = bptr->bookmark.pszTitle;
		hFont   = CreateFont(-12, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Tahoma"));
		rect.bottom = rect.top + 14;
	}
	else
	{
		dwStyle = WS_CHILD|ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL;
		szText  = bptr->bookmark.pszText;
		hFont   = CreateFont(-12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Tahoma"));
	}

	//HighlightRect(hip->hp
	m_hwndEdit = CreateWindowEx(0, TEXT("EDIT"), 
		szText, dwStyle,
		rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
		m_hWnd, 0, 0, 0);

	SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)hFont, 0);
	g_oldEditProc = (WNDPROC)GetWindowLongPtr(m_hwndEdit, GWLP_WNDPROC);
	SetWindowLongPtr(m_hwndEdit, GWLP_USERDATA, (LONG_PTR)bptr);
	SetWindowLongPtr(m_hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
				
	//SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
	ShowWindow(m_hwndEdit, SW_SHOW);
	SetFocus(m_hwndEdit);
	POINT pt;
	GetCursorPos(&pt);
	
	//ScreenToClient(m_hwndEdit, &pt);
	//SendMessage(m_hwndEdit, WM_LBUTTONDOWN, nFlags, MAKELPARAM(pt.x,pt.y));
	INPUT inp = { INPUT_MOUSE, { pt.x, pt.y, 0, MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN }};
	SendInput(1, &inp, sizeof(inp));
	
	return 0;
}



LRESULT HexView::OnLButtonDown(UINT nFlags, int x, int y)
{
	RECT hir;

	UINT ht = HitTest(x, y, &hir, &m_HighlightCurrent);
	m_HighlightHot	 = m_HighlightCurrent;
	m_HitTestCurrent = ht;
	m_HitTestHot	 = ht;

	// if clicked on either resize bar
	if(ht & HVHT_RESIZE)//!CheckStyle(HVS_FITTOWINDOW) && CheckStyle(HVS_RESIZEBAR) && IsOverResizeBar(x))
	{
		if(ht == HVHT_RESIZE)
			m_fResizeBar = true;
		else if(ht == HVHT_RESIZE0)
			m_fResizeAddr = true;

		SetCapture(m_hWnd);
		return 0;
	}	
	// clicked on a bookmark?
	else if(ht & HVHT_BOOKMARK)
	{
		SetCapture(m_hWnd);

		if(ht == HVHT_BOOKCLOSE)
		{
			NMHVBOOKMARK nmbm;
			//nmbm.hp = 
			NotifyParent(HVN_BOOKCLOSE, (NMHDR *)&nmbm);

			RECT clip;
			CloseButton(NULL, &hir, &clip, 0);
			InvalidateRect(m_hWnd, &clip, 0);
			UpdateWindow(m_hWnd);
		}
		else if(ht == HVHT_BOOKEDIT1 || ht == HVHT_BOOKEDIT2)
		{
			hir.left += BOOKMARK_XOFFSET - 7;
			hir.right -= 16;

			if(ht == HVHT_BOOKEDIT2)
			{
				hir.top += 15;
			}
			else
			{
				hir.top += 1;
				hir.right -= 140;
			}

			InflateRect(&hir, -3, -3);

			//if(m_hwndEdit == 0)
			{
				DestroyWindow(m_hwndEdit);
				EditBookmark(m_HighlightCurrent, &hir, ht == HVHT_BOOKEDIT1 ? true : false);
			}
		}
	}
	else
	{
		// convert actual coords to font-based
		m_nSubItem = 0;
		m_nCursorOffset = OffsetFromPhysCoord(x, y, &m_nWhichPane, &x, &y, &m_nSubItem);
		
		// if the mouse is pressed when it is over a selection, 
		// then start a drag-drop as soon as the mouse moves
		if(inrange(m_nCursorOffset, m_nSelectionStart, m_nSelectionEnd))
		{
			m_fStartDrag = true;
			m_fDigDragDrop = false;
			
		}
		else
		{
			m_nSelectionMode = SEL_NORMAL;
			
			// if the shift key is pressed, extend the selection
			if(nFlags & MK_SHIFT)
			{
				InvalidateRange(m_nCursorOffset, m_nSelectionEnd);
				m_nSelectionEnd   = m_nCursorOffset;		
			}
			else
			{
				InvalidateRange(m_nSelectionStart, m_nSelectionEnd);
				m_nSelectionStart = m_nCursorOffset;
				m_nSelectionEnd   = m_nCursorOffset;
			}
		}
		
		PositionCaret(x, y, m_nWhichPane);
		
		NotifyParent(HVN_CURSOR_CHANGE);
		
		UpdateWindow(m_hWnd);
		
		SetCapture(m_hWnd);
	}

	return 0;
}

int ScrollDir(int counter, int distance)
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

LRESULT HexView::OnTimer(UINT_PTR Id)
{
	int dx = 0, dy = 0;

	RECT rect;
	POINT pt;
	
	// find client area, but make it an even no. of lines
	GetClientRect(m_hWnd, &rect);

	if(m_nSelectionMode == SEL_DRAGDROP)
	{
		InflateRect(&rect, -DRAGBORDER_SIZE, -DRAGBORDER_SIZE);
	}
	else
	{
		rect.bottom -= rect.bottom % m_nFontHeight;
		// rect.left += LeftMarginWidth();
	}

	// get the mouse's client-coordinates
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);

	// scrolling up / down??
	if(pt.y < rect.top)					
		dy = ScrollDir(m_nScrollCounter, pt.y - rect.top);

	else if(pt.y >= rect.bottom)	
		dy = ScrollDir(m_nScrollCounter, pt.y - rect.bottom);

	// scrolling left / right?
	if(pt.x < rect.left)					
		dx = ScrollDir(m_nScrollCounter, pt.x - rect.left);

	else if(pt.x > rect.right)		
		dx = ScrollDir(m_nScrollCounter, pt.x - rect.right);

	//
	// Scroll the window but don't update any invalid
	// areas - we will do this manually after we have 
	// repositioned the caret
	//
	HRGN hrgnUpdate = ScrollRgn(dx, dy, true);

	//
	// do the redraw now that the selection offsets are all 
	// pointing to the right places and the scroll positions are valid.
	//
	if(hrgnUpdate != NULL)
	{
		// We perform a "fake" WM_MOUSEMOVE for two reasons:
		//
		// 1. To get the cursor/caret/selection offsets set to the correct place
		//    *before* we redraw (so everything is synchronized correctly)
		//
		// 2. To invalidate any areas due to mouse-movement which won't
		//    get done until the next WM_MOUSEMOVE - and then it would
		//    be too late because we need to redraw *now*
		//
		OnMouseMove(0, pt.x, pt.y);

		// invalidate the area returned by ScrollRegion
		InvalidateRgn(m_hWnd, hrgnUpdate, FALSE);
		DeleteObject(hrgnUpdate);

		// the next time we process WM_PAINT everything 
		// should get drawn correctly!!
		UpdateWindow(m_hWnd);
	}

	/*int x = pt.x, y = pt.y;
	m_nSelectionEnd = OffsetFromPhysCoord(x, y, &pane, &x, &y);	

	if(pane != m_nWhichPane)
	{
		m_nWhichPane = pane;
		if(m_ColourList[HVC_SELECTION] != m_ColourList[HVC_SELECTION2])
			InvalidateRange(m_nSelectionStart, m_nSelectionEnd);
	}

	PositionCaret(x, y, m_nWhichPane);
	NotifyParent(HVN_CURSOR_CHANGE);*/

	// keep track of how many WM_TIMERs we process because
	// we might want to skip the next one
	InterlockedIncrement(&m_nScrollCounter);
	return 0;
}

void HexView::StartDrag()
{
	IDataObject *pDataObject;
	IDropSource *pDropSource;
	DWORD		 dwEffect;
	DWORD		 dwResult;
		
	// Create the IDropSource COM object
	CreateDropSource(&pDropSource);

	// Create an IDataObject that represents the specified range of data
	// in the current document
	CreateDataObject(SelectionStart(), SelectionSize(), &pDataObject);
	
	// Specify which drop-effects we allow based on the current edit mode
	dwEffect = DROPEFFECT_COPY;	

	if(m_nEditMode == HVMODE_INSERT)
		dwEffect |= DROPEFFECT_MOVE;

	//
	//	** ** ** The drag-drop operation starts here! ** ** **
	//
	size_w selstart = SelectionStart();
	size_w sellen   = SelectionSize();
	dwResult = DoDragDrop(pDataObject, pDropSource, dwEffect, &dwEffect);
	
	// success! check to see if we copied/moved data
	if(dwResult == DRAGDROP_S_DROP)
	{
		if(dwEffect & DROPEFFECT_MOVE)
		{
			// remove selection from edit control
			OnClear();
			//m_pDataSeq->erase(selstart, sellen);
			//m_pDataSeq->breakopt();
		}


	}
	// cancelled
	else if(dwResult == DRAGDROP_S_CANCEL)
	{
	}
	
	pDataObject->Release();
	pDropSource->Release();
	
	//ReleaseCapture();
}

TCHAR * CursorFromHittest(UINT ht)
{
	switch(ht)
	{
	case HVHT_NONE:		return IDC_ARROW;
	case HVHT_MAIN:		return IDC_IBEAM;
	case HVHT_SELECTION:return IDC_ARROW;
//	case HVHT_ADDRESS:	return IDC_IBEAM;
//	case HVHT_HEXCOL:	return IDC_IBEAM;
//	case HVHT_ASCCOL:	return IDC_IBEAM;
	case HVHT_RESIZE:	return IDC_SIZEWE;
	case HVHT_RESIZE0:	return IDC_SIZEWE;
	case HVHT_BOOKMARK:	return IDC_ARROW;
	case HVHT_BOOKCLOSE:return IDC_ARROW;
	case HVHT_BOOKSIZE:	return IDC_SIZENWSE;
	case HVHT_BOOKGRIP:	return IDC_SIZEALL;
	case HVHT_BOOKEDIT:	return IDC_IBEAM;
	case HVHT_BOOKEDIT1:return IDC_IBEAM;
	case HVHT_BOOKEDIT2:return IDC_IBEAM;
	case HVHT_BOOKEDIT3:return IDC_ARROW;
	default:			return IDC_ARROW;
	}
}

LRESULT HexView::OnMouseMove(UINT nFlags, int x, int y)
{
	int mx = x, my = y;
	const int SCROLLAMT = 1;
	size_w offset;
	RECT rect;
	RECT client;
	int pane;

	POINT	pt = { mx, my };

	GetClientRect(m_hWnd, &client);
	CopyRect(&rect, &client);
	rect.bottom -= rect.bottom % m_nFontHeight;

	offset = OffsetFromPhysCoord(mx, my, &pane, &x, &y);

	if(m_fStartDrag)
	{
		TRACEA("Starting Drag\n");
		m_fStartDrag = false;

		StartDrag();
		m_fDigDragDrop = true;
		return 0;
	}

	//
	// convert actual coords to font-based
	//
	if(m_nSelectionMode)// == SEL_NORMAL)
	{
		if(pane != m_nWhichPane)
		{
			m_nWhichPane = pane;
			
			if(m_ColourList[HVC_SELECTION] != m_ColourList[HVC_SELECTION2])
				InvalidateRange(m_nSelectionStart, m_nSelectionEnd);
		}

		if(m_nSelectionMode != SEL_DRAGDROP)
		{
			// update display if the cursor moved
			if(m_nCursorOffset != offset)
			{
				m_nCursorOffset = offset;
				m_nSubItem = 0;

				InvalidateRange(m_nCursorOffset, m_nSelectionEnd);
				m_nSelectionEnd = m_nCursorOffset;
				PositionCaret(x, y, m_nWhichPane);

				NotifyParent(HVN_SELECTION_CHANGE);
				NotifyParent(HVN_CURSOR_CHANGE);
			}

			//SetCursor(LoadCursor(NULL, IDC_IBEAM));
		}
		else
		{
			if(m_nCursorOffset != offset)
			{
				m_nCursorOffset = offset;
				m_nSubItem = 0;

				PositionCaret(x, y, m_nWhichPane);

				NotifyParent(HVN_SELECTION_CHANGE);
				NotifyParent(HVN_CURSOR_CHANGE);
			}

			InflateRect(&rect, -DRAGBORDER_SIZE, -DRAGBORDER_SIZE);
		}

		

		// If mouse is within this area, we don't need to scroll
		if(PtInRect(&rect, pt) || 
			m_nSelectionMode == SEL_DRAGDROP && !PtInRect(&client, pt))
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
				m_nScrollTimer = SetTimer(m_hWnd, 1, 30, 0);
			}
		}			
	}
	// Otherwise is the user moving the mouse to resize the vertical pane splitters?
	else if(m_fResizeBar)
	{
		int width     = mx /= m_nFontWidth + m_nHScrollPos;
		int prevbpl	  = m_nBytesPerLine;
		int unitwidth = UnitWidth();

		if(CheckStyle(HVS_HEX_INVISIBLE) == false)
		{
			// work out the size of hex area
			width -= m_nAddressWidth + m_nHexPaddingLeft;
		
			// work out how many bytes-per-line will fit into specified width
			m_nBytesPerLine = (width * m_nBytesPerColumn) / 
						  (m_nBytesPerColumn * (unitwidth) + 1);
		}
		else
		{
			// ascii 
			width -= m_nAddressWidth + m_nHexPaddingRight;
			m_nBytesPerLine = width;
		}

		// force whole-sized columns if necessary
		int minunit = 1;//CheckStyle(HVS_FORCE_FIXEDCOLS) ? m_nBytesPerColumn : 1;

		m_nBytesPerLine -= m_nBytesPerLine % minunit;

		// make sure we stay within legal limits
		m_nBytesPerLine = max(m_nBytesPerLine, minunit);

		//m_nBytesPerLine = min(m_nBytesPerLine, HV_MAX_COLS);

		// update display if anything has changed
		if(prevbpl != m_nBytesPerLine)
		{
			m_fCursorAdjustment = FALSE;

			// maintain the vertical scrollbar position, such that the 
			// offset at the top-left is always 'locked' at the same value.
			// this requires that we 
			if(m_nVScrollPos > 0)
				PinToOffset(m_nVScrollPinned);

			TRACEA("VSP2: %I64x - %d    (%I64x)\n", m_nVScrollPinned, m_nDataShift, m_nVScrollPos * m_nBytesPerLine+ m_nDataShift);
			//SetupScrollbars();
			
			RecalcPositions();
			FakeSize();
			SetupScrollbars();

			if(m_nVScrollPos > m_nVScrollMax)
			{
				m_nVScrollPos = m_nVScrollMax;
				//SetupScrollbars();
				FakeSize();
			}

			RepositionCaret();
			RefreshWindow();
			TRACEA("VSP3: %I64x - %I64x\n", m_nVScrollPos, m_nVScrollPos * m_nBytesPerLine);
		}

		return 0;
	}
	else if(m_fResizeAddr)
	{
		// the resizebar next to the address column is used to
		// set the m_nDataShift value
		int pos1 = (m_nAddressWidth - m_nHScrollPos) * m_nFontWidth + 
					(m_nHexPaddingLeft * m_nFontWidth) / 2;

		int oldds = m_nDataShift;

		int pos = mx / m_nFontWidth + m_nHScrollPos;
		m_nDataShift = (pos - pos1 / m_nFontWidth) / 2;
		
		m_nDataShift = max(0, m_nDataShift);		// don't allow negative
		m_nDataShift = min(m_nDataShift, m_nBytesPerLine-1);

		if(m_nDataShift != oldds)
		{
			TRACEA("addr: %d\n", m_nDataShift);

			RecalcPositions();
			FakeSize();
			SetupScrollbars();
			RepositionCaret();
			RefreshWindow();
		}
		return 0;
	}
	//
	//	Mouse not being held down, just set the cursor
	//
	else 
	{
		UINT lastht = m_HitTestHot;
		BOOKNODE *lasthl = m_HighlightHot;

		RECT hitrc;
		
		m_HitTestHot = HitTest(mx, my, &hitrc, &m_HighlightHot);
		
		//size_w curoff = (y + m_nVScrollPos) * m_nBytesPerLine + x;
		//else if(inrange(curoff, m_nSelectionStart, m_nSelectionEnd))

		if(m_HighlightCurrent == 0)
		{
			SetCursor(LoadCursor(NULL, CursorFromHittest(m_HitTestHot)));
		}
		else
		{

		}

		

		//TRACEA("ht = %d, last=%d\n", ht, m_LastHitTest);

		// over a bookmark's close button?
		//if( (ht != m_LastHitTest && (ht == HVHT_BOOKCLOSE || m_LastHitTest == HVHT_BOOKCLOSE)) || 0
			//(ht != m_LastHitTest && (ht == HVHT_BOOKCLOSE || m_LastHitTest == HVHT_BOOKCLOSE)) ||
		//	)
		if(lasthl != m_HighlightHot || lastht != m_HitTestHot)
		{
			RECT clip;

			if(lasthl != m_HighlightHot && lasthl)
			{
				BookmarkRect(&lasthl->bookmark, &clip);
				InvalidateRect(m_hWnd, &clip, FALSE);
			}

			CloseButton(NULL, &hitrc, &clip, 0);

			//TRACEA("redraw %d,%d-%d,%d!!\n", clip.left,clip.top,clip.right,clip.bottom);
			//RefreshWindow();
			InvalidateRect(m_hWnd, &clip, 0);
			UpdateWindow(m_hWnd);
		}
		
	}

	//if(offset != m_nCursorOffset)
	//	NotifyParent(HVN_CURSOR_CHANGE);

//	UpdateWindow(m_hWnd);

	return 0;
}

LRESULT HexView::OnSetCursor(WPARAM wParam, LPARAM lParam)
{
	if(LOWORD(lParam) == HTCLIENT)
	{
		// don't set the cursor for the client area, 
		// we will do this during WM_MOUSEMOVE
		return TRUE;
	}
	else
	{
		// set the default cursor for any non-client areas
		return DefWindowProc(m_hWnd, WM_SETCURSOR, wParam, lParam);
	}
}

HMENU HexView::CreateContextMenu()
{
	HMENU hMenu = CreatePopupMenu();

	// do we have a selection?
	UINT fSelection = (m_nSelectionStart == m_nSelectionEnd) ?
		MF_DISABLED| MF_GRAYED : MF_ENABLED;

	// is there text on the clipboard?
	UINT fClipboard = IsClipboardFormatAvailable(CF_TEXT) ?
		MF_ENABLED : MF_GRAYED | MF_DISABLED;

	UINT fCanUndo = CanUndo() ? MF_ENABLED : MF_GRAYED | MF_DISABLED;
	UINT fCanRedo = CanRedo() ? MF_ENABLED : MF_GRAYED | MF_DISABLED;

	AppendMenu(hMenu, MF_STRING|fCanUndo,				WM_UNDO, _T("&Undo"));
	AppendMenu(hMenu, MF_STRING|fCanRedo,				HVM_REDO, _T("&Redo"));
	AppendMenu(hMenu, MF_SEPARATOR,						0, 0);
	AppendMenu(hMenu, MF_STRING|fSelection,				WM_CUT,    _T("Cu&t"));
	AppendMenu(hMenu, MF_STRING|fSelection,				WM_COPY,   _T("&Copy"));
	AppendMenu(hMenu, MF_STRING|fClipboard,				WM_PASTE,  _T("&Paste"));
	AppendMenu(hMenu, MF_STRING|fSelection,				WM_CLEAR,  _T("&Delete"));
	AppendMenu(hMenu, MF_SEPARATOR,						0, 0);
	AppendMenu(hMenu, MF_STRING|MF_ENABLED,				HVM_SELECTALL, _T("&Select All"));

	return hMenu;
}

LRESULT HexView::OnContextMenu(HWND hwndParam, int x, int y)
{
	if(m_hUserMenu)
	{
		// if there is a user-specified context-menu, use it
		UINT uCmd = TrackPopupMenu(m_hUserMenu, TPM_RETURNCMD, x, y, 0, m_hWnd, 0);

		if(uCmd != 0)
			PostMessage(GetParent(m_hWnd), WM_COMMAND, MAKEWPARAM(uCmd, 0), (LPARAM)GetParent(m_hWnd));
	}
	else
	{
		// otherwise use the default
		HMENU hMenu = CreateContextMenu();
		UINT  uCmd  = TrackPopupMenu(hMenu, TPM_RETURNCMD, x, y, 0, m_hWnd, 0);

		if(uCmd != 0)
			PostMessage(m_hWnd, uCmd, 0, 0);
	}
	
	return DefWindowProc(m_hWnd, WM_CONTEXTMENU, (WPARAM)hwndParam, MAKELONG(x,y));
}

LRESULT HexView::OnRButtonDown(UINT nFlags, int x, int y)
{
	return 0;

	//m_Highlight->nNumItems = 0;
	/*HIGHLIGHT_PARAM hp = 

	AddHighlight(m_nSelectionStart, m_nSelectionEnd, 
		0,
		GetSysColor(COLOR_HIGHLIGHT)
		//0xCBF2FA
		//RGB(rand(),rand(),rand()),
		//RGB(rand(),rand(),rand())
		);*/

	return 0;

	POINT pt = {x,y};
	//ClientToScreen(m_hWnd, &pt);

	CreateWindowEx(WS_EX_TOOLWINDOW,
		_T("EDIT"), _T("text here"), WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_CHILD|ES_MULTILINE,
		pt.x, pt.y, 300,200,m_hWnd, 0, 0, 0);

	RefreshWindow();
	return 0;
}

static double HuetoRGB(double m1, double m2, double h )
{
	if( h < 0 ) h += 1.0;
	if( h > 1 ) h -= 1.0;

	if( 6.0*h < 1 )
		return (m1+(m2-m1)*h*6.0);
	
	if( 2.0*h < 1 )
		return m2;

	if( 3.0*h < 2.0 )
		return (m1+(m2-m1)*((2.0/3.0)-h)*6.0);

	return m1;
}

COLORREF HSLtoRGB( double H, double S, double L )
{
	double r,g,b;
	double m1, m2;
	
	if(S==0)
	{
		r=g=b=L;
	} 
	else
	{
		if(L <=0.5)
			m2 = L*(1.0+S);
		else
			m2 = L+S-L*S;
		m1 = 2.0*L-m2;

		r = HuetoRGB(m1,m2,H+1.0/3.0);
		g = HuetoRGB(m1,m2,H);
		b = HuetoRGB(m1,m2,H-1.0/3.0);
		
	} 
	
	return RGB((BYTE)(r*255),(BYTE)(g*255),(BYTE)(b*255));
}

LRESULT HexView::OnLButtonUp(UINT nFlags, int x, int y)
{	
	// if a drag-drop operation never got started (mouse didn't move)
	// then clear current selection
	if(m_fStartDrag)
	{
		m_nSelectionStart = m_nSelectionEnd;
		m_fStartDrag = false;
		RefreshWindow();
		OnMouseMove(0, x, y);
	}

	// resize bar?
	if(m_fResizeBar || m_fResizeAddr)
	{
		m_fResizeBar = false;
		m_fResizeAddr = false;
		ReleaseCapture();
	}
	// normal mouse selection
	else if(m_nSelectionMode)
	{
		/*if(m_fHighlighting && m_nSelectionStart != m_nSelectionEnd)
		{
			//static double d = 0;
			//AddHighlight(m_nSelectionStart, m_nSelectionEnd, RGB(0,0,0), HSLtoRGB(d, 0.5, 0.6));
			//d+=0.02; if(d > 1) d = 0;

			AddHighlight(m_nSelectionStart, m_nSelectionEnd, 
				GetHexColour(HVC_BOOKMARK_FG),
				GetHexColour(HVC_BOOKMARK_BG));
			
			m_nSelectionStart = m_nSelectionEnd;
		}*/

	

		// cancel the scroll-timer if it is still running
		if(m_nScrollTimer != 0)
		{
			KillTimer(m_hWnd, m_nScrollTimer);
			m_nScrollTimer = 0;
		}

		if(m_nSelectionMode == SEL_NORMAL)
		{
			ReleaseCapture();
			m_nSelectionMode = SEL_NONE;
		}

		/*if(m_fDigDragDrop == false)
		{
			m_nSelectionStart = m_nSelectionEnd = m_nCursorOffset;
			RefreshWindow();
			m_fDigDragDrop = false;
		}*/

		/*if(m_fHighlighting)
		{
			RefreshWindow();
			UpdateWindow(m_hWnd);
		}*/
	}
	else
	{
		if(m_HighlightCurrent)
		{
			if(m_HitTestHot == HVHT_BOOKCLOSE)
			{
				RECT hir, clip;

				ReleaseCapture();

				CloseButton(NULL, &hir, &clip, 0);
				InvalidateRect(m_hWnd, &clip, 0);
				UpdateWindow(m_hWnd);
			}
		}

		m_HighlightCurrent = 0;
	}

	if(GetCapture() == m_hWnd)
	{
		ReleaseCapture();
	}

	return 0;
}

LRESULT HexView::OnMouseWheel(int nDelta)
{
#ifndef	SPI_GETWHEELSCROLLLINES	
#define SPI_GETWHEELSCROLLLINES   104
#endif

	int nScrollLines;

	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);

	if(nScrollLines == -1 || nScrollLines <= 1)
		nScrollLines = 3;

	int nScrollAmount = nDelta + m_nScrollMouseRemainder;
	m_nScrollMouseRemainder = nScrollAmount % (120 / nScrollLines);
	Scroll(0, -nScrollAmount * nScrollLines / 120);
	RepositionCaret();
	
	return 0;
}

