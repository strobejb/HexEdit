//
//  DockPaint.c
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

#include "trace.h"
#include "docklib.h"
#include "dockwnd.h"

extern ATOM g_DockWndAtom;
//HWND  hwndTransPanel;

// from ToolPanel.c
void DrawGripper(HTHEME hTheme, HDC hdc, int x, int y, int height, int numgrips);

//BOOL DockWnd_ShowInternal(HWND hwndMain, UINT uId, BOOL fShow, BOOL fRemoving);

//
//	Wrapper around OpenThemeData
//	requires that we delay-load uxtheme.dll/OpenThemeData, so we can catch the exception
//	if it isn't found
//
static HTHEME OpenThemeShim(HWND hwnd, LPCWSTR pszClassList)
{
	__try
	{
		return OpenThemeData(hwnd, pszClassList);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return NULL;
	}
}



//DOCKPANEL * NewDockPanel(DOCKSERVER *dsp, /*UINT uGroupId, */LPCTSTR szTitle);
//VOID TRACE_DOCKSERVER(DOCKSERVER *dsp);

//POINT ptInitial;
//POINT ptCurrent;
//RECT  rectDrag;
//RECT  rectInitial;


void DrawDockPanelBackground(DOCKPANEL *dpp, HDC hdc)
{
	RECT rect;
	RECT rect2;

	GetClientRect(dpp->hwndPanel, &rect);

	if(dpp->fDocked && (dpp->dwStyle & DWS_DOCKED_TITLEBAR))
	{
		rect.top += 18;
	}

	if(!(dpp->dwStyle & DWS_THEMED_BACKGROUND))
	{
		FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DFACE));
	}

	if(dpp->fDocked && (dpp->dwStyle & DWS_DOCKED_TITLEBAR))
	{
		RECT rect2;
		
		rect.top -= 18;
		rect2 = rect;
		
		rect2.bottom = rect2.top + 2;

		DrawEdge(hdc, &rect2, EDGE_ETCHED, BF_RECT);
		//OffsetRect(&rect2, 0, 4);
		//DrawEdge(hdc, &rect2, EDGE_ETCHED, BF_RECT);
	}

	//
	//	Draw the splitter bar
	//
	if(dpp->fDocked && (dpp->dwStyle & (DWS_SPLITTER)))// | DWS_DOCKED_TITLEBAR)))
	{
		rect2 = rect;
		
		if(dpp->dwStyle & DWS_DOCKED_TOP)
		{
			rect2.top	 = rect.bottom - 10;
			rect2.bottom = rect2.top + 6;
		}
		else if(dpp->dwStyle & DWS_DOCKED_RIGHT)
		{
			rect2.right = rect2.left + 6;
			rect.left  += 6;
		}
		else if(dpp->dwStyle & DWS_DOCKED_LEFT)
		{
			rect2.left	 = rect.right - 8;
			rect2.right  = rect2.left + 6;
			rect.right   -= 8;
		}
		else if(dpp->dwStyle & DWS_DOCKED_BOTTOM)
		{
			rect2.top	 = rect.top;
			rect2.bottom = rect2.top + 6;
			rect.top	 += 7;
		}

		FillRect(hdc, &rect2, GetSysColorBrush(COLOR_3DDKSHADOW));
		OffsetRect(&rect2, 0, 1);

		DrawCheckedRect(hdc, &rect2, 
			0xA1877D,//GetSysColor(COLOR_3DSHADOW), 
			GetSysColor(COLOR_3DDKSHADOW)
			);
	}

	if(dpp->fDocked && (dpp->dwStyle & DWS_DOCKED_TITLEBAR))
	{
		DOCKWND *dwp = DOCKWNDFromId(dpp->hwndMain, dpp->uCurrentTabId);
		static HFONT hFont;
		LOGFONT logfont;

		rect2		 = rect;
		rect2.top    = rect.top;
		rect2.bottom = rect2.top + 18;
		//rect2.top	 = rect2.top + 2;
	
		//DrawCaption(dpp->hwndPanel, hdc, &rect2,DC_ACTIVE|DC_TEXT|DC_BUTTONS);

		if(hFont == 0)
		{
			hFont = GetStockObject(DEFAULT_GUI_FONT);
			GetObject(hFont, sizeof(logfont), &logfont);
		
			// create it with the 'bold' attribute
			logfont.lfWeight   = FW_BOLD;
			lstrcpy(logfont.lfFaceName, TEXT("MS Shell Dlg"));
			logfont.lfHeight -= 1;
			hFont = CreateFontIndirect(&logfont);
		}
		
		SetTextColor(hdc, 0xffffff);
		SetBkColor(hdc, GetSysColor(COLOR_3DSHADOW));
		SelectObject(hdc, hFont);

		ExtTextOut(hdc, rect2.left+3, rect2.top+1,
			ETO_OPAQUE|ETO_CLIPPED, 
			&rect2, dwp->szTitle, lstrlen(dwp->szTitle), 0);

		rect.top += 18;
	}
	
	if((dpp->dwStyle & DWS_THEMED_BACKGROUND) || (dpp->dwStyle & DWS_DOCKED_TOP) && (dpp->dwStyle & DWS_THEMED_BACKGROUND))
	{
		HTHEME hTheme = OpenThemeShim(dpp->hwndPanel, L"Rebar");

		if(hTheme)
		{
			rect.top -= 10;//16;
			rect.right += 10;	// urrrghh.... fixes the non-repainting bug on the right-side 
			DrawThemeBackground(hTheme, hdc, 0, 0, &rect, 0);

			if(dpp->dwStyle & DWS_DRAWGRIPPER)
			{
				DrawGripper(hTheme, hdc, 1, 2, rect.bottom - 4, 2);
			}

			CloseThemeData(hTheme);
		}
	}
	else
	{
		if(dpp->dwStyle & DWS_DRAWGRIPPER)
		{
			DrawGripper(0, hdc, 1, 2, rect.bottom - 4, 2);
		}
	}
}


/*
//
// Return the DOCKPANEL* from the specified window handle
//
DOCKPANEL *ValidateDockWnd(HWND hwnd)
{
	DWORD pid;

	GetWindowThreadProcessId(hwnd, &pid);

	// make sure window is owned by current process
	if(pid != GetCurrentProcessId())
		return NULL;

	// make sure it's the same window class
	if(g_DockWndAtom != GetClassLong(hwnd, GCW_ATOM))
		return NULL;
	
	// return the DOCKPANEL
	return (DOCKPANEL *)GetWindowLongPtr(hwnd, 0);
}
*/


