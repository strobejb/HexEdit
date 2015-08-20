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

#include "..\GridView\GridView.h"
#include "..\DockLib\DockLib.h"
#include "..\HexEdit\ToolPanel.h"

static TCHAR *szStringTypes[] = 
{ 
	TEXT("Ascii"),
	TEXT("Unicode"),  
	TEXT("Ascii AND Unicode"),  
	NULL
};

HWND CreateStringsGridView(HWND hwndParent, HFONT hFont)
{	
	GVCOLUMN gvcol = { 0 };
	HWND hwndGridView;

//
	//	Create the gridview!
	//
	InitGridView();

	hwndGridView = CreateGridView(hwndParent, 0xdeadbeef, 0, WS_EX_CLIENTEDGE);


	GridView_SetStyle(hwndGridView, -1, GVS_SINGLE|GVS_FULLROWSELECT|GVS_GRIDLINES);
	
	SendMessage(hwndGridView, WM_SETFONT, (WPARAM)hFont, 0);

	//
	//	Setup the columns
	//
	gvcol.xWidth  = 150;//widthArray[COLIDX_TYPE];//300;
	gvcol.pszText = TEXT("Offset");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 0, &gvcol);

	gvcol.xWidth  = 400;//widthArray[COLIDX_TYPE];//300;
	gvcol.pszText = TEXT("String");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 2, &gvcol);

	gvcol.xWidth  = 100;//widthArray[COLIDX_TYPE];//300;
	gvcol.pszText = TEXT("Length");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 1, &gvcol);

	return hwndGridView;
}

//
//	Process WM_NOTIFY and WM_COMMAND messages sent to the ToolPanel by it's childen
//
LRESULT CALLBACK StringViewCommandHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_NOTIFY)
	{
		NMGRIDVIEW *nmgv = (NMGRIDVIEW *)lParam;

		if(nmgv->hdr.code == GVN_SELCHANGED)
		{
			HWND hwndHV = GetActiveHexView(g_hwndMain);
			GVITEM gvi	= { 0 };
			size_w startoff, endoff;

			gvi.iSubItem	= 0;
			gvi.mask		= GVIF_PARAM;
			GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
			startoff		= gvi.param;

			gvi.iSubItem	= 1;
			gvi.mask		= GVIF_PARAM;
			GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
			endoff			= gvi.param;

			HexView_SetCurSel(hwndHV, startoff, endoff);
		}

	}

	return 0;
}

HWND CreateStringsView(HWND hwndParent)
{
	HWND hwndGridView;
	HWND hwndPanel;
	HWND hwndProgress;
	HWND hwndStatus;
	HFONT hFont;

	hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0, TEXT("Verdana"));

	hwndPanel = ToolPanel_Create(hwndParent, StringViewCommandHandler);

	ToolPanel_AddGripper(hwndPanel);

	hwndStatus = CreateWindowEx(0, TEXT("Static"), TEXT("Searching:"), WS_CHILD|WS_VISIBLE,0,0,100,20, hwndPanel, 0, 0, 0);
	SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFont, 0);
	ToolPanel_AddItem(hwndPanel, hwndStatus, 0);

	hwndProgress = CreateWindowEx(0, PROGRESS_CLASS, 0, WS_CHILD|WS_VISIBLE,
			10, 2,
			200, 20,
			hwndPanel, 0, 0, 0);

	SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hwndProgress, PBM_SETPOS, 50, 0);
	ToolPanel_AddItem(hwndPanel, hwndProgress, 0);

	hwndGridView = CreateStringsGridView(hwndPanel, hFont);//, IDC_TYPEVIEW_GRIDVIEW, 0, WS_EX_CLIENTEDGE);
	ToolPanel_AddNewLine(hwndPanel, 4);
	ToolPanel_AddItem(hwndPanel, hwndGridView, 0);
	ToolPanel_AddAnchor(hwndPanel, 0, 0);//32);

	ToolPanel_AutoSize(hwndPanel);
	SetWindowHeight(hwndPanel, 200, NULL);

	ShowWindow(hwndPanel, SW_SHOW);

	SetWindowSize(hwndPanel, 400,400,0);

	return hwndPanel;
}


size_t FindStringsBuf(BYTE *buf, size_t len, size_t minlen, size_w *foundstart, size_w *foundend)
{
	size_t i;
	BOOL fFound = FALSE;

	*foundstart = -1;
	*foundend   = -1;

	for(i = 0; i < len; i++)
	{
		if(isalnum(buf[i]))
		{
			if(*foundstart == -1)
			{
				*foundstart = i;			
			}
		}
		else 
		{
			if(*foundstart != -1 && (i - *foundstart) >= minlen)
			{
				*foundend = i;
				break;
			}
			else
			{
				*foundstart = -1;
				*foundend   = -1;
			}
		}
	}

	return i;
}

typedef void (* ENUMSTRINGSPROC)(size_w start, size_w end, int i);

BOOL FindStrings(HWND hwndHV, HWND hwndGV, size_w offset, size_w length, int minLen )
{
	TCHAR  samp[100];
	BYTE   buf[0x100];
	ULONG  len;

	size_w startmatch = -1;
	size_w endmatch   = -1;

	//size_t

	while(length > 0)
	{
		len = (ULONG)min(length, sizeof(buf));
		//length -= len;

		if(HexView_GetData(hwndHV, offset, buf, len))
		{
			size_t i = 0;
			while(i < len)
			{
				size_t pos = FindStringsBuf(buf + i, len - i, minLen, &startmatch, &endmatch);

				// did we find the beginning?
				if(startmatch != -1)
				{
					size_t l = min(len-i,90);
					if(endmatch != -1)
						l = (size_t)min(90,endmatch-startmatch);
					_stprintf_s(samp, 100, TEXT("%.*hs"), (int)l, buf+i+(startmatch-offset));
				}
				
				// did we find the end of the string?
				if(endmatch != -1)
				{
					GVITEM gvitem = { 0 };
					TCHAR  ach[100];
					HGRIDITEM hItem;

					// make the offsets relative to the starting offset
					startmatch += offset + i;
					endmatch   += offset + i;

					wsprintf(ach, TEXT("%08I64X"), startmatch);

					gvitem.pszText		= ach;
					gvitem.iSubItem		= 0;
					gvitem.state		= 0;
					gvitem.mask			= GVIF_TEXT | GVIF_STATE | GVIF_PARAM;
					gvitem.param		= startmatch;
					hItem = GridView_InsertChild(hwndGV, GVI_ROOT, &gvitem);

					wsprintf(ach, TEXT("%I64X"), endmatch - startmatch);
					gvitem.iSubItem     = 1;
					gvitem.param		= endmatch;
					GridView_SetItem(hwndGV, hItem, &gvitem);

					//wsprintf(ach, TEXT("%s"), buf);
					gvitem.iSubItem     = 2;
					gvitem.pszText		= samp;
					GridView_SetItem(hwndGV, hItem, &gvitem);


					startmatch = -1;
					endmatch   = -1;
				}		

				i += pos;
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
	HWND hwndGridView;
	HWND hwndPanel;
	size_w start;
	size_w length;
	int  minLen;

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

			DockWnd_Show(g_hwndMain, DWID_STRINGS, TRUE);
			hwndPanel = DockWnd_GetContents(g_hwndMain, DWID_STRINGS);
			hwndGridView = GetDlgItem(hwndPanel, 0xdeadbeef);
			
			minLen = GetDlgItemInt(hwnd, IDC_EDIT1, 0, FALSE);

			FindStrings(hwndHV, hwndGridView, start, length, max(3, minLen));

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