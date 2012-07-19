//
//  DockLib.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#include <windows.h>
#include <tchar.h>
#include <uxtheme.h>
#include "..\HexEdit\hexutils.h"
#include "trace.h"
#include "docklib.h"
#include "dockwnd.h"



//#pragma comment(lib, "uxtheme.lib")
//#pragma comment(linker, "/DELAYLOAD:uxtheme.dll")

VOID DockPanel_NotifyParent(DOCKPANEL *dpp, UINT nNotifyCode, NMHDR *optional)
{
	NMHDR nmhdr   = { dpp->hwndPanel, 0, nNotifyCode };
	NMHDR *nmptr  = &nmhdr;  

	DOCKWND *dwp;
	
	if(optional)
	{
		nmptr  = optional;
		*nmptr = nmhdr;
	}

	// send a notification from each DOCKWND in turn
	for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = dwp->flink)
	{
		nmptr->idFrom = dwp->uWndId;
		SendMessage(dpp->hwndMain, WM_NOTIFY, dwp->uWndId, (LPARAM)nmptr);
	}
}

LRESULT DockWnd_NotifyParent(DOCKPANEL *dpp, DOCKWND *dwp, UINT nNotifyCode, NMHDR *optional)
{
	NMHDR nmhdr   = { dpp->hwndPanel, dwp->uWndId, nNotifyCode };
	NMHDR *nmptr  = &nmhdr;  
	
	if(optional)
	{
		nmptr  = optional;
		*nmptr = nmhdr;
	}

	return SendMessage(dpp->hwndMain, WM_NOTIFY, dwp->uWndId, (LPARAM)nmptr);
}

//
//	Work out how big a floating window should be,
//  taking into account the window contents, and current 
//  system settings
//
void CalcFloatingRect(HWND hwnd, RECT *rect)
{
	AdjustWindowRectEx(rect, 
					GetWindowLong(hwnd, GWL_STYLE),
					FALSE, 
					GetWindowLong(hwnd, GWL_EXSTYLE));
}

//
//	dir -> -1: subtract
//			1: increase
//
void AdjustRectBorders(DOCKPANEL *dwp, RECT *rect, int dir)
{
	RECT *border;
	UINT  status;

	status = dwp->fDragging ? dwp->fDragStatus : dwp->fDocked;
	border = status ? &dwp->rcDockBorder : &dwp->rcFloatBorder;

	rect->left    -= dir * border->left;
	rect->right   += dir * border->right;
	rect->top     -= dir * border->top;

	if(!(dwp->dwStyle & DWS_TABSTRIP))
		rect->bottom  += dir * border->bottom;

	if(dwp->dwStyle & DWS_DRAWGRIPPER)
		rect->left -= dir * GRIP_SIZE;
}

void AddPanelBorders(DOCKPANEL *dpp, RECT *rect)
{
	AdjustRectBorders(dpp, rect, 1);
}

void SubPanelBorders(DOCKPANEL *dpp, RECT *rect)
{
	AdjustRectBorders(dpp, rect, -1);
}

//
//	return the entire size of the panel's client area,
//  taking into account any borders, splitter bars etc
//
void GetPanelClientSize(DOCKPANEL *dpp, RECT *rect, BOOL fDocked)
{
	if(fDocked)
		SetRect(rect, 0, 0, dpp->DockSize.cx, dpp->DockSize.cy);
	else
		SetRect(rect, 0, 0, dpp->FloatSize.cx, dpp->FloatSize.cy);

	AdjustRectBorders(dpp, rect, 1);
}


BOOL IsHorizontallyDocked(DOCKPANEL *dpp)
{
	if(dpp->dwStyle & (DWS_DOCKED_TOP | DWS_DOCKED_BOTTOM))
		return TRUE;
	else
		return FALSE;
}

BOOL IsVerticallyDocked(DOCKPANEL *dpp)
{
	if(dpp->dwStyle & (DWS_DOCKED_LEFT | DWS_DOCKED_RIGHT))
		return TRUE;
	else
		return FALSE;
}

void PositionContent(DOCKPANEL *dpp)
{
	RECT rect;
	RECT *border;

	if(dpp->hwndContents == 0)
		return;

	GetClientRect(dpp->hwndPanel, &rect);

	border = dpp->fDocked ? &dpp->rcDockBorder : &dpp->rcFloatBorder;

	//
	//	Take any user-defined borders into account
	//
	rect.left	+= border->left;
	rect.top	+= border->top;
	rect.right	-= border->right;

	if(!(dpp->dwStyle & DWS_TABSTRIP))
		rect.bottom -= border->bottom;

	// is there a gripper?
	if(dpp->dwStyle & DWS_DRAWGRIPPER)
	{
		if(IsHorizontallyDocked(dpp))
			rect.left += GRIP_SIZE;

		else if(IsVerticallyDocked(dpp))
			rect.top += GRIP_SIZE;
	}

	if(dpp->fDocked)
	{
		if(dpp->dwStyle & DWS_SPLITTER)
		{
			switch(dpp->dwStyle & DWS_DOCKED_MASK)
			{
			case DWS_DOCKED_TOP:    rect.bottom  -= 20; break;
			case DWS_DOCKED_LEFT:   rect.right   -= 9;  break;
			case DWS_DOCKED_RIGHT:  rect.left    += 9;  break;
			case DWS_DOCKED_BOTTOM: rect.top     += 11; break;
			}
		}

		if(dpp->dwStyle & DWS_DOCKED_TITLEBAR)
		{
			rect.top += 20;
		}
	}

	// is there a tabview?
	if(dpp->hwndTabView)
	{
		//if(dpp->
		SetWindowPos(dpp->hwndTabView, 0, rect.left, rect.bottom-32, RectWidth(&rect), 32,
			SWP_NOZORDER|SWP_NOACTIVATE);

		rect.bottom -= 32;	
	}

	SetWindowPos(dpp->hwndContents, 0, rect.left, rect.top, 
		RectWidth(&rect), RectHeight(&rect),
		SWP_NOZORDER|SWP_NOACTIVATE|(dpp->dwStyle&(DWS_FIXED_VERT|DWS_FIXED_HORZ)?SWP_NOSIZE:0));
}

DOCKWND *DOCKWNDFromId(HWND hwndMain, UINT uId)
{
	DOCKSERVER *ds = GetDockServer(hwndMain);
	DOCKPANEL *dpp;

    for(dpp = ds ? ds->PanelListHead : 0; dpp; dpp = dpp->flink)
    {
        DOCKWND *dwp;

		for(dwp = dpp->WndListHead; dwp; dwp = dwp->flink)
	    {
		    if(uId == dwp->uWndId)
			    return dwp;
	    }
    }

	return 0;
}

void SizeToContent(DOCKPANEL *dwp)
{
	RECT  rect = { 0,0,0,0 }, rect2;
	int x = 0, y = 0;

	if(dwp->hwndPanel == 0 || dwp->hwndContents == 0)
		return;
	
	if(dwp->DockSize.cx == 0 || (dwp->dwStyle & DWS_FIXED_HORZ))
		dwp->DockSize.cx = GetWindowWidth(dwp->hwndContents);
	
	if(dwp->FloatSize.cx == 0 || (dwp->dwStyle & DWS_FIXED_HORZ))
		dwp->FloatSize.cx = GetWindowWidth(dwp->hwndContents);
	
	if(dwp->DockSize.cy == 0 || (dwp->dwStyle & DWS_FIXED_VERT))
		dwp->DockSize.cy = GetWindowHeight(dwp->hwndContents);
	
	if(dwp->FloatSize.cy == 0 || (dwp->dwStyle & DWS_FIXED_VERT))
		dwp->FloatSize.cy = GetWindowHeight(dwp->hwndContents);
	
	if(dwp->fDocked)
	{
		rect.right = dwp->DockSize.cx;
		rect.bottom = dwp->DockSize.cy;
	}
	else
	{
		rect.right = dwp->FloatSize.cx;
		rect.bottom = dwp->FloatSize.cy;
	}

	CopyRect(&rect2, &rect);
	AdjustRectBorders(dwp, &rect, 1);
	
	TRACEA("SizeToContent: client=%d %d %d %d  adjusted=%d %d %d %d\n",
				rect2.left, rect2.top, rect2.right, rect2.bottom,
				rect.left, rect.top, rect.right, rect.bottom
				);

	if(!dwp->fDocked)
		CalcFloatingRect(dwp->hwndPanel, &rect);

	SetWindowSize(dwp->hwndPanel, RectWidth(&rect), RectHeight(&rect), NULL);

	PositionContent(dwp);
}



static BOOL DockWnd_CreatePanel(DOCKPANEL *dpp)
{
	DWORD dwStyleEx, dwStyle;
	RECT  rect = { 0 };

	DOCKWND *dwp;

	if(dpp->fDocked)
	{
		dwStyle		= CHILD_STYLES;
		dwStyleEx	= CHILD_EXSTYLES;
		rect.right	= 0;
		rect.bottom	= 0;
	}
	else
	{
		dwStyle		= POPUP_STYLES;
		dwStyleEx	= POPUP_EXSTYLES;
		rect.right	= dpp->FloatSize.cx;
		rect.bottom	= dpp->FloatSize.cy;
	}

	if(!IsRectEmpty(&rect))
	{
		//
		//	Add on room for borders/splitters
		//
		AdjustRectBorders(dpp, &rect, 1);

		// if it's floating then add on the non-client area
		if(dpp->fDocked == FALSE)
			AdjustWindowRectEx(&rect, POPUP_STYLES, FALSE, POPUP_EXSTYLES);
	}

	// create the DOCKPANEL
	dwp = DOCKWNDFromId(dpp->hwndMain, dpp->uCurrentTabId);

	dpp->hwndPanel = CreateWindowEx(dwStyleEx,
								WC_DOCKBAR, 
								dwp ? dwp->szTitle : TEXT(""),
								dwStyle,
								dpp->xpos, dpp->ypos, RectWidth(&rect), RectHeight(&rect),
								dpp->hwndMain, 
								(HMENU)0,//dpp->uPanelId, 
								0, dpp);

	ForceVisibleDisplay(dpp->hwndPanel);

	// reassign any open content-panels to the new dockwnd
	{
		DOCKWND *dwp;
		for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = dwp->flink)
		{
			SetParent(dwp->hwndContents, dpp->hwndPanel);
		}
	}
    

	if(dpp->hwndTabView)
		SetParent(dpp->hwndTabView, dpp->hwndPanel);

	// content might not exist. But if it does (i.e. we are
	// creating a new DOCKPANEL because of dock/undock) then we
	// will size appropriately
	SizeToContent(dpp);

	return TRUE; 
}



VOID DockWnd_DeferShowPopups(HWND hwndMain)
{
	DOCKSERVER *dsp = GetDockServer(hwndMain);

	if(dsp)
	{
		dsp->fDeferShowWindow = TRUE;
		//dsp->hdwp = BeginDeferWindowPos(nNumWindows);
	}

	//return TRUE;//dsp->hdwp;
}

VOID DockWnd_ShowDeferredPopups(HWND hwndMain)
{
	DOCKSERVER *dsp = GetDockServer(hwndMain);

	if(dsp && dsp->fDeferShowWindow)
	{
		DOCKPANEL *dpp;

		//r = EndDeferWindowPos(dsp->hdwp);
		dsp->fDeferShowWindow = FALSE;

		for(dpp = dsp->PanelListHead->flink; dpp != dsp->PanelListTail; dpp = dpp->flink)
		{
			if(dpp->uDeferShowCmd)
			{
				ShowWindow(dpp->hwndPanel, dpp->uDeferShowCmd);
				dpp->uDeferShowCmd = 0;
			}
		}
	}
}


HDWP DockWnd_DeferPanelPos(HDWP hdwp, HWND hwndMain, RECT *rect)
{
	DOCKSERVER *dsp = GetDockServer(hwndMain);
	DOCKPANEL *dpp;

	if(dsp == 0)
		return 0;

	CopyRect(&dsp->DockRect, rect);
	CopyRect(&dsp->ClientRect, rect);

	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{
		RECT rc = *rect;
		RECT rc2;
	                 
		if(dpp->fDocked == FALSE || dpp->fVisible == FALSE)
		{
			continue;
		}

		GetPanelClientSize(dpp, &rc2, TRUE);
		
		if(dpp->dwStyle & DWS_DOCKED_LEFT)
		{
			rc.right = rc.left + RectWidth(&rc2);
		}
		else if(dpp->dwStyle & DWS_DOCKED_RIGHT)
		{
			rc.left = rc.right - RectWidth(&rc2);	
		}
		else if(dpp->dwStyle & DWS_DOCKED_TOP)
		{
			rc.bottom = rc.top + RectHeight(&rc2);
		}
		else if(dpp->dwStyle & DWS_DOCKED_BOTTOM)
		{
			rc.top = rc.bottom - RectHeight(&rc2);
		}

		SubtractRect(rect, rect, &rc);

		hdwp = DeferWindowPos(hdwp, dpp->hwndPanel, 0, rc.left, rc.top, rc.right-rc.left,rc.bottom-rc.top,
			SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
	}

    CopyRect(&dsp->ClientRect, rect);

	return hdwp;
}

void DockWindow(DOCKPANEL *dpp)
{
	HWND hwndOld = dpp->hwndPanel;

	dpp->fDocked = !dpp->fDocked;
	
    if(dpp->hwndPanel == 0)
        return;
	
	DockWnd_CreatePanel(dpp);

	PositionContent(dpp);
	SendFakeWMSize(dpp->hwndMain);

	// blah
	DockPanel_NotifyParent(dpp, DWN_DOCKCHANGE, 0);

	DestroyWindow(hwndOld);
	ShowWindow(dpp->hwndPanel, SW_SHOW);
}



BOOL IsMouseOverSplitter(DOCKPANEL *dwp)
{
	POINT pt;
	BOOL  fSplitting = FALSE;
	RECT  rect;

	const int splitterHeight = 8;

	GetCursorPos(&pt);
	GetClientRect(dwp->hwndPanel, &rect);
	ScreenToClient(dwp->hwndPanel, &pt);

	switch(dwp->dwStyle & DWS_DOCKED_MASK)
	{
	case DWS_DOCKED_TOP:
		if(pt.y >= rect.bottom - splitterHeight && pt.y < rect.bottom)
			fSplitting = TRUE;
		break;

	case DWS_DOCKED_BOTTOM:
		if(pt.y >= 0 && pt.y < splitterHeight)
			fSplitting = TRUE;
		break;

	case DWS_DOCKED_LEFT:
		if(pt.x >= rect.right - splitterHeight && pt.x < rect.right)
			fSplitting = TRUE;
		break;

	case DWS_DOCKED_RIGHT:
		if(pt.x >= 0 && pt.x < splitterHeight)
			fSplitting = TRUE;
		break;

	default:
		fSplitting = FALSE;
	}

	return fSplitting;
}

//
//	Ensure the TabView reflects the dockwnds
//
void UpdateDockTabView(DOCKPANEL *dpp)
{
	DOCKWND *dwp = 0;
	int idx      = 0;
	int selidx   = 0;
	DOCKWND *seldwp = 0;

	TabCtrl_DeleteAllItems(dpp->hwndTabView);

	//TRACEA("Updating tab: %x %x\n", dpp->hwndPanel, dpp);
	
	for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = dwp->flink)
	{
		if(dwp->fVisible)
		{
			TCITEM tci = { TCIF_PARAM|TCIF_TEXT };
		
			tci.lParam  = dwp->uWndId;
			tci.pszText = dwp->szTitle;

			if(dwp->uWndId == dpp->uCurrentTabId)
			{
				selidx = idx;
				seldwp = dwp;
			}
		
			TabCtrl_InsertItem(dpp->hwndTabView, idx++, &tci);
		}
	}

	TabCtrl_SetCurSel(dpp->hwndTabView, selidx);
	
	if(seldwp)
	{
		SetWindowText(dpp->hwndPanel, seldwp->szTitle);
	}

	InvalidateRect(dpp->hwndPanel, 0, TRUE);
}

BOOL DockWnd_CreateContent(DOCKPANEL *dpp, DOCKWND *dwp, HKEY hKey)
{
	HKEY hUserKey = 0;
	NMDOCKWNDCREATE nmdw = { {0}, dwp->uWndId, hUserKey };
	
	if(RegOpenKeyEx(hKey, TEXT("Settings"), 0, KEY_READ, &hUserKey) == S_OK)
		nmdw.hKey = hUserKey;

	// ask the main window to create the specified child-window
	dwp->hwndContents = (HWND)DockWnd_NotifyParent(dpp, dwp, DWN_CREATE_CONTENT, (NMHDR *)&nmdw);

	// don't set the dockpanel field yet!
	dpp->hwndContents = dwp->hwndContents;

	if(dwp->hwndContents == 0)
		return FALSE;

	DockWnd_NotifyParent(dpp, dwp, DWN_UPDATE_CONTENT, (NMHDR *)&nmdw);

	if(hUserKey)
		RegCloseKey(hUserKey);

	// set content's parent to be the dock-window
	SetParent(dpp->hwndContents, dpp->hwndPanel);

	// resize the dockwnd to fit around the content
	SizeToContent(dpp);		

	UpdateDockTabView(dpp);

	return TRUE;
}

DOCKWND * ActiveDockWnd(DOCKPANEL *dpp)
{
	//TCITEM tci = { TCIF_PARAM };
	//TabCtrl_GetItem(dpp->hwndTabView, i, &tci);

	DOCKWND *dwp;
	int i = 0;

	for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = dwp->flink)
	{
		if(i++ == dpp->uCurrentTabId)
		{
			return dwp;
		}
	}

	return 0;
}

void SetDockPanel(DOCKPANEL *dpp, DOCKWND *dwp)
{
	DOCKPANEL *dppOld = dwp->pDockPanel;

	// remove the DOCKWND from it's current DOCKPANEL
	RemoveObj(dwp);

	// insert it into the new one!
	InsertBefore(dwp, dpp->WndListTail);
	
	dwp->pDockPanel = dpp;

	SetParent(dwp->hwndContents, dpp->hwndPanel);
}

void GetDockPanelRect(DOCKPANEL *dpp, RECT *rect)
{
	//SetRect(&rect, dpp->xpos, dpp->ypos, 0, 0);
	

}

void RemoveObj(void *obj)
{
	DOCKOBJ *smeg = (DOCKOBJ*)obj;
	if(obj && ((DOCKOBJ *)obj)->blink)
	{
		((DOCKOBJ *)obj)->blink->flink = ((DOCKOBJ *)obj)->flink;
	}

	if(obj && ((DOCKOBJ *)obj)->flink)
	{
		((DOCKOBJ *)obj)->flink->blink = ((DOCKOBJ *)obj)->blink;
	}
}

void InsertBefore(void *obj, void *_this)
{
	((DOCKOBJ *)obj)->flink = _this;
	((DOCKOBJ *)obj)->blink = ((DOCKOBJ *)_this)->blink;

	((DOCKOBJ *)_this)->blink->flink = obj;
	((DOCKOBJ *)_this)->blink = obj;
}

int NextId(DOCKSERVER *dsp)
{
	DOCKPANEL *dpp;
	int id = 1;

	for(;;)
	{
		BOOL matched = FALSE;

		for(dpp = dsp->PanelListHead->flink; dpp != dsp->PanelListTail; dpp = dpp->flink)
		{
			if(dpp->uPanelId == id)
			{
				matched = TRUE;
				id++;
				break;
			}
		}

		if(matched == FALSE)
			break;
	}

	return id;
}

DOCKPANEL * NewDockPanel(DOCKSERVER *dsp)
{
	DOCKPANEL *dpp;

	// Allocate a DOCKPANEL and insert into the main PanelList
    if((dpp = AllocDockObj(0, 0, NextId(dsp), sizeof(DOCKPANEL))) == 0)
		return 0;

	InsertBefore(dpp, dsp->PanelListTail);

    // intialise the DOCKPANEL's WndList
	dpp->WndListHead = AllocDockObj(0, 0, -1, sizeof(DOCKWND));
	dpp->WndListTail = AllocDockObj(0, 0, -2, sizeof(DOCKWND));
	dpp->WndListHead->flink = dpp->WndListTail;
	dpp->WndListTail->blink = dpp->WndListHead;

	SetRect(&dpp->rcDockBorder,  DEFAULT_BORDER, DEFAULT_BORDER, DEFAULT_BORDER, DEFAULT_BORDER);
	SetRect(&dpp->rcFloatBorder, 0, DEFAULT_BORDER, 0, 0);
	
	dpp->hwndMain	  = dsp->hwndMain;
	dpp->uCurrentTabId = 0;
	dpp->FloatSize.cx = 0;
	dpp->FloatSize.cy = 0;
	dpp->DockSize.cx  = 0;
	dpp->DockSize.cy  = 0;
	dpp->dwStyle	  = 0;
	dpp->xpos		  = 0;
	dpp->ypos		  = 0;
	dpp->pDockServer  = dsp;
		
	return dpp;
}

//
//	register a new docking window!
//
BOOL WINAPI DockWnd_RegisterEx(HWND hwndMain, UINT uId, UINT uGroupWith, LPCTSTR szTitle)
{
	DOCKSERVER *dsp;
	DOCKPANEL *dpp = 0;
	DOCKWND *dwp;
	
	if((dsp = GetDockServer(hwndMain)) == 0)
		return FALSE;
	
	// if the DOCKWND already exists then FAIL
	if((dwp = DOCKWNDFromId(hwndMain, uId)) != 0)
		return TRUE;
	
	// if uGroupId is specified, then we want this DOCKWND to 
	// be grouped in the same panel as the referenced uGroupId DOCKWND
	if(uGroupWith != 0)
	{
		if((dwp = DOCKWNDFromId(hwndMain, uGroupWith)) != 0)
			dpp = dwp->pDockPanel;
	}


	// create a new panel if nothing else worked
	if(dpp == 0 && (dpp = NewDockPanel(dsp)) == 0)
		return FALSE;

	// insert into the PANEL's wndlist
    dwp = AllocDockObj(0, 0, 0, sizeof(DOCKWND));
	//InsertBefore(dwp, dpp->WndListTail);

	SetDockPanel(dpp, dwp);

	// insert into tablist
	lstrcpyn(dwp->szTitle, szTitle, 200);
	dwp->uWndId			= uId;
	//dwp->uGroupId		= uGroupId;
	dwp->hwndContents   = 0;
	dwp->fVisible		= TRUE;
	
	return TRUE;
}

BOOL WINAPI DockWnd_Register(HWND hwndMain, UINT uId, LPCTSTR szTitle)
{
	return DockWnd_RegisterEx(hwndMain, uId, 0, szTitle);
}

UINT WINAPI DockWnd_SetStyle(HWND hwndMain, UINT uId, UINT uStyle, UINT uMask)
{
	DOCKPANEL *dpp;

	DOCKWND *dwp;
	
	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
		return 0;

	dpp = dwp->pDockPanel;
	
	if(dpp)
	{
		DWORD oldstyle = dpp->dwStyle;
		dpp->dwStyle = (dpp->dwStyle & ~uMask) | uStyle;

		if((uStyle & DWS_DRAWGRIPPER) || (uMask & DWS_DRAWGRIPPER))
		{
			SizeToContent(dpp);
		}

		return oldstyle;
	}

	return 0;
}


BOOL WINAPI DockWnd_Dock(HWND hwndMain, UINT uId)
{
    DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) != 0)
	{
		if(dwp->pDockPanel->fDocked == FALSE)
		{
			DockWindow(dwp->pDockPanel);
		}
	}

	return TRUE;
}

BOOL WINAPI DockWnd_Undock(HWND hwndMain, UINT uId)
{
    DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) != 0)
	{
		if(dwp->pDockPanel->fDocked == TRUE)
		{
			DockWindow(dwp->pDockPanel);
		}
	}

	return TRUE;
}

HKEY DockWnd_GetKey(HWND hwndMain, DWORD dwAccess, UINT uId)
{
	DOCKSERVER *dsp;
	HKEY hKey = 0;
	TCHAR szKeyName[200];

	if((dsp = GetDockServer(hwndMain)) == 0)
		return 0;

	wsprintf(szKeyName, TEXT("%s\\Content\\Content%05d"), dsp->szRegLoc, uId);
	
	RegCreateKeyEx(HKEY_CURRENT_USER, szKeyName, 0, 0, 0, dwAccess, 0, &hKey, 0);
	return hKey;
}

//
//	Show/Hide a collection of dockwindows based on the group-id
//
BOOL WINAPI DockWnd_ShowGroup(HWND hwndMain, UINT uGroupId, BOOL fShow)
{
	DOCKSERVER *dsp = GetDockServer(hwndMain);
    DOCKPANEL *dpp;
    DOCKWND *dwp;

	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
    {
		for(dwp = dpp->WndListHead; dwp; dwp = dwp->flink)
	    {  
		    if(dwp->uGroupId == uGroupId)
		    {
			    DockWnd_Show(hwndMain, dwp->uWndId, fShow);
		    }
	    }
    }

	return TRUE;
}

BOOL WINAPI DockWnd_SetGroupId(HWND hwndMain, UINT uId, UINT uGroupId)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
		return FALSE;

	dwp->uGroupId = uGroupId;
	return TRUE;
}

BOOL DockWnd_ShowInternal2(DOCKWND *dwp)
{
	DOCKPANEL *dpp = dwp->pDockPanel;
	HWND hwndOldContent = dpp->hwndContents;

	//
	// Create containing panel if necessary 
	// (it might already be visible if another tab is open)
	//
	if(dpp->hwndPanel == 0)
	{
		DockWnd_CreatePanel(dpp);
	}

	//
	// Create the user-contents if necessary
	//
	if(dwp->hwndContents == 0)
	{
		HKEY hSubKey = DockWnd_GetKey(dpp->hwndMain, KEY_READ, dwp->uWndId);
			
		// ask the parent of the panel to create the content
		DockWnd_CreateContent(dpp, dwp, hSubKey);
		RegCloseKey(hSubKey);
	}

	// if it's not already visible...
	if(hwndOldContent != dpp->hwndContents || dwp->hwndContents != dpp->hwndContents)
	{
		if(hwndOldContent == 0) hwndOldContent = dpp->hwndContents;
		ShowWindow(hwndOldContent, SW_HIDE);//dpp->hwndContents, SW_HIDE);
		dpp->hwndContents = dwp->hwndContents;
		ShowWindow(dwp->hwndContents, SW_SHOW);

		InvalidateRect(dpp->hwndPanel, 0, 0);
	}

	//SetRect(&rect, dpp->xpos, dpp->ypos, dpp->
	//if(IsRectEmpty(
	PositionContent(dpp);
	ShowWindow(dwp->hwndContents, SW_SHOW);

	// only show if it's floating; the main window resizing will cause
	// any docked windows to be shown
	if(dpp->fDocked == FALSE)
	{
		// DeferWindowPos if the HDWP is set
		if(dpp->pDockServer->fDeferShowWindow)
		{
			dpp->uDeferShowCmd = SW_SHOW;
		}
		else
		{
			ShowWindow(dpp->hwndPanel, SW_SHOW);
			//dpp->pDockServer->hdwp = ShowHideWindow(dpp->hwndPanel, SW_SHOW, dpp->pDockServer->hdwp);
		}
		//
	}

	dpp->fVisible		= TRUE;
	dwp->fVisible		= TRUE;
	dpp->uCurrentTabId	= dwp->uWndId;

	UpdateDockTabView(dpp);

	return TRUE;
}

BOOL DockWnd_HideInternal2(DOCKWND *dwp, BOOL fPreserveContent)
{
	DOCKPANEL *dpp = dwp->pDockPanel;
	int idx;

	dwp->fVisible = FALSE;

	if(fPreserveContent == FALSE)
	{
		// destroy the content
		DestroyWindow(dwp->hwndContents);
		dwp->hwndContents = 0;
	}

	// remove the tab
	idx = TabCtrl_GetCurSel(dpp->hwndTabView);
	TabCtrl_DeleteItem(dpp->hwndTabView, idx);
	UpdateDockTabView(dpp);

	// destroy the DOCKPANEL if this is the last tab
	if(dpp->hwndTabView && TabCtrl_GetItemCount(dpp->hwndTabView) == 0 || !dpp->hwndTabView)
	{
		DestroyWindow(dpp->hwndPanel);
		dpp->fVisible = FALSE;
	}
	// show the next item
	else
	{
		TCITEM tci = { TCIF_PARAM };
		DOCKWND *dwpnext;
				
		idx = TabCtrl_GetCurSel(dpp->hwndTabView);
		TabCtrl_GetItem(dpp->hwndTabView, idx, &tci);

		dpp->uCurrentTabId = (UINT)tci.lParam;
		dwpnext = DOCKWNDFromId(dpp->hwndMain, (UINT)tci.lParam);

		DockWnd_ShowInternal2(dwpnext);//, TRUE, fRemoving);
	}

	return TRUE;
}

BOOL WINAPI DockWnd_Show(HWND hwndMain, UINT uId, BOOL fShow)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
		return FALSE;

	if(fShow)
	{
		DockWnd_ShowInternal2(dwp);
	}
	else
	{
		DockWnd_HideInternal2(dwp, FALSE);
	}

	SendFakeWMSize(hwndMain);
	InvalidateRect(hwndMain, 0, 0);
	return TRUE;
}

//
// Does the specified DOCKPANEL exist?
//
BOOL WINAPI DockWnd_Undefined(HWND hwndMain, UINT uId)
{
	return DOCKWNDFromId(hwndMain, uId) ? FALSE : TRUE;
}



/*BOOL WINAPI DockWnd_ShowGui(HWND hwndMain)
{
	int i;
	DOCKSERVER *dsp;

	if((dsp = GetDockServer(hwndMain)) == 0)
		return FALSE;

	for(i = 0; i < dsp->nNumDockPanels; i++)
	{
		DOCKWND *dtp = &dsp->WndList[i];
		
		if(dtp->fVisible)
			DockWnd_Show(hwndMain, dtp->uTabId, TRUE);
	}

	return TRUE;
}*/



//
//	Update the specified server-window
//
BOOL WINAPI DockWnd_Update(HWND hwndMain)
{
	DOCKSERVER *dsp;
	DOCKPANEL *dpp;

	if((dsp = GetDockServer(hwndMain)) == 0)
		return FALSE;
	
//	for(i = 0; i < dsp->nNumDockPanels; i++)
	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{	
		//if(dsp->PanelList[i]hwndPanel)
		//	DockWnd_Show(hwndMain, dsp->PanelList[i].uId, TRUE);

		if(dpp->fVisible)// && dpp->fDocked == FALSE && dsp->fDeferShowWindow == FALSE)
		{
			// show the 'last' current tab, this will force the window visible
			DockWnd_Show(hwndMain, dpp->uCurrentTabId, TRUE);
		}

	}

	return TRUE;
}

//
//	Is the specified DOCKPANEL visible or not?
//
BOOL WINAPI DockWnd_IsOpen(HWND hwndMain, UINT uId)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
		return FALSE;

	if(dwp->hwndContents)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	return dwp->hwndContents/* && IsWindowVisible(dwp->hwndPanel)*/ ? TRUE : FALSE;
}

//
//	Return HWND to the specified DOCKWND's contents
//
HWND WINAPI DockWnd_GetContents(HWND hwndMain, UINT uId)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
	{
		return 0;
	}

	return dwp->hwndContents;
}
/*
VOID WINAPI DockWnd_SetContents(HWND hwndMain, UINT uId, HWND hwndContent)
{
	DOCKPANEL *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) != 0)
		dwp->hwndContents = hwndContent;
}
*/
HWND WINAPI DockWnd_GetWindow(HWND hwndMain, UINT uId)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
		return 0;

	return dwp->pDockPanel->hwndPanel;
}

//
//	Call in main message loop:
//
//	while(GetMessage(&msg, 0, 0, 0) > 0)
//	{
//		if( !DockWnd_IsDialogMessage(g_hwndMain, DWID_xx1, &msg) &&
//			!DockWnd_IsDialogMessage(g_hwndMain, DWID_xx2, &msg))
//		{
//			TranslateMessage(&msg);
//			DispatchMessage(&msg);
//		}	
//	}
//
BOOL WINAPI DockWnd_IsDialogMessage(HWND hwndMain, UINT uId, MSG *msg)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
		return FALSE;

	if(dwp->fVisible)
		return IsDialogMessage(dwp->hwndContents, msg);
	else
		return FALSE;
}

//
//	Usually called in response to a Ctrl+Tab keyboard accelerator
//
BOOL WINAPI DockWnd_NextWindow(HWND hwndMain)
{
	HWND hwndFocus;
	HWND hwndLast = 0;
	DOCKSERVER *dsp;
	DOCKPANEL *dpp;
	BOOL fMainFocus = TRUE;

	if((dsp = GetDockServer(hwndMain)) == 0)
		return FALSE;

	hwndFocus = GetFocus();

	// is focus on one of the dockwindows?
	//for(i = 0; i < dsp->nNumDockPanels; i++)
	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{
		//DOCKPANEL *dwp = &dsp->PanelList[i];

		if(hwndFocus == dpp->hwndPanel || IsChild(dpp->hwndPanel, hwndFocus))
			fMainFocus = FALSE;
	}

	//for(i = 0; i < dsp->nNumDockPanels; i++)
	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{
	//	DOCKPANEL *dwp = &dsp->PanelList[i];

		if(dpp->hwndPanel)
		{
			// is the focus set to this window?
			if(hwndFocus == dpp->hwndPanel || IsChild(dpp->hwndPanel, hwndFocus))
			{
				hwndLast = dpp->hwndPanel;
			}
			// look for next window that doesn't have focus
			else if((hwndLast || fMainFocus) && !(dpp->dwStyle & DWS_NOSETFOCUS))
			{
				SetFocus(dpp->hwndPanel);	
				return TRUE;
			}
		}
	}

	// not found? go to the main window
	SetFocus(hwndMain);
	return TRUE;
}


BOOL WINAPI DockWnd_UpdateContent(HWND hwndMain, UINT uId)
{
	DOCKWND *dwp;

	if((dwp = DOCKWNDFromId(hwndMain, uId)) == 0)
	{
		return 0;
	}

	DockWnd_NotifyParent(dwp->pDockPanel, dwp, DWN_UPDATE_CONTENT, 0);
	return TRUE;
}

//
//  idx = 0-based offset
//	nGroupId = 0 for 'no group'
//
/*
UINT WINAPI DockWnd_EnumVisible(HWND hwndMain, UINT nGroupId, int idx)
{
	DOCKSERVER *dsp;
	DOCKWND *dwp;
	int i, cnt = 0;

	if((dsp = GetDockServer(hwndMain)) == 0)
		return 0;

//	if(idx >= dsp->nNumDockWnds)
//		return 0;

//	for(i = 0; i < dsp->nNumDockWnds; i++)
	for(dwp = dsp->WndListHead; dwp; dwp = dwp->flink)
	{
		//DOCKWND *dwp = &dsp->WndList[i];

		if(dwp->fVisible)
		{
			if(nGroupId == 0 || nGroupId && dwp->uGroupId == nGroupId)
			{
				if(cnt++ == idx)
					return dwp->uWndId;
			}
		}
	}

	return 0;
}
*/



