//
//  DlgCompare.c
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

INT_PTR CALLBACK CompareDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndHV = g_hwndHexView;

	switch (iMsg)
	{
	case WM_INITDIALOG:
		//AddListStringList(GetDlgItem(hwnd, IDC_ALGORITHM), szAlgorithmList, 0);
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
		return HandleContextHelp(hwnd, lParam, IDD_COMPARE);

	default:
		break;
	}
	return FALSE;

}


BOOL ShowCompareDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_COMPARE), hwnd, CompareDlgProc);
}

