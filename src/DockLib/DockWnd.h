//
//  DockWnd.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef DOCKWND_INCLUDED
#define DOCKWND_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define POPUP_STYLES   (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME)
#define POPUP_EXSTYLES (WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE)
#define CHILD_STYLES   (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)
#define CHILD_EXSTYLES (0)

#define MAX_DOCK_WINDOWS 32
#define MAX_DOCK_CHILDREN 100
#define DEFAULT_BORDER 2
#define GRIP_SIZE 10

#define WC_DOCKBAR		TEXT("DockBar")
#define WC_TRANSWINDOW	TEXT("TransWindow")

typedef struct _DOCKOBJ 
{
	struct _DOCKOBJ * flink;
	struct _DOCKOBJ * blink;
    UINT	id;

} DOCKOBJ;

void RemoveObj(void *obj);
void InsertBefore(void *obj, void *_this);
void * AllocDockObj(void * flink, void * blink, UINT id, UINT size);

struct _DOCKPANEL;
struct _DOCKSERVER; 

//
//	Single 'DOCK' page/tab
//
typedef struct _DOCKWND
{
	struct _DOCKWND * flink;
	struct _DOCKWND * blink;
	UINT			  uWndId;	// the id of the tab

	HWND	hwndContents;
	UINT	uGroupId;			// the id of the tab
	//UINT	uPanelId;			// which panel we belong to

	struct _DOCKPANEL *pDockPanel;
	BOOL	fVisible;
	TCHAR	szTitle[200];

} DOCKWND, *PDOCKWND;

//
//	DOCKPANEL is a container for one or more DOCKWNDs
//
typedef struct _DOCKPANEL
{
	struct _DOCKPANEL * flink;
	struct _DOCKPANEL * blink;
	UINT				uPanelId;

	UINT	uCurrentTabId;
	//int size;

	SIZE	DockSize;
	SIZE	FloatSize;

//	UINT	uPanelGroupId;

//	int		side;

	int		xpos;		
	int		ypos;

	DWORD	dwStyle;
	BOOL	fDocked;		
	UINT	fSplitting;		// is the splitter-bar being used?

	RECT	rcDockBorder;
	RECT	rcFloatBorder;

	//TCHAR	szTitle[200];

	HWND	hwndPanel;
	HWND	hwndContents;	// current contents!

	HWND	hwndTabView;
	HWND	hwndMain;

	UINT	fDragStatus;
	BOOL	fDragging;		// is the dockpanel currently being dragged around?
	BOOL	fSticky;		// indicates if the dockpanel overlaps the main window and should 'stick' to it
	BOOL	fUndockNextMsg;	// indicates whether the dockpanel should be floated as a separate window
	BOOL	fVisible;		// is the dockpanel visible?


	UINT	uDeferShowCmd;	// internal flag

    DOCKWND		* WndListHead, * WndListTail;
	struct _DOCKSERVER  * pDockServer;

	//DOCKCONTENTS	dockContent

} DOCKPANEL, *PDOCKPANEL;

//
//
//
typedef struct _DOCKGROUP {
	
	//UINT	uPanelGroupId;	// id of the panel-group  (not same as the DOCKWND group id)
	UINT	uFirstPanelId;	// id of first panel in group
	UINT	uDockPos;		// left/right/top/bottom etc
	INT		nSize;			// docked size
} DOCKGROUP, *PDOCKGROUP;

//
//	DOCKSERVER is the host (main) top-level window 
//
typedef struct _DOCKSERVER
{
	DWORD		dwStyle;
	WNDPROC		oldproc;
	HWND		hwndMain;
	RECT		DockRect;
	RECT		ClientRect;

	BOOL		fDeferShowWindow;

	TCHAR		szRegLoc[100];

	//int			nNumDockPanels;
	//DOCKPANEL	PanelList[MAX_DOCK_WINDOWS];
	//int			PanelOrder[MAX_DOCK_WINDOWS];

	// each index specifies the *first* panel in the group
	//int			PanelGroup[100];
	DOCKGROUP	PanelGroup[100];
	int			nNumGroups;

	//int			nNumDockWnds;
	//DOCKWND		WndList[MAX_DOCK_CHILDREN];

	//DOCKWND		* WndListHead, * WndListTail;
	DOCKPANEL   * PanelListHead, * PanelListTail;

} DOCKSERVER;


// dockserver.c
DOCKSERVER *GetDockServer(HWND hwndMain);

// dockutil.c
void DrawCheckedRect(HDC hdc, RECT *rect, COLORREF fg, COLORREF bg);
BOOL RemoveWindowTrans(HWND hwnd);
BOOL SetWindowTrans(HWND hwnd, int percent, COLORREF transColor);
HWND GetOwner(HWND hwnd);
BOOL IsOwnedBy(HWND hwndMain, HWND hwnd);
void SendFakeWMSize(HWND hwnd);
int RectWidth(RECT *rect);
int RectHeight(RECT *rect);

// dockwnd.c
void DockWindow(DOCKPANEL *dwp);

// docklib.c
//void	DockWnd_SavePanelSettings(DOCKSERVER *dsp, DOCKPANEL *dwp, int idx);
VOID    DockPanel_NotifyParent(DOCKPANEL *dpp, UINT nNotifyCode, NMHDR *optional);
LRESULT DockWnd_NotifyParent(DOCKPANEL *dpp, DOCKWND *dwp, UINT nNotifyCode, NMHDR *optional);

DOCKWND * DOCKWNDFromId(HWND hwndMain, UINT uId);
DOCKPANEL * DOCKPANELFromId(HWND hwndMain, UINT uId);

DOCKPANEL * NewDockPanel(DOCKSERVER *dsp);
void SetDockPanel(DOCKPANEL *dpp, DOCKWND *dwp);

HWND ShowTransWindow(DOCKPANEL *dppUnder, HWND hwnd, RECT *rect, DWORD side, DWORD type);

void DockPanel_SetKeyboardHook(DOCKPANEL *dpp);
void DockPanel_RemoveKeyboardHook(DOCKPANEL *dpp);

BOOL DockWnd_ShowInternal2(DOCKWND *dwp);
BOOL DockWnd_HideInternal2(DOCKWND *dwp, BOOL fPreserveContent);

BOOL IsMouseOverSplitter(DOCKPANEL *dwp);
void PositionContent(DOCKPANEL *dwp);
void UpdateDockTabView(DOCKPANEL *dpp);
void AdjustRectBorders(DOCKPANEL *dwp, RECT *rect, int dir);
void CalcFloatingRect(HWND hwnd, RECT *rect);

void DrawDockPanelBackground(DOCKPANEL *dpp, HDC hdc);

#define DOCKSERVER_CLIENTAREA ((DOCKPANEL *)1)

#ifdef __cplusplus
}
#endif

#endif