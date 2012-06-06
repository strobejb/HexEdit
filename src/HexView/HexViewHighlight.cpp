//
//  HexViewHighlight.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//
//
#include <windows.h>
#include <tchar.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"

#if 0

int FindHighlight(size_w startoff, size_w endoff, HIGHLIGHT_LIST *m_Highlight);

bool __inline overlaps(size_w a1, size_w b1, size_w a2, size_w b2)
{
	return (a1 >= a2 && a1 < b2 || b1 >= a2 && b1 < b2 || a1 <= a2 && b1 > a2);
}

static int sortproc(const void *elem1, const void *elem2)
{
	HIGHLIGHT *r1 = (HIGHLIGHT *)elem1;
	HIGHLIGHT *r2 = (HIGHLIGHT *)elem2;

	if(r1->nRangeStart < r2->nRangeStart)
	{
		return -1;
	}
	else if(r1->nRangeStart > r2->nRangeStart)
	{
		return 1;
	}
	else
	{
		if(r1->nRangeEnd < r2->nRangeEnd)
			return -1;
		else if(r1->nRangeEnd > r2->nRangeEnd)
			return 1;
		else
			return 0;
	}
}

//
HIGHLIGHT * AddHighlightList(HIGHLIGHT_LIST *HighlightList, size_w startoff, size_w endoff, COLORREF foregnd, COLORREF backgnd)
{
	HIGHLIGHT *Highlight = &HighlightList->Highlight[HighlightList->nNumItems++];

	Highlight->nRangeStart		= min(startoff, endoff);
	Highlight->nRangeEnd		= max(startoff, endoff);
	Highlight->colFG			= foregnd;
	Highlight->colBG			= backgnd;
	Highlight->Stack			= 0;

	return Highlight;
}


HIGHLIGHT * HexView::AddHighlight2(HIGHLIGHT_PARAM *hp)//size_w nHighlightStart, size_w nHighlightEnd, COLORREF foregnd, COLORREF backgnd)
{
	size_w startoff = hp->start;//min(hp->start, nHighlightEnd);
	size_w endoff   = hp->length;//max(nHighlightStart, nHighlightEnd);
	
	int idx = FindHighlight(startoff, endoff, m_Highlight);

	HIGHLIGHT *Highlight = 0;

	if(idx < 0)
	{
		Highlight = AddHighlightList(m_Highlight, startoff, endoff, hp->colFG, hp->colBG);
		
		qsort(m_Highlight->Highlight, m_Highlight->nNumItems, sizeof(HIGHLIGHT), sortproc);
	}
	else
	{
		Highlight = &m_Highlight->Highlight[0];

		// loop over any highlight that the new one overlaps
		for(unsigned i = idx; i < m_Highlight->nNumItems && 
			overlaps(
				startoff, 
				endoff, 
				Highlight[i].nRangeStart, 
				Highlight[i].nRangeEnd
				)
			; i++)
		{
			startoff = min(startoff, Highlight[i].nRangeStart);
			endoff   = max(endoff,   Highlight[i].nRangeEnd);
		}

		// expand the "outer" highlight to encompass
		Highlight[idx].nRangeStart = startoff;
		Highlight[idx].nRangeEnd   = endoff;

		// remove any overlapped highlights
		memmove(Highlight+idx+1, Highlight+i, (m_Highlight->nNumItems - i) * sizeof(HIGHLIGHT));
		m_Highlight->nNumItems -= (i-idx-1);

		// update with new colour
		Highlight[idx].colFG = hp->colFG;
		Highlight[idx].colBG = hp->colBG;
	}

	return Highlight;
}
/*
int HexView::AddHighlight(size_w nHighlightStart, size_w nHighlightEnd, COLORREF foregnd, COLORREF backgnd)
{
	size_w startoff = min(nHighlightStart,nHighlightEnd);
	size_w endoff   = max(nHighlightStart,nHighlightEnd);
	
	int idx = FindHighlight(startoff, endoff, m_Highlight);

	if(idx < 0)
	{
		AddHighlightList(m_Highlight, startoff, endoff, foregnd, backgnd);
		
		qsort(m_Highlight->Highlight, m_Highlight->nNumItems, sizeof(HIGHLIGHT), sortproc);
	}
	else
	{
		HIGHLIGHT *Highlight=&m_Highlight->Highlight[0];

		// loop over any highlight that the new one overlaps
		for(unsigned i = idx; i < m_Highlight->nNumItems && 
			overlaps(
				startoff, 
				endoff, 
				Highlight[i].nRangeStart, 
				Highlight[i].nRangeEnd
				)
			; i++)
		{
			startoff = min(startoff, Highlight[i].nRangeStart);
			endoff   = max(endoff,   Highlight[i].nRangeEnd);
		}

		// expand the "outer" highlight to encompass
		Highlight[idx].nRangeStart = startoff;
		Highlight[idx].nRangeEnd   = endoff;

		// remove any overlapped highlights
		memmove(Highlight+idx+1, Highlight+i, (m_Highlight->nNumItems - i) * sizeof(HIGHLIGHT));
		m_Highlight->nNumItems -= (i-idx-1);

		// update with new colour
		Highlight[idx].TextCol.colFG = foregnd;
		Highlight[idx].TextCol.colBG = backgnd;
	}

	return 0;

}

*/

/*
int HexView::AddHighlight(size_w nHighlightStart, size_w nHighlightEnd, COLORREF foregnd, COLORREF backgnd)
{
	size_w startoff = min(nHighlightStart,nHighlightEnd);
	size_w endoff   = max(nHighlightStart,nHighlightEnd);
	
	int idx = FindHighlight(startoff, endoff, m_Highlight);

	// highlight doesn't intersect any others, so add it to main sorted list
	if(idx < 0)
	{
		AddHighlightList(m_Highlight, startoff, endoff, RGB(0,0,0), RGB(rand()%128+128,rand()%128+128,rand()%128+128));
		
		qsort(m_Highlight->Highlight, m_Highlight->nNumItems, sizeof(HIGHLIGHT), sortproc);
	}
	else if(idx >= 0)
	{
		// it does intersect!
		HIGHLIGHT *Highlight=&m_Highlight->Highlight[idx];
		
		size_w first = Highlight->nRangeStart;
		size_w last  = Highlight->nRangeEnd;
		
		if(Highlight->Stack == 0)
		{
			Highlight->Stack = (HIGHLIGHT_LIST*)malloc(0x1000);
			Highlight->Stack->nNumItems = 0;
			
			AddHighlightList(Highlight->Stack, Highlight->nRangeStart, Highlight->nRangeEnd, 
				Highlight->TextCol.colFG, Highlight->TextCol.colBG);
		} 
		
		// add the already existing highlights which intersect the new one
		for(unsigned i = 1; i+idx<m_Highlight->nNumItems && 
			overlaps(startoff, endoff, Highlight[i].nRangeStart, Highlight[i].nRangeEnd); 
			i++)
		{
			last = Highlight[i].nRangeEnd;
			
			if(Highlight[i].Stack)
			{
				for(unsigned j = 0; j < Highlight[i].Stack->nNumItems; j++)
				{
					AddHighlightList(Highlight->Stack, Highlight[i].Stack->Highlight[j].nRangeStart,
						Highlight[i].Stack->Highlight[j].nRangeEnd,
						Highlight[i].Stack->Highlight[j].TextCol.colFG,
						Highlight[i].Stack->Highlight[j].TextCol.colBG);
				}
				
				if(Highlight[i].Stack->nNumItems > 0)
				{
					free(Highlight[i].Stack);
					Highlight[i].Stack = 0;
				}
			}
			else
			{
				AddHighlightList(Highlight->Stack, Highlight[i].nRangeStart, 
					Highlight[i].nRangeEnd, Highlight[i].TextCol.colFG, Highlight[i].TextCol.colBG);
			}
			
		}
		
		// now remove them from the outer list
		memmove(Highlight+1, Highlight+i, (m_Highlight->nNumItems - (idx+i)) * sizeof(HIGHLIGHT));
		m_Highlight->nNumItems -= (i-idx-1);
		
		
		// expand the "outer" highlight to encompass
		Highlight->nRangeStart = min(first,  startoff);
		Highlight->nRangeEnd   = max(last,   endoff);
		
		// add the new highlight on top of all others
		AddHighlightList(Highlight->Stack, 
			startoff, endoff,
			RGB(0,0,0), RGB(rand()%128+128,rand()%128+128,rand()%128+128)
			);
	}

	return 0;
}*/

HIGHLIGHT_ITEM * FindHighlight(size_w startoff, size_w endoff, HIGHLIGHT_ITEM *HighlightList)
{
	HIGHLIGHT_ITEM *ptr = 0;

	for(ptr = HighlightList; ptr; ptr = ptr->sortedList)
	{
		//if(startoff >= ptr->hp.start && startoff < ptr->hp.start + ptr->hp.length)
		if(overlaps(startoff, endoff, ptr->hp.start, ptr->hp.start+ptr->hp.length))
		{
			return ptr;
		}
	}

	return ptr;
}

int FindHighlight(size_w startoff, size_w endoff, HIGHLIGHT_LIST *HighlightList)
{
	int m_nNumHighlights = HighlightList->nNumItems;
	HIGHLIGHT *Highlight = HighlightList->Highlight;

	int first = 0;
	int last  = m_nNumHighlights - 1;
	int i;

	// perform a "binary search" to find a highlight which intersects the
	// specified data range
	while(last >= first)
	{
		i = (last + first) / 2;

		if(overlaps(startoff, endoff, Highlight[i].nRangeStart, Highlight[i].nRangeEnd))
		{
			// found one...iterate backwards to make sure we have the first one
			while(i > 0 && overlaps(startoff, endoff, Highlight[i-1].nRangeStart, Highlight[i-1].nRangeEnd))
				i--;

			return i;
		}
		else if(startoff < Highlight[i].nRangeStart)
		{
			last = i-1;
		}
		else
		{
			first = i+1;	
		}
	}

	return -1;
}

extern "C" COLORREF MixRgb(COLORREF col1, COLORREF col2);

BOOL HexView::GetHighlightCol(size_w offset, int pane, HIGHLIGHT_ITEM *itemStart,  
							  HEXCOL *col1, HEXCOL *col2, 
							  bool fModified, bool fMatched, bool fIncSelection)
{
	//int idx = -1;
	//int m_nNumHighlights = m_Highlight->nNumItems;
	//HIGHLIGHT *Highlight = m_Highlight->Highlight;

	HIGHLIGHT_ITEM *hi = itemStart;

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
		nSchemeIdxFG = ((offset % m_nBytesPerLine) / m_nBytesPerColumn) & 1 ? HVC_HEXEVEN : HVC_HEXODD;
		nSchemeIdxBG = HVC_BACKGROUND;
	}
	else
	{
		nSchemeIdxFG = HVC_ASCII;
		nSchemeIdxBG = HVC_BACKGROUND;
	}

	// modified bytes override normal settings
	if(fModified)
		nSchemeIdxFG = HVC_MODIFY;

	if(fMatched)
	{
		//nSchemeIdxFG = HVC_BACKGROUND;
		nSchemeIdxBG = HVC_MATCHED;
	}

	// search forward to find the highlight under specified offset
	for(hi = itemStart; hi; hi = hi->sortedList)
	{
		if(offset >= hi->hp.start && 
			offset < hi->hp.start + hi->hp.length)
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
		offset >= hi->hp.start && 
		offset < hi->hp.start + hi->hp.length)// >= 0)
	{
		//col1->colFG = Highlight[idx].colFG;
		//col1->colBG = Highlight[idx].colBG;


		col1->colFG = hi->hp.colFG;
		col1->colBG = hi->hp.colBG;

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
	if(hi && offset == hi->hp.start + hi->hp.length - 1)
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

BOOL HexView::AddHighlight(HIGHLIGHT_PARAM *hp)
{
	HIGHLIGHT_ITEM *hi = new HIGHLIGHT_ITEM;
	HIGHLIGHT_ITEM *ptr, *next;

	if(m_HighlightOrderList == 0)
	{
		m_HighlightOrderList  = hi;
	}
	else
	{
		// add to end of list
		for(ptr = m_HighlightOrderList; ptr->orderList; ptr = ptr->orderList)
			;

		ptr->orderList = hi;
	}

	// add to the sorted list, so we can search for highlights
	// based on starting index
	for(ptr = m_HighlightSortedList; ptr; ptr = next)
	{
		next = ptr->sortedList;
		if(next == 0)
		{
			ptr->sortedList = hi;
		}
		else if(hp->start < next->hp.start)
		{
			hi->sortedList = next;
			ptr->sortedList = hi;
			//hi->sortedList = ptr;
			break;
		}
	}

	if(m_HighlightSortedList == 0)
		m_HighlightSortedList = hi;

	hi->hp			 = *hp;
	hi->hp.pszText  = _tcsdup(hp->pszText);
	hi->hp.pszTitle = _tcsdup(hp->pszTitle);

	//HIGHLIGHT *h = AddHighlight2(hp);//hp->start, hp->start+hp->length, hp->colFG, hp->colBG);

	return TRUE;
}

BOOL HexView::DelHighlight(int idx)
{
	HIGHLIGHT_ITEM *ptr, *prev = 0;
	HIGHLIGHT_ITEM *delthis = 0;
	int i = 0;

	for(ptr = m_HighlightOrderList; ptr; ptr = ptr->orderList)
	{
		if(i++ == idx)
		{
			if(prev == 0)
				m_HighlightOrderList = ptr->orderList;
			else
				prev->orderList = ptr->orderList;				

			delthis = ptr;

			break;
		}

		prev = ptr;
	}

	for(i = 0, prev = 0, ptr = m_HighlightSortedList; ptr; ptr = ptr->sortedList)
	{
		if(ptr == delthis)
		{
			if(prev == 0)
				m_HighlightSortedList = ptr->sortedList;
			else
				prev->sortedList = ptr->sortedList;				

			break;
		}

		prev = ptr;
	}

	delete delthis;

	return TRUE;
}

/*BOOL HexView::ClearHighlights()
{
	m_Highlight->nNumItems = 0;
	return TRUE;
}*/

BOOL HexView::SetHighlight(int idx, HIGHLIGHT_PARAM *param)
{
	HIGHLIGHT_ITEM *hip;
	int i = 0;
	
	for(hip = m_HighlightOrderList; hip; hip = hip->orderList)
	{
		if(i++ == idx)
		{
			if(param->start != hip->hp.start || param->length != hip->hp.length)
			{
				InvalidateRange(hip->hp.start, hip->hp.start + hip->hp.length);
				InvalidateRange(param->start, param->start + param->length);
			}

			hip->hp			 = *param;
			hip->hp.pszText  = _tcsdup(param->pszText);
			hip->hp.pszTitle = _tcsdup(param->pszTitle);
			return TRUE;
		}
	}

	return FALSE;
}
	
BOOL HexView::GetHighlight(int idx, HIGHLIGHT_PARAM *param)
{
	HIGHLIGHT_ITEM *hip;
	int i = 0;
	
	for(hip = m_HighlightOrderList; hip; hip = hip->orderList)
	{
		if(i++ == idx)
		{
			*param = hip->hp;
			return TRUE;
		}
	}

/*	for(int i = 0; i < m_Highlight->nNumItems; i++)
	{
		if(i == idx)
		{
			param->start	= m_Highlight->Highlight[i].nRangeStart;
			param->length	= m_Highlight->Highlight[i].nRangeEnd - param->start;
			param->colFG	= m_Highlight->Highlight[i].colFG;
			param->colBG	= m_Highlight->Highlight[i].colBG;

			return TRUE;
		}
	}*/

	return FALSE;
}

BOOL HexView::HighlightRect(HIGHLIGHT_PARAM *hp, RECT *rect)
{
	size_w windowstart = m_nVScrollPos * m_nBytesPerLine;

	if(hp->start >= windowstart && hp->start + hp->length < windowstart + m_nWindowLines*m_nBytesPerLine)
	{
		rect->top	 = (hp->start / m_nBytesPerLine - m_nVScrollPos) * m_nFontHeight;
		rect->bottom = rect->top + 50;
		rect->left	 = m_nWindowColumns * m_nFontWidth + BOOKMARK_XOFFSET - m_nHScrollPos * m_nFontWidth;
		rect->right  = rect->left + 300;

		HDC hdc = GetDC(0);
				
		DrawText(hdc, hp->pszText, lstrlen(hp->pszText), rect, DT_EDITCONTROL|DT_CALCRECT);

		ReleaseDC(0, hdc);
		
		rect->right = rect->left + 300;
		rect->bottom += 20;

		
		return TRUE;
	}

	return FALSE;
}

#endif
