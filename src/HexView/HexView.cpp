//
//  HexView.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define STRICT
#define _WIN32_WINNT 0x501
#include <windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"

// delay-load the UXTHEME library
//#pragma comment(lib, "DelayImp.lib")

// set via property pages
//#pragma comment(linker, "/DELAYLOAD:uxtheme.dll")

static HexView * GetHexView(HWND hwndHexView)
{
	return (HexView *)GetWindowLongPtr(hwndHexView, 0);
}

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

size_w HexView::SelectionSize()
{
	if(m_nSelectionStart < m_nSelectionEnd)
		return m_nSelectionEnd - m_nSelectionStart;
	else
		return m_nSelectionStart - m_nSelectionEnd;
}

size_w HexView::SelectionStart()
{
	return min(m_nSelectionStart, m_nSelectionEnd);
}

size_w HexView::SelectionEnd()
{
	return max(m_nSelectionStart, m_nSelectionEnd);
}


LRESULT HexView::NotifyParent(UINT nNotifyCode, NMHDR *optional /* = 0*/)
{
	UINT  nCtrlId = GetWindowLong(m_hWnd, GWL_ID);
	NMHDR nmhdr   = { m_hWnd, nCtrlId, nNotifyCode };
	NMHDR *nmptr  = &nmhdr;  
	
	if(optional)
	{
		nmptr  = optional;
		*nmptr = nmhdr;
	}

	return SendMessage(GetParent(m_hWnd), WM_NOTIFY, (WPARAM)nCtrlId, (LPARAM)nmptr);
}

HexView::HexView(HWND hwnd)	:
//, byte_seq *seq = //new byte_seq(0xffffffff)) :
				//new byte_seq(0x1000000)) :
				 //new byte_seq(0xffffffffffffffff)) :
				 //new byte_seq(0xff+2)) :
//				 new byte_seq(0x1000000000)) :
				//new byte_seq(0x12873)) :

	
	m_hWnd(hwnd),
	m_hwndEdit(0),
	m_pDataSeq(new sequence),
	m_hUserMenu(0),

	//m_nFileLength(seq->length()),//0x3000),

	// 
	m_nAddressDigits(8),
	m_nAddressWidth(8),
	m_nHexPaddingLeft(3),
	m_nHexPaddingRight(3),
	m_nBytesPerLine(16),
	m_nHexWidth(16*3-1),
	m_nTotalWidth(0),
	m_nWindowLines(0),
	m_nWindowColumns(0),

	m_nSearchLen(0),

	m_nBytesPerColumn(2),		//1/2/4/8

	//m_fMouseDown(FALSE),
	m_nSelectionMode(SEL_NONE),
	m_fResizeBar(false),
	m_fResizeAddr(false),
	m_nScrollCounter(0),
	m_nScrollTimer(0),
	m_nScrollMouseRemainder(0),
	m_fCursorAdjustment(FALSE),
	
	//m_fHighlighting(false),
	//m_HighlightFG(RGB(0,0,0)),
	//m_HighlightBG(RGB(255,255,0)),

	m_nHScrollPos(0),
	m_nVScrollPos(0),
	m_nHScrollMax(0),
	m_nVScrollMax(0),

	m_pLastDataObject(0),
	m_lRefCount(1),
	m_nWhichPane(0),
	m_nSelectionStart(0),
	m_nSelectionEnd(0),
	m_nCursorOffset(0),
	m_fStartDrag(false),
	m_fDigDragDrop(false),
	m_nSubItem(0),
	m_fRedrawChanges(true),

	//m_HighlightOrderList(0),
	//m_HighlightSortedList(0),

	m_HighlightCurrent(0),
	m_HighlightHot(0),
	m_HitTestCurrent(0),
	m_HitTestHot(0),
	m_nAddressOffset(0),
	m_nDataShift(0),
	m_nVScrollPinned(0),
	m_nLastEditOffset(0),
	m_fCursorMoved(true)


{
	m_nEditMode			= HVMODE_OVERWRITE;
	m_nControlStyles	= 0;	// styles cleared by default

	//
	//	Initialize the default colour scheme
	//
	m_ColourList[HVC_BACKGROUND]  = COLOR_WINDOW       | HEX_SYS_COLOR;
	m_ColourList[HVC_SELECTION]   = COLOR_HIGHLIGHT    | HEX_SYS_COLOR;
	m_ColourList[HVC_SELECTION2]  = COLOR_HIGHLIGHT    | HEX_SYS_COLOR;
	m_ColourList[HVC_ADDRESS]     = COLOR_WINDOWTEXT   | HEX_SYS_COLOR;
	m_ColourList[HVC_HEXODD]      = RGB(0, 0, 196);
	m_ColourList[HVC_HEXEVEN]     = RGB(0, 0, 128);
	m_ColourList[HVC_HEXODDSEL]   = COLOR_HIGHLIGHTTEXT| HEX_SYS_COLOR;//RGB(255, 255, 0);
	m_ColourList[HVC_HEXODDSEL2]  = COLOR_HIGHLIGHTTEXT| HEX_SYS_COLOR;//RGB(255, 255, 0);
	m_ColourList[HVC_HEXEVENSEL]  = COLOR_HIGHLIGHTTEXT| HEX_SYS_COLOR;//RGB(255, 255, 127);
	m_ColourList[HVC_HEXEVENSEL2] = COLOR_HIGHLIGHTTEXT| HEX_SYS_COLOR;//RGB(255, 255, 127);
	m_ColourList[HVC_ASCII]       = COLOR_WINDOWTEXT   | HEX_SYS_COLOR;
	m_ColourList[HVC_ASCIISEL]    = COLOR_HIGHLIGHTTEXT| HEX_SYS_COLOR;
	m_ColourList[HVC_ASCIISEL2]   = COLOR_HIGHLIGHTTEXT| HEX_SYS_COLOR;
	m_ColourList[HVC_MODIFY]      = RGB(255, 0, 0);
	m_ColourList[HVC_MODIFYSEL]   = RGB(200,60,60);//RGB(0, 255, 255);
	m_ColourList[HVC_MODIFYSEL2]  = RGB(200,50,60);//RGB(0, 255, 255);
	m_ColourList[HVC_BOOKMARK_FG] = RGB(0, 0, 0);
	m_ColourList[HVC_BOOKMARK_BG] = GetSysColor(COLOR_GRADIENTACTIVECAPTION);//RGB(219,233,249);//RGB(255, 255, 127);
	m_ColourList[HVC_BOOKSEL]	  = RGB(255, 255, 127);
	m_ColourList[HVC_RESIZEBAR]   = COLOR_3DFACE | HEX_SYS_COLOR;

	m_ColourList[HVC_MATCHED]		 = RGB(255,200,128);
	m_ColourList[HVC_MATCHEDSEL]	 =  RGB(200,100,60);//0xDBE2D8;//RGB(200,128,100);
	m_ColourList[HVC_MATCHEDSEL2]	  = 0xDBE2D8;//RGB(255,200,128);

	m_ColourList[HVC_SELECTION3]  = 0xdddddd;//COLOR_3DFACE | HEX_SYS_COLOR;
	m_ColourList[HVC_SELECTION4]  = COLOR_WINDOWTEXT| HEX_SYS_COLOR;

	// uncomment for dual-colour selection
	/*m_ColourList[HVC_SELECTION2]  = COLOR_GRADIENTACTIVECAPTION | HEX_SYS_COLOR;
	m_ColourList[HVC_ASCIISEL2]   = COLOR_WINDOWTEXT| HEX_SYS_COLOR;
	m_ColourList[HVC_HEXODDSEL2]  = COLOR_WINDOWTEXT| HEX_SYS_COLOR;
	m_ColourList[HVC_HEXEVENSEL2] = COLOR_WINDOWTEXT| HEX_SYS_COLOR;*/

	m_hTheme = OpenThemeShim(m_hWnd, L"edit");

	OnSetFont((HFONT)GetStockObject(ANSI_FIXED_FONT));

	RecalcPositions();

	SetCaretPos((m_nAddressWidth + m_nHexPaddingLeft) * m_nFontHeight, 0);

	RegisterDropWindow();

	m_szFilePath[0] = '\0';

	ZeroMemory(&m_HighlightGhost, sizeof(m_HighlightGhost));

	m_BookHead = new BOOKNODE;
	m_BookTail = new BOOKNODE;
	m_BookHead->next = m_BookTail;
	m_BookTail->prev = m_BookHead;

}

HexView::~HexView()
{
	CloseFile();

	UnregisterDropWindow();

	m_pDataSeq->release();

	ClearBookmarks();

	if(m_hTheme)
		CloseThemeData(m_hTheme);
}

HMENU HexView::SetContextMenu(HMENU hMenu)
{
	HMENU hOld = m_hUserMenu;
	m_hUserMenu = hMenu;
	return hMenu;
}

LRESULT HexView::OnSetFocus()
{
	//TRACEA("setfocus %d\n", GetTickCount());
	CreateCaret(m_hWnd, NULL, 2, m_nFontHeight);
	RepositionCaret();
	ShowCaret(m_hWnd);
	RefreshWindow();
	return 0;
}

LRESULT HexView::OnKillFocus()
{
	//TRACEA("killfocus\n");
	OnLButtonUp(0, 0, 0);

	HideCaret(m_hWnd);
	DestroyCaret();
	RefreshWindow();
	return 0;
}


bool HexView::CheckStyle(UINT uStyleFlag)
{
	return (m_nControlStyles & uStyleFlag) ? true : false;
}

int HexView::UnitWidth()
{
	static const int unitlook[] = { 2, 3, 3, 8 };
	return unitlook[GetStyleMask(HVS_FORMAT_MASK)];
}

UINT HexView::SetStyle(UINT uMask, UINT uStyles)
{
	UINT uOldStyle = m_nControlStyles;

	m_nControlStyles = (m_nControlStyles & ~uMask) | uStyles;

	SetGrouping(m_nBytesPerColumn);

	// some of the styles could have caused the address column to change
	// so recalc as if the file-length changed.
	RecalcPositions();
	UpdateMetrics();

	RefreshWindow();
	return uOldStyle;
}

BOOL HexView::SetPadding(int left, int right)
{
	left  = max(left, 0);
	right = max(right, 0);
	left  = min(left, 20);
	right = min(right, 20);

	m_nHexPaddingLeft  = left;
	m_nHexPaddingRight = right;

	return 0;
}

UINT HexView::GetStyle(UINT uMask)
{
	return m_nControlStyles;
}

UINT HexView::GetGrouping()
{
	return m_nBytesPerColumn;
}

UINT HexView::SetGrouping(UINT nBytes)
{
	int numcols;
	int unitwidth = UnitWidth();

	if(nBytes < 1 || nBytes >= 32)
		return 0;

	m_nBytesPerColumn = nBytes;

	numcols = m_nBytesPerLine / m_nBytesPerColumn;
			
	if(!CheckStyle(HVS_HEX_INVISIBLE))
	{
		m_nHexWidth = (unitwidth * m_nBytesPerColumn + 1) * numcols - 1;

		// take into account partial columns
		if(m_nBytesPerLine % m_nBytesPerColumn)
			m_nHexWidth += (m_nBytesPerLine % m_nBytesPerColumn) * unitwidth + 1;
	}
	else
	{
		m_nHexWidth = 0;
	}

	m_nTotalWidth =  CalcTotalWidth();
	
	UpdateMetrics();
	RefreshWindow();

	return 0;
}

UINT HexView::GetLineLen()
{
	return m_nBytesPerLine;
}

UINT HexView::SetLineLen(UINT nLineLen)
{
	m_nBytesPerLine = nLineLen;
	RecalcPositions();
	FakeSize();
	return m_nBytesPerLine;
}

VOID HexView::UpdateResizeBarPos()
{
	m_nResizeBarPos = (-m_nHScrollPos * m_nFontWidth+(m_nTotalWidth - 
		m_nBytesPerLine - 1) * m_nFontWidth 
			- ((m_nHexPaddingRight*m_nFontWidth)/2));

	m_nResizeBarPos = -m_nHScrollPos;
	m_nResizeBarPos += CheckStyle(HVS_ADDR_INVISIBLE) ? 0 : m_nAddressWidth;
	m_nResizeBarPos += CheckStyle(HVS_HEX_INVISIBLE)  ? 0 : m_nHexPaddingLeft;
	m_nResizeBarPos += CheckStyle(HVS_HEX_INVISIBLE)  ? 0 : m_nHexWidth;

	if(CheckStyle(HVS_HEX_INVISIBLE) == true)
	{
		m_nResizeBarPos += m_nBytesPerLine;
		m_nResizeBarPos += m_nHexPaddingRight;
	}
	
	m_nResizeBarPos *= m_nFontWidth;
	m_nResizeBarPos += (m_nHexPaddingRight * m_nFontWidth)/2;
}

VOID HexView::RecalcPositions()
{
	RECT rect;
	GetClientRect(m_hWnd, &rect);

	OnLengthChange(m_pDataSeq->size());

	m_nDataShift %= m_nBytesPerLine;
	SetGrouping(m_nBytesPerColumn);

	m_nWindowColumns = min((rect.right-rect.left) / m_nFontWidth, m_nTotalWidth);
	
	UpdateResizeBarPos();

	if(m_nVScrollPos > 0)
		PinToOffset(m_nVScrollPinned);
}

UINT HexView::GetStyleMask(UINT uStyleFlag)
{
	return m_nControlStyles & uStyleFlag;
}

VOID HexView::OnLengthChange(size_w nNewLength)
{
	TCHAR buf[40];

	if(nNewLength % m_nBytesPerLine == 0 && nNewLength > 0)
		nNewLength--;

	if(CheckStyle( HVS_ADDR_DEC ))
		m_nAddressDigits = max(10, _stprintf_s(buf, 40, _T(" %I64u"), (UINT64)nNewLength));
	else
		m_nAddressDigits = max(8, _stprintf_s(buf, 40, _T(" %I64X"), (UINT64)nNewLength));

	m_nAddressWidth = m_nAddressDigits;

	if(CheckStyle( HVS_ADDR_MIDCOLON ) && !CheckStyle( HVS_ADDR_DEC ))
		m_nAddressWidth++;

	if(CheckStyle( HVS_ADDR_ENDCOLON ))
		m_nAddressWidth++;

	// leading space
	m_nAddressWidth++;
}

BOOL HexView::SetFontSpacing(int x, int y)
{
	HDC hdc;
	TEXTMETRIC tm;
	HANDLE hOld;

	hdc  = GetDC(m_hWnd);
	hOld = SelectObject(hdc, m_hFont);

	GetTextMetrics(hdc, &tm);

	//glyphset = (GLYPHSET*)malloc(GetFontUnicodeRanges(hdc, 0));
	//GetFontUnicodeRanges(hdc, glyphset);
	//free(glyphset)

	m_nFontHeight = tm.tmHeight + y;
	m_nFontWidth  = tm.tmAveCharWidth + x;

	SelectObject(hdc, hOld);
	ReleaseDC(m_hWnd, hdc);
	
	return TRUE;
}

LRESULT HexView::OnSetFont(HFONT hFont)
{
	m_hFont = hFont;
	SetFontSpacing(0, 0);
	return 0;
}

size_w HexView::NumFileLines(size_w length)
{
	if(length == 0)
		return 0;

	size_w olen = length + m_nDataShift;

	length = olen / m_nBytesPerLine; 
	
	if(olen % m_nBytesPerLine)
		length++;

	//if((olen + m_nDataShift )% m_nBytesPerLine < m_nDataShift)
		//length++;

	return length;
}

bool HexView::PinToBottomCorner()
{
	bool repos = false;

	if(m_nHScrollPos + m_nWindowColumns > m_nTotalWidth && !CheckStyle(HVS_FITTOWINDOW))
	{
		m_nHScrollPos = m_nTotalWidth - m_nWindowColumns;
		repos = true;
	}

	if(m_nVScrollPos + m_nWindowLines > NumFileLines(m_pDataSeq->size()))
	{
		m_nVScrollPos = NumFileLines(m_pDataSeq->size()) - m_nWindowLines;
		repos = true;
	}

	return repos;
}

// maintain the vertical scrollbar position, such that the 
// offset at the top-left is always 'locked' at the same value.
// this requires that we shift the starting position of the
// document so that the specified offset always locates at the
// top-left of the viewport
void HexView::PinToOffset(size_w offset)
{
	// work out the datashift first of all
	m_nDataShift  = m_nBytesPerLine - offset % m_nBytesPerLine;
	m_nDataShift %= m_nBytesPerLine;

	// now work out the corresponding scrollbar position 
	m_nVScrollPos = (offset + m_nDataShift) / m_nBytesPerLine;
}

LRESULT HexView::OnSize(UINT nFlags, int width, int height)
{
	// fit to window!
	if(CheckStyle(HVS_FITTOWINDOW))
	{
		int logwidth   = width / m_nFontWidth;			//logical width, in chars
		int prevbpl    = m_nBytesPerLine;

		// work out size of hex+ascii parts
		logwidth -= CheckStyle(HVS_ADDR_INVISIBLE) ? 0 : m_nAddressWidth;

		if(CheckStyle(HVS_HEX_INVISIBLE) == true)
		{
			// just ascii
			logwidth -= m_nHexPaddingRight + 1;

			m_nBytesPerLine = logwidth;
		}
		else
		{
			if(CheckStyle(HVS_ASCII_INVISIBLE) == true)
			{
				logwidth -= m_nHexPaddingLeft;

				// just hex
				m_nBytesPerLine = (logwidth * m_nBytesPerColumn) /
					(m_nBytesPerColumn * UnitWidth() + 1);
			}
			else
			{
				logwidth -= m_nHexPaddingLeft + m_nHexPaddingRight;

				// ascii + hex
				m_nBytesPerLine = (logwidth * m_nBytesPerColumn) /
					(m_nBytesPerColumn * UnitWidth() + m_nBytesPerColumn + 1);
			}
		}

		//
		int minunit = CheckStyle(HVS_FORCE_FIXEDCOLS) ? m_nBytesPerColumn : 1;		
		m_nBytesPerLine -= m_nBytesPerLine % m_nBytesPerColumn;

		// keep within legal limits
		m_nBytesPerLine = max(m_nBytesPerLine, minunit);

		// update display if anything has changed
		if(m_nBytesPerLine != prevbpl)
		{
			if(m_nVScrollPos > 0)
				PinToOffset(m_nVScrollPinned);

			m_nHScrollPos = 0;
			RecalcPositions();
			RefreshWindow();
			RepositionCaret();
		}
	}

	m_nWindowLines   = (int)min((unsigned)height / m_nFontHeight, NumFileLines(m_pDataSeq->size()));
	m_nWindowColumns = (int)min(width  / m_nFontWidth, m_nTotalWidth);

	if(PinToBottomCorner())
	{
		RefreshWindow();
		RepositionCaret();
	}
	
	SetupScrollbars();

	return 0;
}

size_w HexView::Size()
{
	return m_pDataSeq->size();
}

VOID HexView::FakeSize()
{
	RECT rect;
	GetClientRect(m_hWnd, &rect);

	OnSize(0, rect.right, rect.bottom);
}

VOID HexView::UpdateMetrics()
{
	FakeSize();
	RefreshWindow();

	RepositionCaret();
}

bool HexView::AllowChange(size_w offset, size_w length, UINT method, BYTE *data /*=0*/, UINT mask /*=0*/)
{
	NMHVCHANGED nmchanging = { { 0,0,0 }, method, mask, offset, length, data };

	UINT result = (UINT)NotifyParent(HVN_CHANGING, (NMHDR *)&nmchanging);
	
	return (result == -1) ? false : true;
}

void HexView::ContentChanged(size_w offset, size_w length, UINT method)
{
	NMHVCHANGED nmchanged = { { 0,0,0 }, method, 0, offset, length, NULL };
	UpdateMetrics();
	ScrollToCaret();
	NotifyParent(HVN_CHANGED, (NMHDR *)&nmchanged);
}

BOOL HexView::SetCurSel(size_w selStart, size_w selEnd)
{
	if(selStart > m_pDataSeq->size() || selEnd > m_pDataSeq->size())
		return FALSE;

	if(selStart == m_nSelectionStart && selEnd == m_nSelectionEnd)
		return FALSE;

	InvalidateRange(m_nSelectionStart, m_nSelectionEnd);

	m_nSelectionStart	= selStart;
	m_nSelectionEnd		= selEnd;
		
	if(m_nCursorOffset != m_nSelectionEnd)
	{
		m_nCursorOffset		= selEnd;
		ScrollToCaret();

		InvalidateRange(selStart, selEnd);
	}

	return TRUE;
}

//
//
//
ULONG WINAPI HexView_SetData(HWND hwnd, size_w offset, BYTE *buf, ULONG len)
{
	HexView *hvp;
	
	if((hvp = GetHexView(hwnd)) == 0)
		return FALSE;

	return hvp->SetData(offset, buf, len);
}

ULONG WINAPI HexView_GetData(HWND hwnd, size_w offset, BYTE *buf, ULONG len)
{
	HexView *hvp;
	
	if((hvp = GetHexView(hwnd)) == 0)
		return FALSE;

	return hvp->GetData(offset, buf, len);

}

ULONG WINAPI HexView_SetCurSel(HWND hwnd, size_w selStart, size_w selEnd)
{
	HexView *hvp;
	
	if((hvp = GetHexView(hwnd)) == 0)
		return FALSE;

	return hvp->SetCurSel(selStart, selEnd);
}

BOOL HexView::SetRedraw(BOOL fRedraw)
{
	bool oldredraw = m_fRedrawChanges;

	if(fRedraw)
	{
		m_fRedrawChanges = true;
		ContentChanged(0, 0, 0);
	}
	else
	{
		m_fRedrawChanges = false;
	}

	return oldredraw ? TRUE : FALSE;
}


ULONG WINAPI HexView_FillData(HWND hwnd, BYTE *buf, ULONG buflen, size_w len)
{
	HexView *hvp;
	
	if((hvp = GetHexView(hwnd)) == 0)
		return FALSE;

	return hvp->FillData(buf, buflen, len);
}

LRESULT HexView::OnSelectAll()
{
	m_nSelectionStart	= 0;
	m_nSelectionEnd		= m_pDataSeq->size();
	m_nCursorOffset		= m_nSelectionEnd;

	if(m_nCursorOffset % m_nBytesPerLine == 0)
		m_fCursorAdjustment = TRUE;

	size_w oldpos = m_nVScrollPos;
	ScrollToCaret();

	if(oldpos == m_nVScrollPos)
		RefreshWindow();
	return 0;
}

LRESULT HexView::OnSetCurPos(size_w pos)
{
	if(pos > m_pDataSeq->size())
		return FALSE;

	if(m_nCursorOffset != pos)
	{
		m_nCursorOffset = pos;

		if(m_nSelectionEnd != m_nSelectionStart)
		{
			m_nSelectionEnd = m_nSelectionStart = pos;
			RefreshWindow();
		}

		ScrollToCaret();
	}

	return TRUE;
}

LRESULT HexView::OnSetSelStart(size_w pos)
{
	if(pos > m_pDataSeq->size())
		return FALSE;

	m_nSelectionStart = pos;
	return TRUE;
}

LRESULT HexView::OnSetSelEnd(size_w pos)
{
	if(pos > m_pDataSeq->size())
		return FALSE;

	m_nSelectionEnd = pos;

	if(m_nCursorOffset != pos)
	{
		m_nCursorOffset = pos;
		ScrollToCaret();
	}

	return TRUE;
}


LRESULT HexView::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	size_w xparam;

	switch(msg)
	{
	case WM_PAINT:
		return OnPaint();

	case WM_NCPAINT:
		return OnNcPaint((HRGN)wParam);
	
	case WM_ERASEBKGND:
		return 1;

	case WM_SETFOCUS:
		return OnSetFocus();

	case WM_KILLFOCUS:
		return OnKillFocus();

	case WM_TIMER:
		return OnTimer((UINT_PTR)wParam);

	case WM_MOUSEACTIVATE:
		return OnMouseActivate((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

	case WM_LBUTTONDOWN:
		return OnLButtonDown((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_LBUTTONDBLCLK:
		return OnLButtonDblClick((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_RBUTTONDOWN:
		OnRButtonDown((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return DefWindowProc(m_hWnd, msg, wParam, lParam);

	case WM_LBUTTONUP:
		return OnLButtonUp((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_MOUSEMOVE:
		return OnMouseMove((UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_MOUSEWHEEL:
		return OnMouseWheel((short)HIWORD(wParam));

	case WM_SETCURSOR:
		return OnSetCursor((UINT)wParam, (UINT)lParam);

	case WM_VSCROLL:
		return OnVScroll(LOWORD(wParam), HIWORD(wParam));

	case WM_HSCROLL:
		return OnHScroll(LOWORD(wParam), HIWORD(wParam));

	case WM_KEYDOWN:
		return OnKeyDown((UINT)wParam, LOWORD(lParam), HIWORD(lParam));

	case WM_CHAR:
		return OnChar((UINT)wParam);

	case WM_INITMENUPOPUP:
		return SendMessage(GetParent(m_hWnd), msg, wParam, lParam);
		//return OnInitMenuPopup((HWND)wParam, (HMENU)wParam, LOWORD(lParam), HIWORD(lParam));;
	case WM_CONTEXTMENU:
		//return DefWindowProc(m_hWnd, msg, wParam, lParam);
		return OnContextMenu((HWND)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam, RGB(245,245,245));
		return 0;

	case WM_COMMAND:
		if(HIWORD(wParam) == EN_KILLFOCUS)
		{
			DestroyWindow(m_hwndEdit);
			m_hwndEdit = 0;
		}
		return 0;

	case WM_COPY:
		return OnCopy();

	case WM_CUT:
		return OnCut();

	case WM_PASTE:
		return OnPaste();

	case WM_CLEAR:
		return OnClear();

	case WM_GETDLGCODE:
		// allow hexview to operate in a dialogbox
		return DLGC_WANTCHARS | DLGC_WANTARROWS; 

	case WM_UNDO: case HVM_UNDO:
		return Undo();

	case HVM_CANUNDO:
		return CanUndo();

	case HVM_REDO:
		return Redo();

	case HVM_CANREDO:
		return CanRedo();

	case HVM_ISREADONLY:
		return m_pDataSeq->isreadonly();

	case WM_SIZE:
		return OnSize((UINT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

	case WM_SETFONT:
		return OnSetFont((HFONT)wParam);

	case WM_SETREDRAW:
		return SetRedraw((BOOL)wParam);

	case WM_THEMECHANGED:
		if(m_hTheme)
			CloseThemeData(m_hTheme);

		m_hTheme = OpenThemeShim(m_hWnd, L"Edit");
		return 0;

	case HVM_CLEAR:
		return ClearFile();

	case HVM_OPENFILE:
		return OpenFile((LPCTSTR)lParam, (UINT)wParam);

	case HVM_SAVEFILE:
		return SaveFile((LPCTSTR)lParam, (UINT)wParam);

	case HVM_INITBUF:
		return InitBuf((const BYTE *)wParam, (size_t)lParam, true, false);

	case HVM_INITBUF_SHARED:
		return InitBuf((const BYTE *)wParam, (size_t)lParam, false, false);

	case HVM_IMPORTFILE:
		return ImportFile((LPCTSTR)lParam, (UINT)wParam);

	case HVM_GETFILESIZE:
		xparam = m_pDataSeq->size();
		if(lParam) *(size_w *)lParam = xparam;
		return (LONG)xparam;

	// set data at current cursor position, don't update cursor
	case HVM_SETDATACUR:
		return (LONG)EnterData((BYTE *)wParam, lParam, true/*false*/, true, false);

	// set data at current cursor position and update cursor
	case HVM_SETDATAADV:
		return (LONG)EnterData((BYTE *)wParam, lParam, true, true, false);

	// get data at current cursor position 
	case HVM_GETDATACUR:
		return GetData(m_nCursorOffset, (BYTE *)wParam, (UINT)lParam);

	// get data at current cursor position and update cursor
	case HVM_GETDATAADV:
		size_t n;
		n = GetData(m_nCursorOffset, (BYTE *)wParam, (UINT)lParam);
		m_nCursorOffset += n;
		return n;
		
	case HVM_GETCURPOS:
		xparam = m_nCursorOffset;
		if(lParam) *(size_w *)lParam = xparam;
		return (LONG)xparam;

	case HVM_GETSELSTART:
		xparam = SelectionStart();
		if(lParam) *(size_w *)lParam = xparam;
		return (LONG)xparam;

	case HVM_GETSELEND:
		xparam = SelectionEnd();
		if(lParam) *(size_w *)lParam = xparam;
		return (LONG)xparam;

	case HVM_SCROLLTO:
		xparam = MAKE_SIZEW(wParam, lParam);

		if(xparam > m_pDataSeq->size())
			return FALSE;

		return ScrollTo(xparam);

	case HVM_SCROLLTOP:
		xparam = MAKE_SIZEW(wParam, lParam);

		if(xparam > m_pDataSeq->size())
			return FALSE;

		return ScrollTop(xparam);

	case HVM_SETCURPOS:
		return OnSetCurPos(MAKE_SIZEW(wParam, lParam));

	case HVM_SETSELSTART:
		return OnSetSelStart(MAKE_SIZEW(wParam, lParam));

	case HVM_SETSELEND:
		return OnSetSelEnd(MAKE_SIZEW(wParam, lParam));

	case HVM_SELECTALL:
		return OnSelectAll();

	case HVM_GETSELSIZE:
		xparam = SelectionSize();
		if(lParam) *(size_w *)lParam = xparam;
		return (LONG)xparam;

	case HVM_SETSTYLE:
		return SetStyle((UINT)wParam, (UINT)lParam);

	case HVM_GETSTYLE:
		return GetStyle((UINT)wParam);

	case HVM_GETEDITMODE:
		return m_nEditMode;

	case HVM_SETEDITMODE:
		if(m_pDataSeq->isreadonly())
			return -1;

		lParam = m_nEditMode;
		m_nEditMode = (UINT)wParam;
		return lParam;
		
	case HVM_SETSEARCHPAT:
		m_nSearchLen = min((UINT)wParam, sizeof(m_pSearchPat));
		memcpy(m_pSearchPat, (BYTE *)lParam, m_nSearchLen);
		return 1;

	case HVM_SETGROUPING:
		return SetGrouping((UINT)wParam);

	case HVM_GETGROUPING:
		return GetGrouping();

	case HVM_SETCOLOR:
		return SetHexColour((UINT)wParam, (COLORREF)lParam);

	case HVM_GETCOLOR:
		return GetHexColour((UINT)wParam);

	case HVM_SETPADDING:
		return SetPadding(LOWORD(lParam), HIWORD(lParam));

	//case HVM_HIGHLIGHT:
	//	return Highlight(wParam);

	case HVM_ADDBOOKMARK:
		return (LRESULT)AddBookmark((BOOKMARK *)lParam);

	case HVM_DELBOOKMARK:
		return DelBookmark((BOOKNODE *)wParam);

	case HVM_CLEARBOOKMARKS:
		return ClearBookmarks();

	case HVM_SETCONTEXTMENU:
		return (LRESULT)SetContextMenu((HMENU)wParam);

	case HVM_GETBOOKMARK:
		return (LRESULT)GetBookmark((BOOKNODE *)wParam, (BOOKMARK *)lParam);

	case HVM_ENUMBOOKMARK:
		return (LRESULT)EnumBookmark((BOOKNODE *)wParam, (BOOKMARK *)lParam);

	case HVM_SETBOOKMARK:
		return SetBookmark((BOOKNODE *)wParam, (BOOKMARK *)lParam);

	case HVM_FORMATDATA:
		return FormatData((HEXFMT_PARAMS *)lParam);

	case HVM_GETLINELEN:
		return GetLineLen();

	case HVM_GETLINECHARS:
		return m_nTotalWidth+1;

	case HVM_SETLINELEN:
		return SetLineLen((UINT)wParam);

	case HVM_FINDINIT:
		return FindInit((BYTE *)lParam, wParam, FALSE, FALSE);

	case HVM_FINDNEXT:
		return FindNext((size_w *)lParam, (UINT)wParam);

	case HVM_FINDPREV:
		return FALSE;

	case HVM_FINDCANCEL:
		return FALSE;

	case HVM_GETFILEHANDLE:
		return (LRESULT)m_pDataSeq->_handle();

	case HVM_GETCURPANE:
		return m_nWhichPane;

	case HVM_SETCURPANE:
		m_nWhichPane = wParam == 0 ? 0 : 1;
		ScrollToCaret();
		return 0;

	case HVM_GETFILENAME:
		
		if(m_szFilePath && m_szFilePath[0])
		{
			lstrcpyn((TCHAR *)lParam, m_szFilePath, (int)wParam);
			return TRUE;
		}
		else
		{
			if(lParam && wParam > 0)
				((TCHAR *)lParam)[0] = '\0';

			return FALSE;
		}

	case HVM_REVERT:
		return RevertFile();

	case HVM_ISDRAGLOOP:
		return m_nSelectionMode == SEL_DRAGDROP ? TRUE : FALSE;

	case HVM_GETCURCOORD:
		{
			int x,y;
			CaretPosFromOffset(m_nCursorOffset, &x, &y);
			//PositionCaret(x, y, m_nWhichPane);
			//, 
			((POINT *)lParam)->x = LogToPhyXCoord(x, m_nWhichPane);
			((POINT *)lParam)->y = y * m_nFontHeight;
		}
		return TRUE;

	case HVM_SETFONTSPACING:
		return SetFontSpacing((short)LOWORD(lParam), (short)HIWORD(lParam));

	case HVM_SETADDROFFSET:
		m_nAddressOffset = MAKE_SIZEW(wParam, lParam);
		return 0;

	case HVM_SETDATASHIFT:
		m_nDataShift = max(0, (int)wParam);//(int)MAKE_SIZEW(wParam, lParam);
		RepositionCaret();
		return 0;


	default:
		return DefWindowProc(m_hWnd, msg, wParam, lParam);
	}
}

LRESULT CALLBACK HexViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HexView *hvp = GetHexView(hwnd);

	switch(msg)
	{
	// First message received by any window - make a new HexView object
	// and store pointer to it in our extra-window-bytes
	case WM_NCCREATE:
		
		if((hvp = new HexView(hwnd)) == 0)
			return FALSE;

		SetWindowLongPtr(hwnd, 0, (LONG_PTR)hvp);
		return TRUE;

	// Last message received by any window - delete the HexView object
	case WM_NCDESTROY:
		delete hvp;
		SetWindowLongPtr(hwnd, 0, 0);
		return 0;

	// Pass everything to the HexView window procedure
	default:
		return hvp ? hvp->WndProc(msg, wParam, lParam) : 0;
	}
}

ATOM InitHexView()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.cbWndExtra    = sizeof(wc);
	wc.style		 = CS_DBLCLKS;
	wc.hCursor       = 0;//LoadCursor(NULL, IDC_IBEAM);
	wc.hInstance     = GetModuleHandle(0);
	wc.lpfnWndProc   = HexViewWndProc;
	wc.lpszClassName = WC_HEXVIEW;

	return RegisterClassEx(&wc);
}


HWND CreateHexView(HWND hwndParent)
{
	InitHexView();
	
	return CreateWindowEx(WS_EX_CLIENTEDGE, WC_HEXVIEW, TEXT(""),
			WS_CHILD|WS_VSCROLL|WS_CLIPCHILDREN,
			0,0,0,0, hwndParent, 0, GetModuleHandle(0), 0);
}
