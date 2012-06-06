//
//  ToolView.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	Custom toolbar
//
#include <windows.h>
#include <tchar.h>

#define WC_TOOLBAR _T("Toolbar32")

class Toolbar
{
public:
	Toolbar(HWND hwnd)
	{
		m_hWnd = hwnd;
	}

private:
	HWND	m_hWnd;

};

LRESULT WINAPI ToolbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Toolbar *ptb = (Toolbar *)GetWindowLong(hwnd, 0);

	switch(msg)
	{
	case WM_NCCREATE:
		if((ptb = new Toolbar(hwnd)) == 0)
			return FALSE;

		return TRUE;

	case WM_NCDESTROY:
		delete ptb;
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

ATOM InitToolbarClass()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.cbWndExtra		= sizeof(Toolbar *);
	wc.hCursor			= LoadCursor(0, IDC_ARROW);
	wc.hbrBackground	= 0;
	wc.hInstance		= GetModuleHandle(0);
	wc.lpfnWndProc		= ToolbarWndProc;
	wc.lpszMenuName		= WC_TOOLBAR;

	return RegisterClassEx(&wc);
}

HWND CreateToolbarWnd(HWND hwndParent)
{
	InitToolbarClass();


	return CreateWindowEx(0, WC_TOOLBAR, 0, WS_CHILD|WS_VISIBLE, 0,0,0,0,hwndParent,0,0,0);
}