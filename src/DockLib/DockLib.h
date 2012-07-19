//
//  DockLib.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef DOCKLIB_INCLUDED
#define DOCKLIB_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DWN_BASE			(0U - 2048U)
#define DWN_CREATE_CONTENT	(DWN_BASE - 0)
#define DWN_DOCKCHANGE		(DWN_BASE - 1)
#define DWN_CLOSING			(DWN_BASE - 2)
#define DWN_SAVESETTINGS	(DWN_BASE - 3)
#define DWN_UPDATE_CONTENT  (DWN_BASE - 4)

#define DWSZ_CONTENT		(-2)
#define DWSZ_FIXED			(-1)

//
//	DockWnd frame styles
//
#define DWFS_HIDEFLOATING		0x0001	// hide floating toolbars when inactive

//
//	DockWnd client styles
//
#define DWS_BORDERTOP			0x0001	//draw a top    etched border WHEN DOCKED
#define DWS_BORDERBOTTOM		0x0002	//draw a bottom etched border WHEN DOCKED
#define DWS_BORDERLEFT			0x0004	//draw a top    etched border WHEN DOCKED
#define DWS_BORDERRIGHT			0x0008	//draw a bottom etched border WHEN DOCKED
#define DWS_DRAWGRIPPERDOCKED	0x0010	//draw a gripper when docked
#define DWS_DRAWGRIPPERFLOATING	0x0020	//draw a gripper when floating
#define DWS_DRAWGRIPPER			(DWS_DRAWGRIPPERDOCKED|DWS_DRAWGRIPPERFLOATING)
#define DWS_FORCEDOCK			0x0040
#define DWS_FORCEFLOAT			0x0080
#define DWS_FIXED_HORZ			0x0100	//prevent horizontal resize
#define DWS_FIXED_VERT			0x0200	//prevent vertical resize
#define DWS_FIXED_SIZE			(DWS_FIXED_VERT|DWS_FIXED_HORZ)

#define DWS_USEBORDERS			0x1000	//use the rcBorders member to define additional border space
#define DWS_NOSETFOCUS			0x2000
#define DWS_NODESTROY			0x4000	//hides the dock-window instead of destroying it

#define DWS_ALLOW_DOCKLEFT	   0x10000
#define DWS_ALLOW_DOCKRIGHT	   0x20000
#define DWS_ALLOW_DOCKTOP	   0x40000
#define DWS_ALLOW_DOCKBOTTOM   0x80000

#define DWS_DOCKED_FLOATING	  0
#define DWS_DOCKED_LEFT		  0x100000
#define DWS_DOCKED_RIGHT	  0x200000
#define DWS_DOCKED_TOP		  0x400000
#define DWS_DOCKED_BOTTOM	  0x800000
#define DWS_DOCKED_MASK		  ( DWS_DOCKED_LEFT		 | \
								DWS_DOCKED_RIGHT	 | \
								DWS_DOCKED_TOP		 | \
								DWS_DOCKED_BOTTOM)


#define DWS_ALLOW_DOCKALL		(DWS_ALLOW_DOCKLEFT   | \
								 DWS_ALLOW_DOCKBOTTOM | \
								 DWS_ALLOW_DOCKRIGHT  | \
								 DWS_ALLOW_DOCKTOP)

#define DWS_SPLITTER			0x1000000
#define DWS_THEMED_BACKGROUND	0x2000000
#define DWS_DOCKED_TITLEBAR		0x4000000
#define DWS_TABSTRIP			0x8000000

typedef struct _NMDOCKWND
{
	NMHDR hdr;

} NMDOCKWND, *PNMDOCKWND;

typedef struct _NMDOCKWNDCREATE
{
	NMHDR hdr;
	UINT  uId;
	HKEY  hKey;
	HWND  hwndClient;
} NMDOCKWNDCREATE, *PNMDOCKWNDCREATE;

//
//	DockWnd_Initialize
//
//	Initialize docking library for the specified top-level window,
//  use the specified registry location to load the docking configuration
//
//	hwndMain will receive DWN_xxx notifications
//
BOOL WINAPI DockWnd_Initialize(HWND hwndMain, LPCTSTR szRegLoc);

//
//	DockWnd_SaveSettings
//
//	Save current docking configuration to the registry
//
BOOL WINAPI DockWnd_SaveSettings(HWND hwndMain);
BOOL WINAPI DockWnd_LoadSettings(HWND hwndMain, BOOL fRestoreWindow);

BOOL WINAPI DockWnd_Undefined(HWND hwndMain, UINT uId);

BOOL WINAPI DockWnd_Update(HWND hwndMain);

BOOL WINAPI DockWnd_IsOpen(HWND hwndMain, UINT uId);

HWND WINAPI DockWnd_GetContents(HWND hwndMain, UINT uId);

//
//	DockWnd_Register
//
//	Register a docking window with the specified main frame window. If the
//  dock-window is already registered (i.e. with Initialize) the function
//  returns false, otherwise returns true
//
BOOL WINAPI DockWnd_Register(HWND hwndMain, UINT uId, LPCTSTR szTitle);
BOOL WINAPI DockWnd_RegisterEx(HWND hwndMain, UINT uId, UINT uGroupId, LPCTSTR szTitle);

//
//	DockWnd_SetBorders
//
//	Save current docking configuration to the registry
//
BOOL WINAPI DockWnd_SetBorders(HWND hwndMain, UINT uId, RECT *floating, RECT *docked);

UINT WINAPI DockWnd_SetStyle(HWND hwndMain, UINT uId, UINT uStyle, UINT uMask);
//HWND WINAPI DockWnd_GetDockWindow(HWND hwndMain, UINT uId);
//HWND WINAPI DockWnd_GetDockContents(HWND hwndMain, UINT uId);
BOOL WINAPI DockWnd_Show(HWND hwndMain, UINT uId, BOOL fShow);
BOOL WINAPI DockWnd_ShowGroup(HWND hwndMain, UINT uGroupId, BOOL fShow);
BOOL WINAPI DockWnd_SetGroupId(HWND hwndMain, UINT uId, UINT uGroupId);
BOOL WINAPI DockWnd_SetPanelId(HWND hwndMain, UINT uId, UINT uPanelId);
PVOID ShowHideWindow(HWND hwnd, BOOL fShow, HDWP hdwp);

HWND WINAPI DockWnd_GetWindow(HWND hwndMain, UINT uId);
HWND WINAPI DockWnd_GetContents(HWND hwndMain, UINT uId);
VOID WINAPI DockWnd_SetContents(HWND hwndMain, UINT uId, HWND hwndContent);
BOOL WINAPI DockWnd_Dock(HWND hwndMain, UINT uId);
BOOL WINAPI DockWnd_Undock(HWND hwndMain, UINT uId);
UINT WINAPI DockWnd_GetId(HWND hwndMain, HWND hwndDockWnd);

BOOL WINAPI DockWnd_UpdateContent(HWND hwndMain, UINT uId);


VOID DockWnd_DeferShowPopups(HWND hwndMain);//, int nNumWindows);
VOID DockWnd_ShowDeferredPopups(HWND hwndMain);
//BOOL DockWnd_EndDefer(HWND hwndMain);

HDWP DockWnd_DeferPanelPos(HDWP hdwp, HWND hwndMain, RECT *rect);

LRESULT HANDLE_NCACTIVATE(HWND hwndMain, HWND hwnd, WPARAM wParam, LPARAM lParam, WNDPROC oldProc OPTIONAL);

BOOL WINAPI DockWnd_IsDialogMessage(HWND hwndMain, UINT uId, MSG *msg);

BOOL WINAPI DockWnd_NextWindow(HWND hwndMain);
BOOL WINAPI DockWnd_ShowGui(HWND hwndMain);

UINT WINAPI DockWnd_EnumVisible(HWND hwndMain, UINT nGroupId, int idx);

#ifdef __cplusplus
}
#endif

#endif