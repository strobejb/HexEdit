//
//  GridViewKeyboard.cpp
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

bool IsKeyPressed(UINT);

//
//	A character has been 
//
LRESULT GridView::OnChar(UINT nChar)
{
	TCHAR ch[] = { (TCHAR)nChar, '\0' };

	if(nChar < 32)		return 0;

	// pop up the field-edit control
	EnterEditMode();

	// overwrite it's text with this single character
	SetWindowText(m_hwndEdit, ch);
	SendMessage(m_hwndEdit, EM_SETSEL, 1, 1);
	return 0;
}

ULONG GridView::L2V(ULONG lidx)
{
	for(ULONG i = 0; i < m_nNumColumns; i++)
	{
		if(lidx == (ULONG)Header_OrderToIndex(m_hWndHeader, i))
			return i;
	}
	return 0;
}

ULONG GridView::V2L(ULONG vidx)
{
	return Header_OrderToIndex(m_hWndHeader, vidx);
}


LRESULT GridView::OnKeyDown(UINT nKeyCode, UINT nFlags)
{
	bool fCtrlDown	= IsKeyPressed(VK_CONTROL);
	bool fShiftDown	= IsKeyPressed(VK_SHIFT);
	ULONG oldLine   = m_nCurrentLine;
	ULONG oldCol	= m_nCurrentColumn;

	GVRow *gvrow = GetRowItem(m_nCurrentLine, m_nCurrentColumn, 0);
	GVITEM gvitem = { 0 };

	switch(nKeyCode)
	{
	case VK_RETURN: case VK_F2:
		
		if(CheckStyle(GVS_READONLY) == FALSE)
		{
			EnterEditMode();
		}

		return 0;
		
	// ignore shift/control by themselves
	case VK_SHIFT: case VK_CONTROL:
		return 0;

	// insert a new row
	case VK_INSERT:

		if(fCtrlDown)
		{
			OnCopy(gvrow);
		}
		// ask the parent we are allowed
		else if(CheckStyle(GVS_READONLY) == FALSE && NotifyParent(GVN_CANINSERT, gvrow) == 0)
		{
			if(fShiftDown)
			{
				OnPaste(gvrow, NULL);
			}
			else
			{
				OnPaste(gvrow, _T("\0\0"));
			}

			m_nCurrentColumn = 0;
			EnterEditMode();
		}
		
		return 0;

	// delete current row
	case VK_DELETE:

		if(CheckStyle(GVS_READONLY) == FALSE && NotifyParent(GVN_CANDELETE, gvrow) == 0)
		{
			// notify the parent *before* deleting the item!
			// this way the parent can query the item to see what was deleted :)
			NotifyParent(GVN_DELETED, gvrow);

			if(fShiftDown)
				OnCut(gvrow);
			else
				OnClear(gvrow);			
		}

		return 0;

	case VK_LEFT:
		if(fCtrlDown) 
		{
			Scroll(-10, 0);
		}
		else
		{
			if(gvrow->HasChildren() && m_nCurrentColumn == 0 && gvrow->Expanded()) // in the tree thing?
			{
				ToggleRow(gvrow, m_nCurrentLine);
			}
			else 
			{
				ULONG pos = L2V(m_nCurrentColumn);
				if(pos > 0) pos--;
				m_nCurrentColumn = V2L(pos);
			}
		}
		break;

	case VK_RIGHT:
		if(fCtrlDown) 
			Scroll(10, 0);
		else
		{
			if(gvrow->HasChildren() && m_nCurrentColumn == 0 && !gvrow->Expanded()) // in the tree thing?
			{
				ToggleRow(gvrow, m_nCurrentLine);
			}
			else 
			{
				ULONG pos = L2V(m_nCurrentColumn);
				if(pos < m_nNumColumns - 1) pos++;
				m_nCurrentColumn = V2L(pos);
			}
		}

		break;

	case VK_UP:
		if(fCtrlDown) 
			Scroll(0, -1);
		else if(m_nCurrentLine > 0)
			m_nCurrentLine--;

		break;

	case VK_DOWN:
		if(fCtrlDown) 
			Scroll(0, 1);
		else if(m_nCurrentLine < m_gvData.VisibleRows()-1)
			m_nCurrentLine++;
		break;

	case VK_PRIOR:
		if(!fCtrlDown)
			Scroll(0, -m_nWindowLines);
		break;

	case VK_NEXT:
		if(!fCtrlDown)
			Scroll(0, m_nWindowLines);
		break;

	case VK_HOME:
		if(fCtrlDown)
			m_nCurrentLine = 0;
		else
			m_nCurrentColumn = V2L(0);
		break;

	case VK_END:
		if(fCtrlDown)
			m_nCurrentLine = m_gvData.VisibleRows()-1;
		else
			m_nCurrentColumn = V2L(m_nNumColumns-1);
		break;
	}

	// did the current selection change?
	if(m_nCurrentLine != oldLine || m_nCurrentColumn != oldCol)
	{
		gvrow = GetRowItem(m_nCurrentLine, m_nCurrentColumn, 0);
		NotifyParent(GVN_SELCHANGED, gvrow);

		RedrawLine(m_nCurrentLine);
		RedrawLine(oldLine);
		ScrollToLine(m_nCurrentLine);
	}

	return 0;
}