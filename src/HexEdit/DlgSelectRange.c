//
//  DlgSelectRange.c
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

INT_PTR CALLBACK SelectRangeDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL		fSelectLen = TRUE;
	static BOOL		fHexOffset = TRUE;
	static size_w	nLastLen   = 0;

	HWND hwndHV = g_hwndHexView;

	switch (iMsg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hwnd, IDC_HEX, fHexOffset);
		CheckDlgButton(hwnd, IDC_SELLEN, fSelectLen ? TRUE : FALSE);
		CheckDlgButton(hwnd, IDC_SELEND, fSelectLen ? FALSE : TRUE);
		SetDlgItemBaseInt(hwnd, IDC_RANGETEXT, nLastLen, fHexOffset, 0);
		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			fHexOffset  = IsDlgButtonChecked(hwnd, IDC_HEX);
			fSelectLen  = IsDlgButtonChecked(hwnd, IDC_SELLEN) ? TRUE : FALSE;
			nLastLen	= GetDlgItemBaseInt(hwnd, IDC_RANGETEXT, fHexOffset);
			
			if(fSelectLen)
			{
				size_w start;

				HexView_GetSelStart(hwndHV, &start);
				HexView_SetSelEnd(hwndHV, start + nLastLen);
			}
			else
			{
				HexView_SetSelEnd(hwndHV, nLastLen);
			}

			EndDialog(hwnd, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		case IDC_HEX:
			nLastLen	= GetDlgItemBaseInt(hwnd, IDC_RANGETEXT, fHexOffset);
			fHexOffset	= IsDlgButtonChecked(hwnd, IDC_HEX);
			SetDlgItemBaseInt(hwnd, IDC_RANGETEXT, nLastLen, fHexOffset, 0);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_SELECTRANGE);

	default:
		break;
	}
	return FALSE;

}


int ShowSelectRangeDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SELECTRANGE), hwnd, SelectRangeDlgProc);
}
