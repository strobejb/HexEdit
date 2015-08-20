//
//  DockWnd.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#include <windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <uxtheme.h>

#include "trace.h"
#include "docklib.h"
#include "dockwnd.h"

#include "..\HexEdit\TabView.h"

#define LEFT        0x100000
#define RIGHT       0x200000
#define TOP         0x400000
#define BOTTOM      0x800000
#define TAB			0x1000000

static DOCKPANEL * dppUnderLast;
static DOCKPANEL * g_dppTargetDockPanel = 0;
static UINT		   lastArea;

HWND	hwndAnimPanel;
HWND	hwndTransPanel;
BOOL	fOverTab;

#pragma comment(lib, "uxtheme.lib")
//#pragma comment(linker, "/DELAYLOAD:uxtheme.dll")

//
//	Return the dock-window, position under the mouse
//	This is called whenever a dockwindow is being dragged, and we want to determine
//  if the current dockwindow is being dragged on top of another
//
DOCKPANEL * DockPanelFromPt(DOCKSERVER *dsp, DOCKPANEL *dppDragging, POINT pt, OUT RECT *rect, OUT UINT_PTR *hit)
{
	DOCKPANEL *dpp;

	if(hit) *hit = 0;

	//
	// which window is under the mouse?
	// can't use WindowFromPoint as it'll always return the window we are dragging,
	// so enumerate all the dock-panels manually
	//
	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{
		RECT panelRect;

		if(dpp != dppDragging && 
			GetWindowRect(dpp->hwndPanel, &panelRect) &&
			PtInRect(&panelRect, pt) &&
			!(dpp->dwStyle & DWS_FIXED_SIZE)
			)
		{
            // if docked
            if(dpp->fDocked)
            {
                CopyRect(rect, &panelRect);
			    return dpp;
            }
            // if floating
            else
            {
                UINT_PTR uHit = DefWindowProc(dpp->hwndPanel, WM_NCHITTEST, 0, MAKELONG(pt.x, pt.y));
                RECT rcTab;

				if(hit) *hit = uHit;

                if(uHit == HTCAPTION || GetWindowRect(dpp->hwndTabView, &rcTab) && PtInRect(&rcTab, pt))
                {
                    CopyRect(rect, &panelRect);
			        return dpp;
                }
            }
		}
	}

	return 0;
}

int DistanceFromEdge(RECT *rect, POINT pt, int edge)
{
    //if(PtInRect(rect, pt))
	//{
	switch(edge)
    {
    case LEFT:    return abs(pt.x         - rect->left);
    case RIGHT:   return abs(rect->right  - pt.x);
    case TOP:     return abs(pt.y         - rect->top);
    case BOTTOM:  return abs(rect->bottom - pt.y);
	default:	  return 0;
    }
}

int GetClosestSide(IN RECT *rect, IN POINT pt, OUT int * distance)
{
    int sides[] = { LEFT, RIGHT, TOP, BOTTOM };
    int side = LEFT;
    int i;

    *distance = INT_MAX;

    for(i = 0; i < 4; i++)
    {
        int d = DistanceFromEdge(rect, pt, sides[i]);

        if(d < *distance)
        {
            *distance = d;
            side = sides[i];
        }
    }

    return side;
}

UINT GetDockTarget(DOCKSERVER *dsp, DOCKPANEL *dppDragging, POINT pt, RECT *rectDrag, DOCKPANEL **pdppUnder)
{
	DOCKPANEL *dppUnder;
	UINT_PTR uHit;

	if(GetKeyState(VK_CONTROL) & 0x80000000)
	{
		*pdppUnder = 0;
		return 0;
	}
	
	// get the DOCKPANEL that is under the mouse
	dppUnder = DockPanelFromPt(dsp, dppDragging, pt, rectDrag, &uHit);

	if(pdppUnder)
		*pdppUnder = dppUnder;

	// if over a dock-window
	if(dppUnder)
	{
		RECT rcTab;
		
		// if over the tab
		if(uHit == HTCAPTION || GetWindowRect(dppUnder->hwndTabView, &rcTab) && PtInRect(&rcTab, pt))
		{
			return TAB;
		}
	}
	else
	{
		//RECT rc;
		CopyRect(rectDrag, &dsp->ClientRect);
		MapWindowPoints(dsp->hwndMain, 0, (POINT *)rectDrag, 2);

        // over the client area?
		if(PtInRect(rectDrag, pt))
		{
			dppUnder = DOCKSERVER_CLIENTAREA;
			if(pdppUnder)
				*pdppUnder = dppUnder;

			//return 0;
		}
        // not over the window, bail out!
        else
        {
            *pdppUnder = 0;
			//GetClientRect(dsp->hwndMain, rectDrag);
			CopyRect(rectDrag, &dsp->DockRect);
			MapWindowPoints(dsp->hwndMain, 0, (POINT *)rectDrag, 2);
            //return 0;
        }
	}

    // ok, we have identified the dock-window under the mouse
    // now see if we are close enough to it's border to actually 
    // dock with it
    
    {
        int side, distance;
   
        side = GetClosestSide(rectDrag, pt, &distance);

        if(distance < 32)
        {
            // it's close enough to dock, trim the rectangle to reflect the
            // actual docking position
            switch(side)
            {
            case DWS_DOCKED_LEFT:	rectDrag->right		-= RectWidth(rectDrag)/2; break;
			case DWS_DOCKED_TOP:	rectDrag->bottom	-= RectHeight(rectDrag)/2; break;
			case DWS_DOCKED_RIGHT:	rectDrag->left		+= RectWidth(rectDrag)/2; break;
			case DWS_DOCKED_BOTTOM:	rectDrag->top		+= RectHeight(rectDrag)/2; break;
            }

            return side;
        }
    }

	return 0;
}

void UndockAndRemoveTab(DOCKPANEL *dpp, UINT uWndId)
{
	DOCKPANEL  *dppNew;
	DOCKWND    *dwp;
	POINT pt;

	SendMessage(dpp->hwndTabView, WM_CANCELMODE, 0, 0);
	dwp = DOCKWNDFromId(dpp->hwndMain, uWndId);

	// create a new DOCKPANEL for the floating DOCKWND
	dppNew  = NewDockPanel(dpp->pDockServer);

	// make a few tweaks
	dppNew->dwStyle			= dpp->dwStyle;
	dppNew->DockSize		= dpp->DockSize;
	dppNew->FloatSize		= dpp->FloatSize;

	dppNew->fDocked			= FALSE;
	dppNew->uCurrentTabId	= uWndId;
//	dwpNew->uPanelId		= nextid;
	//dwpNew->hwndContents	= dwp->hwndContents;
	dppNew->hwndTabView		= 0;
	dppNew->hwndPanel		= 0;

	// delete the old one if this was the last tab 
	// that we dragged off
	//if(dpp->WndListHead->flink == dpp->WndListTail)
	//{
	//	RemoveObj(dpp);
	//}

	// start a mouse-drag at curr
	GetCursorPos(&pt);
    dppNew->xpos = pt.x - 20;
    dppNew->ypos = pt.y - 20;


	// hide the tab that we are about to undock.... but KEEP the hwndContent!!!
	DockWnd_HideInternal2(dwp, TRUE);//, FALSE, TRUE);
	SetDockPanel(dppNew, dwp);

	// show the tab in the new dockpanel
	DockWnd_Show(dppNew->hwndMain, uWndId, TRUE);

	dppNew->fUndockNextMsg = TRUE;
    dppNew->fDragging = TRUE;

	//DelKey(dsp, dpp->uPanelId, uWndId);
	DockPanel_SetKeyboardHook(dppNew);

	//SendMessage(dwpNew->hwndPanel, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(dwpNew->xpos, dwpNew->ypos));
    SendMessage(dppNew->hwndPanel, WM_SYSCOMMAND, SC_MOVE+HTCAPTION, MAKELPARAM(dppNew->xpos, dppNew->ypos));
}


void DragOff(DOCKPANEL *dpp)
{
	if(dpp->fUndockNextMsg)
	{
		POINT pt;

		dpp->fDragging		= TRUE;
		dpp->fUndockNextMsg = FALSE;
		GetCursorPos(&pt);
		
		// position window so that the top/left of the *caption*
		// falls under the mouse pointer - this will ensure that the SC_MOVE
		// message makes the window move. (-20,-20) seems to do the trick
        dpp->xpos = pt.x - 20;
        dpp->ypos = pt.y - 20;

		if(dpp->fDocked)
		{
			// if the panel is docked... then undock it
			DockWindow(dpp);
		}
		else
		{
			// otherwise do nothing - it is already floating!
			SetWindowPos(dpp->hwndPanel, 0, dpp->xpos, dpp->ypos, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER);
		}
		
		dpp->fDragging = TRUE;
		DockPanel_SetKeyboardHook(dpp);

		SendMessage(dpp->hwndPanel, WM_SYSCOMMAND, SC_MOVE|HTCAPTION, MAKELPARAM(dpp->xpos, dpp->ypos));
	}
}
//
//	Called when the dockpanel receivees a WM_NOTIFY from one of its child-windows
//  i.e. either the TabView, or one of the DOCKWND's
//
LRESULT DockPanel_OnNotify(DOCKPANEL *dpp, WPARAM wParam, NMHDR *hdr)
{
	// notification message from the TabView
	if(hdr->hwndFrom == dpp->hwndTabView)
	{
		TCITEM tci = { TCIF_PARAM };
		DOCKWND *dwp;

		// get the active selection
		int iItem = TabCtrl_GetCurSel(dpp->hwndTabView);

		// every tab-item's lParam is a dockwindow-ID
		TabCtrl_GetItem(dpp->hwndTabView, iItem, &tci);

		switch(hdr->code)
		{
		case TCN_SELCHANGE:
			// show the appropriate DOCKWND
			DockWnd_Show(dpp->hwndMain, (UINT)tci.lParam, TRUE);
			break;

		case TCN_CLOSE:
			// hide the DOCKWND
			DockWnd_Show(dpp->hwndMain, (UINT)tci.lParam, FALSE);

			// delete the tab
			TabCtrl_DeleteItem(dpp->hwndTabView, iItem);
			break;

		case TCN_MOUSELEAVE:
			// the mouse has moved outside of the tab,
			// whilst being clicked - so we need to drag
			// the tab off into its own floating dock-window
			dwp = DOCKWNDFromId(dpp->hwndMain, (UINT)tci.lParam);
			
			if(dpp->WndListHead->flink == dwp && dpp->WndListTail->blink == dwp)
			{
				// if this is the last tab, then do a normal undock
				SendMessage(dpp->hwndTabView, WM_CANCELMODE, 0, 0);

				dpp->fUndockNextMsg = TRUE;
				DragOff(dpp);
			}
			else
			{
				// otherwise detach the tab and float it in a new DOCKPANEL
				UndockAndRemoveTab(dpp, (UINT)tci.lParam);
			}
			break;
		}

		return 0;
	}
	else
	{
		// forward it straight on
		return SendMessage(dpp->hwndMain, WM_NOTIFY, wParam, (LPARAM)hdr);
	}
}

static HHOOK g_hKeyboardHook;
static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (g_hKeyboardHook == 0)
		return 0;

	if(code < 0)
		return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);

	if(wParam == VK_CONTROL)
	{
		// cause another WM_MOVING message to be received -
		// our WM_MOVING handler will be called and it will determine
		// that the <control> key has been pressed
		SendMessage(GetActiveWindow(), WM_MOVING, 0, 0);
	}

	return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);
}

void DockPanel_SetKeyboardHook(DOCKPANEL *dpp)
{
	g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, 0, GetCurrentThreadId());
}

void DockPanel_RemoveKeyboardHook(DOCKPANEL *dpp)
{
	UnhookWindowsHookEx(g_hKeyboardHook);
	g_hKeyboardHook = 0;
}

LRESULT DockPanel_OnNcLButtonDown(DOCKPANEL *dpp, UINT_PTR uHitTest)
{
	// the window will start moving, but keep track of
	// the initial window-rectangles and drag status
	if(uHitTest == HTCAPTION)
	{
		dpp->fDragging			= TRUE;
		dpp->fDragStatus		= 0;

		DockPanel_SetKeyboardHook(dpp);
	}

	return 0;
}

LRESULT DockPanel_OnLButtonDown(DOCKPANEL *dpp, int x, int y)
{
	// if we click on the splitter bar, the user
	// intends to resize the dockwindow
	if(dpp->dwStyle & DWS_SPLITTER)
	{
		int part;

		if((part = IsMouseOverSplitter(dpp)) != 0)
		{
			SetCapture(dpp->hwndPanel);
			dpp->fSplitting = TRUE;
			return 0;
		}
	}

	if(dpp->fDocked)
	{
		// tell the next mouse-move message that
		// we want to undock + start floating
		dpp->fUndockNextMsg = TRUE;
	}

	return 0;
}

LRESULT DockPanel_OnMouseMove(DOCKPANEL *dpp, int x, int y)
{
	// make the splitter-bar work
	if(dpp->fSplitting)
	{
		SIZE oldsize = dpp->DockSize;
		if(dpp->dwStyle & DWS_DOCKED_TOP)
		{
			dpp->DockSize.cy = y;
		}
		else if(dpp->dwStyle & DWS_DOCKED_BOTTOM)
		{
			dpp->DockSize.cy -= y;
		}
		else if(dpp->dwStyle & DWS_DOCKED_RIGHT)
		{
			dpp->DockSize.cx -= x;
		}
		else if(dpp->dwStyle & DWS_DOCKED_LEFT)
		{
			dpp->DockSize.cx = x;
		}

		if(dpp->DockSize.cy < 4)
			dpp->DockSize.cy = 4;

		if(dpp->DockSize.cx < 4)
			dpp->DockSize.cx = 4;

		if(oldsize.cx != dpp->DockSize.cx || oldsize.cy != dpp->DockSize.cy)
		{
			PositionContent(dpp);
			SendFakeWMSize(dpp->hwndMain);
			InvalidateRect(dpp->hwndMain, 0, TRUE);
			InvalidateRect(dpp->hwndPanel, 0, TRUE);
			UpdateWindow(dpp->hwndMain);
		}
	}
	// should we detach this tab from the current dockpanel
	// and float it as a new window?
	else if(dpp->fUndockNextMsg)
	{
		DragOff(dpp);
	}

	return 0;
}

LRESULT DockPanel_OnLButtonUp(DOCKPANEL *dpp, int x, int y)
{
	dpp->fUndockNextMsg	= FALSE;

	if(dpp->fSplitting)
	{
		dpp->fSplitting = FALSE;
		ReleaseCapture();
	}

	return 0;
}

LRESULT DockPanel_OnDestroy(DOCKPANEL *dpp, HWND hwndMsg)
{
	DOCKWND *dwp;

	// if we are being destroyed during a dock/undock transition then do nothing - 
	// the DOCKPANEL does not belong to us anymore
	if(dpp->hwndPanel != hwndMsg)
		return 0;

	// save ALL DOCKPANEL's position, and also any child-tab's settings too
	DockWnd_SaveSettings(dpp->hwndMain);//dpp, dpp->uPanelId);

	// tell the parent window that we've closed
//	DockPanel_NotifyParent(dpp, DWN_CLOSING, (NMHDR *)&nmdw);

	DestroyWindow(dpp->hwndTabView);

	// the DOCKPANEL always remains... just clear out
	// the content
	dpp->hwndContents = 0;
	dpp->hwndTabView  = 0;
	dpp->hwndPanel    = 0;
	//dpp->fVisible	  = FALSE;

	for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = dwp->flink)
	{
		dwp->hwndContents = 0;
	}

	return 0;
}

/*

void TRACE_DOCKSERVER(DOCKSERVER *dsp)
{
	DOCKPANEL *dpp;
	DOCKWND *dwp;

	TRACEA("********** DOCKWND ********** \n");

	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{
		TRACEA("PANEL %x: ", dpp);//->uPanelId);

		for(dwp = dpp->WndListHead ? dpp->WndListHead->flink : 0; dwp && dwp != dpp->WndListTail; dwp = dwp->flink)
		{
			//TRACEA("   WND %d:\n", dwp->uPanelId);
			TRACEA(" %d (%ls,%d) - ", dwp->uWndId, dwp->szTitle, 0xdeadbeef);//dwp->pDockPanel->uPanelId);
		}

		TRACEA("\n");
	}
}
*/
//
//	Called when the dockpanel has stopped being dragged around
//  need to decide whether to dock/undock/leave it as it is
//
LRESULT DockPanel_ExitSizeMove(DOCKPANEL *dpp)
{
	extern HWND hwndTransPanel;		
	DestroyWindow(hwndTransPanel);
	hwndTransPanel = 0;

	DockPanel_RemoveKeyboardHook(dpp);

	dpp->fUndockNextMsg = FALSE;

	if(dpp->fDragging)
	{
		DOCKSERVER *dsp = dpp->pDockServer;
		DOCKPANEL  *dppUnder;
		RECT rectDrag;
		POINT pt;
		UINT area;
		HWND hwnd;

		DestroyWindow(hwndTransPanel);
		hwndTransPanel = 0;

		dpp->fDragging = FALSE;

		GetCursorPos(&pt);
		area = GetDockTarget(dpp->pDockServer, dpp, pt, &rectDrag, &dppUnder);

		// did we want to dock onto anything?
		if(area != 0)//dpp->fDragStatus || g_dppTargetDockPanel)
		{
			ShowWindow(dpp->hwndPanel, SW_HIDE);
			RemoveWindowTrans(dpp->hwndPanel);

			// docking onto an existing DOCKPANEL (i.e. not the main window,
			// but becoming a tab in another panel)
			if(area == TAB)
			{
				// move all DOCKWNDs in the current panel to the DOCKPANEL
				// that we are dragging on to
				DOCKWND *dwp, *next = 0;

				for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = next)//dwp->flink)
				{
					next = dwp->flink;

					SetDockPanel(dppUnder, dwp);
					DockWnd_Show(dpp->hwndMain, dwp->uWndId, TRUE);
				}

				UpdateDockTabView(dppUnder);

				// oof!
				RemoveObj(dpp);

				hwnd = dpp->hwndPanel;
				dpp->hwndPanel = 0;
				DestroyWindow(hwnd);//dpp->hwndPanel);
			}
			// otherwise dock with the main window
			else
			{
				// if docking on the inside,
				// move to end of dock-panel list
				if(dppUnder == (DOCKPANEL *)1)			// NASTY HACK!!  WHY!???
				{
					RemoveObj(dpp);
					InsertBefore(dpp, dsp->PanelListTail);
				}
				// if docking
				else if(dppUnder)
				{
					if( area == TOP && (dppUnder->dwStyle & BOTTOM) ||
						area == BOTTOM && (dppUnder->dwStyle & TOP) ||
						area == LEFT && (dppUnder->dwStyle & RIGHT) ||
						area == RIGHT && (dppUnder->dwStyle & LEFT))
					{
						// inside of target panel
						RemoveObj(dpp);
						InsertBefore(dpp, dppUnder->flink);
						if(area == TOP) area = BOTTOM;
						else if(area == BOTTOM) area = TOP;
						else if(area == LEFT) area = RIGHT;
						else if(area == RIGHT) area = LEFT;
					}
					else if( area == TOP && (dppUnder->dwStyle & TOP) ||
						area == BOTTOM && (dppUnder->dwStyle & BOTTOM) ||
						area == LEFT && (dppUnder->dwStyle & LEFT) ||
						area == RIGHT && (dppUnder->dwStyle & RIGHT))
					{
						// outside of target panel
						RemoveObj(dpp);
						InsertBefore(dpp, dppUnder);
					}
				}
				// if docking against the outside edge
				else
				{
					RemoveObj(dpp);
					InsertBefore(dpp, dsp->PanelListHead->flink);
				}

				dpp->dwStyle = (dpp->dwStyle & ~DWS_DOCKED_MASK) | area;
				DockWindow(dpp);
			}

			ShowWindow(dpp->hwndPanel, SW_SHOW);
		}

		dpp->fDragging   = FALSE;
		dpp->fDragStatus = 0;
	}

	// if the dock-panel is still floating, determine if it's edges
	// intersect the main window, and turn on the 'sticky' flag 
	if(dpp->fDocked == FALSE)
	{
		RECT r1, r2, r3;
		GetWindowRect(dpp->hwndMain,  &r2);
		GetWindowRect(dpp->hwndPanel, &r3);
		dpp->fSticky = IntersectRect(&r1, &r2, &r3);
	}

	return 0;
}

//
//	timer - used to animate the fade in/out of the transparent window
//
LRESULT DockPanel_Timer(DOCKPANEL *dpp, UINT_PTR id)
{
	extern HWND hwndAnimPanel;
	HWND hwndParam = (HWND)id;

	// get current alpha value
	BYTE alpha = (BYTE)GetWindowLongPtr(hwndParam, GWLP_USERDATA);
	BLENDFUNCTION blendPixelFunction = { AC_SRC_OVER, 0, -1, AC_SRC_ALPHA };		

	if(hwndAnimPanel != hwndParam || alpha == 0)
	{
		KillTimer(dpp->hwndPanel, id);
		DestroyWindow(hwndParam);
		return 0;
	}

	// adjust alpha value towards '0'
	alpha = alpha < 24 ? 0 : alpha - 24;
	SetWindowLongPtr(hwndParam, GWLP_USERDATA, alpha);

	// update the layered window transparency
	blendPixelFunction.SourceConstantAlpha = alpha;
	UpdateLayeredWindow(hwndParam, 0, 0, 0, 0, 0, 0, &blendPixelFunction, ULW_ALPHA);

	return 0;
}

LRESULT DockPanel_SetCursor(DOCKPANEL *dpp, WPARAM wParam, LPARAM lParam)
{
	// mouse over the splitter-bar?
	if(dpp->fDocked && (dpp->dwStyle & DWS_SPLITTER))
	{
		int part;

		if((part = IsMouseOverSplitter(dpp)) != 0)
		{
			if(dpp->dwStyle & (DWS_DOCKED_TOP|DWS_DOCKED_BOTTOM))
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
			else
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));

			return 1;
		}

		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	// mouse over the titlebar?
	else if(dpp->fDocked && (dpp->dwStyle & DWS_DOCKED_TITLEBAR))
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}

	return DefWindowProc(dpp->hwndPanel, WM_SETCURSOR, wParam, lParam);
}

HRESULT DockPanel_OnSize(DOCKPANEL *dpp)
{
	if(dpp->fDocked == FALSE)
	{
		RECT rect;//, rect2;

		// get the new client-area, and subtract any borders/splitter
		// to get the working area
		GetClientRect(dpp->hwndPanel, &rect);
		//GetClientRect(dpp->hwndPanel, &rect2);
		
		if(!IsRectEmpty(&rect))
			AdjustRectBorders(dpp, &rect, -1);

		//TRACEA("WM_SIZE: client=%d %d %d %d  adjusted=%d %d %d %d\n",
		//	rect2.left, rect2.top, rect2.right, rect2.bottom,
		//	rect.left, rect.top, rect.right, rect.bottom
		//	);

		dpp->FloatSize.cx = RectWidth(&rect);
		dpp->FloatSize.cy = RectHeight(&rect);
	}

	PositionContent(dpp);
	return 0;
}

LRESULT DockPanel_GetMinMaxInfo(DOCKPANEL *dpp, MINMAXINFO *pmmi)
{
	if(dpp && (dpp->dwStyle & DWS_FIXED_SIZE))
	{
		RECT rect;

		GetClientRect(dpp->hwndContents, &rect);
		AdjustRectBorders(dpp, &rect, 1);
		CalcFloatingRect(dpp->hwndPanel, &rect);

		if(dpp->dwStyle & DWS_FIXED_HORZ)
		{
			pmmi->ptMaxTrackSize.x = RectWidth(&rect);
			pmmi->ptMinTrackSize.x = RectWidth(&rect);
		}

		if(dpp->dwStyle & DWS_FIXED_VERT)
		{
			pmmi->ptMaxTrackSize.y = RectHeight(&rect);
			pmmi->ptMinTrackSize.y = RectHeight(&rect);
		}
	}

	return 0;
}

//
// if the dockpanel is moving, and it's caused by us,
// then we need to decide whether to dock it or keep it floating
//
LRESULT DockPanel_OnMoving(DOCKPANEL *dppThis)
{
	POINT ptCurrent;
	RECT rect;
	POINT pt; 
	RECT r; 
	DOCKPANEL *dppUnder;
	UINT area;

	if(!dppThis->fDragging)
		return 0;

	GetCursorPos(&pt);
	area = GetDockTarget(dppThis->pDockServer, dppThis, pt, &r, &dppUnder);

	if(dppUnderLast != dppUnder || lastArea != area)
	{
		if(1)//dppUnderLast != dppUnder)
		{
			// kick off timer
			if(hwndTransPanel != 0)
			{
				hwndAnimPanel = hwndTransPanel;
				SetWindowLongPtr(hwndAnimPanel, GWLP_USERDATA, 255);
				SetTimer(dppThis->hwndPanel, (UINT_PTR)hwndAnimPanel, 10, 0);
				hwndTransPanel = 0;
			}
		}
		else
		{
			DestroyWindow(hwndTransPanel);
			hwndTransPanel = 0;
		}
	}

	dppUnderLast = dppUnder;
    lastArea = area;
	
	if(hwndTransPanel == 0)
	{
		hwndTransPanel = ShowTransWindow(dppThis, dppThis->hwndPanel, &r, 
        area ? area : DWS_DOCKED_MASK,
        area ? 3 : 0);
	}

	GetWindowRect(dppThis->hwndPanel, &rect);
	dppThis->xpos = rect.left;//(short)LOWORD(lParam);
	dppThis->ypos = rect.top;///(short)HIWORD(lParam);

	g_dppTargetDockPanel = 0;

	GetCursorPos(&ptCurrent);

	return 0;
}



LRESULT CALLBACK DockPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DOCKPANEL *dpp = (DOCKPANEL *)GetWindowLongPtr(hwnd, 0);

	switch(msg)
	{
	case WM_NCCREATE:
		dpp = (DOCKPANEL*)((CREATESTRUCT *)lParam)->lpCreateParams;
		dpp->hwndPanel = hwnd;

		SetWindowLongPtr(hwnd, 0, (LONG_PTR)dpp);
		SetWindowText(hwnd, ((CREATESTRUCT *)lParam)->lpszName);
		return TRUE;

	case WM_NCACTIVATE:
		return HANDLE_NCACTIVATE(dpp->hwndMain, hwnd, wParam, lParam, NULL);

	case WM_CREATE:
		dpp->fVisible = TRUE;

		// if the tabview hasn't already been created...
		if((dpp->dwStyle & DWS_TABSTRIP) && dpp->hwndTabView == 0)	
		{
			RegisterTabView();

			dpp->hwndTabView = CreateWindowEx(0, WC_TABVIEW,
				0, WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, hwnd, 0, 0, 0);
		}

		return 0;

	//case WM_ENABLE:
	//	return HANDLE_ENABLE(dpp->hwndMain, hwnd, wParam, lParam);

	case WM_CLOSE:
		DestroyWindow(hwnd);
		dpp->fVisible = FALSE;
		return 0;

	case WM_ERASEBKGND:
		DrawDockPanelBackground(dpp, (HDC)wParam);
		return 1;

	case WM_COMMAND:
		return SendMessage(dpp->hwndMain, msg, wParam, lParam);

	case WM_NOTIFY:
		return DockPanel_OnNotify(dpp, wParam, (NMHDR *)lParam);

	case WM_SETFOCUS:
		SetFocus(dpp->hwndContents);
		return 0;
	
	case WM_SETCURSOR:
		return DockPanel_SetCursor(dpp, wParam, lParam);

	case WM_LBUTTONDOWN:
		return DockPanel_OnLButtonDown(dpp, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_MOUSEMOVE:
		return DockPanel_OnMouseMove(dpp, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_LBUTTONUP:
		return DockPanel_OnLButtonUp(dpp, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_MOVING:
		return DockPanel_OnMoving(dpp);

	case WM_EXITSIZEMOVE:
		return DockPanel_ExitSizeMove(dpp);
	
	case WM_NCLBUTTONDOWN:
		DockPanel_OnNcLButtonDown(dpp, wParam);
		break;

	case WM_SIZE:
		return DockPanel_OnSize(dpp);
		
	case WM_GETMINMAXINFO:
		DockPanel_GetMinMaxInfo(dpp, (MINMAXINFO *)lParam);
		break;

	case WM_DESTROY:
		return DockPanel_OnDestroy(dpp, hwnd);

	case WM_TIMER:
		return DockPanel_Timer(dpp, wParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}



