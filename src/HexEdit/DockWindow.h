//
//  DockWindow.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	DockWindow.h
//
//	A floating tool-window library
//	
//	Copyright J Brown 2001
//	Freeware
//
#ifndef DOCKWINDOW_INCLUDED
#define DOCKWINDOW_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

typedef struct
{
	DWORD	dwStyle;		//styles..

	int		xpos;			//coordinates of FRAME when floating
	int		ypos;

	int		cxFloating;		//size of CONTENTS when floating 
	int		cyFloating;

	int		nDockedSize;	//width/height of window when docked..
	BOOL	fDocked;		//docked/floating?
	UINT	uDockedState;	//left/right/top/bottom when docked?

	RECT	rcBorderDock;	// border to place around each side of contents
	RECT	rcBorderFloat;	// border to place around each side of contents

	DWORD	dwUser;			//whatever..

	//
	//	Internal to DockWindow library, do not modify
	//
	int		nFrameWidth;	//window frame width when floating
	int		nFrameHeight;	//(Private)

	HWND	hwndContents;	//handle to the window to host
	HWND	hwnd;			//handle to container (THIS!!!)

	BOOL	fDragging;		//Are we dragging?
	RECT	oldrect;		//                


} DockWnd;

// Use in DWN_ISDOCKABLE

typedef struct
{
    NMHDR	 hdr;
	HWND     hwndDock;      /* Handle to dock-window */
    DockWnd *pDockWnd;      /* Pointer to the DockWnd structure for that window */
    RECT    *dragrect;      /* Current drag-rectangle */

} NMDOCKWNDQUERY;

#pragma pack(pop)

//
//	Function declarations
//
HWND CreateDockWnd(DockWnd *dwp, HWND hwndParent, TCHAR szCaption[]);
void DockWnd_ToggleDockingMode(HWND hwnd);

BOOL DockWnd_Position	 (HWND hwndMain, HDWP hdwp, DockWnd dwnd[], int nNumDockWnds, RECT *rect);
UINT DockWnd_GetDockSide (HWND hwnd, NMDOCKWNDQUERY *nmdwq, RECT *prc1, RECT *prc2);


LRESULT HANDLE_NCACTIVATE (WNDPROC oldProc, HWND hwndMain, HWND hwndMsg, WPARAM wParam, LPARAM lParam);
LRESULT HANDLE_ENABLE     (WNDPROC oldProc, HWND hwndMain, HWND hwndMsg, WPARAM wParam, LPARAM lParam);

//
//	DockWnd styles
//
#define DWS_BORDERTOP			0x0001	//draw a top    etched border WHEN DOCKED
#define DWS_BORDERBOTTOM		0x0002	//draw a bottom etched border WHEN DOCKED
#define DWS_BORDERLEFT			0x0004	//draw a top    etched border WHEN DOCKED
#define DWS_BORDERRIGHT			0x0008	//draw a bottom etched border WHEN DOCKED
#define DWS_DRAWGRIPPERDOCKED	0x0010	//draw a gripper when docked
#define DWS_DRAWGRIPPERFLOATING	0x0020	//draw a gripper when floating
#define DWS_FORCEDOCK			0x0040
#define DWS_FORCEFLOAT			0x0080
#define DWS_RESIZABLE			0x0100	//is resizable when docked?
#define DWS_NORESIZE			0x0200	//prevent resize when floating

#define DWS_USEBORDERS			0x1000	//use the rcBorders member to define additional border space
#define DWS_NOSETFOCUS			0x2000
#define DWS_NODESTROY			0x4000	//hides the dock-window instead of destroying it

#define DWS_ALLOW_DOCKLEFT	   0x10000
#define DWS_ALLOW_DOCKRIGHT	   0x20000
#define DWS_ALLOW_DOCKTOP	   0x40000
#define DWS_ALLOW_DOCKBOTTOM   0x80000

#define DWS_ALLOW_DOCKALL		(DWS_ALLOW_DOCKLEFT   | \
								 DWS_ALLOW_DOCKBOTTOM | \
								 DWS_ALLOW_DOCKRIGHT  | \
								 DWS_ALLOW_DOCKTOP)

#define DWS_DOCKED_NOTDOCKED	0
#define DWS_DOCKED_FLOATING		0
#define DWS_DOCKED_LEFT			0x10000
#define DWS_DOCKED_RIGHT		0x20000
#define DWS_DOCKED_TOP			0x40000
#define DWS_DOCKED_BOTTOM		0x80000

//
//	DockWnd message notifications...
//
#define DWN_BASE		(0U - 2048U)
#define DWN_HIDDEN		(DWN_BASE - 0)
#define DWN_DOCKED		(DWN_BASE - 1)
#define DWN_UNDOCKED	(DWN_BASE - 2)
#define DWN_CLOSED		(DWN_BASE - 3)
#define DWN_ISDOCKABLE	(DWN_BASE - 4)


#ifdef __cplusplus
}
#endif

#endif
