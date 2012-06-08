//
//  DlgStrings.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include <ctype.h>
#include <commctrl.h>
#include "resource.h"
#include "..\HexView\HexView.h"
#include "HexUtils.h"
#include "HexEdit.h"

static TCHAR *szStringTypes[] = 
{ 
	TEXT("Ascii"),
	TEXT("Unicode"),  
	TEXT("Ascii AND Unicode"),  
	NULL
};


int FindStringsBuf(BYTE *buf, size_t len, int minlen, size_t *foundstart, size_t *foundend)
{
	size_t i;
	BOOL fFound = FALSE;

	*foundstart = -1;
	*foundend   = -1;

	for(i = 0; i < len; i++)
	{
		if(isascii(buf[i]))
		{
			if(i >= minlen && *foundstart == -1)
			{
				*foundstart = i;			
			}
		}
		else 
		{
			break;
		}
	}

	if(*foundstart != -1)
	{
		*foundend = i;
		return 1;
	}

	return 0;
}

BOOL FindStrings(HWND hwndHV, size_w offset, size_w length, int minLen)
{
	BYTE   buf[0x100];
	ULONG  len;

	size_t startmatch = -1;
	size_t endmatch   = -1;

	//size_t

	while(length > 0)
	{
		len = (ULONG)min(length, sizeof(buf));

		if(HexView_GetData(hwndHV, offset, buf, len))
		{
			size_t i;
			while(i < len)
			{
				size_t pos;
				// did we find something?
				pos = FindStringsBuf(buf + i, len - i, minLen, &startmatch, &endmatch);

				//if(pos
				{
					//GVITEM gvitem;
					// ADD ADD ADD
				}
			}
			offset += len;
			length -= len;
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

INT_PTR CALLBACK StringsDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndHV = g_hwndHexView;
	size_w start;
	size_w length;

	switch (iMsg)
	{
	case WM_INITDIALOG:
		AddComboStringList(GetDlgItem(hwnd, IDC_STRING_TYPES), szStringTypes, 2);	
		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:

			if(HexView_GetSelSize(hwndHV, 0) > 0)
			{
				HexView_GetSelStart(hwndHV, &start);
				HexView_GetSelSize(hwndHV, &length);
			}
			else
			{
				start  = 0;
				HexView_GetFileSize(hwndHV, &length);
			}

			FindStrings(hwndHV, 0,start, length);

			EndDialog(hwnd, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_STRINGS);

	default:
		break;
	}
	return FALSE;

}


BOOL ShowStringsDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_STRINGS), hwnd, StringsDlgProc);
}