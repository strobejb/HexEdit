//
//  DlgReverse.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "..\HexView\HexView.h"
#include "HexUtils.h"
#include "HexEdit.h"
#include "endian.h"

BOOL ReverseData(HWND hwndHV, unsigned sublen)
{
	size_w offset;
	size_w start;
	size_w len;
	size_w length;

	BYTE   buf[100];

	HexView_GetSelStart(hwndHV, &start);
	offset = start;

	HexView_GetSelSize(hwndHV, &length);
	len = length;
	
	if(len <= 0 || sublen <= 0)
		return FALSE;

	//HexView_Group(hwndHV);
	
	while(len > 0)
	{
		if(HexView_GetData(hwndHV, offset, buf, sublen))
		{
			reverse(buf, sublen);
			HexView_SetData(hwndHV, offset, buf, sublen);
		}

		if(len < sublen)
			break;

		len    -= sublen;
		offset += sublen;
	}

	//HexView_UnGroup(hwndHV);

	HexView_SetSelStart(hwndHV, start);
	HexView_SetSelEnd(hwndHV, start+length);

	return TRUE;
}

INT_PTR CALLBACK ReverseDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndHV = g_hwndHexView;
	static BOOL   nWidth = sizeof(DWORD);
	int num;
	TCHAR buf[80];

	switch (iMsg)
	{
	case WM_INITDIALOG:
		CenterWindow(hwnd);

		SendDlgItemMessage(hwnd, IDC_ITEMWIDTH, CB_ADDSTRING, 0, (LPARAM)TEXT("2 bytes (16bit word)"));
		SendDlgItemMessage(hwnd, IDC_ITEMWIDTH, CB_ADDSTRING, 0, (LPARAM)TEXT("4 bytes (32bit dword)"));
		SendDlgItemMessage(hwnd, IDC_ITEMWIDTH, CB_ADDSTRING, 0, (LPARAM)TEXT("8 bytes (64bit qword)"));

		SetDlgItemInt(hwnd, IDC_ITEMWIDTH, nWidth, FALSE);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:

			GetDlgItemText(hwnd, IDC_ITEMWIDTH, buf, 80);

			if(_stscanf(buf, TEXT("%d"), &num) == 1)
				nWidth = num;
			
			ReverseData(hwndHV, nWidth);
			EndDialog(hwnd, TRUE);
			return 0;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return 0;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_REVERSE);

	default:
		break;
	}
	return FALSE;

}


BOOL ShowReverseDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_REVERSE), hwnd, ReverseDlgProc);
}