//
//  about.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#define STRICT

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include "hexedit.h"
#include "hexutils.h"
#include "resource.h"

//
//	About dialog-proc
//
INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HICON	hIcon;
	HFONT	hFont;
	TCHAR	szCurExe[MAX_PATH];
	TCHAR	szVersion[100] = TEXT("HexEdit ");
	TCHAR   szCopyright[100];
	HDC		hdc;

	PNMLINK pNMLink; 
	
	switch(msg)
	{
	case WM_INITDIALOG:

		CenterWindow(hwnd);

		//
		//	Set the version from the version resource
		//
		GetModuleFileName(0, szCurExe, MAX_PATH);
		GetVersionString(szCurExe, TEXT("FileVersion"), szVersion+8, 80);
		SetDlgItemText(hwnd, IDC_ABOUT_APPNAME, szVersion);

		//
		//  Set copyright info from version resource
		//
		GetVersionString(szCurExe, TEXT("LegalCopyright"), szCopyright, 100);
		SetDlgItemText(hwnd, IDC_ABOUT_LEGALSTR, szCopyright);

		//
		//	Set the dialog-icon 
		//
		hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 64, 64, 0);
		SendDlgItemMessage(hwnd, IDC_HEADER2, STM_SETIMAGE, IMAGE_ICON, (WPARAM)hIcon);

		//
		//	Get the current font for the dialog and create a BOLD version,
		//	set this as the AppName static-label's font
		//
		hFont = CreateBoldFontFromHwnd(hwnd);
		SendDlgItemMessage(hwnd, IDC_ABOUT_APPNAME, WM_SETFONT, (WPARAM)hFont, 0);
		SendDlgItemMessage(hwnd, IDC_ABOUT_WEBSITE, WM_SETFONT, (WPARAM)hFont, 0);

		return TRUE;

	case WM_CTLCOLORSTATIC:
		hdc = (HDC)wParam;

		// set the background of the following STATIC controls to WHITE
		switch(GetDlgCtrlID((HWND)lParam))
		{
		case IDC_ABOUT_APPNAME: 
		case IDC_ABOUT_LEGALSTR: 
		case IDC_ABOUT_AUTHOR:
		case IDC_ABOUT_ICON:
			SetBkColor(hdc, RGB(255, 255, 255));
			SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR)GetStockObject(WHITE_BRUSH));
			return (LONG_PTR)GetStockObject(WHITE_BRUSH);//TRUE;
		}

		return FALSE;


	case WM_NOTIFY:

		// Spawn the default web-browser when the SysLink control is clicked
		switch(((NMHDR *)lParam)->code)
		{
		case NM_CLICK: case NM_RETURN:
			pNMLink = (PNMLINK)lParam;
			ShellExecute(hwnd, TEXT("open"), pNMLink->item.szUrl, 0, 0, SW_SHOWNORMAL);
			return 0;
		}

		break;

	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return TRUE;

	case WM_COMMAND:

		if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			EndDialog(hwnd, TRUE);

		break;
	}

	return FALSE;
}

//
//	Display the About dialog-box
//
void ShowAboutDlg(HWND hwndParent)
{
	DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_ABOUT), hwndParent, AboutDlgProc, 0);
	//DialogBoxWithFont(0, MAKEINTRESOURCE(IDD_ABOUT), hwndParent, AboutDlgProc, 0, TEXT("MetaCondNormal-Roman"), 9);//TEXT("Bell MT Bold"));
}
