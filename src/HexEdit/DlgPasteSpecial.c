//
//  DlgPasteSpecial.c
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
#include "trace.h"

IMPEXP_OPTIONS g_PasteOptions = { FORMAT_HEXDUMP, SEARCHTYPE_BYTE };

BOOL Import(TCHAR *szFileName, HWND hwndHexView, IMPEXP_OPTIONS *ieopt);


void MakeStaticSplitter(HWND hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	SetWindowPos(hwnd, 0, 0, 0, rect.right-rect.left, 2, SWP_NOMOVE|SWP_NOZORDER);
}

TCHAR *szTransformList[] = 
{
	TEXT("Don't interpret (paste as raw data)"),
	TEXT("Full Hex Dump"),
	TEXT("Single Hex Bytes"),
	TEXT("HTML"),
	TEXT("C/C++ Source"),
	TEXT("Assembler"),
	TEXT("Intel Hex-Records"),
	TEXT("Motorola S-Records"),
	TEXT("Base64"),
	TEXT("UUEncode"),
	NULL
};

/*
UINT HexDumpToBinary(char * szPlainHex, int nPlainLen, BYTE * pRawData, int nRawLen)
{
	char *ptr = szPlainHex;
	
	size_w i, j;
	BYTE val = 0;
	int count = 0;

	for(i = 0, j = 0; i < nPlainLen && ptr[i]; i++)
	{
		if(isxdigit(ptr[i]))
		{
			if(count) val <<= 4;
			else      val = 0;
			val |= hex2dec(ptr[i]);

			pRawData[j] = val;

			//TRACEA("%x ", val);
						
			count = !count;
			if(count == 0)
				j++;
		}
		else
		{
			if(count)
				j++;
			count = 0;
		}
	}

	return j;
}*/

const TCHAR * FormatName(UINT uFormat)
{
	static TCHAR szName[128];

	switch(uFormat)
	{
	case CF_BITMAP:				return TEXT("CF_BITMAP"); 
	case CF_DIB:				return TEXT("CF_DIB"); 
	case CF_DIBV5:				return TEXT("CF_DIBV5"); 
	case CF_DIF:				return TEXT("CF_DIF"); 
	case CF_DSPBITMAP:			return TEXT("CF_DSPBITMAP");
	case CF_DSPENHMETAFILE:		return TEXT("CF_DSPENHMETAFILE"); 
	case CF_DSPMETAFILEPICT:	return TEXT("CF_DSPMETAFILEPICT"); 
	case CF_DSPTEXT:			return TEXT("CF_DSPTEXT"); 
	case CF_ENHMETAFILE:		return TEXT("CF_ENHMETAFILE"); 
	case CF_GDIOBJFIRST:		return TEXT("CF_GDIOBJFIRST"); 
	case CF_HDROP:				return TEXT("CF_HDROP"); 
	case CF_LOCALE:				return TEXT("CF_LOCALE"); 
	case CF_METAFILEPICT:		return TEXT("CF_METAFILEPICT"); 
	case CF_OEMTEXT:			return TEXT("CF_OEMTEXT"); 
	case CF_OWNERDISPLAY:		return TEXT("CF_OWNERDISPLAY"); 
	case CF_PALETTE:			return TEXT("CF_PALETTE"); 
	case CF_PENDATA:			return TEXT("CF_PENDATA"); 
	case CF_PRIVATEFIRST:		return TEXT("CF_PRIVATEFIRST"); 
	case CF_RIFF:				return TEXT("CF_RIFF"); 
	case CF_SYLK:				return TEXT("CF_SYLK"); 
	case CF_TEXT:				return TEXT("CF_TEXT"); 
	case CF_WAVE:				return TEXT("CF_WAVE"); 
	case CF_TIFF:				return TEXT("CF_TIFF"); 
	case CF_UNICODETEXT:		return TEXT("CF_UNICODETEXT"); 
	default:					
		
		if(GetClipboardFormatName(uFormat, szName, sizeof(szName) / sizeof(szName[0])))
		{
			return szName;
		}
		else
		{
			_stprintf(szName, TEXT("unknown: <0x%x>"), uFormat);
			return szName;
		}
	}
}

//
//	Enumerate all formats currently stored in the clipboard, and
//	store them in the IDC_CLIPLIST listbox
//
//	If available, the format identified by uSelectFormat will be selected
//
BOOL InitClipboardFormatList(HWND hwndList, UINT uSelectFormat)
{
//	HWND hwndList;
	UINT uFormat = 0;
	int  idx;

	// Add items to combo box
	//hwndList = GetDlgItem(hwndDlg, IDC_CLIPLIST);
	
	if(!OpenClipboard(hwndList)) 
		return FALSE;
	
	// Enumerate all formats on the clipboard
	while(uFormat = EnumClipboardFormats(uFormat))
	{
		idx = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)FormatName(uFormat));
		SendMessage(hwndList, LB_SETITEMDATA, idx, uFormat);

		if(uFormat == uSelectFormat)
			SendMessage(hwndList, LB_SETCURSEL, idx, 0);
	}
	
	CloseClipboard();
	return TRUE;
}

//
//
//
BOOL InitTransformList(HWND hwndDlg, UINT uCtrlId, UINT nLastTransform)
{
	int i;

	for(i = 0; szTransformList[i]; i++)
	{
		int idx = (int)SendDlgItemMessage(hwndDlg, uCtrlId, CB_ADDSTRING, 0, (LPARAM)szTransformList[i]);
		SendDlgItemMessage(hwndDlg, uCtrlId, CB_SETITEMDATA, idx, i);

	}

	if(IsClipboardFormatAvailable(CF_TEXT) == FALSE)
	{
		EnableDlgItem(hwndDlg, uCtrlId, FALSE);
		nLastTransform = 0;
	}
		
	SendDlgItemMessage(hwndDlg, uCtrlId, CB_SETCURSEL, nLastTransform, 0);
	return TRUE;
}

typedef struct
{
	HANDLE	hPipe;
	HGLOBAL hMem;
} PIPE_PARAMS;

HANDLE CreatePipeThread(TCHAR *szPipeName, LPTHREAD_START_ROUTINE pThreadProc, PIPE_PARAMS *params)
{
	GUID	guid;
	OLECHAR	szGuid[100];

	// create a unique pipe name
	CoCreateGuid(&guid);
	StringFromGUID2(&guid, szGuid, 100);
	//wsprintf(szGuid, TEXT("catch22.hexedit"));
	wsprintf(szPipeName, TEXT("\\\\.\\pipe\\%ls"), szGuid);

	// create a pipe that the client (us) can write data to using fprintf() etc
	params->hPipe = CreateNamedPipe(
					szPipeName, 
					PIPE_ACCESS_DUPLEX|FILE_FLAG_WRITE_THROUGH,
					PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES, 0, 0, 0, 0
					);

	// spawn a thread to read data from the pipe and convert it
	// to a HGLOBAL memory object
	return CreateThread(0, 0, pThreadProc, params, 0, 0);
}

//
//	Pipe-writing thread - writes the specified global-memory buffer
//	to the pipe, the client the other side will read the data from it
//
DWORD WINAPI PipeWriteProc(PIPE_PARAMS *param)
{
	BYTE	*pGlobalMem;
	DWORD   dwGlobalSize	= 0;
	DWORD	len;

	// wait for someone to connect
	if(!ConnectNamedPipe(param->hPipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED)
	{
		CloseHandle(param->hPipe);
		return 0;
	}

	// lock down the memory
	if((pGlobalMem = GlobalLock(param->hMem)) == 0)
	{
		HexWinErrorBox(GetLastError(), 0);
		CloseHandle(param->hPipe);
		return 0;
	}

	dwGlobalSize = (DWORD)GlobalSize(pGlobalMem);

	len = (DWORD)strlen((char *)pGlobalMem);
	dwGlobalSize = len + 1;

	// write the entire memory block to the pipe
	if(!WriteFile(param->hPipe, pGlobalMem, dwGlobalSize, &len, 0))
	{
		HexWinErrorBox(GetLastError(), 0);
		return 0;
	}


	GlobalUnlock(param->hMem);

	// close the server side. if the client is still writing 
	// he will fail to do so (that's a good thing)
	CloseHandle(param->hPipe);
	param->hPipe = 0;
	//MessageBox(GetForegroundWindow(), L"exiting thread", 0, 0);
	return 0;
}

size_w PasteTransform(HWND hwndDlg, HGLOBAL hGlobalMem, IMPEXP_OPTIONS *ieopt)
{
	HANDLE		hThread;
	PIPE_PARAMS param = { 0, hGlobalMem };
	size_w      count = 0;

	TCHAR	szPipeName[MAX_PATH];
	
	// create a pipe that will supply a client with a stream of data
	hThread = CreatePipeThread(szPipeName, PipeWriteProc, &param);

	// import from the pipe! We need +1 to get into the 1....n range,
	if((count = Import(szPipeName, g_hwndHexView, ieopt)) == 0)
	{
		if(GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			HexWinErrorBox(GetLastError(), 0);
		}
		else
		{
			HexErrorBox(TEXT("%s"), TEXT("Clipboard data could not be imported\n"));
		}
	}

	// wait for the pipe-thread to finish reading our data
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	// did it work?
	return count;
}

BOOL PasteSpecial(HWND hwndHV, BOOL fMaskSel, UINT uFormat, IMPEXP_OPTIONS *ieopt)
{
	HANDLE	hData;
	PVOID	pData;
	size_w	nLength;

	//hwndList = GetDlgItem(hwndDlg, IDC_CLIPLIST);
	
	if(!OpenClipboard(hwndHV))
		return FALSE;

	// check the format is still on the clipboard
	if(!IsClipboardFormatAvailable(uFormat))
	{
		// HexErrorBoxId(IDS_MSG_CLIPBOARDFAIL);
		return FALSE;
	}

	// get the data-handle
	if((hData = GetClipboardData(uFormat)) == 0)
	{
		CloseClipboard();
		// HexErrorBoxId(IDS_MSG_CLIPBOARDFAIL);
		return FALSE;
	}

	// can only do a 'masked paste' when there is data selected
	// and we are in over-strike mode
	if(fMaskSel)
	{
		HexView_GetSelSize(hwndHV, &nLength);
		nLength  = min(nLength, GlobalSize(hData));
	}
	else
	{
		nLength = GlobalSize(hData);
	}
			
	if((pData = (BYTE *)GlobalLock(hData)) != 0)
	{
		// convert the data from a hex-dump to raw binary!!
		if(ieopt->format > FORMAT_RAWDATA)
		{
			PasteTransform(hwndHV, hData, ieopt);
		}
		else
		{
			// FORMAT_RAWDATA - do a regular data import (don't transform!)
			HexView_SetDataCur(hwndHV, pData, nLength);
		}

		InvalidateRect(hwndHV, 0, 0);
	}
	else
	{
		// show error
	}

	GlobalUnlock(hData);
	CloseClipboard();
	return TRUE;
}

INT_PTR CALLBACK PasteDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL fMask = FALSE;
	static int  nLastClipFormat	= 0;
	static int  nLastTransform	= 0;
	static BOOL fLastMask		= FALSE;

	HWND hwndHV = g_hwndHexView;

	switch (iMsg)
	{
	case WM_INITDIALOG:
		
		if(HexView_GetSelSize(hwndHV, 0))
			CheckDlgButton(hwnd, IDC_MASKSEL, fMask ? BST_CHECKED : BST_UNCHECKED);
		else
			EnableDlgItem(hwnd, IDC_MASKSEL, FALSE);

		InitClipboardFormatList(GetDlgItem(hwnd, IDC_CLIPLIST), nLastClipFormat);
		InitTransformList(hwnd, IDC_IMPORTFORMAT, nLastTransform);
		CenterWindow(hwnd);

		CheckDlgButton(hwnd, IDC_ENDIAN, g_PasteOptions.fBigEndian);

		if(g_PasteOptions.format == FORMAT_CPP || g_PasteOptions.format == FORMAT_ASM)
			EnableDlgItem(hwnd, IDC_ENDIAN, TRUE);
		else
			EnableDlgItem(hwnd, IDC_ENDIAN, FALSE);

		CheckDlgButton(hwnd, IDC_USEADDRESS, g_PasteOptions.fUseAddress);

		if(g_PasteOptions.format == FORMAT_HEXDUMP || g_PasteOptions.format  == FORMAT_INTELHEX || g_PasteOptions.format == FORMAT_SRECORD)
			EnableDlgItem(hwnd, IDC_USEADDRESS, TRUE);
		else
			EnableDlgItem(hwnd, IDC_USEADDRESS, FALSE);

		MakeStaticSplitter(GetDlgItem(hwnd, IDC_SPLIT1));
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_IMPORTFORMAT:

			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				int idx = (int)SendDlgItemMessage(hwnd, IDC_IMPORTFORMAT, CB_GETCURSEL, 0, 0);
				
				// force the selection of CF_TEXT
				if(idx != FORMAT_RAWDATA)
					SendDlgItemMessage(hwnd, IDC_CLIPLIST, LB_SELECTSTRING, 0, (LPARAM)TEXT("CF_TEXT"));

				if(idx == FORMAT_HEXDUMP || idx == FORMAT_INTELHEX || idx == FORMAT_SRECORD)
					EnableDlgItem(hwnd, IDC_USEADDRESS, TRUE);
				else
					EnableDlgItem(hwnd, IDC_USEADDRESS, FALSE);

				if(idx == FORMAT_CPP || idx == FORMAT_ASM)
					EnableDlgItem(hwnd, IDC_ENDIAN, TRUE);
				else
					EnableDlgItem(hwnd, IDC_ENDIAN, FALSE);
			}

			return TRUE;

		case IDC_CLIPLIST:

			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				UINT uFormat = ListBox_GetDlgSelData(hwnd, IDC_CLIPLIST);
				
				if(uFormat != CF_TEXT)
					SendDlgItemMessage(hwnd, IDC_IMPORTFORMAT, CB_SETCURSEL, 0, 0);

				//EnableDlgItem(hwnd, IDC_HEXDUMP, uFormat == CF_TEXT ? TRUE : FALSE);//BST_CHECKED : BST_UNCHECKED);
				return 0;
			}

			if(HIWORD(wParam) == LBN_DBLCLK)
			{
				fMask = IsDlgButtonChecked(hwnd, IDC_MASKSEL);

				nLastClipFormat	= ListBox_GetDlgSelData(hwnd, IDC_CLIPLIST);
				nLastTransform	= (int)SendDlgItemMessage(hwnd, IDC_IMPORTFORMAT,	  CB_GETCURSEL, 0, 0);
				fLastMask		= IsDlgButtonChecked(hwnd, IDC_MASKSEL);

				g_PasteOptions.format		= ComboBox_GetDlgSelData(hwnd, IDC_IMPORTFORMAT);
				g_PasteOptions.fBigEndian	= IsDlgButtonChecked(hwnd, IDC_ENDIAN);
				g_PasteOptions.fUseAddress	= IsDlgButtonChecked(hwnd, IDC_USEADDRESS);

				PasteSpecial(hwndHV, fLastMask, nLastClipFormat, &g_PasteOptions);

				EndDialog(hwnd, TRUE);
			}

			return 0;

		case IDOK:
			fMask			= IsDlgButtonChecked(hwnd, IDC_MASKSEL);

			nLastClipFormat	= ListBox_GetDlgSelData(hwnd, IDC_CLIPLIST);
			nLastTransform	= (int)SendDlgItemMessage(hwnd, IDC_IMPORTFORMAT, CB_GETCURSEL, 0, 0);
			fLastMask		= IsDlgButtonChecked(hwnd, IDC_MASKSEL);

			g_PasteOptions.format		= ComboBox_GetDlgSelData(hwnd, IDC_IMPORTFORMAT);
			g_PasteOptions.fBigEndian	= IsDlgButtonChecked(hwnd, IDC_ENDIAN);
			g_PasteOptions.fUseAddress	= IsDlgButtonChecked(hwnd, IDC_USEADDRESS);

			PasteSpecial(hwndHV, fLastMask, nLastClipFormat, &g_PasteOptions);

			EndDialog(hwnd, TRUE);
			return 0;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return 0;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_PASTESPECIAL);

	default:
		break;
	}
	return FALSE;

}


void AddPasteTabs(HWND hwnd)
{
	TCITEM tcitem;

	HWND hwndTab = GetDlgItem(hwnd, IDC_TAB1);

	tcitem.mask		= TCIF_TEXT;
	tcitem.pszText	= TEXT("Raw Formats");
	TabCtrl_InsertItem(hwndTab, 0, &tcitem);


	tcitem.mask		= TCIF_TEXT;
	tcitem.pszText	= TEXT("Text Formats");
	TabCtrl_InsertItem(hwndTab, 1, &tcitem);
}

HWND g_hwndPaste;

INT_PTR CALLBACK PasteModalDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		AddPasteTabs(hwnd);
		InitClipboardFormatList(GetDlgItem(hwnd, IDC_LIST1), 0);
		return TRUE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			DestroyWindow(hwnd);
			return TRUE;

		case IDOK:
			return TRUE;
		}

		return TRUE;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;

	case WM_DESTROY:
		g_hwndPaste = 0;
		return TRUE;
	}
	return FALSE;
}


int HexPasteSpecialDlg2(HWND hwnd)
{
	if(g_hwndPaste == 0)
	{
		g_hwndPaste = CreateDialog(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOG3), hwnd, PasteModalDlg);
	}

	ShowWindow(g_hwndPaste, SW_SHOW);
	return 0;
}

int HexPasteSpecialDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_PASTESPECIAL), hwnd, PasteDlgProc);
}
