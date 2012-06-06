//
//  ToolPanel.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include <uxtheme.h>
#include <vssym32.h>
#include "ToolPanel.h"
#include "hexutils.h"

void DrawGripper(HTHEME hTheme, HDC hdc, int x, int y, int height, int numgrips);

// Does what it says on the tin
void DrawGripper(HTHEME hTheme, HDC hdc, int x, int y, int height, int numgrips)
{
	RECT rect;

	SetRect(&rect, x, y, x+3, y+height);

	if(hTheme)
	{
		rect.right += 6;
		DrawThemeBackground(hTheme, hdc, RP_GRIPPERVERT, 0, &rect, &rect);
	}
	else
	{
		
		if(numgrips == 1)
			OffsetRect(&rect, 1, 0);
		
		while(numgrips--)
		{
			DrawEdge(hdc, &rect,  BDR_RAISEDINNER, BF_RECT);
			OffsetRect(&rect, 3, 0);
		}
		
	}
	//DrawEdge(hdc, &rect,  BDR_RAISEDINNER, BF_RECT);
}

typedef struct
{
	HWND	hwndCtrl;
	int		position;
	int		width;
	int		height;
	int		style;

} TOOLITEM;

typedef struct
{
	TOOLITEM	ToolList[MAX_TOOLS];
	int			nListCount;
	int			panelwidth;
	int			panelheight;
	WNDPROC		userproc;
	HWND		hwndCurCtrl;

} TOOLPANEL;

static TOOLPANEL * GetToolPanel(HWND hwndPanel)
{
	return (TOOLPANEL *)GetWindowLongPtr(hwndPanel, 0);
}

//
//
//
void PositionChildren(HWND hwnd, TOOLPANEL *tp, int width, int height)
{
	HDWP hdwp = BeginDeferWindowPos(tp->nListCount);

	int ypos   = 0;
	int lineheight = 0;
	int i;

	for(i = 0; i < tp->nListCount; i++)
	{
		TOOLITEM *ti = &tp->ToolList[i];

		if(ti->style & TIS_CONTROL)
		{
			int w = ti->width;
			int h = ti->height;
			
			// check the *next* item
			if(ti[1].style & TIS_NEWLINE)//i == tp->nListCount-1)
			{
				w = width  - ti->position;
				
				if(ti[1].style == TIS_ANCHOR_BOTTOM)
				{
					h = height - ypos - ti[1].height;
				}

			}

			//if(ti->style & TIS_FIXEDHEIGHT)
			//{
			//	h = ti->height;
			//}

			lineheight = max(lineheight, ti->height);

			//RECT rect;
			//SendMessage(ti->hwndCtrl, EM_GETRECT, 0, (LPARAM)&rect);
			hdwp = DeferWindowPos(hdwp, ti->hwndCtrl, 0, ti->position,
				ypos + (lineheight - ti->height)/2, 
				w, h, 
				SWP_NOACTIVATE|SWP_NOZORDER);
			//SendMessage(ti->hwndCtrl, EM_SETRECT, 0, (LPARAM)&rect);

			lineheight = h;
			//if(ti[1].style & TIS_NEWLINE)
			//	ypos += lineheight + 2;
		}
		else if(ti->style == TIS_NEWLINE)
		{
			ypos += lineheight + ti->height;
			lineheight = 0;
		}
		//else if(ti->style == TIS_SPACER)
		//{
		//	ypos += lineheight + ti->height;
		//	lineheight = 0;
		//}
	}

	EndDeferWindowPos(hdwp);
}

void ToolPanel_Paint(HWND hwnd, TOOLPANEL *tp, HDC hdc)
{
	RECT rect;
	int  i;
	
	GetClientRect(hwnd, &rect);
	FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DFACE));

	/*hTheme = OpenThemeShim(hwnd, L"Rebar");
	rect.top -= 20;
	rect.bottom += 4;
	DrawThemeBackground(hTheme, hdc, 0, 0, &rect, 0);
	CloseThemeData(hTheme);*/

	for(i = 0; i < tp->nListCount; i++)
	{
		TOOLITEM *ti = &tp->ToolList[i];
		
		if(ti->style == TIS_GRIPPER)
		{
			DrawGripper(0, hdc, ti->position+1, 0, ti->height, 2);
		}
	}
}

//
//	ToolPanel window procedure
//
LRESULT CALLBACK ToolPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TOOLPANEL *tp = GetToolPanel(hwnd);
	UINT n;

	switch(msg)
	{
	case WM_NCCREATE:
		
		if((tp = (TOOLPANEL *)malloc(sizeof(TOOLPANEL))) == 0)
			return FALSE;

		tp->nListCount		  = 0;
		tp->ToolList[0].style = TIS_LAST;
		tp->hwndCurCtrl		  = 0;
		tp->userproc = (WNDPROC)((LPCREATESTRUCT)lParam)->lpCreateParams;

		SetWindowLongPtr(hwnd, 0, (LONG_PTR)tp);
		return TRUE;

	case WM_COMMAND:
	case WM_NOTIFY:
		if(tp->userproc)
			return tp->userproc(hwnd, msg, wParam, lParam);
		else
			break;

	case WM_NCHITTEST:
		return HTTRANSPARENT;
		
		if((n = (UINT)DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam)) == HTCLIENT)
			n = HTTRANSPARENT;

		return n;

	case WM_SIZE:
		PositionChildren(hwnd, tp, LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_ERASEBKGND:
		ToolPanel_Paint(hwnd, tp, (HDC)wParam);
		return 1;

	//case WM_KILLFOCUS:
	//	tp->hwndCurCtrl = GetFocus();
	//	return 0;

	case WM_SETFOCUS:
		{
			HWND h;

			// set focus to first child that has the WS_TABSTOP style set
			for(h = GetWindow(hwnd, GW_CHILD); h; h = GetNextWindow(h, GW_HWNDNEXT))
			{
				DWORD dwStyle = GetWindowLong(h, GWL_STYLE);
				if(dwStyle & WS_TABSTOP)
				{
					SetFocus(h);
					break;
				}
			}

		}
		return 0;

	case WM_NEXTDLGCTL:

		if(LOWORD(lParam) == TRUE)
		{
			tp->hwndCurCtrl = (HWND)wParam;
		}
		else
		{
			HWND hwndCtrl   = GetFocus();
			tp->hwndCurCtrl = GetNextDlgTabItem(hwnd, hwndCtrl, (BOOL)wParam);
			
		}

		SetFocus(tp->hwndCurCtrl);
		return 0;

	/*case WM_PAINT:
		{PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		}
		return 0;*/

	case WM_NCDESTROY:
		free(tp);
		return 0;
	}

	//return DefDlgProc(hwnd, msg, wParam, lParam);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ToolPanel_Initialize()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.cbWndExtra	 = sizeof(TOOLPANEL *);
	wc.hbrBackground = 0;
	wc.hCursor		 = LoadCursor(0, IDC_ARROW);
	wc.hInstance	 = GetModuleHandle(0);
	wc.lpfnWndProc	 = ToolPanelProc;
	wc.lpszClassName = WC_TOOLPANEL;

	RegisterClassEx(&wc);
}


int ToolPanel_Width(TOOLPANEL *tp, int *maxwidth)
{
	int width = 0;
	int i;
	
	if(maxwidth != 0)
		*maxwidth = 0;

	for(i = 0; i < tp->nListCount; i++)
	{
		width += tp->ToolList[i].width + ITEM_BORDER;

		if(maxwidth)
			*maxwidth = max(*maxwidth, width);

		if(tp->ToolList[i].style == TIS_NEWLINE)
			width = 0;
	}

	return width;// + ITEM_BORDER;
}

int ToolPanel_Height(TOOLPANEL *tp)
{
	int height = 0;
	int maxheight = 0;
	int i;
	
	for(i = 0; i < tp->nListCount; i++)
	{
		height = max(tp->ToolList[i].height, height);

		if(tp->ToolList[i].style == TIS_NEWLINE)
		{
			maxheight += height;
			height = 0;
		}
	}

	return max(maxheight, height);
}

TOOLITEM * Internal_NewItemPtr(HWND hwndPanel)
{
	TOOLPANEL *tp = GetToolPanel(hwndPanel);
		
	return &tp->ToolList[tp->nListCount++];
}

//
//	Add new control to tool panel
//
void ToolPanel_AddItem(HWND hwndPanel, HWND hwndCtrl, UINT uStyle)
{
	TOOLPANEL *tp = (TOOLPANEL *)GetWindowLongPtr(hwndPanel, 0);
	TOOLITEM  *ti = &tp->ToolList[tp->nListCount];

	ti->hwndCtrl = hwndCtrl;
	ti->style	 = TIS_CONTROL | uStyle;
	ti->width	 = GetWindowWidth(hwndCtrl);
	ti->height	 = GetWindowHeight(hwndCtrl);
	ti->position = ToolPanel_Width(tp, 0);

	tp->ToolList[++tp->nListCount].style = TIS_LAST;
}

void ToolPanel_AddGripper(HWND hwndPanel)
{
	TOOLPANEL *tp = GetToolPanel(hwndPanel);
	TOOLITEM  *ti = &tp->ToolList[tp->nListCount];

	ti->hwndCtrl = 0;
	ti->style	 = TIS_GRIPPER;
	ti->position = ToolPanel_Width(tp, 0);
	ti->width    = 10;
	ti->height	 = 22;

	tp->ToolList[++tp->nListCount].style = TIS_LAST;
}

void ToolPanel_AddNewLine(HWND hwndPanel, int vspace)
{
	TOOLPANEL *tp = GetToolPanel(hwndPanel);
	TOOLITEM  *ti = &tp->ToolList[tp->nListCount];
	
	ti->style = TIS_NEWLINE;
	ti->width = 0;
	ti->height = vspace;
	ti->position = 0;
	tp->ToolList[++tp->nListCount].style = TIS_LAST;
}

//
//	
//
void ToolPanel_AddAnchor(HWND hwndPanel, int type, int height)
{
	TOOLPANEL *tp = GetToolPanel(hwndPanel);
	TOOLITEM  *ti = &tp->ToolList[tp->nListCount];
	
	ti->style		= TIS_ANCHOR_BOTTOM;
	ti->width		= 0;
	ti->height		= height;
	ti->position	= 0;
	tp->ToolList[++tp->nListCount].style = TIS_LAST;
}

void ToolPanel_AddVSpace(HWND hwndPanel, int height)
{
	TOOLITEM *ti = Internal_NewItemPtr(hwndPanel);

	ti->style		= TIS_SPACER;
	ti->height		= height;
}

//
//	Size the toolpanel to its contents
//	
void ToolPanel_AutoSize(HWND hwndPanel)
{
	TOOLPANEL *tp = GetToolPanel(hwndPanel);

	int width, height;
	ToolPanel_Width(tp, &width);
	height = ToolPanel_Height(tp);

	SetWindowSize(hwndPanel, width, height, NULL);
}

HWND ToolPanel_Create(HWND hwndParent, WNDPROC userproc)
{
	ToolPanel_Initialize();
	return CreateWindowEx(WS_EX_CONTROLPARENT,//WS_EX_TRANSPARENT  ,
		WC_TOOLPANEL, 0, 
		DS_CONTROL|WS_CLIPCHILDREN|
		WS_CHILD|WS_VISIBLE,0,0,100,32,hwndParent,0,0,userproc
		);
}