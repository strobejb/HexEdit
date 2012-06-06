//
//  GridViewEdit.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "GridViewInternal.h"
#include "trace.h"


// variables needed by the g_hMouseHook
//static GridView *g_gvpHook;
static HHOOK	g_hMouseHook = NULL;		//the mouse hook
static HWND		g_hwndCombo;
static HWND		g_hwndEdit;
static HWND		g_hwndParent;
static BOOL		g_fMouseReleased;

//
//	WH_MOUSE hook procedure for the ComboLBox
//
//	The only reason this hook is necessary is to detect when the mouse is clicked 
//  outside of the ComboLBox. In this event the mouse message is 'eaten' and
//  prevented from occurring - it basically gives the user the ability to cancel
//  the dropdown by clicking anywhere outside the list, making it closeup - eating
//  the message ensures that the click does not affect any underlying windows
//  If we just wanted to close the droplist every time the user clicks somewhere else,
//	we could catch the EN_KILLFOCUS of the editbox and do it that way. But by
//	using the hook our behaviour is now identical to standard combobox
//
LRESULT CALLBACK GridView::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode < 0)
		return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);

	// get the window under the mouse
	POINT	pt = *(POINT*)lParam;
	DWORD	pid;
	HWND	hwndCurrent = WindowFromPoint(pt);

	if(wParam == WM_LBUTTONUP)
		g_fMouseReleased = TRUE;
	
	// get the process that owns the window
	GetWindowThreadProcessId(hwndCurrent, &pid);

	// 'eat' any non-left-button messages on our own process's windows
	if(pid == GetCurrentProcessId() && (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP))
		return -1;	

	// If the mouse moves over the listbox and the left mouse-button
	// is still pressed down after double-clicking the gridview item, then 
	// go into 'mouse-selection' straight away
	if(hwndCurrent == g_hwndCombo)
	{
		if(g_fMouseReleased == FALSE)
		{
			// fake a left mouse-click on the droplist to start a 'mouse selection'
			ScreenToClient(g_hwndCombo, &pt);
			PostMessage(g_hwndCombo, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));

			g_fMouseReleased = TRUE;
			return 0;
		}

		return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
	}
	
	// Otherwise, we have clicked outside the combo box.
	if(hwndCurrent != g_hwndEdit && (wParam == WM_LBUTTONDOWN || wParam == WM_NCLBUTTONDOWN))
	{
		// notify the parent that it should cancel the edit operation and hide the list
		PostMessage(g_hwndParent, WM_COMMAND, MAKEWPARAM(0, CBN_SELENDCANCEL), (LPARAM)g_hwndCombo);
		
		// If we have clicked on a window belonging to our application,
		// then disregard this mouse message, otherwise just let the
		// system forward the message and task-switch for us.
		
		// make an exception for the containing window that hosts us
		if(pid == GetCurrentProcessId() && hwndCurrent != g_hwndParent)
			return -1;		
	}

	// Just behave like normal and pass the message through
	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}


void SelectComboItem(HWND hwndCombo, size_t idx)
{
	UINT style = GetWindowLong(hwndCombo, GWL_STYLE);
	
	if(style & LBS_MULTIPLESEL)//|LBS_EXTENDEDSEL))
	{
		if(SendMessage(hwndCombo, LB_GETSEL, idx, 0))
			SendMessage(hwndCombo, LB_SETSEL, FALSE, idx);
		else
			SendMessage(hwndCombo, LB_SETSEL, TRUE, idx);
	}
	else
	{
		SendMessage(hwndCombo, LB_SETCURSEL, (WPARAM)idx, 0);
	}
}

//
//	Subclass for the COMBOLBOX
//
//	For some reason that builtin ComboLBox is completely unfunctional, it does not
//  respond to mouseclicks or other events. Therefore this subclass handles mouse messages
//  and introduces the ability to select listitems using the mouse.
//
LRESULT CALLBACK GridView::ComboLBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINT	pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
	size_t	idx;
	static  size_t curidx;

	GridView *gvp = (GridView *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	switch(msg)
	{
	// Left button is clicked inside the list
	case WM_LBUTTONDOWN:
			
		// see what item the mouse is over, then select it. 
		idx		= SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y)); 
		curidx	= idx;
			
		SelectComboItem(hwnd, idx);
		
		SetCapture(hwnd);

		// we don't need the hook anymore
		UnhookWindowsHookEx(g_hMouseHook);		
		g_hMouseHook = 0;
		break;

	// Timer is simply used to generate regular WM_MOUSEMOVE messages
	// whilst scrolling with the mouse
	case WM_TIMER:

		// just get the *client* cursor position and fall through to the mousemove handler
		GetCursorPos(&pt);
		ScreenToClient(hwnd, &pt);

	// the mouse has moved over the list
	case WM_MOUSEMOVE:

		if(GetCapture() == hwnd)
		{
			// get the listitem under the mouse. If the mouse is outside the 
			// list, then HIWORD(idx) will be '1' (no selected item)
			size_t idx    = SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y)); 
			size_t topidx = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);	
			bool   sel    = false;
			
			// if mouse is at bottom of control then scroll downwards
			if(topidx != LOWORD(idx) && curidx != idx)
			{
				SelectComboItem(hwnd, LOWORD(idx));
				sel=true;
			}

			// if the mouse is over the listbox then select the item underneath
			if(HIWORD(idx) == 0)
			{
				if(!sel && curidx != idx)SelectComboItem(hwnd, LOWORD(idx));
				KillTimer(hwnd, 0xdeadbeef);
			}
			// otherwise the mouse is outside. 
			else
			{
				// if the mouse is at the top of the control then scroll up
				if(topidx == LOWORD(idx))
					SendMessage(hwnd, LB_SETTOPINDEX, (WPARAM)topidx - 1, 0);
				
				// remove any selection from the list, because the mouse is outside
				SelectComboItem(hwnd, -1);
				SetTimer(hwnd, 0xdeadbeef, 100, 0);
			}

			curidx = idx;
		}

		return 0;

	// mouse has been released
	case WM_LBUTTONUP:

		KillTimer(hwnd, 0xdeadbeef);

		if(GetCapture() == hwnd)
		{
			ReleaseCapture();

			// see what item was selected (HIWORD of idx will be '1' if no item)
			size_t idx    = SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y)); 
			UINT   cbnmsg = HIWORD(idx) == 0 ? CBN_SELENDOK : CBN_SELENDCANCEL;

			// notify the parent if a selection was made, or cancelled.
			// leave it to them to destroy the droplist
			SendMessage(gvp->m_hWnd, WM_COMMAND, MAKEWPARAM(0, cbnmsg), (LPARAM)hwnd);
			ShowWindow(hwnd, SW_HIDE);
		}

		return 0;
	}

	return CallWindowProc(gvp->m_oldComboProc, hwnd, msg, wParam, lParam);
}

//
//	Create a 'ComboLBox' - the same kind of listbox used for
//	the droplist in a standard ComboBox control
//
//	The list will be created in a certain way to make it 'popup'
//  over all other windows:
//
//	It is WS_EX_TOOLWINDOW so it doesn't gain focus
//	It is a child of the desktop, so it doesn't get clipped by the 'owner' window
//  It must be 'owned' by the gridview so we have a place to send messages
//
//	You can use the standard LB_xxx messages
//
HWND GridView::CreateComboLBox(HWND hwndOwner)
{
	HWND hwnd;

	UINT uStyles = WS_BORDER | WS_VSCROLL | WS_OVERLAPPED | 
			WS_CHILDWINDOW | WS_CLIPSIBLINGS|
			//LBS_COMBOBOX|
			//LBS_OWNERDRAWFIXED|
			LBS_HASSTRINGS|LBS_NOTIFY|LBS_MULTIPLESEL;//LBS_EXTENDEDSEL   ;

	// create the window. The parent/owner specified is the
	// one that will receive messages from the combolbox
	hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
			_T("ComboLBox"), 
			_T("Choose Color"), uStyles,
			500,300,100,10, 
			hwndOwner, 
			0, GetModuleHandle(0), 0);

	// subclass it to provide mouse-selection functionality
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
	m_oldComboProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)GridView::ComboLBoxProc);

	// make it a 'child' of the desktop window
	SetParent(hwnd, GetDesktopWindow());

	// install a WH_MOUSE hook so we have a way to detect when the mouse is clicked
	// outside the list. We could always rely on EM_KILLFOCUS from the associated edit control,
	// but we want the hook to 'eat' messages as well
	g_hMouseHook		= SetWindowsHookEx(WH_MOUSE, GridView::MouseHookProc, 0, GetCurrentThreadId());
	g_hwndCombo			= hwnd;
	g_hwndParent		= hwndOwner;
	g_fMouseReleased	= FALSE;

	return hwnd;
}

VOID GridView::ShowComboLBox(int x, int y, int width, int height)
{
	RECT rect = { x, y, x+width, y+height };
	HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(mi) };
	
	GetMonitorInfo(hMonitor, &mi);
	if(y + height > mi.rcWork.bottom)
		y -= height + m_nLineHeight;

	SetWindowPos(m_hwndComboLBox, HWND_TOPMOST, x, y, width, height, 
		SWP_NOACTIVATE
		);

}

//
//	Subclass for the EDIT control
//
//	Used to detect cursor keys (up/down) to allow navigation of the combolist
//	Also used to detect Enter/Esc for accepting/cancelling the edit operation
//
LRESULT CALLBACK GridView::EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	GridView *gvp = (GridView *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	size_t idx,count;
	TCHAR szText[200];

	switch(msg)
	{
	case WM_MOUSEACTIVATE:
		SetFocus(hwnd);
		return MA_ACTIVATE;

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_CHAR:

		if(wParam == _T('\r'))
			return 0;

		// make sure the character gets entered into the EDIT box
		CallWindowProc(gvp->m_oldEditProc, hwnd, msg, wParam, lParam);

		// autocomplete of the combo droplist:
		// search the ComboLBox for the text in the edit box
		GetWindowText(hwnd, szText, 200);
		idx = SendMessage(gvp->m_hwndComboLBox, LB_FINDSTRING, -1, (LPARAM)szText);

		// if we found a match then select the corresponding entry in the ComboLBox
		if(idx != LB_ERR)
			SendMessage(gvp->m_hwndComboLBox, LB_SETCURSEL, idx, 0);

		return 0;

	case WM_KEYDOWN:
	
		switch(wParam)
		{			
		// Escape - cancel the edit operation
		case VK_ESCAPE:
			gvp->ExitEditMode(FALSE);
			break;

		// Return - accept the edit changes
		case VK_RETURN:
			gvp->ExitEditMode(TRUE);
			break;

		// Arrow Up - scroll up through the ComboLBox
		case VK_UP:

			if(gvp->m_hwndComboLBox)
			{
				// Select the previous item in the droplist
				idx = SendMessage(gvp->m_hwndComboLBox, LB_GETCURSEL, 0, 0);
				if(--idx < 0) idx = 0;		
				SendMessage(gvp->m_hwndComboLBox, LB_SETCURSEL, idx, 0);

				// Update the editbox text to reflect the current selection
				SendMessage(gvp->m_hwndComboLBox, LB_GETTEXT, idx, (LPARAM)szText);
				SetWindowText(hwnd, szText);
				SendMessage(hwnd, EM_SETSEL, 0,-1);		
			}

			return 0;

		// Arrow Down - scroll down through the ComboLBox
		case VK_DOWN:

			if(gvp->m_hwndComboLBox)
			{
				// Select the next item in the droplist
				idx   = SendMessage(gvp->m_hwndComboLBox, LB_GETCURSEL, 0, 0);
				count = SendMessage(gvp->m_hwndComboLBox, LB_GETCOUNT, 0, 0);
				if(++idx >= count) idx = count-1;
				SendMessage(gvp->m_hwndComboLBox, LB_SETCURSEL, idx, 0);

				// Update the editbox text to reflect the current selection
				SendMessage(gvp->m_hwndComboLBox, LB_GETTEXT, idx, (LPARAM)szText);
				SetWindowText(hwnd, szText);
				SendMessage(hwnd, EM_SETSEL, 0,-1);
			}

			return 0;
		}

		break;
	}

	return CallWindowProc(gvp->m_oldEditProc, hwnd, msg, wParam, lParam);
}

HFONT GridView::GetFont(GVITEM *gvitem, GVCOLUMN *gvcol)
{
	HFONT hFont;

	hFont = m_hFont[0];

	if(gvcol->hFont != 0)
		hFont = gvcol->hFont;


	if(gvitem->mask & GVIF_FONT)
	{
		int idx = 0;
		idx = gvitem->iFont;
				
		if(idx < 0 || idx >= MAX_GRIDVIEW_FONTS)
			idx = 0;

		hFont = m_hFont[idx];
	}
				
	return hFont;
}

bool GridView::EnterEditMode()
{
	RECT rect;
	RECT border;
	DWORD dwEditStyle = ES_WANTRETURN | ES_MULTILINE|ES_AUTOHSCROLL| WS_CHILD;

	GVRow *gvrow;
	GVCOLUMN *gvcol;
	GVITEM  *gvitem;

	if((gvrow = m_gvData.GetRow(m_nCurrentLine)) == 0)
		return false;

	if((gvcol = GetColumn(m_nCurrentColumn, NULL)) == 0)
		return false;

	if((gvitem	= &gvrow->items[m_nCurrentColumn]) == 0)
		return false;

	// is the item, column or the entire grid readonly?
	if((gvitem->state & GVIS_READONLY) || (m_uState & GVS_READONLY) || (gvcol->uState & GVCS_READONLY))
		return false;

	bool fDropDown = gvitem->state & (GVIS_DROPDOWN|GVIS_DROPLIST) ? true : false;

	GetItemRect(m_nCurrentColumn, m_nCurrentLine, &rect);
	rect.left += m_nCurrentColumn == 0 ? (gvrow->TreeIndent() + 1) * LEVEL_WIDTH + 4 : 0;
	
	if(gvitem->mask & GVIF_IMAGE)
		rect.left += 14;
	OffsetRect(&rect, -m_nHScrollPos, 0);

		
	if(gvcol->uState & GVCS_ALIGN_RIGHT)
		dwEditStyle |= ES_RIGHT;

	if(fDropDown)
		m_hwndComboLBox = CreateComboLBox(m_hWnd);

	if(gvitem->state & GVIS_DROPLIST)
		dwEditStyle |= ES_READONLY;

	m_hwndEdit = CreateWindowEx(0, TEXT("EDIT"), TEXT("blah"), 
		dwEditStyle,
		0,0,0,0,
		m_hWnd, 0, 0, 0);

	m_fEditError = false;

	SetWindowLongPtr(m_hwndEdit, GWLP_USERDATA, (LONG_PTR)this);
	m_oldEditProc = (WNDPROC)SetWindowLongPtr(m_hwndEdit, GWLP_WNDPROC, (LONG_PTR)GridView::EditProc);

	HFONT hFont = GetFont(gvitem, gvcol);
	SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)hFont, 0);

	// set the text to be the same as the underlying grid cell
	SetWindowText(m_hwndEdit, gvitem->pszText);
	//m_gvData.GetRowItem(gvrow, 

	SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);

	// resize the window *before* setting the borders
	if(fDropDown)
		rect.right -= 18;//GetSystemMetrics(SM_CXVSCROLL);

	SetWindowPos(m_hwndEdit, 0,//m_hWnd, 
		rect.left+6, 
		rect.top+1, 
		rect.right-rect.left-1-6, 
		rect.bottom-rect.top-1-1,
		0);

	SendMessage(m_hwndEdit, EM_GETRECT, 0, (LPARAM)&border);
	border.left = 0;
	SendMessage(m_hwndEdit, EM_SETRECT, 0, (LPARAM)&border);
//	border.top  = 1;
	//bor
//	border.bottom = m_nLineHeight-1;
/*	SendMessage(m_hwndEdit, EM_SETRECT, 0, (LPARAM)&border);
	SendMessage(m_hwndEdit, EM_GETRECT, 0, (LPARAM)&border);*/
	
	SetFocus(m_hwndEdit);
	RedrawLine(m_nCurrentLine);
	UpdateWindow(m_hWnd);

	ShowWindow(m_hwndEdit, SW_SHOW);

	// does this item require a droplist?
	if(fDropDown)
	{
		g_hwndEdit = m_hwndEdit;

		MapWindowPoints(m_hWnd, 0, (POINT *)&rect, 2);
		SendMessage(m_hwndComboLBox, WM_SETFONT, (WPARAM)hFont, FALSE);

		size_t idx = SendMessage(m_hwndComboLBox, LB_FINDSTRING, -1, (LPARAM)gvitem->pszText);
		SendMessage(m_hwndComboLBox, LB_SETCURSEL, idx, 0);
		SendMessage(m_hwndComboLBox, LB_SETTOPINDEX, idx, 0);
		ShowComboLBox(rect.left, rect.top+m_nLineHeight-1, (rect.right-rect.left)*3/2, 160);

		// tell the parent that we are about to show the dropdown. this gives them
		// the chance to add items to it etc
		NotifyParent(GVN_DROPDOWN, (HGRIDITEM)gvrow);

		ShowWindow(m_hwndComboLBox, SW_SHOW);
	}

	return true;
}

//
//	ExitEditMode
//
//	Hide the edit/droplist and update the gridview if required
//
bool GridView::ExitEditMode(BOOL fAcceptChange)
{
	TRACEA("ExitEdit\n");
	TRACEA("UNHOOKING3???\n");

	if(m_fInNotify)
		return false;

	if(fAcceptChange)
	{
		GVITEM *gvitem = 0;
		GVITEM  gvold;
		TCHAR  *oldtxt = 0;
		TCHAR  *newtxt = 0;
		ULONG   newlen = 0;
		LRESULT result = 0;

		GVRow *gvrow = GetRowItem(m_nCurrentLine, m_nCurrentColumn, &gvitem);
	
		// get the new text for the grid-item
		if(gvrow)
		{
			newlen = max(GetWindowTextLength(m_hwndEdit) + 1, gvitem->cchTextMax);

			if((newtxt = new TCHAR[newlen]) == 0)
				return false;

			gvold  = *gvitem;
			//oldtxt = gvitem->pszText;
			gvitem->pszText = newtxt;

			GetWindowText(m_hwndEdit, newtxt, newlen);			
		}

		// notify the parent that we've inserted/changed an item
		if(m_pTempInsertItem)
			result = NotifyParent(GVN_INSERTED, m_pTempInsertItem);
		else
			result = NotifyParent(GVN_CHANGED, gvrow);

		// default behaviour is to allow the change
		if(result == 0)
		{
			m_pTempInsertItem = 0;

			delete[] gvold.pszText;
			//gvitem->pszText = oldtxt;
			//lstrcpy(gvitem->pszText, tmpbuf);
		}
		// any non-zero value returned from WM_NOTIFY cancels the change,
		// so restore the grid-item's text and flag up an error
		else
		{
			delete[] gvitem->pszText;
			*gvitem = gvold;//->pszText = oldtxt;

			//gvitem->pszText = oldtxt;
			//SendMessage(m_hwndEdit, EM_SETSEL, 0,-1);
			SetFocus(m_hwndEdit);
			InvalidateRect(m_hwndEdit, 0, TRUE);
			MessageBeep(MB_ICONINFORMATION);
			m_fEditError = true;
			return false;
		}
	}
	// remove the temporary-inserted row if we are cancelling the edit
	else if(m_pTempInsertItem)
	{
		DeleteRow((GVRow *)m_pTempInsertItem);
		m_pTempInsertItem = 0;
		InvalidateRect(m_hWnd, 0, 0);
	}


	// remove the g_hMouseHook
	UnhookWindowsHookEx(g_hMouseHook);
	g_hMouseHook = 0;

	// grab the text in the EDIT box and update the griditem
	/*if(fAcceptChange)
	{
		GVITEM *gvitem;

		if(GetRowItem(m_nCurrentLine, m_nCurrentColumn, &gvitem))
		{
			GetWindowText(m_hwndEdit, gvitem->pszText, 100);//gvitem->cchTextMax);
		}
	}*/
	
	DestroyWindow(m_hwndEdit);	
	m_hwndEdit = 0;

	DestroyWindow(m_hwndComboLBox);
	m_hwndComboLBox =0;
 
	RedrawLine(m_nCurrentLine);
	
	return true;
}

/*LRESULT GridView::OnDrawItem(UINT uId, DRAWITEMSTRUCT *dis)
{
	if(dis->hwndItem == m_hwndEdit)
	{
		dis->
	}
	return 0;
}*/

LRESULT GridView::OnColorEdit(HDC hdc, HWND hwndCtrl)
{
	if(m_fEditError)
		SetTextColor(hdc, RGB(200,0,0));

	SetBkColor(hdc, GetColour(GVC_BACKGROUND));
	return 0;
}

