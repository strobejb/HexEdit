//
//  HexViewDraw.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS

#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <vsstyle.h>
//#include <tmschema.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include <trace.h>

//extern	HIGHLIGHT m_Highlight[];

//void DrawNoteStrip(HDC hdc, int x, int y, HIGHLIGHT_PARAM *hparam);

//extern UINT	 m_nNumHighlights;

//HIGHLIGHT_ITEM * FindHighlight(size_w startoff, size_w endoff, HIGHLIGHT_ITEM *HighlightList);
//int FindHighlight(size_w startoff, size_w endoff, HIGHLIGHT_LIST *HighlightList);

bool operator != (HEXCOL &c1, HEXCOL &c2)
{
	return memcmp(&c1, &c2, sizeof(HEXCOL)) != 0;
}

bool operator == (HEXCOL &c1, HEXCOL &c2)
{
	return memcmp(&c1, &c2, sizeof(HEXCOL)) == 0;
}

static size_t int_to_bin(TCHAR *buf, UINT width, UINT num)
{
	TCHAR *start = buf;
	size_t i;

	while(width > 0)
	{
		BYTE val = (num >> ((width * 8) - 8)) & 0x000000ff;

		for(i = 0; i < 8; i++)
		{
			*buf++ = (num & 0x80) ? '1' : '0';
			num <<= 1;
		}

		width--;
	}

	*buf = 0;

	return (buf-start);
}


//
//	Format an address into the specified buffer, using
//  current address-column settings
//
size_t HexView::FormatAddress(size_w addr, TCHAR *buf, size_t buflen)
{
	if(CheckStyle( HVS_ADDR_INVISIBLE ))
	{
		buf[0] = '\0';
		return 0;
	}

	addr += m_nAddressOffset;

	if(CheckStyle( HVS_ADDR_DEC ))
	{
		_stprintf_s(buf, buflen, _T(" %0.*I64u"), m_nAddressDigits, (UINT64)addr);
	}
	else
	{
		TCHAR *szHexFmt;

		if(CheckStyle( HVS_ADDR_MIDCOLON ))
		{
			/*if(m_nFileLength > 0xffffffff)
			{
				szHexFmt = CheckStyle( HVS_LOWERCASEHEX ) ? _T("%0.*I64x:%08x") : _T("%0.*I64X:%08X");
				_stprintf(buf, szHexFmt, m_nAddressDigits-8, (UINT64)addr >> 32L, addr & 0xFFFFFFFF);
			}
			else*/
			{
				szHexFmt = CheckStyle( HVS_LOWERCASEHEX ) ? _T(" %0.*I64x:%04x") : _T(" %0.*I64X:%04X");
				_stprintf_s(buf, buflen, szHexFmt, m_nAddressDigits-4, (UINT64)addr >> 16L, addr & 0xFFFF);
			}
		}
		else
		{
			szHexFmt = CheckStyle( HVS_LOWERCASEHEX ) ? _T(" %0.*I64x") : _T(" %0.*I64X");	//_T("%0.*I64x") : _T("%0.*I64X");
			//szHexFmt = CheckStyle( HVS_LOWERCASEHEX ) ? _T("%8x") : _T("%8x");	//_T("%0.*I64x") : _T("%0.*I64X");
			_stprintf_s(buf, buflen, szHexFmt, m_nAddressDigits, (UINT64)addr);
			//_stprintf(buf, szHexFmt, (UINT64)addr);
		}
	}

	if(CheckStyle( HVS_ADDR_ENDCOLON ))
		_tcscat(buf, _T(":"));

	size_t len = _tcslen(buf);

	if(len > m_nAddressWidth)
		wmemmove(buf+1, buf + (len - m_nAddressWidth) + 1, len-m_nAddressWidth+1);

	return _tcslen(buf);
}

void reverse(BYTE *buf, int len)
{
	for(int i = 0; i < len/2; i++)
	{
		BYTE tmp = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = tmp;
	}
}

BOOL HexView::SetHexColour(UINT uIndex, COLORREF col)
{
	if(uIndex >= HV_MAX_COLS) 
		return FALSE;

	m_ColourList[uIndex] = col;
	return TRUE;
}

COLORREF HexView::GetHexColour(UINT uIndex)
{
	if(uIndex >= HV_MAX_COLS) 
		return FALSE;

	return RealiseColour(m_ColourList[uIndex]);
}

COLORREF HexView::RealiseColour(COLORREF cr)
{
	return HexView_RealiseColour(cr);
}

size_t HexView::FormatHexUnit(BYTE *data, TCHAR *buf, size_t buflen)
{
	TCHAR *szFmt;

	switch(m_nControlStyles & HVS_FORMAT_MASK)
	{
	case HVS_FORMAT_HEX:
		szFmt = CheckStyle( HVS_LOWERCASEHEX ) ? _T("%02x") : _T("%02X");
		return _stprintf(buf, szFmt, data[0]);

	case HVS_FORMAT_DEC:
		return _stprintf(buf, _T("%03d"), data[0]);
	
	case HVS_FORMAT_OCT:
		return _stprintf(buf, _T("%03o"), data[0]);

	case HVS_FORMAT_BIN:
		return int_to_bin(buf, 1, data[0]);

	default:
		buf[0] = '\0';
		break;
	}

	return _tcslen(buf);
}

VOID HexView::InvalidateRange(size_w start, size_w finish)
{
	int m_nPageMaxLines = m_nWindowLines;

	size_w screenstartoffset = m_nVScrollPos * m_nBytesPerLine;
	size_w screenendoffset   = (m_nVScrollPos + m_nPageMaxLines + 1) * m_nBytesPerLine;

	// take into account any data shift
	start             += m_nDataShift;//Start;
	finish            += m_nDataShift;//Start;

	if(screenendoffset < screenstartoffset) 
		screenendoffset = -1;

	if(start > finish)
	{
		size_w t0 = start;
		start	  = finish;
		finish	  = t0;
	}

	//clip the offsets to within the visible view
	if(start  < screenstartoffset) 	start  = screenstartoffset;
	if(start  > screenendoffset)   	start  = screenendoffset;
	if(finish < screenstartoffset)	finish = screenstartoffset;
	if(finish > screenendoffset)  	finish = screenendoffset;

	size_w length = finish - start;
	int y = (int)(start / m_nBytesPerLine - m_nVScrollPos);

	//SendMessage(m_hWnd, WM_SETREDRAW, 0, 0);

	while(length != 0)
	{
		RECT   rect;
		int	x   = (int)(start % m_nBytesPerLine);
		int len = (int)min(m_nBytesPerLine - x, length);

		// hex column
		SetRect(&rect, LogToPhyXCoord(x, 0),
			           (y * m_nFontHeight),
					   LogToPhyXCoord(x+len, 0),
					   (y * m_nFontHeight + m_nFontHeight) );

		InvalidateRect(m_hWnd, &rect, FALSE);

		// ascii column
		SetRect(&rect, LogToPhyXCoord(x, 1),
					   (y * m_nFontHeight),
					   LogToPhyXCoord(x+len, 1),
					   (y * m_nFontHeight + m_nFontHeight) );
					   
		InvalidateRect(m_hWnd, &rect, FALSE);

		y++;
		start   = 0;
		length -= len;
	}

	//SendMessage(m_hWnd, WM_SETREDRAW, 1, 0);
}

void AddAttr(ATTR **attrListPtr, COLORREF fg, COLORREF bg, size_t count)
{
	ATTR *attrList = *attrListPtr;

	for(size_t i = 0; i < count; i++)
	{
		attrList[i].colFG = fg;
		attrList[i].colBG = bg;
	}

	(*attrListPtr) += count;
}
int fixthis = 0;
size_t HexView::FormatLine(
	BYTE *          data,		// the data to display
	size_t          length,		// length of data
	size_w          offset,		// address-offset (display only)
	TCHAR *         szBuf,		// text buffer to print into
	size_t          nBufLen,    // size of szBuf
	ATTR *          attrList,   // attribute-buffer (must be sizeof(szBuf))
	seqchar_info *  infobuf,    // seqchar info (must be sizeof(szBuf))
	bool            fIncSelection   // include the selection colors, if the data is selected
	)
{
	TCHAR *    ptr = szBuf;
	size_t     i;
	BOOKNODE * highlight;
	int        colidx = 0;
	ATTR     * attrPtr = attrList;


	//
	//	Format the offset as a hex/dec address
	//
	ptr += FormatAddress(offset, ptr, nBufLen);
	
	AddAttr(&attrPtr, GetHexColour(HVC_ADDRESS), GetHexColour(HVC_BACKGROUND), ptr - szBuf);

	highlight = FindBookmark(offset, offset+length);

	//
	//	Hex column
	//
	if(CheckStyle(HVS_HEX_INVISIBLE) == false)
	{
		// left padding
		for(i = 0; i < (size_t)m_nHexPaddingLeft; i++)
			*ptr++ = _T(' ');

		AddAttr(&attrPtr, GetHexColour(HVC_ADDRESS), GetHexColour(HVC_BACKGROUND), m_nHexPaddingLeft);

		for(i = 0; i < (int)length/* m_nBytesPerLine*/; i++)
		{
			HEXCOL col1, col2;

			size_t len = FormatHexUnit(&data[i], ptr, 0);
			ptr += len;

			GetHighlightCol(offset+i, 0, highlight, &col1, &col2, 
				infobuf[i].buffer != 1 ? true : false,
				infobuf[i].userdata != 0 ? true : false,
				fIncSelection
				);

			if(i < fixthis)
			{
				col1.colBG = col2.colBG = GetHexColour(HVC_BACKGROUND);
				wmemset(ptr-len, ' ', len);
			}


			// add the colour information
			if(col1 != col2 || i == m_nBytesPerLine - 1 || (i+1) % (m_nBytesPerColumn) != 0)
			{
				AddAttr(&attrPtr, col1.colFG, col1.colBG, len);

				if((i+1) % (m_nBytesPerColumn) == 0 && (i < length/*m_nBytesPerLine*/ - 1))
				{
					*ptr++ = ' ';
					AddAttr(&attrPtr, col2.colFG, col2.colBG, 1);
				}
			}
			else if(i < length - 1)
			{
				*ptr++ = ' ';
				AddAttr(&attrPtr, col1.colFG, col1.colBG, len+1);
			}
			else
			{
				AddAttr(&attrPtr, col1.colFG, col1.colBG, len);
			}
		}

		// dead space 
		if(i != m_nBytesPerLine)
		{
			size_t len = m_nHexWidth - (ptr - (szBuf+(m_nAddressWidth + m_nHexPaddingLeft)));
		
			for(i = 0; i < len; i++)
				*ptr++ = ' ';

			AddAttr(&attrPtr, GetHexColour(HVC_ASCII), GetHexColour(HVC_BACKGROUND), len);
		}
	}

	if(CheckStyle(HVS_ASCII_INVISIBLE) == false)
	{
		// right-side padding
		for(i = 0; i < (size_t)m_nHexPaddingRight; i++)
			*ptr++ = _T(' ');

		AddAttr(&attrPtr, GetHexColour(HVC_ASCII), GetHexColour(HVC_BACKGROUND), m_nHexPaddingRight);

		//
		//	Ascii column
		//
		for(i = 0; i < (int)length;/*m_nBytesPerLine*/ i++)
		{
			HEXCOL col1, col2;
			BYTE v = data[i];

			const int ctrlChar = '.';

			if(CheckStyle(HVS_ASCII_SHOWCTRLS) == 0 && (v < 32) )
			{
				*ptr++ = ctrlChar;
			}
			else if(CheckStyle(HVS_ASCII_SHOWEXTD) == 0 && (v >= 0x80 && v <= 0xa0) )
			{
				*ptr++ = ctrlChar;
			}
			else
			{
				*ptr++ = v;
			}

			//*ptr++ = (data[i] < 32) ? data[i] = '.' : (data[i] >;

			GetHighlightCol(offset+i, 1, highlight, &col1, &col2, 
				infobuf[i].buffer != 1 ? true : false,
				infobuf[i].userdata != 0 ? true : false,
				fIncSelection
				);

			if(i < fixthis)
			{
				col1.colBG = col2.colBG = GetHexColour(HVC_BACKGROUND);
				*(ptr-1) = ' ';
			}

			//if(col1.colBG == 0xffffff)
			//	col1.colBG = RGB(244,243,241);

			AddAttr(&attrPtr, col1.colFG, col1.colBG, 1);
		}

		if(i != m_nBytesPerLine)
		{
			size_t len = m_nBytesPerLine - i;

			for(i = 0; i < len; i++)
				*ptr++ = ' ';

			AddAttr(&attrPtr, GetHexColour(HVC_ASCII), GetHexColour(HVC_BACKGROUND), len);
		}


		*ptr = '\0';
	}

	return (ptr-szBuf);
}

size_t HexView::FormatData(HEXFMT_PARAMS *fmtParams)//size_w offset, int length, TCHAR *out, int outlen, ATTR *attrList)
{
	BYTE			*buf;
	seqchar_info	*infobuf;
	size_t			length = 0;

	if(fmtParams == 0 || fmtParams->length == 0)
		return 0;

	length  = fmtParams->length;
	buf		= new BYTE[length];
	infobuf = new seqchar_info[length];

	if(buf && infobuf)
	{
		//length = min(fmtParams->length, 100);
		length = m_pDataSeq->render(fmtParams->offset, buf, length, infobuf);
		length = FormatLine(buf, length, fmtParams->offset, fmtParams->szText, fmtParams->bufferSize, fmtParams->attrList, infobuf, false);
	}

	delete[] buf;
	delete[] infobuf;

	return length;
}

//
//	Draw the specified line of data
//
//	data - the data to render
//  datalen - expected to be no bigger than m_nBytesPerLine
//
int HexView::PaintLine(HDC hdc, size_w nLineNo, BYTE *data, size_t datalen, seqchar_info *infobuf)
{
	TCHAR	*buf;//[0x200];
	//BYTE	data[100];
	//seqchar_info infobuf[100];
	ATTR	*attrList;//[0x200];
	int		*advanceWidth;//[0x100];
	RECT	rect;
	RECT	clip;
	size_w	offset;
	size_t	i, len, lasti;	

	SelectObject(hdc, m_hFont);

	GetClientRect(m_hWnd, &rect);

	rect.left       = (long)(-m_nHScrollPos * m_nFontWidth);
	rect.top		= (long)((nLineNo - m_nVScrollPos) * m_nFontHeight);
	rect.right		= (long)(rect.right);
	rect.bottom		= (long)(rect.top + m_nFontHeight);

	CopyRect(&clip, &rect);

	// work out what data we want to draw
	// include the display-offset if we have shifted the
	// hex/ascii data by a certain amount
	offset	 = nLineNo * m_nBytesPerLine;// + m_nDataStart;

	// check we have data to draw on this line!
	if(offset - min(m_nDataShift, offset) >= m_pDataSeq->size())
	{
		SetTextColor(hdc, GetHexColour(HVC_BACKGROUND));
		SetBkColor(hdc,   GetHexColour(HVC_BACKGROUND));

		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
		return 0;
	}

	SetTextAlign(hdc, TA_UPDATECP);
	MoveToEx(hdc, -(int)(m_nHScrollPos * m_nFontWidth), (int)(nLineNo - m_nVScrollPos) * m_nFontHeight, NULL);

	//
	//	Construct a text buffer and colour description buffer
	//
	buf				= new TCHAR[m_nTotalWidth+100];
	attrList		= new ATTR[m_nTotalWidth];
	advanceWidth	= new int[m_nTotalWidth];

	//
	offset -= m_nDataShift;
	len		 = FormatLine(data, datalen, offset, buf, m_nTotalWidth, attrList, infobuf, true);

	for(i = 0; i < len; i++)
		advanceWidth[i] = m_nFontWidth;

	// 
	//	Display the line of text, as a series of coloured spans of text 
	//
	for(i = 0, lasti = 0; i <= len; i++)
	{
		// detect changes in colour
		if( attrList[i].colFG != attrList[lasti].colFG  ||
			attrList[i].colBG != attrList[lasti].colBG ||
			i == len)
		{
			SetTextColor(hdc, attrList[lasti].colFG);
			SetBkColor(hdc,   attrList[lasti].colBG);

			// need clipping because of the ClearType 'bleed'
			clip.right = clip.left + (int)(i - lasti) * m_nFontWidth;

			// output an entire 'span' of text in a single colour
			ExtTextOut(hdc, 0, 0, ETO_OPAQUE|ETO_CLIPPED, &clip, buf + lasti, (UINT)(i - lasti), advanceWidth);
			clip.left = clip.right;

			lasti = i;
		}
	}

	//
	//	Dead space to right of ascii column
	//
	SetTextColor(hdc, GetHexColour(HVC_BACKGROUND));
	SetBkColor(hdc,   GetHexColour(HVC_BACKGROUND));

	POINT pt;

	MoveToEx(hdc, 0, 0, &pt);
//	SetRect(&rect, pt.x, pt.y, rect.right, pt.y + m_nFontHeight);
//	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);

	delete[] buf;
	delete[] attrList;
	delete[] advanceWidth;

	return pt.x;
}

void DrawVLine(PPAINTSTRUCT pps, COLORREF col, int pos)
{
	RECT rc = pps->rcPaint;

	rc.left  = pos;
	rc.right = pos + 1;

	// paint a vertical line
	SetBkColor(pps->hdc, col);
	ExtTextOut(pps->hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	// mask it so the regular painting logic doesn't overwrite it
	ExcludeClipRect(pps->hdc, rc.left, rc.top, rc.right, rc.bottom);
}

void HexView::IdentifySearchPatterns(BYTE *data, size_t len, seqchar_info *infobuf)
{
	size_t i;
	BYTE *ptr = data;

	if(m_nSearchLen == 0)
		return;

	while((ptr = (BYTE *)memchr(ptr, m_pSearchPat[0], len-(ptr-data))) != 0)
	{
		size_t slen = min(m_nSearchLen, len-(ptr-data));

		for(i = 1; i < slen; i++)
		{
			if(ptr[i] != m_pSearchPat[i])
				break;
		}

		if(i == m_nSearchLen)
		{
			for(i = 0; i < m_nSearchLen; i++)
				infobuf[i + (ptr - data)].userdata = 1;

			ptr += m_nSearchLen;
		}
		else
		{
			ptr += 1;
		}
	}
}

//
//	WM_PAINT message handler
//
LRESULT HexView::OnPaint()
{
	PAINTSTRUCT ps;

	size_w i, first, last;

	BeginPaint(m_hWnd, &ps);

	// figure out which lines to redraw
	first = m_nVScrollPos + ps.rcPaint.top / m_nFontHeight;
	last  = m_nVScrollPos + ps.rcPaint.bottom / m_nFontHeight;

	if(ps.rcPaint.bottom % m_nFontHeight)
		last++;

	if(last < first) last = -1;

	// paint the resize bar
	if(CheckStyle(HVS_RESIZEBAR))
	{
		int pos1 = (m_nAddressWidth - m_nHScrollPos) * m_nFontWidth + 
					(m_nHexPaddingLeft * m_nFontWidth) / 2;

		pos1 += m_nFontWidth/2;
		DrawVLine(&ps, GetHexColour(HVC_RESIZEBAR), pos1);
		DrawVLine(&ps, GetHexColour(HVC_RESIZEBAR), m_nResizeBarPos);
	}

	// work out what data we want to draw
	// include the display-offset if we have shifted the
	// hex/ascii data by a certain amount
	size_w offset = first * m_nBytesPerLine;// + m_nDataStart;
	size_t buflen = (size_t)(last - first + 1) * m_nBytesPerLine + m_nDataShift;

	BYTE *bigbuf;
	seqchar_info *bufinfo;

	if((bigbuf = new BYTE[buflen]) == 0)
		return 0;

	if((bufinfo = new seqchar_info[buflen]) == 0)
	{
		delete[] bigbuf;
		return 0;
	}
	
	int shift = 0;
	int shift2 = 0;
	if(m_nDataShift > offset)
	{
		shift = max(m_nDataShift, 0);//m_nDataStart % m_nBytesPerLine;
		memset(bigbuf, '?', shift);
	}
	else
	{
		shift2 = m_nDataShift;//abs(min(m_nDataShift, 0));
	}
	
	buflen = m_pDataSeq->render(offset - shift2, bigbuf + shift, buflen - shift, bufinfo + shift);
	buflen += shift;

	// identify any characters that match the current search-pattern
	IdentifySearchPatterns(bigbuf, buflen, bufinfo);

	RECT rect;
	GetClientRect(m_hWnd, &rect);
	rect.left = LogToPhyXCoord(m_nBytesPerLine, 1);

	//TRACEA("filling from %d\n", rect.left);

	// draw the display line-by-line
	for(i = first; i <= last; i++)
	{
		offset = (i - first) * m_nBytesPerLine;

		size_t len = (size_t)min(buflen - offset, m_nBytesPerLine);
		fixthis = (i == 0) ? m_nDataShift : 0;
		
		int width = PaintLine(ps.hdc, i, bigbuf + offset, len, bufinfo + offset);

		
		// draw the ghost item
		/*if(offset >= SelectionStart() && offset < SelectionStart() + m_nBytesPerLine && offset < SelectionEnd())
		{
			m_HighlightGhost.hp.start = SelectionStart();
			m_HighlightGhost.hp.length = SelectionSize();
			m_HighlightGhost.hp.colBG  = RGB(255,200,200);

			DrawNoteStrip(ps.hdc, width+BOOKMARK_XOFFSET, (i-m_nVScrollPos) * m_nFontHeight, 
				&m_HighlightGhost);
		}*/

		BOOKNODE * bnp;
		for(bnp = m_BookHead->next; bnp != m_BookTail; bnp = bnp->next)
		//for(j = 0; j < m_Highlight->nNumItems; j++)
		{
			BOOKMARK *bookmark = &bnp->bookmark;

			if(bookmark->pszText || bookmark->pszTitle)
			{
				if( bookmark->offset >= i*m_nBytesPerLine && 
					bookmark->offset < (i+1)*m_nBytesPerLine && 
					bookmark->offset < m_pDataSeq->size())
				{
					//ExcludeClipRect(ps.hdc, rect.left + 20,
					DrawNoteStrip(ps.hdc, width+BOOKMARK_XOFFSET, (int)(i-m_nVScrollPos) * m_nFontHeight, bnp);
				}
			}
		}
	}

	ExtTextOut(ps.hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);

	EndPaint(m_hWnd, &ps);

	delete[] bigbuf;
	delete[] bufinfo;

	return 0;
}

HRGN ThemeEditBorder(HWND hwnd, HTHEME hTheme, HRGN hrgnUpdate)
{
	HDC hdc = GetWindowDC(hwnd);
	RECT rc;
	RECT rcWindow;
	DWORD state = ETS_NORMAL;
	HRGN hrgnClip;

	//TRACEA("ThemeEditBorder\n");
	
	if(!IsWindowEnabled(hwnd))
	{
		state = ETS_DISABLED;
	}
	else if(GetFocus() == hwnd)
	{
		state = ETS_NORMAL;//ETS_HOT;
	}
	else
	{
		state = ETS_NORMAL;
	}
	
	GetWindowRect(hwnd, &rcWindow);
	GetClientRect(hwnd, &rc);
	ClientToScreen(hwnd, (POINT *)&rc.left);
	ClientToScreen(hwnd, (POINT *)&rc.right);

	//TRACERECT(

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
		6,//EP_EDITTEXT,//6,
		state,
		//EP_EDITTEXT, 
		//state, 
		//3,0,
		&rcWindow, NULL);
	
	ReleaseDC(hwnd, hdc);

	return hrgnClip;
}



LRESULT HexView::OnNcPaint(HRGN hrgnUpdate)
{
	HRGN	hrgnClip = hrgnUpdate;
	LRESULT res;

	if(m_hTheme != 0)
	{
		hrgnClip = ThemeEditBorder(m_hWnd, m_hTheme, hrgnUpdate);
	}

	res = DefWindowProc(m_hWnd, WM_NCPAINT, (WPARAM)hrgnClip, 0);	
	DeleteObject(hrgnClip);
	return res;
}


void HexView::RefreshWindow()
{
	InvalidateRect(m_hWnd, NULL, FALSE);
}