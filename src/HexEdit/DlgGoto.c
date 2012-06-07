//
//  DlgGoto.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "..\HexView\HexView.h"
#include "HexUtils.h"
#include "HexEdit.h"

extern HWND g_hwndGoto;
HWND CreatePinToolbar(HWND hwndDlg, UINT nCtrlId, BOOL fRightAligned);
BOOL UpdatePin(HWND hwndDlg, UINT uId, BOOL fChecked);
BOOL IsToolbarButtonChecked(HWND hwndTB, UINT nCtrlId);

static size_w	nLastOffset	 = 0;
static int		nGotoRelative = IDC_GOTO_BEGIN;

void Goto(HWND hwndHexView, size_w offset, int nGotoRelative)
{
	if(nGotoRelative == IDC_GOTO_RELATIVE)
	{
		size_w curpos;
		HexView_GetCurPos(hwndHexView, &curpos);

		offset += curpos;
	}
	else if(nGotoRelative == IDC_GOTO_EOF)
	{
		size_w filesize;
		HexView_GetFileSize(hwndHexView, &filesize);
			
		offset = filesize - offset;
	}

	HexView_SetCurPos(hwndHexView, offset);
	SetFocus(hwndHexView);

	InvalidateRect(hwndHexView, 0, 0);

	HexView_CursorChanged(g_hwndMain, hwndHexView);
}

void RepeatGoto(HWND hwndHexView)
{
	Goto(hwndHexView, nLastOffset, nGotoRelative);
}

size_w GetBookmarkComboOffset(HWND hwndCombo)
{
	int idx;
	TCHAR buf[32];
	UINT64 num = 0;

	idx = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);

	SendMessage(hwndCombo, CB_GETLBTEXT, idx, (LPARAM)buf);

	_stscanf(buf, TEXT("%I64x"), &num); 
	return (size_w)num;
}

INT_PTR CALLBACK GotoDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	int idx;
	TCHAR buf[200];
	BOOKMARK bm;

	// keep state between dialog invokations
//	static size_w	nLastOffset	 = 0;
	static BOOL		fKeepVisible = FALSE;
	static BOOL		fHexOffset   = TRUE;
	static int		nJumpIdx;
	static HWND		hwndPin;
	static POINT	ptLastPos = { -1, -1 };
	static BOOL		fFirstTime = TRUE;
	
	static size_w   nGotoHistory[200];
	static int		nGotoHistoryCount;

	HWND hwndHexView = g_hwndHexView;
	BOOL   fEnableBookList = FALSE;

	HBOOKMARK hbm;

	switch (iMsg)
	{
	case WM_INITDIALOG:

		hwndPin = CreatePinToolbar(hwnd, IDC_KEEPVISIBLE, FALSE);
		UpdatePin(hwnd, IDC_KEEPVISIBLE, fKeepVisible);

		//AlignWindow(hwndPin, GetDlgItem(hwnd, IDC_GOTO_OFFSET), ALIGN_LEFT);
		//AlignWindow(hwndPin, GetDlgItem(hwnd, IDCANCEL),  ALIGN_BOTTOM);

		//AlignWindow(hwndPin, GetDlgItem(hwnd, IDC_GOTO_OFFSET), ALIGN_LEFT);
		AlignWindow(hwndPin, GetDlgItem(hwnd, IDC_GOTO_OFFSET),  ALIGN_LEFT);


		// setup the 'bookmark' combo
		for(hbm = 0, i = 0; (hbm = HexView_EnumBookmark(hwndHexView, hbm, &bm)) != 0; i++)
		{
			if((bm.flags & HVBF_NOPERSIST) == 0)
			{
				TCHAR *s = bm.pszTitle ? bm.pszTitle : bm.pszText;
				_stprintf(buf, TEXT("%08I64X   %s"), bm.offset, s);

				idx = (int)SendDlgItemMessage(hwnd, IDC_BOOKMARKS, CB_ADDSTRING, 0, (LPARAM)buf);
				SendDlgItemMessage(hwnd, IDC_BOOKMARKS, CB_SETITEMDATA, idx, (LPARAM)hbm);

				fEnableBookList = TRUE;
			}
		}

		EnableDlgItem(hwnd, IDC_BOOKMARKS, fEnableBookList);

		SendDlgItemMessage(hwnd, IDC_BOOKMARKS, CB_SETCURSEL, 0, 0);
		//SendDlgItemMessage(hwnd, IDC_BOOKMARKS, CB_SETDROPPEDWIDTH, 400, 0);

		CheckRadioButton(hwnd, IDC_GOTO_BEGIN, IDC_GOTO_EOF, nGotoRelative);
		//SendDlgItemMessage(hwnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)TEXT("Current Position"));
		//SendDlgItemMessage(hwnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)TEXT("Start of File"));
		//SendDlgItemMessage(hwnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)TEXT("End of File"));
		//SendDlgItemMessage(hwnd, IDC_COMBO1, CB_SETCURSEL, nJumpIdx, 0);

		CheckDlgButton(hwnd, IDC_GOTO_HEX,			fHexOffset);

		SetDlgItemBaseInt(hwnd, IDC_GOTO_OFFSET, nLastOffset, fHexOffset, 0);

		for(i = 0; i < nGotoHistoryCount; i++)
		{
			wsprintf(buf, _T("%x"), nGotoHistory[i]);
			SendDlgItemMessage(hwnd, IDC_GOTO_OFFSET, CB_ADDSTRING, 0, (LPARAM)buf);
		}
			
		if(fFirstTime)
		{
			CenterWindow(hwnd);
			fFirstTime = FALSE;
		}
		else
		{
			SetWindowXY(hwnd, ptLastPos.x, ptLastPos.y, NULL);
		}

		return TRUE;
			
	case WM_MOVE:
		{
		RECT rect;
		GetWindowRect(hwnd, &rect);
		ptLastPos.x = rect.left;//(short)LOWORD(lParam);
		ptLastPos.y = rect.top;//(short)HIWORD(lParam);
		}
		return TRUE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDC_BOOKMARKS:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				size_w off = GetBookmarkComboOffset(GetDlgItem(hwnd, IDC_BOOKMARKS));
				//idx = (int)SendDlgItemMessage(hwnd, IDC_BOOKMARKS, CB_GETCURSEL, 0, 0);
				//idx = (int)SendDlgItemMessage(hwnd, IDC_BOOKMARKS, CB_GETITEMDATA, idx, 0);

				SetDlgItemBaseInt(hwnd, IDC_GOTO_OFFSET, off, fHexOffset, 0);
			}

			return TRUE;

		case IDC_KEEPVISIBLE:
			fKeepVisible = IsToolbarButtonChecked(hwndPin, IDC_KEEPVISIBLE);
			UpdatePin(hwnd, IDC_KEEPVISIBLE, fKeepVisible);
			return TRUE;

		case IDC_GOTO_HEX:
			nLastOffset	= GetDlgItemBaseInt(hwnd, IDC_GOTO_OFFSET, fHexOffset);
			fHexOffset	= IsDlgButtonChecked(hwnd, IDC_GOTO_HEX);
			SetDlgItemBaseInt(hwnd, IDC_GOTO_OFFSET, nLastOffset, fHexOffset, 0);
			return TRUE;

		case IDOK:

			if(IsDlgButtonChecked(hwnd, IDC_GOTO_BEGIN))			nGotoRelative = IDC_GOTO_BEGIN;
			else if(IsDlgButtonChecked(hwnd, IDC_GOTO_RELATIVE))	nGotoRelative = IDC_GOTO_RELATIVE;
			else if(IsDlgButtonChecked(hwnd, IDC_GOTO_EOF))			nGotoRelative = IDC_GOTO_EOF;
			
			//nJumpIdx		= SendDlgItemMessage(hwnd, IDC_COMBO1, CB_GETCURSEL, 0, 0);
			nLastOffset		= GetDlgItemBaseInt(hwnd, IDC_GOTO_OFFSET, fHexOffset);

			for(i = 0; i < nGotoHistoryCount; i++)
				if(nGotoHistory[i] == nLastOffset)
					break;

			if(i == nGotoHistoryCount && i < 200)
			{
				nGotoHistory[nGotoHistoryCount++] = nLastOffset;
				
				// add to end of combolist
				wsprintf(buf, _T("%x"), nGotoHistory[i]);
				SendDlgItemMessage(hwnd, IDC_GOTO_OFFSET, CB_ADDSTRING, 0, (LPARAM)buf);
			}

			if(!fKeepVisible)
				DestroyWindow(hwnd);

			Goto(hwndHexView, nLastOffset, nGotoRelative);
			return TRUE;

		case IDCANCEL:
			DestroyWindow(hwnd);
			return TRUE;
		}

		return FALSE;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;

	case WM_DESTROY:
		g_hwndGoto = 0;
		return TRUE;

	default:
		break;
	}

	return FALSE;
}


BOOL ShowGotoDialog(HWND hwndOwner)
{
	if(g_hwndGoto == 0)
	{
		g_hwndGoto = CreateDialog(GetModuleHandle(0), 
			MAKEINTRESOURCE(IDD_GOTO), 
			//MAKEINTRESOURCE(IDD_DIALOG2), 
			hwndOwner, GotoDlgProc);
		
		ShowWindow(g_hwndGoto, SW_SHOW);
	}

	return TRUE;
}