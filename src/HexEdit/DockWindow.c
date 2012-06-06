//
//  DockWindow.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	DockWindow.c
//
//	A floating tool-window library
//	
//	Copyright J Brown 2001
//	Freeware
//
#include <windows.h>
#include <tchar.h>
#include "DockWindow.h"

// Need to keep track of the dock-windows for all the non-client painting stuff (See Tut#1)
#define MAX_DOCK_WINDOWS 64

#define NUMGRIPS 2

static HWND  hDockList[MAX_DOCK_WINDOWS];
static int   nNumDockWnds = 0;

static TCHAR szDockClass[] = _T("DockWnd32");
static ATOM  aDockClass    = 0;

//
//	Used for dragging
//
static HHOOK	draghook = 0;
static BOOL		fControl = FALSE;
static BOOL		fOldControl;
static BOOL		fOldDrawDocked;
static HWND		g_hwndDockWnd;

#define POPUP_STYLES   (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME)
#define POPUP_EXSTYLES (WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE)
#define CHILD_STYLES   (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)
#define CHILD_EXSTYLES (0)

static HWND GetOwner(HWND hwnd)
{
	return GetWindow(hwnd, GW_OWNER);
}

static BOOL IsOwnedBy(HWND hwndMain, HWND hwnd)
{
	return (hwnd == hwndMain) || (GetOwner(hwnd) == hwndMain);
}

// Does what it says on the tin
void DrawGripper(HDC hdc, int x, int y, int height, int numgrips)
{
	RECT rect;
	SetRect(&rect, x,y,x+3,y+height);

	if(numgrips == 1)
		OffsetRect(&rect, 1, 0);

	while(numgrips--)
	{
		DrawEdge(hdc, &rect,  BDR_RAISEDINNER, BF_RECT);
		OffsetRect(&rect, 3, 0);
	}
	//DrawEdge(hdc, &rect,  BDR_RAISEDINNER, BF_RECT);
}

//
//	Toggle any window between WS_POPUP and WS_CHILD
//
void TogglePopupStyle(HWND hwnd)
{
	DWORD dwStyle   = GetWindowLong(hwnd, GWL_STYLE);
	DWORD dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	
	if(dwStyle & WS_CHILD)
	{
		SetWindowLong(hwnd, GWL_STYLE,   (dwStyle   & ~CHILD_STYLES)   | POPUP_STYLES);
		SetWindowLong(hwnd, GWL_EXSTYLE, (dwExStyle & ~CHILD_EXSTYLES) | POPUP_EXSTYLES);
		SetParent(hwnd, NULL);
	}
	else
	{
		SetWindowLong(hwnd, GWL_STYLE,   (dwStyle   & ~POPUP_STYLES)   | CHILD_STYLES);
		SetWindowLong(hwnd, GWL_EXSTYLE, (dwExStyle & ~POPUP_EXSTYLES) | CHILD_EXSTYLES);
		SetParent(hwnd, GetOwner(hwnd));
	}

	// Send the window a WM_NCCALCSIZE message, because the
	// frame-style has changed.
	SetWindowPos(hwnd, 0, 0, 0, 0, 0,   SWP_NOMOVE   | SWP_NOSIZE     |
										SWP_NOZORDER | SWP_NOACTIVATE |
										SWP_FRAMECHANGED);
}

//
//	Work out how big a floating window should be,
//  taking into account the window contents, and current 
//  system settings
//
static void CalcFloatingRect(HWND hwnd, DockWnd *dwp)
{
	RECT rect;

	SetRect(&rect, 0, 0, dwp->cxFloating, dwp->cyFloating);
	AdjustWindowRectEx(&rect, POPUP_STYLES, FALSE, POPUP_EXSTYLES);

	dwp->nFrameWidth  = rect.right  - rect.left;
	dwp->nFrameHeight = rect.bottom - rect.top;
}

//
//	Adjust a dock-window's floating size
//
static void SetFloatingWinPos(HWND hwnd, DockWnd *dwp, DWORD dwSWFlags)
{
	CalcFloatingRect(hwnd, dwp);

	SetWindowPos(hwnd, HWND_TOP, 
			dwp->xpos, 
			dwp->ypos,
			dwp->nFrameWidth,
			dwp->nFrameHeight,
			dwSWFlags);
}

// 
//	Send a "fake" WM_SIZE to the specified window
//
static void Send_WM_SIZE(HWND hwnd)
{
	RECT rect;

	GetClientRect(hwnd, &rect);
	SendMessage(hwnd, WM_SIZE,  SIZE_RESTORED, MAKELPARAM(rect.right-rect.left,rect.bottom-rect.top));
}

//
//	Toggle the specified dock-window between docked/floating status
//
void DockWnd_ToggleDockingMode(HWND hwnd)
{
	HWND hwndParent = GetOwner(hwnd);

	DockWnd *dwp = (DockWnd *)GetWindowLong(hwnd, GWL_USERDATA);

	if(dwp == 0) return;

	// Hide the window, because we don't want it to
	// be in the wrong position when it is docked/undocked
	ShowWindow(hwnd, SW_HIDE);

	TogglePopupStyle(hwnd);
	dwp->fDocked = !dwp->fDocked;

	// Get parent to reposition if docked
	Send_WM_SIZE(hwndParent);

	if(dwp->fDocked == FALSE)
	{
		//SetWindowPos(hwnd, HWND_TOP, 
			//dwp->wpFloating.x, dwp->wpFloating.y,
			//dwp->wpFloating.cx, dwp->wpFloating.cy, 0);

		SetFloatingWinPos(hwnd, dwp, 0);
	}

	if(!(dwp->dwStyle & DWS_NOSETFOCUS))
		SetFocus(dwp->hwndContents);

	// Finally show the toolwindow when it is in the right place
	ShowWindow(hwnd, SW_SHOW);
}

//
//	DockWnd API
//	
//	Given an array of DockWnds, position any docked windows 
//	in the specified "Main" window,
//  The HDWP parameter must be obtained via BeginDeferWindowPos
//
BOOL DockWnd_Position(HWND hwndMain, HDWP hdwp, DockWnd dwnd[], int nNumDockWnds, RECT *rect)
{
	int i;

	// Dock the horizontal bars first (across the TOP+BOTTOM)
	for(i = 0; i < nNumDockWnds; i++)
	{
		if(dwnd[i].hwnd && dwnd[i].fDocked)
		{
			int n = dwnd[i].nDockedSize;
			
			switch(dwnd[i].uDockedState)
			{
			case DWS_DOCKED_TOP:
				//MoveWindow(dwnd[i].hwnd, rect->left, rect->top, rect->right-rect->left, n, TRUE);
				DeferWindowPos(hdwp, dwnd[i].hwnd, 0, rect->left, rect->top, rect->right-rect->left, n, SWP_NOZORDER);
				rect->top += n;
				break;
				
			case DWS_DOCKED_BOTTOM:
				//MoveWindow(dwnd[i].hwnd, rect->left, rect->bottom-n, rect->right-rect->left, n, TRUE);
				DeferWindowPos(hdwp, dwnd[i].hwnd, 0, rect->left, rect->bottom-n, rect->right-rect->left, n, SWP_NOZORDER);
				rect->bottom -= n;
				break;
			}
		}
	}
	
	// Only position if docked
	for(i = 0; i < nNumDockWnds; i++)
	{
		if(dwnd[i].hwnd && dwnd[i].fDocked)
		{
			int n = dwnd[i].nDockedSize;
			
			switch(dwnd[i].uDockedState)
			{
			case DWS_DOCKED_LEFT:
				//MoveWindow(dwnd[i].hwnd, rect->left, rect->top, n, rect->bottom-rect->top, TRUE);
				DeferWindowPos(hdwp, dwnd[i].hwnd, 0, rect->left, rect->top, n, rect->bottom-rect->top, SWP_NOZORDER);
				rect->left += n;
				break;
				
			case DWS_DOCKED_RIGHT:
				//MoveWindow(dwnd[i].hwnd, rect->right-n, rect->top, n, rect->bottom-rect->top, TRUE);
				DeferWindowPos(hdwp, dwnd[i].hwnd, 0, rect->right-n, rect->top, n, rect->bottom-rect->top, SWP_NOZORDER);
				rect->right -= n;
				break;
			}
		}
	}
	
	
	
	return TRUE;
}

//
//	DockWnd API
//
//	For the specified "Main" window, calculate where (if at all) the 
//  specified DockWindow might dock. Return DWS_DOCKED_FLOATING if
//  you don't want the window to dock in a certain place.
//
//	The NMDOCKWNDQUERY structure is obtained via the WM_NOTIFY message - 
//  you would probably only call this function whilst processing the DWN_ISDOCKABLE notification
//
//	ARGUMENTS:	hwnd		- Container window 
//				nmdwq		- pointer to NMDOCKWNDQUERY structure
//				rc1			- outer bounding rectangle of where docking is allowed (i.e. window frame)
//				rc2			- inner bounding rectangle of where docking is allowed (i.e. client area)
//
UINT DockWnd_GetDockSide(HWND hwnd, NMDOCKWNDQUERY *nmdwq, RECT *prc1, RECT *prc2)
{
	DockWnd *dwp      = nmdwq->pDockWnd;
	RECT    *dragrect = nmdwq->dragrect;
	HWND     hwndDock = nmdwq->hwndDock;
	
	RECT rc, rc1, rc2, inter;
	HDC hdc;

	// Make local copies of parameters
	rc1 = *prc1;
	rc2 = *prc2;
	
	hdc = GetWindowDC(hwnd);
	
	// Check intersection at top
	SetRect(&rc, rc1.left,rc1.top,rc1.right,rc2.top);
	if(IntersectRect(&inter, dragrect, &rc))
		return DWS_DOCKED_TOP;
	
	// Check intersection at bottom
	SetRect(&rc, rc1.left,rc2.bottom, rc1.right,rc1.bottom);
	if(IntersectRect(&inter, dragrect, &rc))
		return DWS_DOCKED_BOTTOM;
	
	// Check intersection at left
	SetRect(&rc, rc1.left,rc2.top,rc2.left,rc2.bottom);
	if(IntersectRect(&inter, dragrect, &rc))
		return DWS_DOCKED_LEFT;
	
	// Check intersection at right
	SetRect(&rc, rc2.right, rc2.top, rc1.right, rc2.bottom);
	if(IntersectRect(&inter, dragrect, &rc))
		return DWS_DOCKED_RIGHT;
	
	
	return DWS_DOCKED_FLOATING;
	
}

//
//	Return if the specified dock-window is dockable
//
static UINT IsDockable(HWND hwnd, DockWnd *dwp, RECT *dragrect)
{
	NMDOCKWNDQUERY dwq;

	int id = GetWindowLong(hwnd, GWL_ID);

	dwq.hdr.code     = DWN_ISDOCKABLE;
	dwq.hdr.hwndFrom = hwnd;
	dwq.hdr.idFrom   = id;
	dwq.dragrect     = dragrect;
	dwq.hwndDock     = hwnd;
	dwq.pDockWnd     = dwp;

	return SendMessage(GetParent(hwnd), WM_NOTIFY, id, (LPARAM)&dwq);
	//return DockWnd_GetDockSide(GetParent(hwnd), &dwq);
}

//
//	Return if the specified dockwindow is dockable, taking into
//  into account the status of the control key (fCtrlKey)
//
static BOOL IsDockableKey(HWND hwnd, DockWnd *dwp, RECT *dragrect, BOOL fCtrlKey)
{
	return IsDockable(hwnd, dwp, dragrect) && fCtrlKey == FALSE;
}

//
//	Draw both types of frame - checkered and solid line
//
static void DrawXorFrame(HWND hwnd, RECT *rect, BOOL fDocked)
{
	static WORD _dotPatternBmp1[] = 
	{
		0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055, 0x00aa, 0x0055
		//0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555
	};

	static WORD _dotPatternBmp2[] = 
	{
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
	};

	HBITMAP hbm;
	HBRUSH  hbr;
	HANDLE  hbrushOld;
	WORD    *bitmap;

	int width, height, x, y;
	int border;

	HDC hdc = GetDC(0);

	if(fDocked)
	{
		border = 1;
		bitmap = _dotPatternBmp2;
	}
	else
	{
		border = 3;
		bitmap = _dotPatternBmp1;
	}
	
	x = rect->left;
	y = rect->top;
	width  = rect->right-rect->left;
	height = rect->bottom-rect->top;

	hbm = CreateBitmap(8, 8, 1, 1, bitmap);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hdc, x, y, 0);
	hbrushOld = SelectObject(hdc, hbr);

	PatBlt(hdc, x+border,       y,               width-border,  border,        PATINVERT);
	PatBlt(hdc, x+width-border, y+border,        border,        height-border, PATINVERT);
	PatBlt(hdc, x,              y+height-border, width-border,  border,        PATINVERT);
	PatBlt(hdc, x,              y,               border,        height-border, PATINVERT);

	SelectObject(hdc, hbrushOld);
	DeleteObject(hbr);
	DeleteObject(hbm);
	ReleaseDC(0, hdc);
}

//
//	This function only returns the popups which belong
//	to the specified "main" window
//
//	hwndMain - handle to top-level owner of the popups to retrieve
//	hwndList - where to store the list of popups
//	nItems   - [in] - size of hwndList, [out] - returned no. of windows
//	fIncMain - include the main window in the returned list
//
static int GetPopupList(HWND hwndMain, HWND hwndList[], int nSize, BOOL fIncMain)
{
	int i, count = 0;
	
	if(hwndList == 0) 
		return 0;

	for(i = 0; i < nNumDockWnds && i < nSize; i++)
	{
		if(IsOwnedBy(hwndMain, hDockList[i]))
			hwndList[count++] = hDockList[i];
	}

	if(fIncMain && count < nSize)
	{
		hwndList[count++] = hwndMain;
	}

	return count;
}

//
// Keyboard hook for the Drag-Dropping a ToolWindow.
// This hook just lets the user toggle the insert mode
// by monitoring the <control> key
//
static LRESULT CALLBACK draghookproc(int code, WPARAM wParam, LPARAM lParam)
{
	ULONG state = (ULONG)lParam;

	if(code < 0) 
		return CallNextHookEx(draghook, code, wParam, lParam);

	if(wParam == VK_CONTROL)
	{
		if(state & 0x80000000)	fControl = FALSE;
		else					fControl = TRUE;

		SendMessage(g_hwndDockWnd, WM_MOUSEMOVE, 0, 0);
		return -1;
	}

	if(wParam == VK_ESCAPE)
	{
		PostMessage(g_hwndDockWnd, WM_CANCELMODE, 0, 0);
		return 0;
	}

	return CallNextHookEx(draghook, code, wParam, lParam);
}

//
//	Draws a dock-window's etched borders and gripper
//
static LRESULT DockWnd_EraseBkGnd(HWND hwnd, DockWnd *dwp, HDC hdc)
{
	RECT rc;
	
	GetClientRect(hwnd, &rc);
	
	if(dwp->fDocked)
	{
		UINT bf = 0;
		RECT tmp  = rc;
		RECT tmp2 = rc;
		
		if(dwp->dwStyle & DWS_BORDERTOP)	bf |= BF_TOP;
		if(dwp->dwStyle & DWS_BORDERBOTTOM) bf |= BF_BOTTOM;
		if(dwp->dwStyle & DWS_BORDERLEFT)	bf |= BF_LEFT;
		if(dwp->dwStyle & DWS_BORDERRIGHT)  bf |= BF_RIGHT;

		if(dwp->dwStyle & DWS_RESIZABLE)
		{
			if(dwp->uDockedState & DWS_DOCKED_TOP)		tmp.bottom -= 2;//dwp->nInnerBorder;
			if(dwp->uDockedState & DWS_DOCKED_BOTTOM)	tmp.top    += 2;//dwp->nInnerBorder;
			if(dwp->uDockedState & DWS_DOCKED_LEFT)		tmp.right  -= 2;//dwp->nInnerBorder;
			if(dwp->uDockedState & DWS_DOCKED_RIGHT)	tmp.left   += 2;//dwp->nInnerBorder;

			SubtractRect(&tmp2, &tmp2, &tmp);
			SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
			ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &tmp2, _T(""), 0, 0);
			rc = tmp;
		}
		
		//draw whatever edges we need
		if(bf) 
			DrawEdge(hdc, &rc, EDGE_ETCHED, bf | BF_ADJUST);	
	}
	
	SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, _T(""), 0, 0);
	
	if(dwp->fDocked && (dwp->dwStyle & DWS_DRAWGRIPPERDOCKED))
	{
		int y = 1, height = rc.bottom - 1;
		int x = 1;
		
		if(dwp->dwStyle & DWS_BORDERTOP)	{ y += 3; height -= 3; }
		if(dwp->dwStyle & DWS_BORDERBOTTOM)	{ height -= 2; }
		if(dwp->dwStyle & DWS_BORDERLEFT)	{ x += 3; }
		
		DrawGripper(hdc, x, y, height, NUMGRIPS);
	}
	
	if(!dwp->fDocked && (dwp->dwStyle & DWS_DRAWGRIPPERFLOATING))
	{
		DrawGripper(hdc, 1, 3, rc.bottom - 4, NUMGRIPS);
	}
	
	return 1;
	
}

//
//	Dock window procedure
//
static LRESULT CALLBACK DockWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int		i;
	UINT	uHitTest;

	RECT	dragrect;
	RECT	border;
	RECT	rect;

	MINMAXINFO *pmmi;

	// These can be static, even though lots of windows
	// use this procedure - because, only one window can
	// be resized at a time by the user.
	static	BOOL  fMouseMoved = FALSE;
	static	POINT spt;
	static	RECT  rcStart;
	static	POINT oldpt;

	WINDOWPOS *wp;

	DockWnd *dwp = (DockWnd *)GetWindowLong(hwnd, GWL_USERDATA);

	switch(msg)
	{
	case WM_NCCREATE:
		
		// Add this one to list of dock-windows, before any
		// activation messages are sent as this window is created.
		if(nNumDockWnds < MAX_DOCK_WINDOWS)
		{
			hDockList[nNumDockWnds++] = hwnd;
			SetWindowText(hwnd, ((CREATESTRUCT *)lParam)->lpszName);
			return TRUE;
		}
		else
		{
			return FALSE;
		}

	case WM_NCDESTROY:
		
		// remove this window from the list of dock-windows
		for(i = 0; i < nNumDockWnds; i++)
		{
			if(hDockList[i] == hwnd)
			{
				for( ; i < nNumDockWnds - 1; i++)
				{
					hDockList[i] = hDockList[i+1];
				}

				nNumDockWnds--;
				break;
			}
		}

		// do NOT destroy the "attached" DockWnd structure!
		return 0;

	//
	//	Allow dragging by the client area
	//
	case WM_NCHITTEST:
		uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);

		if(uHitTest == HTCLIENT)
			uHitTest = HTCAPTION;

		return uHitTest;

	//
	// Update the floating rectangle coords
	//
	case WM_WINDOWPOSCHANGED:

		wp = (WINDOWPOS *)lParam;
			
		// Is the window floating?
		if(dwp && dwp->fDocked == FALSE)
		{
			// 
			if(!(wp->flags & SWP_NOMOVE))
			{
				dwp->xpos = wp->x;
				dwp->ypos = wp->y;
			}

			if(!(wp->flags & SWP_NOSIZE))
			{
				RECT rect;

				dwp->nFrameWidth  = wp->cx;
				dwp->nFrameHeight = wp->cy;

				// Update content window size, because
				// the frame size is always based on this
				GetClientRect(hwnd, &rect);
				dwp->cxFloating = rect.right;
				dwp->cyFloating = rect.bottom;
			}
		}

		//resize the contents so it fills our client area
		SetRect(&border, 0, 0, 0, 0);

		//Work out what our borders are..
		if(dwp->fDocked)
		{
			if(dwp->dwStyle & DWS_BORDERTOP)    border.top    += 2;
			if(dwp->dwStyle & DWS_BORDERBOTTOM) border.bottom += 2;
			if(dwp->dwStyle & DWS_BORDERLEFT)   border.left   += 2;
			if(dwp->dwStyle & DWS_BORDERRIGHT)  border.right  += 2;

			if(dwp->dwStyle & DWS_USEBORDERS)
			{
				border.left   += dwp->rcBorderDock.left;
				border.right  += dwp->rcBorderDock.right;
				border.top    += dwp->rcBorderDock.top;
				border.bottom += dwp->rcBorderDock.bottom;
			}
		}
		else
		{
			if(dwp->dwStyle & DWS_USEBORDERS)
			{
				border.left   += dwp->rcBorderFloat.left;
				border.right  += dwp->rcBorderFloat.right;
				border.top    += dwp->rcBorderFloat.top;
				border.bottom += dwp->rcBorderFloat.bottom;
			}
		}
		
		//Allow space for the gripper
		if(dwp->dwStyle & (DWS_DRAWGRIPPERDOCKED|DWS_DRAWGRIPPERFLOATING))
		{
			border.left += 7;
		}

		//position the contents in our client area
		if(dwp->fDocked)
		{
			GetClientRect(hwnd, &rect);
		}
		else
		{
			SetRect(&rect, 0, 0, dwp->cxFloating, dwp->cyFloating);
		}

		if(!(wp->flags & SWP_NOSIZE))
		{
			InvalidateRect(hwnd, 0, TRUE);
		}

		
		MoveWindow(dwp->hwndContents, 
			border.left, 
			border.top,
			rect.right  - border.right  - border.left,
			rect.bottom - border.bottom - border.top, TRUE);

		return 0;

	case WM_NCLBUTTONDBLCLK:

		// Prevent standard double-click on the caption area
		if(wParam == HTCAPTION)
		{
			DockWnd_ToggleDockingMode(hwnd);
			return 0;
		}

		break;

	case WM_NCLBUTTONDOWN:	//begin drag

		dwp->fDragging = FALSE;

		if(dwp->fDocked)
		{
			if(dwp->dwStyle & DWS_FORCEDOCK)
				return 0;

			fControl		= FALSE;
			fOldDrawDocked  = TRUE;
		}
		else
		{
			if(dwp->dwStyle & DWS_FORCEFLOAT)
				return 0;

			fControl		= TRUE;
			fOldDrawDocked  = FALSE;
		}

		fOldControl = fControl;

		// Prevent standard dragging by the caption area
		if(wParam == HTCAPTION)
		{
			SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);//|SWP_NOACTIVATE);

			GetWindowRect(hwnd, &dwp->oldrect);
			
			DrawXorFrame(hwnd, &dwp->oldrect, fOldDrawDocked);

			GetCursorPos(&spt);				
			GetWindowRect(hwnd, &rcStart);
			SetCapture(hwnd);

			fMouseMoved		= FALSE;
			dwp->fDragging	= TRUE;
			
			oldpt = spt;

			// Install the mouse hook
			g_hwndDockWnd = hwnd;
			draghook = SetWindowsHookEx(WH_KEYBOARD, draghookproc, GetModuleHandle(0), 0); 

			// Prevent "normal" action here
			return 0;
		}
		else
		{
			// Otherwise, let normal behaviour happen
			break;
		}

	// Mouse released - do we dock/undock the dock-window, or leave it as-is?
	case WM_CANCELMODE:
	case WM_LBUTTONUP:
	
		if(dwp->fDragging == TRUE)
		{
			POINT pt;

			dwp->fDragging = FALSE;

			DrawXorFrame(hwnd, &dwp->oldrect, fOldDrawDocked);//IsDockableKey(hwnd, dwp, &dwp->oldrect, fControl));

			// Remove the keyboard hook
			if(draghook)
			{
				UnhookWindowsHookEx(draghook); 
				draghook = 0;
			}

			GetCursorPos(&pt);
			CopyRect(&dragrect, &rcStart);
			OffsetRect(&dragrect, pt.x-spt.x, pt.y-spt.y);

			// If the mouse moved
			if(msg != WM_CANCELMODE && fMouseMoved != FALSE)
			{
				BOOL fToggleDock;
				BOOL fSetWindowPos;
				UINT uDockSide;
				
				dwp->fDragging = FALSE;
				
				// If the window was docked
				if(dwp->fDocked)
				{
					//uDockSide = IsDockable(hwnd, dwp, &dwp->oldrect);
					uDockSide = IsDockable(hwnd, dwp, &dragrect);

					if(fControl == TRUE || uDockSide == 0)
					{
						fToggleDock   = TRUE;
						fSetWindowPos = FALSE;
						
						//adjust where the dockbar will appear
						//dwp->wpFloating.x = dwp->oldrect.left;
						//dwp->wpFloating.y = dwp->oldrect.top;
						dwp->xpos = dwp->oldrect.left;
						dwp->ypos = dwp->oldrect.top;
					}
					else
					{
						fToggleDock   = FALSE;
						fSetWindowPos = FALSE;
						dwp->uDockedState = uDockSide;
					}
				}
				// If it was floating
				else
				{
					//uDockSide = IsDockable(hwnd, dwp, &dwp->oldrect);
					uDockSide = IsDockable(hwnd, dwp, &dragrect);

					if(fControl == FALSE && uDockSide)
					{
						fToggleDock   = TRUE;
						fSetWindowPos = FALSE;

						dwp->uDockedState = uDockSide;
					}
					else
					{
						fToggleDock   = FALSE;
						fSetWindowPos = TRUE;
					}
				}
				
				if(fToggleDock)
				{
					DockWnd_ToggleDockingMode(hwnd);
				}
				else if(fSetWindowPos)
				{
					SetWindowPos(hwnd, 0, dwp->oldrect.left, dwp->oldrect.top, 0, 0, 
						SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | 
						SWP_DRAWFRAME | SWP_NOSENDCHANGING);
				}
				else
				{
					Send_WM_SIZE(GetOwner(hwnd));
				}
				
			}
			
			ReleaseCapture();
		}

		break;

	// Move the drag rectangle around..
	case WM_MOUSEMOVE:

		if(dwp->fDragging)
		{
			POINT pt;
			static POINT oldpt;
			BOOL fIsDockable;
			RECT rc;
			UINT uDockSide;
			
			GetCursorPos(&pt);
				
			if(oldpt.x == pt.x && pt.y == oldpt.y && fControl == fOldControl)
			{
				return 0;
			}

			fMouseMoved = TRUE;

			fControl = GetKeyState(VK_CONTROL) < 0;
			oldpt = pt;

			CopyRect(&dragrect, &rcStart);
			OffsetRect(&dragrect, pt.x-spt.x, pt.y-spt.y);

			uDockSide   = IsDockable(hwnd, dwp, &dragrect) & ~(DWS_ALLOW_DOCKLEFT|DWS_ALLOW_DOCKRIGHT);
			fIsDockable = uDockSide != DWS_DOCKED_NOTDOCKED && fControl == FALSE;

			if(fIsDockable)
			{
				//offset back again
				if(uDockSide & (DWS_DOCKED_LEFT|DWS_DOCKED_RIGHT))
					OffsetRect(&dragrect, 0, -(pt.y-spt.y));
				else
					OffsetRect(&dragrect, -(pt.x-spt.x), 0);
			}

			if(EqualRect(&dragrect, &dwp->oldrect) == 0 || fOldControl != fControl)
			{
				//Erase where the dragging frame used to be..
				DrawXorFrame(hwnd, &dwp->oldrect, fOldDrawDocked);

				//check to see if we have moved over a dock/nodock area, and
				//update the size of the drag rectangle to reflect size
				if(fIsDockable)
				{
					GetClientRect(GetParent(hwnd), &rc);
					MapWindowPoints(GetParent(hwnd), 0, (POINT *)&rc, 2);

					if(uDockSide & (DWS_DOCKED_LEFT|DWS_DOCKED_RIGHT))
					{
						dragrect.top    = rc.top;
						dragrect.bottom = dragrect.top + (rc.bottom-rc.top);
						dragrect.right  = dragrect.left+ dwp->nDockedSize;
					}
					else
					{
						dragrect.left   = rc.left;
						dragrect.right  = dragrect.left+ (rc.right-rc.left);
						dragrect.bottom = dragrect.top + dwp->nDockedSize;
					}
				}
				else
				{
					dragrect.right  = dragrect.left + dwp->nFrameWidth;//dwp->wpFloating.cx;
					dragrect.bottom = dragrect.top  + dwp->nFrameHeight;//dwp->wpFloating.cy;
				}


				//Draw the new drag frame
				DrawXorFrame(hwnd, &dragrect, fIsDockable);
				fOldDrawDocked = fIsDockable;
			}

			dwp->oldrect = dragrect;
			fOldControl = fControl;
		}

		break;

	case WM_GETMINMAXINFO:
		
		pmmi = (MINMAXINFO *)lParam;

		// Prevent window sizing
		if(dwp && dwp->fDocked == FALSE && (dwp->dwStyle & DWS_NORESIZE))
		{
			pmmi->ptMaxTrackSize.x = dwp->nFrameWidth;
			pmmi->ptMaxTrackSize.y = dwp->nFrameHeight;
			pmmi->ptMinTrackSize.x = dwp->nFrameWidth;
			pmmi->ptMinTrackSize.y = dwp->nFrameHeight;

			return 0;
		}

		break;

	case WM_ERASEBKGND:
		return DockWnd_EraseBkGnd(hwnd, dwp, (HDC)wParam);

	case WM_NCACTIVATE:
		return HANDLE_NCACTIVATE(GetOwner(hwnd), hwnd, wParam, lParam);

	case WM_DESTROY:
		return 0;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_SETTINGCHANGE:
		
		if(dwp->fDocked == FALSE)
			SetFloatingWinPos(hwnd, dwp, SWP_NOACTIVATE|SWP_NOZORDER);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static ATOM InitDockWndClass()
{
	WNDCLASSEX	wc;

	ZeroMemory(&wc, sizeof(wc));

	//Window class for the main application parent window
	wc.cbSize			= sizeof(wc);
	wc.style			= 0;
	wc.lpfnWndProc		= DockWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= GetModuleHandle(0);
	wc.hIcon			= 0;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName		= 0;
	wc.lpszClassName	= szDockClass;
	wc.hIconSm			= 0;

	return RegisterClassEx(&wc);
}

//
//	Create a floating dock-window
//
HWND CreateDockWnd(DockWnd *dwp, HWND hwndParent, TCHAR szCaption[])
{
	HWND hwnd;
	RECT rect;

	UINT dwStyle;
	UINT dwExStyle;
//	WINPOS *winpos;
	
	// Register the window class if necessary
	if(aDockClass == 0)
		aDockClass = InitDockWndClass();

	if(dwp->fDocked == FALSE)
	{
		dwStyle    = POPUP_STYLES;;
		dwExStyle  = POPUP_EXSTYLES;
	}
	else
	{
		dwStyle    = CHILD_STYLES;
		dwExStyle  = CHILD_EXSTYLES;
	}
	
	//if no docking given, then give all docking possibilities
	if((dwp->dwStyle & (DWS_ALLOW_DOCKALL|DWS_FORCEFLOAT)) == 0)
		dwp->dwStyle |= DWS_ALLOW_DOCKALL;

	hwnd = CreateWindowEx(	dwExStyle,
							szDockClass,
							szCaption,
							dwStyle,
							dwp->xpos,
							dwp->ypos,
							0,0,
							//winpos->x,
							//winpos->y, 
							//winpos->cx,
							//winpos->cy,
							hwndParent, 
							0, 
							GetModuleHandle(0), 
							0);

	dwp->hwnd      = hwnd;
	dwp->fDragging = FALSE;

	// Assign the dock-window structure to the physical window
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)dwp);

	SetParent(dwp->hwndContents, hwnd);

	CalcFloatingRect(hwnd, dwp);
	
	if(dwp->nDockedSize <= 0)
	{
		dwp->nDockedSize = dwp->cyFloating;

		if(dwp->dwStyle & DWS_BORDERTOP)    dwp->nDockedSize += 2;
		if(dwp->dwStyle & DWS_BORDERBOTTOM) dwp->nDockedSize += 2;
	}

	if(dwp->fDocked)
	{
		//force the parent frame to resize its contents,
		//which now includes US..
		GetClientRect(hwndParent, &rect);
		PostMessage(hwndParent, WM_SIZE,  SIZE_RESTORED, MAKELPARAM(rect.right-rect.left,rect.bottom-rect.top));
	}
	else
	{
		DWORD dwShowFlags = 0;//SWP_NOACTIVATE;

		//display the dock window in its floating state
		dwShowFlags |= SWP_SHOWWINDOW;

		SetFloatingWinPos(hwnd, dwp, dwShowFlags);
	}

	//ShowWindow(dwp->hwndContents, SW_SHOW);
	
	if(!(dwp->dwStyle & DWS_NOSETFOCUS))
		SetFocus(dwp->hwndContents);
	else
		SendMessage(hwnd, WM_NCACTIVATE, TRUE, 0);

	return hwnd;
}

//
//	The main window of app should call this in response to WM_ENABLE
//
//	hwndMain - handle to top-level owner window
//	hwnd     - handle to window which received message (can be same as hwndMain)
//
LRESULT HANDLE_ENABLE(HWND hwndMain, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndList[MAX_DOCK_WINDOWS+1];
	int i, nNumWnds;

	HWND hParam = (HWND)lParam;
		
	nNumWnds = GetPopupList(hwndMain, hwndList, MAX_DOCK_WINDOWS+1, FALSE);

	for(i = 0; i < nNumWnds; i++)
	{
		if(hwndList[i] != hwnd)
		{
			EnableWindow(hwndList[i], wParam);
		}
	}

	//just do the default
	return DefWindowProc(hwnd, WM_ENABLE, wParam, lParam);
}

//
//	The main window of app should call this in response to WM_NCACTIVATE
//
//	hwndMain - handle to top-level owner window
//	hwnd     - handle to window which received message (can be same as hwndMain)
//
LRESULT HANDLE_NCACTIVATE(HWND hwndMain, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndList[MAX_DOCK_WINDOWS+1];
	int i, nNumWnds;

	HWND   hParam = (HWND)lParam;

	BOOL   fKeepActive = wParam;
	BOOL   fSyncOthers = TRUE;

	// If this message was sent by the synchronise-loop (below)
	// then exit normally
	if(hParam == (HWND)-1)
	{
		return DefWindowProc(hwnd, WM_NCACTIVATE, wParam, 0);
	}

	nNumWnds = GetPopupList(hwndMain, hwndList, MAX_DOCK_WINDOWS+1, TRUE);

	// UNDOCUMENTED FEATURE:
	// if the other window being activated/deactivated (i.e. NOT this one)
	// is one of our popups, then go (or stay) active, otherwise.
	for(i = 0; i < nNumWnds; i++)
	{
		if(hParam == hwndList[i])
		{
			fKeepActive = TRUE;
			fSyncOthers = FALSE;
			break;
		}
	}

	// This window is about to change (inactive/active).
	// Sync all other popups to the same state 
	if(fSyncOthers == TRUE)
	{
		for(i = 0; i < nNumWnds; i++)
		{
			//DO NOT send this message to ourselves!!!!
			if(hwndList[i] != hwnd && hwndList[i] != hParam)
				SendMessage(hwndList[i], WM_NCACTIVATE, fKeepActive, (LONG)-1);
		}
	}

	return DefWindowProc(hwnd, WM_NCACTIVATE, fKeepActive, lParam);
}