//
//  DockUtil.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//
//
//  BOOL SetWindowTrans(HWND hwnd, int percent)
//  BOOL RemoveWindowTrans(HWND hwnd)
//
//	These two functions provide a wrapper around
//  the new SetLayeredWindowAttributes API call (under Win2000/XP)
//
//	These functions dynamically link to the new API call (if present). 
//  If running under win9x / NT, then the function will
//  fail gracefully.
//

#include <windows.h>
#include <tchar.h>

#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED  0x00080000
#endif

// Prototype for Win2000/XP API: SetLayeredWindowAttributes
typedef BOOL (WINAPI * SLWAProc)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002

#define ULW_COLORKEY            0x00000001
#define ULW_ALPHA               0x00000002
#define ULW_OPAQUE              0x00000004

//
//	transColor - (-1 - no transparent color)
//
BOOL SetWindowTrans(HWND hwnd, int percent, COLORREF transColor)
{
	SLWAProc pSLWA;
	HMODULE  hUser32;
	DWORD attr = LWA_ALPHA;

	if(transColor != -1)
		attr |= LWA_COLORKEY;
	else
		transColor = 0;
	
//	return TRUE;

	hUser32 = GetModuleHandle(TEXT("USER32.DLL"));

	// Try to find SetLayeredWindowAttributes
	pSLWA = (SLWAProc)GetProcAddress(hUser32, (char *)"SetLayeredWindowAttributes");

	// The version of Windows running does not support translucent windows!
	if(pSLWA == 0)
		return FALSE;

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

	// Make this window 70% alpha
	return pSLWA(hwnd, transColor, (BYTE)((255 * percent) / 100), attr);
}

BOOL RemoveWindowTrans(HWND hwnd)
{
	// Remove WS_EX_LAYERED from this window styles
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
	
	// Ask the window and its children to repaint
	RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);

	return TRUE;
}

void DrawCheckedRect(HDC hdc, RECT *rect, COLORREF fg, COLORREF bg)
{
	static WORD wCheckPat[8] = 
	{ 
		0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555 
	};

	HBITMAP hbmp;
	HBRUSH  hbr, hbrold;
	COLORREF fgold, bgold;

	hbmp = CreateBitmap(8, 8, 1, 1, wCheckPat);
	hbr  = CreatePatternBrush(hbmp);

	SetBrushOrgEx(hdc, rect->left, 0, 0);
	hbrold = (HBRUSH)SelectObject(hdc, hbr);

	fgold = SetTextColor(hdc, fg);
	bgold = SetBkColor(hdc, bg);
	
	PatBlt(hdc, rect->left, rect->top, 
				rect->right - rect->left, 
				rect->bottom - rect->top, 
				PATCOPY);
	
	SetBkColor(hdc, bgold);
	SetTextColor(hdc, fgold);
	
	SelectObject(hdc, hbrold);
	DeleteObject(hbr);
	DeleteObject(hbmp);
}

static void DrawXorFrame(RECT *rect, BOOL fDocked)
{
	static WORD dotPatternBmp[] = 
	{
		0x00aa, 0x0055, 0x00aa, 0x0055, 
		0x00aa, 0x0055, 0x00aa, 0x0055
	};

	HBITMAP hbm;
	HBRUSH  hbr;
	HANDLE  hbrushOld;

	int width, height, x, y;
	int border = 4;

	HDC hdc = GetDC(0);

	x = rect->left;
	y = rect->top;
	width  = rect->right-rect->left;
	height = rect->bottom-rect->top;

	hbm = CreateBitmap(8, 8, 1, 1, dotPatternBmp);
	hbr = CreatePatternBrush(hbm);

	SetBrushOrgEx(hdc, x, y, 0);
	hbrushOld = SelectObject(hdc, hbr);

	PatBlt(hdc, x+border,       y,               width-border,  border,        PATINVERT);
	PatBlt(hdc, x+width-border, y+border,        border,        height-border, PATINVERT);
	PatBlt(hdc, x,              y+height-border, width-border,  border,        PATINVERT);
	PatBlt(hdc, x,              y,               border,        height-border, PATINVERT);

	SelectObject(hdc, hbrushOld);
	DeleteObject(hbr);
	DeleteObject(hbm);
	ReleaseDC(0, hdc);
}

int RectWidth(RECT *rect)
{
	return rect->right-rect->left;
}

int RectHeight(RECT *rect)
{
	return rect->bottom-rect->top;
}



HWND GetOwner(HWND hwnd)
{
	return GetWindow(hwnd, GW_OWNER);
}

BOOL IsOwnedBy(HWND hwndMain, HWND hwnd)
{
	return (hwnd == hwndMain) || (GetOwner(hwnd) == hwndMain);
}

void SendFakeWMSize(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
}