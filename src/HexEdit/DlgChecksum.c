//
//  DlgChecksum.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "..\HexView\HexView.h"
#include "HexUtils.h"
#include "HexEdit.h"

TCHAR * szAlgorithmList[] = 
{
	TEXT("Checksum (8bit)"),
	TEXT("Checksum (16bit)"),	
	TEXT("Checksum (32bit)"),	
	TEXT("Checksum (64bit)"),	
	TEXT("CRC-16"),	
	TEXT("CRC-16/CCITT"),	
	TEXT("CRC-32"),	
	TEXT("Custom CRC"),	
	TEXT("MD2"),	
	TEXT("MD4"),	
	TEXT("MD5"),
	TEXT("SHA1"),
	0,
};

INT_PTR CALLBACK ChecksumDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndHV = g_hwndHexView;

	switch (iMsg)
	{
	case WM_INITDIALOG:

		AddListStringList(GetDlgItem(hwnd, IDC_ALGORITHM), szAlgorithmList, 0);
		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_CHECKSUM);

	default:
		break;
	}
	return FALSE;

}


BOOL ShowChecksumDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_CHECKSUM), hwnd, ChecksumDlgProc);
}