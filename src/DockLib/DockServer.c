//
//  DockServer.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//
//
//	Window procedure for the DockWnd server window
//
#include <windows.h>
#include <tchar.h>
#include "trace.h"
#include "docklib.h"
#include "dockwnd.h"

LRESULT CALLBACK DockPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TransWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

ATOM g_DockWndAtom;

DOCKSERVER *GetDockServer(HWND hwndMain)
{
	return (DOCKSERVER *)GetWindowLongPtr(hwndMain, GWLP_USERDATA);
}

static LRESULT CHAIN_WNDPROC(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC oldProc)
{
	if(oldProc)
		return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
	else
		return DefWindowProc(hwnd, msg, wParam, lParam);
}

//
//	The main window of app should call this in response to WM_ENABLE
//
//	hwndMain - handle to top-level owner window
//	hwnd     - handle to window which received message (can be same as hwndMain)
//
LRESULT HANDLE_ENABLE(HWND hwndMain, HWND hwnd, WPARAM wParam, LPARAM lParam, WNDPROC oldProc OPTIONAL)
{
	HWND hParam = (HWND)lParam;
	DOCKSERVER *dsp;
	DOCKPANEL  *dpp;
	
	if((dsp = GetDockServer(hwndMain)) == 0)
	{
		return CHAIN_WNDPROC(hwnd, WM_ENABLE, wParam, lParam, oldProc);
	}

	for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
	{
		if(!dpp->fDocked && dpp->hwndPanel != hwnd)
		{
			EnableWindow(dpp->hwndPanel, (BOOL)wParam);
		}
	}

	//just do the default
	return 0;
}

//
//	The main window of app should call this in response to WM_NCACTIVATE
//
//	hwndMain - handle to top-level owner window
//	hwnd     - handle to window which received message (can be same as hwndMain)
//
LRESULT HANDLE_NCACTIVATE(HWND hwndMain, HWND hwnd, WPARAM wParam, LPARAM lParam, WNDPROC oldProc OPTIONAL)
{
	HWND	hParam		= (HWND)lParam;
	BOOL	fKeepActive = (BOOL)wParam;
	BOOL	fSyncOthers = TRUE;

	DOCKSERVER *dsp;
	DOCKPANEL  *dpp;
	
	if((dsp = GetDockServer(hwndMain)) == 0)
	{
		return CHAIN_WNDPROC(hwnd, WM_NCACTIVATE, wParam, lParam, oldProc);
	}

	//
	// If this message was sent by the synchronise-loop (below)
	// then exit normally
	//
	if(hParam == (HWND)-1)
	{
		return CHAIN_WNDPROC(hwnd, WM_NCACTIVATE, wParam, 0, oldProc);
	}

	//
	// UNDOCUMENTED FEATURE:
	// if the other window being activated/deactivated (i.e. NOT this one)
	// is one of our popups, then go (or stay) active, otherwise.
	//
	for(dpp = dsp->PanelListHead; dpp && hParam; dpp = dpp->flink)
	{
		if(!dpp->fDocked && (dpp->hwndPanel == hParam || hwndMain == hParam))
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
		for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
		{
			//DO NOT send this message to ourselves!!!!
			if(!dpp->fDocked && 
				dpp->hwndPanel != hwnd && 
				dpp->hwndPanel != hParam 
				)
			{
				SendMessage(dpp->hwndPanel, WM_NCACTIVATE, fKeepActive, (LPARAM)-1);
			}
		}

		if(hwndMain != hwnd && hwndMain != hParam)
			SendMessage(hwndMain, WM_NCACTIVATE, fKeepActive, (LPARAM)-1);
	}

	return CHAIN_WNDPROC(hwnd, WM_NCACTIVATE, fKeepActive, lParam, oldProc);
}

static LRESULT CALLBACK ServerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DOCKSERVER *dsp = GetDockServer(hwnd);
	static int x, y;
	static BOOL fMoving;
	RECT rect;

	switch(msg)
	{
	case WM_NCACTIVATE:
		return HANDLE_NCACTIVATE(hwnd, hwnd, wParam, lParam, dsp->oldproc);

	case WM_ENABLE:
		return HANDLE_ENABLE(hwnd, hwnd, wParam, lParam, dsp->oldproc);

	case WM_ENTERSIZEMOVE:
		GetWindowRect(hwnd, &rect);
		x = rect.left;
		y = rect.top;
		fMoving = TRUE;
		break;

	case WM_EXITSIZEMOVE:
		fMoving = FALSE;
		break;
	
	case WM_MOVING:
	
		// if the window is being moved via it's titlebar,
		// and the Control key is being pressed, then
		// sync all the dockwindows to keep their position relative
		// to the main window
		if(fMoving && (GetKeyState(VK_CONTROL) & 0x80000000))
		{
			RECT *r = (RECT *)lParam;
			WINDOWPOS wp, *pwp = &wp;
			DOCKPANEL *dpp;

			HDWP hdwp = BeginDeferWindowPos(0);
			
			// position of main window
			wp.x = r->left;
			wp.y = r->top;

			for(dpp = dsp->PanelListHead; dpp; dpp = dpp->flink)
			{
				// if the panel is floating, then move it relative to the main window
				if(dpp->hwndPanel && dpp->fDocked == FALSE)
				{
					GetWindowRect(dpp->hwndPanel, &rect);

					if(dpp->fSticky)
					{
						hdwp = DeferWindowPos(hdwp, dpp->hwndPanel, 0, 
							rect.left+(pwp->x-x), rect.top+(pwp->y-y), 0, 0, 
							SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
					}
				}
			}
			
			EndDeferWindowPos(hdwp);

			x = pwp->x;
			y = pwp->y;
		}

		break;
	
	/*case WM_ACTIVATEAPP:
		
		//
		//	Show/Hide any floating windows if the main window is
		//	the active/non-active application
		//
		if(dsp->dwStyle & DWFS_HIDEFLOATING)
		{
			int i;
			HDWP hdwp = BeginDeferWindowPos(dsp->nNumDockWnds);
			
			for(i = 0; i < dsp->nNumDockWnds; i++)
			{
				hdwp = DeferWindowPos(hdwp, dsp->DockList[i].hwndPanel, 0, 0, 0, 0, 0, 
					SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|
					(wParam ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
			}
			
			EndDeferWindowPos(hdwp);
		}
		
		return 0;*/
	}

	return CallWindowProc(dsp->oldproc, hwnd, msg, wParam, lParam);
}

void InitDockLib()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.cbWndExtra    = sizeof(DOCKPANEL *);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wc.style		 = 0;//0x00020000;//CS_DROPSHADOW;
	wc.hCursor       = 0;//LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc   = DockPanelProc;
	wc.lpszClassName = WC_DOCKBAR;

	g_DockWndAtom	 = RegisterClassEx(&wc);

	wc.style		 = 0;
	wc.lpszClassName = WC_TRANSWINDOW;
	wc.lpfnWndProc   = TransWndProc;

	RegisterClassEx(&wc);
}

/*
BOOL InitDockServer(HWND hwndMain)
{
	LONG_PTR oldproc;

	InitDockLib();

//	hbrBG1 = CreateSolidBrush(RGB(160,160,160));
//	hbrBG2 = CreateSolidBrush(RGB(170,170,170));

	oldproc = GetWindowLongPtr(hwndMain, GWLP_WNDPROC);

	SetWindowLongPtr(hwndMain, GWLP_USERDATA, (LONG_PTR)oldproc);
	SetWindowLongPtr(hwndMain, GWLP_WNDPROC,  (LONG_PTR)ServerWndProc);

	return TRUE;
}
*/

// internal function for linklist management
void * AllocDockObj(void * flink, void * blink, UINT id, UINT size)
{
	DOCKOBJ * obj = malloc(size);

	ZeroMemory(obj, size);

	obj->flink = flink;
	obj->blink = blink;
	obj->id    = id;

	return obj;
}

BOOL WINAPI DockWnd_Initialize(HWND hwndMain, LPCTSTR szRegLoc)
{
	DOCKSERVER	*ds = malloc(sizeof(DOCKSERVER));

	ZeroMemory(ds, sizeof(DOCKSERVER));

	ds->PanelListHead = AllocDockObj(0, 0, -1, sizeof(DOCKPANEL));
	ds->PanelListTail = AllocDockObj(0, 0, -2, sizeof(DOCKPANEL));
	ds->PanelListHead->flink = ds->PanelListTail;
	ds->PanelListTail->blink = ds->PanelListHead;

	GetClientRect(hwndMain, &ds->DockRect);
	lstrcpy(ds->szRegLoc, szRegLoc);

	InitDockLib();

//	hbrBG1 = CreateSolidBrush(RGB(160,160,160));
//	hbrBG2 = CreateSolidBrush(RGB(170,170,170));

	ds->oldproc = (WNDPROC)GetWindowLongPtr(hwndMain, GWLP_WNDPROC);
	ds->hwndMain = hwndMain;

	SetWindowLongPtr(hwndMain, GWLP_USERDATA, (LONG_PTR)ds);
	SetWindowLongPtr(hwndMain, GWLP_WNDPROC, (LONG_PTR)ServerWndProc);

	return TRUE;
}
/*

*/