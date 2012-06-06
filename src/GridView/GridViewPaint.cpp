//
//  GridViewPaint.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <vssym32.h>
#include "GridViewInternal.h"

enum treeLineType
{
	treeLineConnector,
	treeLineTee,
	treeLineEnd,
	treeLineStraight
};

#define TEXT_INDENT_LEFT 6

/*__declspec(dllimport)
COLORREF WINAPI SetDCPenColor(
  HDC hdc,          // handle to DC
  COLORREF crColor  // new pen color
);*/
COLORREF RealizeColour(COLORREF col)
{
	COLORREF result = col;

	if(col & 0x80000000)
		result = GetSysColor(col & 0xff);
	
	return result;
}

void GridView::RefreshWindow()
{
	InvalidateRect(m_hWnd, 0, 0);
}

void PaintRect(HDC hdc, int x, int y, int width, int height, COLORREF col)
{
	RECT rect;
	SetBkColor(hdc, col);
	SetRect(&rect, x, y, x+width, y+height);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
}

void PaintRect(HDC hdc, RECT *rect, COLORREF col)
{
	SetBkColor(hdc, col);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, rect, 0, 0, 0);
}


COLORREF GridView::GetColour(int idx)
{
	return RealizeColour(m_rgbColourList[idx]);
}

bool GridView::CheckStyle(ULONG uStyleMask)
{
	return (m_uState & uStyleMask) ? true : false;
}

static void DrawDottedRect(HDC hdc, int x, int y, int width, int height)
{
	static WORD dotPatternBmp[] = 
	{
		0x00aa, 0x0055, 0x00aa, 0x0055, 
		0x00aa, 0x0055, 0x00aa, 0x0055
	};

	HBITMAP hbm;
	HBRUSH  hbr;
	HANDLE  hbrushOld;

	hbm = CreateBitmap(8, 8, 1, 1, dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hdc, 1, 0, 0);
	hbrushOld = SelectObject(hdc, hbr);

	PatBlt(hdc, x, y, width, height, PATCOPY);
	/*atBlt(hdc, x+border,       y,               width-border,  border,        PATINVERT);
	PatBlt(hdc, x+width-border, y+border,        border,        height-border, PATINVERT);
	PatBlt(hdc, x,              y+height-border, width-border,  border,        PATINVERT);
	PatBlt(hdc, x,              y,               border,        height-border, PATINVERT);*/

	SelectObject(hdc, hbrushOld);
	DeleteObject(hbr);
	DeleteObject(hbm);
}

void HorzLine(HDC hdc, int x1, int y1, int length)
{
	DrawDottedRect(hdc, x1, y1, length, 1);
	//MoveToEx(hdc, x1, y1, &pt);
	//LineTo(hdc, x1 + length, y1);
	//MoveToEx(hdc, pt.x, pt.y, 0);
}

void VertLine(HDC hdc, int x1, int y1, int height)
{
	DrawDottedRect(hdc, x1, y1, 1, height);
	//POINT pt;

//	OffsetDC(hdc, 0, -1);

	//MoveToEx(hdc, x1, y1, &pt);
	//LineTo(hdc, x1, y1 + height);
	//MoveToEx(hdc, pt.x, pt.y, 0);

//	OffsetDC(hdc, 0, 1);
}

int GridView::DrawItem(HDC hdc, GVRow *gvrow, GVITEM *gvitem, GVCOLUMN *gvcol, int x, int y, int width, int height, 
					   bool selected, bool active, bool focussed)
{
//	HPEN hGridPen = CreatePen(0, PS_SOLID, GetSysColor(COLOR_3DFACE));
//	HANDLE hOldPen = SelectObject(hdc, hGridPen);

	RECT rect = { x, y, x + width - CheckStyle(GVS_VERTGRIDLINES), y + height };
	RECT rect2 = { 0 };
	int ox = x;

	if(gvitem->state & GVS_SMEGHEAD)
	{
		rect.right = m_nTotalWidth;
	}
	
	RECT itemRect = rect;
	COLORREF bkcol, fgcol;

	//bool focussed = (GetFocus() == m_hWnd) ? true : false;

	//if(m_hwndEdit)
	//	focussed=true;


	if(selected)
	{
		if(focussed)
		{
			fgcol = GetColour(GVC_HIGHLIGHTTEXT);
			bkcol = GetColour(GVC_HIGHLIGHT);
		}
		else
		{
			fgcol = GetColour(GVC_HIGHLIGHTTEXT2);
			bkcol = GetColour(GVC_HIGHLIGHT2);
		}
	}
	else
	{
		if(gvitem->mask & GVIF_COLOR)
		{
			fgcol = gvitem->color;
			bkcol = GetColour(GVC_BACKGROUND);
		}
		else
		{
			fgcol = GetColour(GVC_FOREGROUND);
			bkcol = GetColour(GVC_BACKGROUND);
		}
	}

	if(CheckStyle(GVS_FULLROWSELECT) && selected)
	{
		PaintRect(hdc, &rect, bkcol);
	}
	else
	{
		PaintRect(hdc, &rect, GetColour(GVC_BACKGROUND));
	}

	if(gvrow)
	{
		int treewidth = DrawTree(hdc, gvrow, x, y, height);
		x += treewidth;
		width -= treewidth;
		rect.left = x;
		//rect.right += x;
	}

		//SetBkColor(hdc, GetColour(GVC_BACKGROUND));
	
	if(gvitem->mask & GVIF_IMAGE)
	{
		DWORD dwDrawIconFlags;
		IMAGELISTDRAWPARAMS imdp = { sizeof(imdp) };
		
		if(selected && (gvcol->uState & GVCS_BLENDIMAGE))
		{
			dwDrawIconFlags = ILD_BLEND50;//ILD_NORMAL;//ILD_BLEND50;
			//SetTextColor(hdc, 0xffffff);
			//SetBkColor(hdc, 0);//GetColour(GVC_HIGHLIGHT));
			//SetTextColor(hdc, 0);//GetColour(GVC_HIGHLIGHT));
			//bkcol = GetColour(GVC_HIGHLIGHT);
			//SetBkColor(hdc, GetColour(GVC_HIGHLIGHT));

			imdp.fStyle = ILD_BLEND50;//dwDrawIconFlags;
		}
		else
		{
			dwDrawIconFlags = ILD_TRANSPARENT;
			imdp.fStyle = 0;//ILD_BLEND50;//dwDrawIconFlags;
		}
/*
		imdp.hdcDst = hdc;
		imdp.himl = m_hImageList;
		imdp.x = x;
		imdp.y = y + (height-16)/2;
		imdp.cx = 0;//16;
		imdp.cy = 0;//16;
		imdp.i = gvitem->iImage;
		imdp.xBitmap = 0;
		imdp.yBitmap = 0;
		
		imdp.rgbBk = CLR_DEFAULT;//RGB(255,0,0);//GetColour(GVC_HIGHLIGHT);
		imdp.rgbFg = 0xff000000;//RGB(255,0,0);//GetColour(GVC_HIGHLIGHT);
		
		
	//	MERGECOPY
		imdp.dwRop = SRCCOPY;//R2_NOTCOPYPEN4;//0;
		imdp.fState = 0;//ILS_SHADOW;//ILS_ALPHA;//SATURATE;
		imdp.Frame = 0;//100;
		imdp.crEffect = 0;// 29;//RGB(255,0,0);//GetColour(GVC_HIGHLIGHT);


		ImageList_DrawIndirect(&imdp);*/

		int imageidx = gvitem->iImage;

		// todo
		if((gvitem->state & GVIS_EXPANDED) && (gvitem->state & GVIS_IMAGEIDX))
			imageidx++;

		ImageList_DrawEx(m_hImageList, imageidx, hdc, x, y + (height - 16)/2, 16, 16, 
			CLR_NONE, bkcol, dwDrawIconFlags);

	//else
		//ImageList_DrawEx(m_hImageList, gvitem->iImage, hdc, x, y + (height - 16)/2, 16, 16, CLR_NONE, CLR_NONE, ILD_TRANSPARENT);// |ILD_NORMAL);
		//ImageList_Draw(m_hImageList, gvitem->iImage, hdc, x, y + (height - 16)/2, ILD_TRANSPARENT);

		rect.left += 16 + 2;
		//x += 16;
		//x += 4;
	}


	//ImageList_DrawEx(m_hImageList, gvitem->iImage, hdc, x, y, 16, 16, CLR_NONE, CLR_NONE, ILD_NORMAL);

	//if(gvitem->pszText)
	//	ExtTextOut(hdc, x+4+LEVEL_WIDTH, y, ETO_OPAQUE, &rect, gvitem->pszText, lstrlen(gvitem->pszText), 0);
	//else

	//RECT smeg = rect;
	//smeg.right = smeg.left + 20;
//	COLORREF d = GetBkColor(hdc);//, 0xffffff);
	//ExtTextOut(hdc, x+4+LEVEL_WIDTH, y, ETO_OPAQUE, &rect, 0, 0, 0);
	//SetBkColor(hdc, d);
	//rect.left += 20;
//	ExtTextOut(hdc, x+4+LEVEL_WIDTH, y, ETO_OPAQUE, &rect, 0, 0, 0);
	//rect.left -= 20;

	if(selected)
		PaintRect(hdc, &rect, bkcol);

	if(gvitem->pszText)
	{
		DWORD dwDrawTextFlags = DT_SINGLELINE|DT_EXPANDTABS;

		if(gvcol->uState & GVCS_ELLIPSIS)
			dwDrawTextFlags |= DT_END_ELLIPSIS;

		if(gvcol->uState & GVCS_ALIGN_RIGHT)
			dwDrawTextFlags |= DT_RIGHT;

		else if(gvcol->uState & GVCS_ALIGN_CENTER)
			dwDrawTextFlags |= DT_CENTER;

		SetTextColor(hdc, fgcol);
		rect.left += TEXT_INDENT_LEFT;
		rect.right -= 4;

		CopyRect(&rect2, &rect);
		rect2.top++;

		DrawText(hdc, gvitem->pszText, -1, &rect2, dwDrawTextFlags);
		DrawText(hdc, gvitem->pszText, -1, &rect2, dwDrawTextFlags|DT_CALCRECT);
	}

	
	if(gvitem->state & GVIS_DROPDOWN)
	{
		//rect
		rect.right += 4;
		rect.left  = rect.right - GetSystemMetrics(SM_CXVSCROLL);
		//rect.bottom   ++;

		if(rect.left > x)// && m_hwndEdit == 0)
		{
			if(m_hComboTheme)
			{
				//InflateRect(&rect, -3, 0);//-1);
				rect.left += 3;
				//FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));
				DrawThemeBackground(m_hComboTheme, hdc, 
					CP_DROPDOWNBUTTON, 
				//	6,
					m_hwndComboLBox && active? CBXS_PRESSED:CBXS_NORMAL, &rect, 0);
			}
			else
			{
				DrawFrameControl(hdc, &rect, DFC_SCROLL, DFCS_SCROLLDOWN | (m_hwndComboLBox && active ? DFCS_FLAT|DFCS_PUSHED : 0));
			}
		}
	}
	
	// draw the vertical seperator
	if(CheckStyle(GVS_VERTGRIDLINES) && (gvrow && (gvrow->items[0].state & GVS_SMEGHEAD)) == 0)
	{
		MoveToEx(hdc, x+width-1, y, 0);
		LineTo(hdc, x+width-1, y + height);
	}

	if(focussed && selected && active && CheckStyle(GVS_SHOWFOCUS))
	{
		//itemRect.left--;
		//itemRect.right++;
		//InflateRect(&itemRect, 1,1 );
		//itemRect.top--;
		DrawFocusRect(hdc, &itemRect);
	}

//	SelectObject(hdc, hOldPen);
//	DeleteObject(hGridPen);
	return (rect2.left - ox) + (rect2.right - rect2.left);
}

void GridView::DrawTreeBox(HDC hdc, RECT *rect, bool plus)
{
	int boxsize = 9;

	int x = rect->left + LINEOFFX - boxsize/2;
	int y = rect->top + (rect->bottom - rect->top) / 2 - boxsize / 2;

	HANDLE hOldPen = SelectObject(hdc, GetStockObject(DC_PEN));
	COLORREF old = SetDCPenColor(hdc, GetColour(GVC_TREEBUT_BORDER));

	// outer rectangle
	Rectangle(hdc, x, y, x + boxsize, y + boxsize);

	SetDCPenColor(hdc, GetColour(GVC_TREEBUT_GLYPH));

	// vertical part of the cross
	if(plus)
	{
		MoveToEx(hdc, x + boxsize/2, y + 2, 0);
		LineTo(hdc,   x + boxsize/2, y + boxsize-2);
	}

	// horizontal part of the cross
	MoveToEx(hdc, x + 2, y + boxsize/2, 0);
	LineTo(hdc,   x + boxsize-2, y + boxsize/2);

	SetDCPenColor(hdc, old);
	SelectObject(hdc, hOldPen);
}

void DrawTreeLine(HDC hdc, RECT *rect, int type)
{
	int x = rect->left + LINEOFFX;
	int y = rect->top;

	int height = rect->bottom-rect->top;

//	LOGBRUSH lb = { BS_SOLID, GetSysColor(COLOR_3DSHADOW), 0 };
//	HPEN hPen = ExtCreatePen(PS_COSMETIC| PS_ALTERNATE, 1, &lb, 0, 0);
//	HANDLE hOldPen = SelectObject(hdc, hPen);

	COLORREF crold = SetTextColor(hdc, GetSysColor(COLOR_3DSHADOW));
	//SetBkColor(hdc, COLOR_3DSHADOW);
	switch(type)
	{
	case 0:	// |-
		VertLine(hdc, x, y, height+1);
		HorzLine(hdc, x, y+height/2, LEVEL_WIDTH-LINEOFFX);
		/*MoveToEx(hdc, x, y, 0);
		LineTo(hdc, x, y + height);
		MoveToEx(hdc, x, y + height/2, 0);
		LineTo(hdc, x+LEVEL_WIDTH-LINEOFFX, y + height/2);*/
		break;

	case 1:	// L
		VertLine(hdc, x, y, height/2+1);
		HorzLine(hdc, x, y+height/2, LEVEL_WIDTH-LINEOFFX);

		//MoveToEx(hdc, x, y, 0);
		//LineTo(hdc, x, y + height/2);
		//LineTo(hdc, x+LEVEL_WIDTH-LINEOFFX, y + height/2);
		break;

	case 2:	// |
		VertLine(hdc, x, y, height+1);
		//MoveToEx(hdc, x, y, 0);
		//LineTo(hdc, x, y + height);
		break;

	case 3:	// -
		HorzLine(hdc, x, y+height/2, LEVEL_WIDTH-LINEOFFX);
		break;

	}

	SetTextColor(hdc, crold);
//	SelectObject(hdc, hOldPen);
//	DeleteObject(hPen);
}

int GridView::DrawTree(HDC hdc, GVRow *rowptr, int x, int y, int height)
{
	RECT rect = { x, y, x + LEVEL_WIDTH, y + height };

	//ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);

	GVRow *parent = rowptr->parent;

	int elemx = (rowptr->TreeIndent() + 1) * LEVEL_WIDTH + 4;
	//rect.left  = elemx;
	rect.right = x + elemx;
	//FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));

	//COLORREF b = SetBkColor(hdc, GetColour(GVC_BACKGROUND));
	//ExtTextOut(hdc, 0,0,ETO_OPAQUE,&rect,0,0,0);
	SetRect(&rect, x+elemx - LEVEL_WIDTH - 4, y, x+elemx - 4, y + height);
	//SetBkColor(hdc, b);

	if(CheckStyle(GVS_TREELINES))
	{
		//rect.left  = elemx;
		// draw a '|-' if there is this item has a sibling
		if(rowptr->HasSibling())
		{
			DrawTreeLine(hdc, &rect, 0);
		}
		// draw a 'L' for the the last item in this branch
		else
		{
			DrawTreeLine(hdc, &rect, 1);
		}
	}

	// if this node have any children, we need to draw a [+] or [-] box
	if(rowptr->HasChildren())
	{
		// if expanded, draw a [-]   (so the user can close up this branch)
		if(rowptr->Expanded())
		{
			DrawTreeBox(hdc, &rect, false);
		}
		// otherwise draw a [+]
		else
		{
			DrawTreeBox(hdc, &rect, true);
		}
	}

	OffsetRect(&rect, -LEVEL_WIDTH, 0);

	if(CheckStyle(GVS_TREELINES))
	{
		// now draw the vertical connector bars for each level in the hierarchy
		while(parent)
		{
			// if this item's parent doesn't have a sibling at the same level, then draw a connector
			if(parent->HasSibling())
			{
				DrawTreeLine(hdc, &rect, 2);
			}
			else
			{
				//DrawTreeLine(hdc, &rect, 3);
			}
			
			OffsetRect(&rect, -LEVEL_WIDTH, 0);
			
			parent = parent->parent;
		}
	}

	return (rowptr->TreeIndent() + 1) * LEVEL_WIDTH;
}

int GridView::DrawRow(HDC hdc, GVRow *rowptr, int x, int y, int height, ULONG colidx, ULONG rowidx)
{
	ULONG i;
	int xoff = 0;
	bool focussed = GetFocus() == m_hWnd ? true : false;

	if(CheckStyle(GVS_GRIDLINES))
	{
	}

	int itemwidth = 0;

	for(i = 0; i < m_nNumColumns; i++)
	{
		// convert visual from visual to logical index
		ULONG lidx = Header_OrderToIndex(m_hWndHeader, i);
		HDITEM hditem = { HDI_WIDTH | HDI_LPARAM }; 
		Header_GetItem(m_hWndHeader, lidx, &hditem);

		GVCOLUMN *gvcol = (GVCOLUMN *)hditem.lParam;
		
		SelectObject(hdc, GetFont(&rowptr->items[lidx], gvcol));

		bool selected = (rowidx == m_nCurrentLine) ? true : false;
		bool active = selected && lidx == m_nCurrentColumn;

		if(lidx == m_nCurrentColumn && m_hwndEdit) 
			selected=false;

		if(lidx > 0 && rowptr->items[0].state & GVS_SMEGHEAD)
		{
			goto oof;
		}	


		itemwidth = DrawItem(hdc, lidx == 0 ? rowptr : 0, &rowptr->items[lidx], gvcol, x+xoff, y, hditem.cxy-xoff, height, 
			selected, active, focussed);

oof:
		// draw the vertical seperator
		if(CheckStyle(GVS_VERTGRIDLINES))// && (rowptr->items[0].state & GVS_SMEGHEAD)) == 0)
		{
			int xx = x+hditem.cxy-xoff-1;

			if((rowptr->items[0].state & GVS_SMEGHEAD) == 0 ||
				(rowptr->items[0].state & GVS_SMEGHEAD) != 0 && xx > itemwidth)
			{
				MoveToEx(hdc, xx+xoff, y, 0);
				LineTo(hdc, xx+xoff, y + height);
			}
		}

		x += hditem.cxy;
	}

//	SetBkColor(hdc, GetColour(GVC_BACKGROUND));


	RECT rect;
	
	if(0)//CheckStyle(GVS_SHOWFOCUS))
	{
		rect.top    = y;
		rect.bottom = y + height;
		rect.left   = 0;//8+treex-4;
		rect.right	= x;
		
		if(m_nCurrentLine == rowidx && GetFocus() == m_hWnd)
			DrawFocusRect(hdc, &rect);
	}

	// fill any 'whitespace' to the right
	GetClientRect(m_hWnd, &rect);
	rect.top    = y;
	rect.bottom = y + height;
	rect.left = x;

	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);

	return 0;
}

LRESULT GridView::OnPaint()
{
	PAINTSTRUCT ps;

	BeginPaint(m_hWnd, &ps);

	RECT rect;
	GetClientRect(m_hWnd, &rect);

	GVRow * rowptr;
	ULONG	rowidx;
	int x = 0, y = m_nHeaderHeight;

	//m_rgbColourList[GVC_BACKGROUND] = rand();

	SelectObject(ps.hdc, m_hFont[0]);

	// create an off-screen DC for all drawing. The mem-DC is the same width+height
	// as a single line in the grid. All drawing goes to (0,0) within the memDC then
	// is blitted to the screen DC on completion
	HBITMAP hbmMem = CreateCompatibleBitmap(ps.hdc, rect.right, m_nLineHeight);
	HDC		hdcMem = CreateCompatibleDC(ps.hdc);
	HANDLE  hbmOld = SelectObject(hdcMem, hbmMem);

	bool focussed=(GetFocus() == m_hWnd)?true:false;
	//if(m_hwndEdit)
	//	focussed=true;
	
	HPEN hPen		= CreatePen(0, PS_SOLID, GetColour(GVC_GRIDLINES));//GetSysColor(COLOR_3DFACE));
	HPEN hPen2		= CreatePen(0, PS_SOLID, GetColour(focussed ? GVC_GRIDLINES2 : GVC_HIGHLIGHT2));//GetSysColor(COLOR_3DFACE));
	HPEN hPen3		= CreatePen(0, PS_SOLID, 0xE8D2A7);
	HANDLE hOldPen	= SelectObject(hdcMem, hPen);

	for(rowptr = GetFirstVisibleRow(), rowidx = 0;
		rowidx < m_nVScrollPos && rowptr != 0;
		rowptr = rowptr->NextVisible(), rowidx++)
		{
		}

	if(m_nCurrentLine - m_nVScrollPos == 0 && rowptr)
		SelectObject(ps.hdc, hPen2);
	else
		SelectObject(ps.hdc, GetStockObject(WHITE_PEN));

	MoveToEx(ps.hdc, 0, m_nHeaderHeight-1, 0);
	LineTo(ps.hdc, rect.right, m_nHeaderHeight-1);

	for(//rowptr = GetFirstVisibleRow(), rowidx = m_nVScrollPos; 
		;
		rowptr != 0 && rowidx <= m_nVScrollPos + m_nWindowLines;//m_gvData.End();
		rowptr = rowptr->NextVisible(), rowidx++)
	{
		bool selected = (rowidx == m_nCurrentLine || rowidx == m_nCurrentLine-1) ? true : false;
		
		x  = -m_nHScrollPos;

		if(CheckStyle(GVS_HORZGRIDLINES))
		{

			//if(rowidx == 0)
			//	SelectObject(hdcMem, GetStockObject(NULL_PEN));
			//else
				SelectObject(hdcMem, selected ? hPen2 : hPen);

			// draw the horizontal gridline 
			MoveToEx(hdcMem, 0, m_nLineHeight-1, 0);
			LineTo(hdcMem, rect.right, m_nLineHeight-1);
		}

		selected = (rowidx == m_nCurrentLine) ? true : false;
		SelectObject(hdcMem, selected &&focussed? hPen3 : hPen);

		// double-buffer each line
		DrawRow(hdcMem, rowptr, x, 0, m_nLineHeight-CheckStyle(GVS_HORZGRIDLINES), 0, rowidx);
		BitBlt(ps.hdc, 0, y, rect.right, m_nLineHeight, hdcMem, 0, 0, SRCCOPY);

		y  += m_nLineHeight;//rowptr->height();
	}
		
	// fill any remaining 'whitespace' at the bottom
	rect.top = y;
	SetBkColor(ps.hdc, GetColour(GVC_BACKGROUND));
	ExtTextOut(ps.hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);

	{
		//GetRowItem(m_nCurrentLine,m_nCurrentColun
		//RECT itemRect;
	//	SelectClipRgn(ps.hdc,0);
		//GetItemRect(m_nCurrentColumn, m_nCurrentLine, &itemRect);
	//	InflateRect(&itemRect, 1,2 );
	//	DrawFocusRect(ps.hdc, &itemRect);
	}

	// draw some vertical column-lines to fill up the dead-space at the bottom
	// (i.e. extend the grid-lines down to the bottom of the window even if there
	// aren't any cells down there)
	SelectObject(ps.hdc, hPen);

	for(ULONG i = 0; i < m_nNumColumns; i++)
	{
		ULONG idx = Header_OrderToIndex(m_hWndHeader, i);
		RECT irect;
		Header_GetItemRect(m_hWndHeader, idx, &irect);

		OffsetRect(&irect, -m_nHScrollPos, 0);
		MoveToEx(ps.hdc, irect.right-1, y, 0);
		LineTo(ps.hdc, irect.right-1, rect.bottom);
	}

	// cleanup!
	SelectObject(hdcMem, hOldPen);
	SelectObject(hdcMem, hbmOld);
	DeleteObject(hPen);
	DeleteObject(hPen2);
	DeleteObject(hPen3);
	DeleteObject(hbmMem);

	DeleteDC(hdcMem);
	EndPaint(m_hWnd, &ps);
	return 0;
}

#include <vssym32.h>

static HRGN ThemeEditBorder(HWND hwnd, HTHEME hTheme, HRGN hrgnUpdate)
{
	HDC hdc = GetWindowDC(hwnd);
	RECT rc;
	RECT rcWindow;
	DWORD state = ETS_NORMAL;
	HRGN hrgnClip;
	
	if(!IsWindowEnabled(hwnd))
		state = ETS_DISABLED;
	else if(GetFocus() == hwnd)
		state = ETS_NORMAL;//ETS_HOT;
	else
		state = ETS_NORMAL;
	
	GetWindowRect(hwnd, &rcWindow);
	GetClientRect(hwnd, &rc);
	ClientToScreen(hwnd, (POINT *)&rc.left);
	ClientToScreen(hwnd, (POINT *)&rc.right);

	rc.right = rcWindow.right - (rc.left - rcWindow.left);
	rc.bottom = rcWindow.bottom - (rc.top - rcWindow.top);
	
	hrgnClip = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
	
	if(hrgnUpdate != (HRGN)1)
		CombineRgn(hrgnClip, hrgnClip, hrgnUpdate, RGN_AND);
	
	OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
	
	ExcludeClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
	OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
	
	//if (IsThemeBackgroundPartiallyTransparent (hTheme, EP_EDITTEXT, state))
	//	DrawThemeParentBackground(m_hWnd, hdc, &rcWindow);
	
	DrawThemeBackground(hTheme, hdc, 
		6,
		state,
		//EP_EDITTEXT, 
		//state, 
		//3,0,
		&rcWindow, NULL);
	
	ReleaseDC(hwnd, hdc);

	return hrgnClip;
}



LRESULT GridView::OnNcPaint(HRGN hrgnUpdate)
{
	HRGN    hrgnClip = hrgnUpdate;
	LRESULT res;

	if(m_hGridTheme != 0)
	{
		hrgnClip = ThemeEditBorder(m_hWnd, m_hGridTheme, hrgnUpdate);
	}

	res = DefWindowProc(m_hWnd, WM_NCPAINT, (WPARAM)hrgnClip, 0);	
	DeleteObject(hrgnClip);

	return res;
}
