//
//  HexViewBoomark.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"

bool __inline overlaps(size_w a1, size_w b1, size_w a2, size_w b2)
{
	return (a1 >= a2 && a1 < b2 || b1 >= a2 && b1 < b2 || a1 <= a2 && b1 > a2);
}

BOOKNODE * HexView::FindBookmark(size_w startoff, size_w endoff)
{
	BOOKNODE *bptr = 0;

	for(bptr = m_BookHead->next; bptr != m_BookTail; bptr = bptr->next)
	{
		if(overlaps(startoff, endoff, bptr->bookmark.offset, bptr->bookmark.offset + bptr->bookmark.length))
			break;
	}

	return bptr;
}

extern "C" COLORREF MixRgb(COLORREF col1, COLORREF col2);

BOOL HexView::GetHighlightCol(size_w offset, int pane, BOOKNODE *itemStart,  
							  HEXCOL *col1, HEXCOL *col2, 
							  bool fModified, bool fMatched, bool fIncSelection)
{
	//int idx = -1;
	//int m_nNumHighlights = m_Highlight->nNumItems;
	//HIGHLIGHT *Highlight = m_Highlight->Highlight;

	BOOKNODE *hi = itemStart;

	size_w selstart = min(m_nSelectionStart, m_nSelectionEnd);
	size_w selend   = max(m_nSelectionStart, m_nSelectionEnd);

	int nSchemeIdxFG;
	int nSchemeIdxBG;

	BOOL fGotFocus = GetFocus() == m_hWnd ? TRUE : FALSE;
	fGotFocus = GetAncestor(GetForegroundWindow(), GA_ROOTOWNER) == GetParent(m_hWnd);
	
	fGotFocus = fGotFocus && IsWindowEnabled(GetParent(m_hWnd));

	//TRACEA("FG = %x (me=%x)\n", GetForegroundWindow(), m_hWnd);

	if(pane == 0)
	{
		nSchemeIdxFG = (((offset + m_nDataShift)% m_nBytesPerLine) / m_nBytesPerColumn) & 1 ? HVC_HEXEVEN : HVC_HEXODD;
		nSchemeIdxBG = HVC_BACKGROUND;
	}
	else
	{
		nSchemeIdxFG = HVC_ASCII;
		nSchemeIdxBG = HVC_BACKGROUND;
	}

	// modified bytes override normal settings
	if(fModified && CheckStyle(HVS_SHOWMODS))
		nSchemeIdxFG = HVC_MODIFY;

	if(fMatched)
	{
		//nSchemeIdxFG = HVC_BACKGROUND;
		nSchemeIdxBG = HVC_MATCHED;
	}

	// search forward to find the highlight under specified offset
	for(hi = itemStart; hi; hi = hi->next)
	{
		if(offset >= hi->bookmark.offset && 
			offset < hi->bookmark.offset + hi->bookmark.length)
		{
			break;
		}
	}
/*

	// search forward to find the highlight under specified offset
	for(int i = highidx; i < m_nNumHighlights && i != -1; i++)
	{
		// find the nearest highlight 
		if(offset >= Highlight[i].nRangeStart && offset < Highlight[i].nRangeEnd)
		{
			idx = i;
			//break;
		}
	}*/

	// matched a highlight...check if its a stack of highlights
	if(hi)//idx != -1)
	{
		// if this is a stack of highlights, iterate down through the stack looking
		// for a highlight which overlaps
	/*	while(Highlight[idx].Stack != 0)
		{
			m_nNumHighlights = Highlight[idx].Stack->nNumItems;
			Highlight = Highlight[idx].Stack->Highlight;

			for(int i = m_nNumHighlights-1; i >= 0; i--)
			{
				if(offset >= Highlight[i].nRangeStart && offset < Highlight[i].nRangeEnd)
				{
					idx = i;
					break;
				}
			}
		}*/
	}

	// found a highlight? set the colour
	if(hi && 
		offset >= hi->bookmark.offset && 
		offset < hi->bookmark.offset + hi->bookmark.length)// >= 0)
	{
		//col1->colFG = Highlight[idx].colFG;
		//col1->colBG = Highlight[idx].colBG;


		col1->colFG = hi->bookmark.col;
		col1->colBG = hi->bookmark.backcol;

		//col1->colFG = RGB(255,255,255);
		//col1->colBG = RGB(128,128,128);

		*col2 = *col1;

		if(fModified)
		{
			col1->colFG = GetHexColour(HVC_MODIFY);
			col2->colFG = GetHexColour(HVC_MODIFY);
		}
	}
	// no highlight, use the default window scheme
	else
	{
		col1->colFG = GetHexColour(nSchemeIdxFG);
		col1->colBG = GetHexColour(nSchemeIdxBG);
		*col2 = *col1;
	}

	// if at the end of the highlight, need to paint in two colours
	//if(idx != -1 && offset == Highlight[idx].nRangeEnd - 1)
	if(hi && offset == hi->bookmark.offset + hi->bookmark.length - 1)
	{
		/*int idx2 = -1;

		// search backwards again
		for(int i = idx - 1; i >= 0; i--)
		{
			if(offset >= Highlight[i].nRangeStart && offset < Highlight[i].nRangeEnd - 1)
			{
				idx2 = i;
				break;
			}
		}

		if(idx2 != -1)
		{
			col2->colFG = Highlight[idx2].colFG;
			col2->colBG = Highlight[idx2].colBG;
		}
		else
		{
			col2->colBG = GetHexColour(nSchemeIdxBG);
		}*/
	}

	// selected data overrides everything else!
	if(fIncSelection && offset >= selstart && offset < selend)
	{
		// selected colour is next sequential index
		if(fGotFocus)
		nSchemeIdxFG++;

		//nSchemeIdxFG = nSchemeIdxBG;
		//nSchemeIdxBG++;
		//nSchemeIdxFG++;
		//nSchemeIdxBG++;


		if(nSchemeIdxBG == HVC_MATCHED)
			nSchemeIdxFG = HVC_MATCHEDSEL;

		//nSchemeIdxBG = HVC_SELECTION;

		if(pane != m_nWhichPane)
		{
			//if(nSchemeIdxBG == HVC_BACKGROUND)
				nSchemeIdxBG = HVC_SELECTION;
		//	nSchemeIdxFG++;
		//	nSchemeIdxBG++;
		}
		else
		{
			//if(nSchemeIdxBG == HVC_BACKGROUND)
				nSchemeIdxBG = HVC_SELECTION;

		}
		
		if(!fGotFocus)
		{
			nSchemeIdxBG = HVC_SELECTION3;
			//nSchemeIdxFG = HVC_SELECTION4;
		}


		if(CheckStyle(HVS_INVERTSELECTION))
		{
			col1->colBG = 0xffffff & ~col1->colBG;
			col1->colFG = 0xffffff & ~col1->colFG;
		}
		else
		{
			//col1->colFG = MixRgb(GetHexColour(HVC_SELECTION), GetHexColour(nSchemeIdxFG));
			//col1->colBG = MixRgb(GetHexColour(HVC_SELECTION), GetHexColour(nSchemeIdxBG));

			//if(!fModified)
			col1->colFG = !hi || fModified ? GetHexColour(nSchemeIdxFG) :col2->colBG;
			col1->colBG = GetHexColour(nSchemeIdxBG);
			
			
			//col1->colFG = GetHexColour((idx == -1) ? nSchemeIdxFG : HVC_BOOKSEL);
			//col1->colBG = idx == -1 ? col1->colBG : 0xffffff & ~col1->colFG;
		}


#ifdef SELECTION_USES_HIGHLIGHT
		if(m_fHighlighting)
		{
			col1->colFG = 0xffffff & ~GetHexColour(HVC_BOOKMARK_FG);
			col1->colBG = 0xffffff & ~GetHexColour(HVC_BOOKMARK_BG);
		}
#endif

		if(offset < selend - 1 && selend > 0)
			*col2 = *col1;
	} 

	// take into account any offset/shift in the datasource
	offset += m_nDataShift;//Start;
	if((offset + 1) % (m_nBytesPerLine) == 0 && offset != 0)
	{
		col2->colFG = col1->colFG;
		col2->colBG = GetHexColour(HVC_BACKGROUND);
	}

	return TRUE;
}

/*LONG HexView::Highlight(BOOL fEnable)
{
	m_fHighlighting = fEnable ? true : false;
	return 0;
}*/

BOOKNODE * HexView::AddBookmark(BOOKMARK * bookm)
{
	BOOKNODE * bnew = new BOOKNODE();//bookm);
	BOOKNODE * bptr;

	// find the best place to insert into
	for(bptr = m_BookHead->next; bptr != m_BookTail; bptr = bptr->next)
	{
		if(bookm->offset < bptr->bookmark.offset)
		{
			break;
		}
	}

	bnew->prev		 = bptr->prev;
	bnew->next		 = bptr;

	bptr->prev->next = bnew;
	bptr->prev		 = bnew;


	bnew->bookmark = *bookm;
	bnew->bookmark.pszText  = bookm->pszText ? _tcsdup(bookm->pszText) : 0;//TEXT("");
	bnew->bookmark.pszTitle = bookm->pszTitle ? _tcsdup(bookm->pszTitle) : 0;//TEXT("");
	

	RefreshWindow();


	return (BOOKNODE *)bnew;
}

BOOL HexView::DelBookmark(BOOKNODE * delthis)
{
	BOOKNODE * bptr;

	for(bptr = m_BookHead->next; bptr != m_BookTail; bptr = bptr->next)
	{
		if(bptr == delthis)
		{
			bptr->prev->next = bptr->next;
			bptr->next->prev = bptr->prev;

			delete delthis;
			RefreshWindow();

			return TRUE;
		}
	}

	return FALSE;
}

BOOL HexView::ClearBookmarks()
{
	BOOKNODE *bptr, *btmp;
	
	for(bptr = m_BookHead->next; bptr != m_BookTail; bptr = btmp)
	{
		btmp = bptr->next;
		delete bptr;
	}

	m_BookHead->next = m_BookTail;
	m_BookTail->prev = m_BookHead;

	RefreshWindow();
	return TRUE;
}

BOOL HexView::SetBookmark(BOOKNODE *bptr, BOOKMARK *param)
{
	// if the offset/length has changed??
	if(param->offset != bptr->bookmark.offset || 
	   param->length != bptr->bookmark.length)
	{
		//InvalidateRange(hip->hp.start, hip->hp.start + hip->hp.length);
		//InvalidateRange(param->start, param->start + param->length);
		RefreshWindow();
	}

	free(bptr->bookmark.pszText);
	free(bptr->bookmark.pszTitle);

	bptr->bookmark = *param;
	bptr->bookmark.pszText  = param->pszText ? _tcsdup(param->pszText) : 0;
	bptr->bookmark.pszTitle = param->pszTitle ? _tcsdup(param->pszTitle) : 0;

	RefreshWindow();

	return TRUE;
}
	
BOOKNODE * HexView::EnumBookmark(BOOKNODE * bptr, BOOKMARK * param)
{
	if(bptr == NULL)
		bptr = m_BookHead;

	while(bptr)
	{
		bptr = bptr->next;

		if(bptr == m_BookTail)
			return NULL;

		if((bptr->bookmark.flags & HVBF_NOENUM) == 0)
		{
			*param = bptr->bookmark;
			return bptr;
		}
	}

	return NULL;
}

BOOL HexView::GetBookmark(BOOKNODE * bptr, BOOKMARK * param)
{
	if(bptr == NULL || bptr == m_BookHead || bptr == m_BookTail)
		return FALSE;

	*param = bptr->bookmark;
	return TRUE;
}

BOOL HexView::BookmarkRect(BOOKMARK *bm, RECT *rect)
{
	size_w windowstart = m_nVScrollPos * m_nBytesPerLine;

	if(bm->offset >= windowstart && bm->offset + bm->length < windowstart + m_nWindowLines*m_nBytesPerLine)
	{
		rect->top	 = (LONG)(bm->offset / m_nBytesPerLine - m_nVScrollPos) * m_nFontHeight;
		rect->bottom = rect->top + 50;
		rect->left	 = m_nWindowColumns * m_nFontWidth + BOOKMARK_XOFFSET - m_nHScrollPos * m_nFontWidth;
		rect->right  = rect->left + 300;

		HDC hdc = GetDC(0);
				
		DrawText(hdc, bm->pszText, lstrlen(bm->pszText), rect, DT_EDITCONTROL|DT_CALCRECT);

		ReleaseDC(0, hdc);
		
		rect->right = rect->left + 300;
		rect->bottom += 20;

		return TRUE;
	}

	return FALSE;
}
