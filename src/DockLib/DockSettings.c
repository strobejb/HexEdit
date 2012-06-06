/*
 *  DockSettings.c
 *
 *  www.catch22.net
 *
 *  Copyright (C) 2012 James Brown
 *  Please refer to the file LICENCE.TXT for copying permission
 *
 *
 *	Window procedure for the DockWnd client window
 *
 */

#define _WIN32_WINNT 0x501
#include <windows.h>
#include <tchar.h>
#include <uxtheme.h>
#include <shlwapi.h>

#include "trace.h"
#include "docklib.h"
#include "dockwnd.h"

#include "..\HexEdit\reglib.h"
#include "..\HexEdit\HexUtils.h"

static DOCKWND * LoadDockWnd(HKEY hKey)
{
    DOCKWND *dwp = AllocDockObj(0, 0, 0, sizeof(DOCKWND));

    GetSettingInt(hKey, TEXT("Id"), &dwp->uWndId, 0);
	GetSettingInt(hKey, TEXT("GroupId"), &dwp->uGroupId, 0);
	GetSettingStr(hKey, TEXT("Title"), dwp->szTitle, 200, TEXT(""));
	GetSettingInt(hKey, TEXT("Visible"), &dwp->fVisible, TRUE);

    return dwp;
}

static DOCKPANEL * LoadDockPanel(HKEY hKey)
{
    int i;
    DOCKPANEL *dpp = AllocDockObj(0, 0, 0, sizeof(DOCKPANEL));

    //dwp->uPanelId = _ttoi(szSubKey);
    GetSettingInt(hKey, TEXT("Id"),			&dpp->uPanelId, 0);
    GetSettingInt(hKey, TEXT("CurId"),		&dpp->uCurrentTabId, 0);
    GetSettingInt(hKey, TEXT("xpos"),		&dpp->xpos, 0);
    GetSettingInt(hKey, TEXT("ypos"),		&dpp->ypos, 0);
    GetSettingInt(hKey, TEXT("FloatWidth"),	&dpp->FloatSize.cx, 0);
    GetSettingInt(hKey, TEXT("FloatHeight"),&dpp->FloatSize.cy, 0);
    GetSettingInt(hKey, TEXT("DockWidth"),	&dpp->DockSize.cx, 0);
    GetSettingInt(hKey, TEXT("DockHeight"),	&dpp->DockSize.cy, 0);
    GetSettingInt(hKey, TEXT("Style"),		&dpp->dwStyle, 0);
    GetSettingInt(hKey, TEXT("Docked"),		&dpp->fDocked, 0);
    GetSettingInt(hKey, TEXT("Sticky"),		&dpp->fSticky, 0);
    GetSettingInt(hKey, TEXT("Visible"),	&dpp->fVisible, 1);

    SetRect(&dpp->rcDockBorder,  DEFAULT_BORDER, DEFAULT_BORDER, DEFAULT_BORDER, DEFAULT_BORDER);
    SetRect(&dpp->rcFloatBorder, 0, DEFAULT_BORDER, 0, 0);

	dpp->WndListHead = AllocDockObj(0, 0, -1, sizeof(DOCKWND));
	dpp->WndListTail = AllocDockObj(0, 0, -2, sizeof(DOCKWND));
	dpp->WndListHead->flink = dpp->WndListTail;
	dpp->WndListTail->blink = dpp->WndListHead;

	TRACEA("Loading Panel %d\n", dpp->uPanelId);

	// load any DOCKWNDs that this panel 
    for(i = 0; ; i++)
	{
        TCHAR szSubKey[200];
        HKEY  hSubKey;
		DWORD nSubKeyLen = 200;

		//if(S_OK != RegEnumKeyEx(hKey, i, szSubKey, &nSubKeyLen, 0, 0, 0, 0))
		//	break;

		wsprintf(szSubKey, TEXT("Wnd%05d"), i+1);

		if(!RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey))
        {
			DOCKWND *dwp = LoadDockWnd(hSubKey);

			TRACEA("Loading DockWnd %d - %d\n", i+1, dwp->uWndId);

			if(dwp)
			{
				SetDockPanel(dpp, dwp);
			}

            RegCloseKey(hSubKey);
        }
        else
        {
            break;
        }
    }

	if(dpp->WndListHead->flink == dpp->WndListTail)
	{
		//free(dpp);
		//dpp = 0;
	}

    return dpp;
}

//
//	Load docking-server's state from specified registry key
//
BOOL WINAPI DockWnd_LoadSettings(HWND hwndMain, BOOL fRestoreWindow)
{
	HKEY hKey;
	WINDOWPLACEMENT wp = { sizeof(wp) };
	DOCKSERVER *dsp;
	int i;
	BOOL  status = TRUE;

	if((dsp = GetDockServer(hwndMain)) == 0)
		return FALSE;

	if(RegCreateKeyEx(HKEY_CURRENT_USER, dsp->szRegLoc, 0, 0, 0, KEY_READ, 0, &hKey, 0))
		return FALSE;

	//
	//	Get the main window's position
	//
	if(GetSettingBin(hKey, TEXT("WinPos"), &wp, sizeof(wp)))
	{
		if(wp.showCmd != SW_RESTORE && wp.showCmd != SW_NORMAL && wp.showCmd != SW_SHOWMAXIMIZED)
		{
			wp.showCmd = SW_RESTORE;
		}

		SetWindowPlacement(hwndMain, &wp);
	}
	else
	{
		//wp.showCmd = SW_SHOW;
		//SetRect(&wp.rcNormalPosition, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100);
		SetWindowPos(hwndMain, 0, 0, 0, 900, 700, SWP_NOMOVE|SWP_NOZORDER);
	}

	ForceVisibleDisplay(hwndMain);

	//
	//	Load the PanelList
	//
	for(i = 0; ; i++)
	{
	    TCHAR szSubKey[200];
        HKEY  hSubKey;
		DWORD nSubKeyLen = 200;

		//if(S_OK != RegEnumKeyEx(hKey, i, szSubKey, &nSubKeyLen, 0, 0, 0, 0))
		//	break;

        wsprintf(szSubKey, TEXT("Panels\\Panel%05d"), i+1);

		if(!RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey))
		{
			DOCKPANEL *dpp = LoadDockPanel(hSubKey);

			if(dpp)
			{
				InsertBefore(dpp, dsp->PanelListTail);
				dpp->hwndMain = dsp->hwndMain;
				dpp->pDockServer = dsp;
				//dpp->uPanelId = i;
			}

			RegCloseKey(hSubKey);
		}
		else
		{
			break;
		}
	}

	RegCloseKey(hKey);

	return status;
}

static void DockWnd_SavePanelSettings(HKEY hSubKey, DOCKSERVER *dsp, DOCKPANEL *dpp)
{
	HKEY  hSubKey2;
	HKEY  hUserKey;

	TCHAR szName[200];
	DOCKWND *dwp;
	int id = 1;

	// save the panel
	WriteSettingInt(hSubKey, TEXT("Id"),			dpp->uPanelId);
	WriteSettingInt(hSubKey, TEXT("CurId"),			dpp->uCurrentTabId);
	WriteSettingInt(hSubKey, TEXT("xpos"),			dpp->xpos);
	WriteSettingInt(hSubKey, TEXT("ypos"),			dpp->ypos);
	WriteSettingInt(hSubKey, TEXT("FloatWidth"),	dpp->FloatSize.cx);
	WriteSettingInt(hSubKey, TEXT("FloatHeight"),	dpp->FloatSize.cy);
	WriteSettingInt(hSubKey, TEXT("DockWidth"),		dpp->DockSize.cx);
	WriteSettingInt(hSubKey, TEXT("DockHeight"),	dpp->DockSize.cy);
	WriteSettingInt(hSubKey, TEXT("Style"),			dpp->dwStyle);
	WriteSettingInt(hSubKey, TEXT("Docked"),		dpp->fDocked);
	WriteSettingInt(hSubKey, TEXT("Sticky"),		dpp->fSticky);
	WriteSettingInt(hSubKey, TEXT("Visible"),		dpp->fVisible);

	// enumerate all the DOCKWND in this panel
	for(dwp = dpp->WndListHead->flink; dwp != dpp->WndListTail; dwp = dwp->flink)
	{
		TRACE(TEXT("  Saving Wnd%05d (%s)\n"), dwp->uWndId, dwp->szTitle);

		wsprintf(szName, TEXT("Wnd%05d"), id++);

		if(RegCreateKeyEx(hSubKey, szName, 0, 0, 0, KEY_WRITE, 0, &hSubKey2, 0) == S_OK)
		{
			WriteSettingInt(hSubKey2, TEXT("Id"),	    dwp->uWndId);
			WriteSettingInt(hSubKey2, TEXT("GroupId"),	dwp->uGroupId);
			WriteSettingStr(hSubKey2, TEXT("Title"),	dwp->szTitle);
			WriteSettingInt(hSubKey2, TEXT("Visible"),  dwp->fVisible);

			// ask the client to save their settings
			wsprintf(szName, TEXT("%s\\Content\\Content%05d"), dsp->szRegLoc, dwp->uWndId);

			if(RegCreateKeyEx(HKEY_CURRENT_USER, szName, 0, 0, 0, KEY_WRITE, 0, &hUserKey, 0) == S_OK)
			{
				NMDOCKWNDCREATE nmdw = { {0}, dwp->uWndId, hUserKey, dwp->hwndContents };

				DockWnd_NotifyParent(dpp, dwp, DWN_SAVESETTINGS, (NMHDR *)&nmdw);
				RegCloseKey(hUserKey);
			}

			RegCloseKey(hSubKey2);
		}				
	}
}

//
//	Save docking-server's state to the specified registry key
//
BOOL WINAPI DockWnd_SaveSettings(HWND hwndMain)
{
	WINDOWPLACEMENT wp = { sizeof(wp) };
	DOCKSERVER *dsp;
	HKEY hKey;
	DOCKPANEL *dpp;

	DWORD id = 1;

	if((dsp = GetDockServer(hwndMain)) == 0)
		return FALSE;

	//if(SHDeleteKey(HKEY_CURRENT_USER, szRegLoc))
	//	return FALSE;

	if(RegCreateKeyEx(HKEY_CURRENT_USER, dsp->szRegLoc, 0, 0, 0, KEY_WRITE, 0, &hKey, 0))
		return FALSE;

	// delete existing panels (because we will recreate it)
	SHDeleteKey(hKey, TEXT("Panels"));

	//RegDeleteKeyEx(hKey, TEXT("PanelList"), 0, 0);

	// save the main window position
	GetWindowPlacement(hwndMain, &wp);
	WriteSettingBin(hKey, TEXT("WinPos"), &wp, sizeof(wp));

	for(dpp = dsp->PanelListHead->flink; dpp != dsp->PanelListTail; dpp = dpp->flink)
	{
		TCHAR szName[30];
		HKEY hSubKey;
		
		wsprintf(szName, TEXT("Panels\\Panel%05d"), id++);

		// save the DOCKPANEL size/position etc
		if(RegCreateKeyEx(hKey, szName, 0, 0, 0, KEY_WRITE, 0, &hSubKey, 0) == S_OK)
		{
			DockWnd_SavePanelSettings(hSubKey, dsp, dpp);
			RegCloseKey(hSubKey);
		}
	}
	
	// delete any remaining panels 
	//DelKey(hKey, TEXT("Panel"), count);

//	WriteSettingBin(hKey, TEXT("DockOrder"), orderList, count * sizeof(DWORD));

	RegCloseKey(hKey);
	return TRUE;
}
