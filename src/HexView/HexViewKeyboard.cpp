//
//  HexViewKeyboard.cpp
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

bool IsKeyDown(UINT uVirtualKey)
{
	return (GetKeyState(uVirtualKey) & 0x80000000) ? true : false;
}

void HexView::ScrollToCaret()
{
	int x, y;
	int dx = 0, dy = 0;

	// first of all bring the view into view
	ScrollTo(m_nCursorOffset);

	if((m_nCursorOffset + m_nDataShift) % m_nBytesPerLine != 0)
	{
		m_fCursorAdjustment = FALSE;
	}

	// now find cursor position
	CaretPosFromOffset(m_nCursorOffset, &x, &y);

	if(y < 0)
		dy = y;
	else if(y > m_nWindowLines - 1)
		dy = y - m_nWindowLines + 1;

	if(x < 0)
		dx = x;
	
	Scroll(dx, dy);

	CaretPosFromOffset(m_nCursorOffset, &x, &y);
	PositionCaret(x, y, m_nWhichPane);
}


bool HexView::ForwardDelete()
{
	size_w length;
	bool success = false;

	if(!AllowChange(m_nCursorOffset, max(SelectionSize(), 1), HVMETHOD_DELETE))
		return false;

	if(SelectionSize() > 0)
	{
		length  = SelectionSize();
		success = m_pDataSeq->erase(SelectionStart(), length);

		m_nCursorOffset = SelectionStart();

		m_pDataSeq->breakopt();
	}
	else
	{
		length = 1;
		success = m_pDataSeq->erase(m_nCursorOffset, length);
	}

	m_nSelectionStart = m_nCursorOffset;
	m_nSelectionEnd   = m_nCursorOffset;

	if(success)
		ContentChanged(m_nCursorOffset, length, HVMETHOD_DELETE);

	return true;
}
	
bool HexView::BackDelete()
{
	size_w offset, length;
	bool success = false;

	if(SelectionSize())
	{
		offset = SelectionStart();
		length = SelectionSize();

		if(!AllowChange(offset, length, HVMETHOD_DELETE))
			return false;

		success = m_pDataSeq->erase(offset, length);
		m_nCursorOffset = offset;

		m_pDataSeq->breakopt();
	}
	else if(m_nCursorOffset > 0)
	{
		offset = --m_nCursorOffset;
		length = 1;

		if(!AllowChange(offset, length, HVMETHOD_DELETE))
			return false;

		success = m_pDataSeq->erase(offset, length);
	}

	m_nSelectionStart = m_nCursorOffset;
	m_nSelectionEnd   = m_nCursorOffset;

	if(success)
		ContentChanged(offset, length, HVMETHOD_DELETE);

	return true;
}


LRESULT HexView::OnKeyDown(UINT nVirtualKey, UINT nRepeatCount, UINT nFlags)
{
	BOOL	fForceUpdate = FALSE;
	bool	fCtrlDown	 = IsKeyDown(VK_CONTROL);
	bool	fShiftDown	 = IsKeyDown(VK_SHIFT);
	size_w  oldoffset    = m_nCursorOffset;

	fForceUpdate = !IsKeyDown(VK_SHIFT);

	if(nVirtualKey == VK_CONTROL || nVirtualKey == VK_SHIFT || nVirtualKey == VK_MENU)
		return 0;

	// take into account any offset/shift in the datasource
	// we'll restore it again after the cursor logic has taken place. this tricks
	// the switch-statement into thinking we are using a zero-based adddressing scheme,
	// which is important as the keyboard logic assumes this when keeping the cursor
	// within the bounds of the hex display
	//m_nCursorOffset += m_nDataShift;//+= min((size_w)-1 - m_nCursorOffset, m_nDataShift);

//	m_nCursorOffset += m_nDataShift;//+= min((size_w)-1 - m_nCursorOffset, m_nDataShift);

	switch(nVirtualKey)
	{
	case VK_ESCAPE:
		fForceUpdate = TRUE;
		break;

	case VK_INSERT:
		
		if(fCtrlDown)
		{
			OnCopy();
		}
		else if(fShiftDown)
		{
			OnPaste();
		}
		else if(CheckStyle(HVS_FIXED_EDITMODE) == 0)
		{
			if(m_nEditMode == HVMODE_INSERT)
				m_nEditMode = HVMODE_OVERWRITE;

			else if(m_nEditMode == HVMODE_OVERWRITE)
				m_nEditMode = HVMODE_INSERT;

			NotifyParent(HVN_EDITMODE_CHANGE);
		}

		return 0;

	case 'z': case 'Z':
		
		m_nSubItem = 0;

		if(fCtrlDown)
			Undo();

		return 0;

	// CTRL+Y redo
	case 'y': case 'Y':
		
		m_nSubItem = 0;

		if(fCtrlDown)
			Redo();

		return 0;

	case VK_DELETE:
		
		// cannot delete in readonly mode
		if(m_nEditMode == HVMODE_READONLY)
		{
			return 0;
		}
		// can only erase when in Insert mode
		else if(m_nEditMode == HVMODE_INSERT || 
			CheckStyle(HVS_ALWAYSDELETE) 
			)
		{
			ForwardDelete();
		}
		// overwrite mode - clear byte values
		else if(SelectionSize() > 0)
		{
			BYTE b[] = { 0 };
			FillData(b, 1, SelectionSize());
		}

		return 0;

	case VK_BACK:
		
		// cannot delete in readonly mode
		if(m_nEditMode == HVMODE_READONLY)
		{
			return 0;
		}
		// can only erase when in Insert mode
		else if(m_nEditMode == HVMODE_INSERT || 
			CheckStyle(HVS_ALWAYSDELETE)
			)
		{
			BackDelete();			
		}
		// overwrite mode - clear byte values
		else if(SelectionSize() > 0)
		{
			BYTE b[] = { 0 };
			FillData(b, 1, SelectionSize());
		}

		return 0;

	case VK_LEFT:

		//if ctrl held down, then scroll the viewport around!
		if(IsKeyDown(VK_CONTROL))
		{
			PostMessage(m_hWnd, WM_HSCROLL, SB_LINEUP, 0L);
			return 0;
		}

		if(m_nCursorOffset > 0) 
			m_nCursorOffset--;

		m_fCursorAdjustment = FALSE;
		break;

	case VK_RIGHT:

		//if ctrl held down, then scroll the viewport around!
		if(IsKeyDown(VK_CONTROL))
		{
			PostMessage(m_hWnd, WM_HSCROLL, SB_LINEDOWN, 0L);
			return 0;
		}
		
		if(m_nCursorOffset < m_pDataSeq->size()) 
		{
			m_nCursorOffset++;

			if(m_nCursorOffset == m_pDataSeq->size() &&  m_pDataSeq->size() % m_nBytesPerLine == 0)
				m_fCursorAdjustment = TRUE;
			else
				m_fCursorAdjustment = FALSE;
		}

		break;

	case VK_UP:

		//if ctrl held down, then scroll the viewport around!
		if(IsKeyDown(VK_CONTROL))
		{
			PostMessage(m_hWnd, WM_VSCROLL, SB_LINEUP, 0L);
			return 0;
		}

		if(m_nCursorOffset > (unsigned)m_nBytesPerLine)
			m_nCursorOffset -= m_nBytesPerLine;
		else if(m_nCursorOffset == m_nBytesPerLine && m_fCursorAdjustment == FALSE)
			m_nCursorOffset = 0;

		break;

	case VK_DOWN:

		//if ctrl held down, then scroll the viewport around!
		if(IsKeyDown(VK_CONTROL))
		{
			PostMessage(m_hWnd, WM_VSCROLL, SB_LINEDOWN, 0L);
			return 0;
		}
		
		m_nCursorOffset += min((size_w)m_nBytesPerLine, m_pDataSeq->size() - m_nCursorOffset);

		// if in the last partial line, don't go to end of file, rather
		// stay at "bottom" of file/window
		if(m_nCursorOffset >= m_pDataSeq->size() && !m_fCursorAdjustment)
		{		
			// test if in a partial line 
			if(	oldoffset % m_nBytesPerLine < m_pDataSeq->size() % m_nBytesPerLine  ||
				m_pDataSeq->size() % m_nBytesPerLine == 0)
			{
				m_nCursorOffset = oldoffset;
				fForceUpdate = TRUE;
			}
		}
	
		break;

	case VK_HOME:

		//if ctrl held down, then scroll the viewport around!
		if(fCtrlDown)
		{
			m_nCursorOffset = 0;
			PostMessage(m_hWnd, WM_VSCROLL, SB_TOP, 0L);
		}
		else
		{
			m_nCursorOffset += m_nDataShift;

			if(m_fCursorAdjustment && m_nCursorOffset > 0)
				m_nCursorOffset--;

			m_nCursorOffset -= m_nCursorOffset % m_nBytesPerLine;

			m_nCursorOffset -= min(m_nDataShift, m_nCursorOffset);
		}

		m_fCursorAdjustment = FALSE;
		break;

	case VK_END:
		
		if(IsKeyDown(VK_CONTROL))
		{
			m_nCursorOffset = m_pDataSeq->size();
		
			if(m_nCursorOffset % m_nBytesPerLine == 0)
				m_fCursorAdjustment = TRUE;

			PostMessage(m_hWnd, WM_VSCROLL, SB_BOTTOM, 0L);
		}
		else
		{
			// take into account the datashift
			// we need to 'shift' the end-of-file as well!
			m_nCursorOffset += m_nDataShift;
			size_w adjdocsize = m_pDataSeq->size() + m_nDataShift;

			// if not already at very end of line
			if(m_fCursorAdjustment == FALSE)
			{
				if(adjdocsize - m_nBytesPerLine >= m_nCursorOffset
					&& adjdocsize >= (size_t)m_nBytesPerLine)
				{
					m_nCursorOffset += m_nBytesPerLine - (m_nCursorOffset % m_nBytesPerLine);
					m_fCursorAdjustment = TRUE;
				}
				else
				{
					m_nCursorOffset += adjdocsize - m_nCursorOffset;
				}
			}

			if(m_nCursorOffset >= adjdocsize && adjdocsize % m_nBytesPerLine == 0)
				m_fCursorAdjustment = TRUE;

			m_nCursorOffset -= min(m_nDataShift, m_nCursorOffset);
		}

		break;

	case VK_PRIOR:		// pageup

		m_nCursorOffset -= min(m_nCursorOffset, (size_w)m_nBytesPerLine * m_nWindowLines);

		break;

	case VK_NEXT:		// pagedown

		m_nCursorOffset += min(m_pDataSeq->size() - m_nCursorOffset, (size_w)m_nBytesPerLine * m_nWindowLines);

		if(m_nCursorOffset >= m_pDataSeq->size() && m_pDataSeq->size() % m_nBytesPerLine == 0)
		{
			m_fCursorAdjustment = TRUE;
		}

		break;

	case VK_TAB:

		m_nWhichPane ^= 1;
		fForceUpdate = TRUE;
		
		if(m_ColourList[HVC_SELECTION] != m_ColourList[HVC_SELECTION2])
		{
			InvalidateRange(m_nSelectionStart, m_nSelectionEnd);
		}
		break;

	default:
		// don't know what this key is, so exit
		return 0;
	}

	m_nSubItem = 0;

	if(m_nCursorOffset != oldoffset || fForceUpdate)
	{
		// SHIFT key being held down?
		if(IsKeyDown(VK_SHIFT))
		{
			// extend the selection
			m_nSelectionEnd = m_nCursorOffset;
			InvalidateRange(oldoffset, m_nSelectionEnd);
		}
		else if(nVirtualKey != VK_TAB)
		{
			// clear any selection
			if(m_nSelectionEnd != m_nSelectionStart)
				InvalidateRange(m_nSelectionEnd, m_nSelectionStart);

			m_nSelectionEnd   = m_nCursorOffset;
			m_nSelectionStart = m_nCursorOffset;
		}

		ScrollToCaret();
		NotifyParent(HVN_CURSOR_CHANGE);

		if(nVirtualKey == VK_NEXT || nVirtualKey == VK_PRIOR)
		{
			RefreshWindow();
		}
	}

	return 0;
}

LRESULT HexView::OnChar(UINT nChar)
{
	if(nChar < 32)
		return 0;

	if(m_nEditMode == HVMODE_READONLY)
	{
		MessageBeep(MB_ICONASTERISK);
		return 0;
	}
		
	if(m_nWhichPane == 0)	// hex column
	{
		int  cl[4] = { 2, 3, 3, 8 };
		int  cb[4] = { 16, 10, 8, 2 };
		//int  cw[4] = { 2, 3, 3, 2 };
		int  cf = m_nControlStyles & HVS_FORMAT_MASK;
		int  val;
		BYTE b = 0;

		// get data under caret
		/*if(m_nSubItem > 0)
		{
			b = m_pDataSeq->getlastmodref();
		}
		else*/ if(m_nEditMode == HVMODE_INSERT)
		{
			b = 0;
		}
		else
		{
			GetData(m_nCursorOffset, &b, 1);
		}

		// check this is an allowed character
		if(cf == HVS_FORMAT_HEX && !isxdigit(nChar) ||
		   cf == HVS_FORMAT_DEC && !(nChar >= '0' && nChar <= '9')  ||
		   cf == HVS_FORMAT_OCT && !(nChar >= '0' && nChar <= '7')  ||
		   cf == HVS_FORMAT_BIN && !(nChar >= '0' && nChar <= '1')
		   )
		{
			MessageBeep(MB_ICONASTERISK);
			return 0;
		}

		int val2;
		if(nChar >= 'a')		val2 = nChar - 'a' + 0x0a;
		else if(nChar >= 'A')	val2 = nChar - 'A' + 0x0A;
		else					val2 = nChar - '0';

		int power = 1;
		int base  = cb[cf];
		for(int i = cl[cf] - 1; i > m_nSubItem; i--)
			power *= base;

		val = b;
		val = (val / power) % base;
		val *= power;
		val = b - val;
		val += val2 * power;

		// check that we won't overflow the underlying value
		if(val > 0xff)
		{
			//MessageBeep(MB_ICONASTERISK);
			//return 0;
			val -= b % power;
		}

		b = (BYTE)val;

		m_nSubItem++;

		if(m_fCursorMoved)
		{	
			// enter the data
			EnterData(&b, 1, m_nWhichPane == 0 ? false : true, true, false);
			
			m_nLastEditOffset = m_nCursorOffset;
			m_fCursorMoved	  = false;
		}
		else
		{
			// directly edit the byte in the sequence - this
			// prevents us from introducing any more spans than necessary
			// and keeps this as a single 'byte' edit
			m_pDataSeq->getlastmodref() = b;
			ContentChanged(m_nCursorOffset, 1, HVMETHOD_OVERWRITE);

			if(m_nSubItem == cl[cf])
			{
				m_nSubItem = 0;
				m_nCursorOffset++;
			}
			
			RepositionCaret();
		}


	}
	else
	{
		BYTE b = nChar;

		//if(!AllowChange(m_nCursorOffset, 1, HVMETHOD_OVERWRITE, &b))
			//return 0;

		// ascii column - enter the data as-is
		m_nSubItem = 0;
		EnterData(&b, 1, true, true, false);
	}

	return 0;
}

UINT method(size_w newlen, size_w oldlen)
{
	if(newlen == oldlen)
	{
		return HVMETHOD_OVERWRITE;
	}
	else if(newlen < oldlen)
	{
		return HVMETHOD_DELETE;
	}
	else if(newlen > oldlen)
	{
		return HVMETHOD_INSERT;
	}
	else
	{
		return 0;
	}
}

bool HexView::Undo()
{
	if(CheckStyle(HVS_DISABLE_UNDO))
		return false;
	
	size_w oldlen = m_pDataSeq->size();

	if(m_pDataSeq->undo())
	{
		m_nSelectionStart = m_pDataSeq->event_index();
		m_nSelectionEnd   = m_pDataSeq->event_length() + m_nSelectionStart;
		m_nCursorOffset   = m_nSelectionEnd;

		ContentChanged(m_pDataSeq->event_index(), m_pDataSeq->event_datalength(), method(m_pDataSeq->size(), oldlen));
		return true;
	}
	else
	{
		return false;
	}
}

bool HexView::Redo()
{
	if(CheckStyle(HVS_DISABLE_UNDO))
		return false;

	size_w oldlen = m_pDataSeq->size();

	if(m_pDataSeq->redo())
	{
		m_nSelectionStart = m_pDataSeq->event_index();
		m_nSelectionEnd   = m_pDataSeq->event_length() + m_nSelectionStart;
		m_nCursorOffset   = m_nSelectionEnd;

		ContentChanged(m_pDataSeq->event_index(), m_pDataSeq->event_datalength(), method(m_pDataSeq->size(), oldlen));
		return true;
	}
	else
	{
		return false;
	}
}


bool HexView::CanUndo()
{
	return m_pDataSeq->canundo();
}

bool HexView::CanRedo()
{
	return m_pDataSeq->canredo();
}
