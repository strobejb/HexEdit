//
//  DlgCopyAs.c
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
#include "trace.h"


static IMPEXP_OPTIONS g_CopyOptions = 
{
	FORMAT_HEXDUMP, SEARCHTYPE_BYTE
};

//BOOL Export(TCHAR *szFileName, int nDataType, int nDataFormat);
BOOL Export(TCHAR *szFileName, HWND hwndHexView, IMPEXP_OPTIONS *eopt);

static TCHAR *szNumberNames[] = 
{ 
	TEXT("8bit Byte"),  //TEXT("Signed Byte"), 
	TEXT("16bit Word"),  //TEXT("Signed Word"), 
	TEXT("32bit Dword"), //TEXT("Signed Dword"),
	TEXT("64bit Qword"), //TEXT("Signed Dword"),
	TEXT("Float (32bit IEEE)"), 
	TEXT("Double (64bit IEEE)"),
	NULL
};

static TCHAR *szRawHexGrouping[] = 
{ 
	TEXT("No grouping"),
	TEXT("Byte"),  //TEXT("Signed Byte"), 
	TEXT("Word"),  //TEXT("Signed Word"), 
	TEXT("Dword"), //TEXT("Signed Dword"),
	TEXT("Float"), 
	TEXT("Double"),
	NULL
};


static TCHAR *szExportFormats[] = 
{ 
	TEXT("Text"),
	TEXT("Raw Hex Bytes"),
	TEXT("HTML"),
	TEXT("C/C++ Source"),
	TEXT("Assembler"),
	TEXT("Intel Hex-Records"),
	TEXT("Motorola S-Records"),
	TEXT("Base64"),
	TEXT("UUEncode"),
	NULL
};


typedef struct
{
	HANDLE	hPipe;
	HGLOBAL hMem;
} PIPE_PARAMS;

HANDLE CreatePipeThread(TCHAR *szPipeName, LPTHREAD_START_ROUTINE pThreadProc, PIPE_PARAMS *params);


//
//	Pipe-reading thread - listens on the specified pipe
//	and reads data from the client, appending it to a HGLOBAL
//	memory object each time new data arrives. This allows the
//	client to write to a regular file-handle (i.e. HANDLE/FILE*),
//	but the data actually gets written to global-memory
//
DWORD WINAPI PipeReadProc(PIPE_PARAMS *param)
{
	HANDLE	hGlobalMem		= 0;
	DWORD   dwGlobalSize	= 0;

	// wait for the 'client' to connect
	if(!ConnectNamedPipe(param->hPipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED)
	{
		CloseHandle(param->hPipe);
		return 0;
	}

	if((hGlobalMem = GlobalAlloc(GHND, 1)) == 0)
	{
		CloseHandle(param->hPipe);
		return 0;
	}

	for(;;)
	{
		BYTE buf[256];
		DWORD len = sizeof(buf); 
		
		// read some data from the client. 
		if(ReadFile(param->hPipe, buf, len, &len, 0) && len > 0)
		{
			HANDLE	hNew;
			DWORD   dwNewSize = dwGlobalSize + len;
			
			// try to grow the memory buffer
			if((hNew = GlobalReAlloc(hGlobalMem, dwNewSize+1, GHND)) != 0)
			{
				BYTE *ptr;
				hGlobalMem = hNew;
				
				// append the data to the global-memory buffer
				if((ptr = GlobalLock(hNew)) != 0)
				{
					memcpy(ptr + dwGlobalSize, buf, len);
					dwGlobalSize = dwNewSize;
					ptr[dwNewSize] = '\0';
					GlobalUnlock(hNew);
				}
			}
			else
			{
				GlobalFree(hGlobalMem);
				hGlobalMem = 0;
				break;
			}
		}
		else
		{
			// no data? just drop out
			break;
		}
	}

	// close the server side. if the client is still writing 
	// he will fail to do so (that's a good thing)
	CloseHandle(param->hPipe);

	param->hPipe = 0;
	param->hMem = hGlobalMem;
	return 0;
}


BOOL CopyAs(HWND hwndHV, IMPEXP_OPTIONS *eopt)
{
	HANDLE		hThread;
	PIPE_PARAMS param = { 0 };
	BOOL		success = FALSE;

	TCHAR	szPipeName[MAX_PATH];

	// create a thread+pipe that will read+append data into a HGLOBAL memory object
	// the HGLOBAL will be returned into PIPE_PARAMS structure when done
	hThread = CreatePipeThread(szPipeName, PipeReadProc, &param);

	// export to the pipe! We need +1 to skip past the 'export as binary' option 
	Export(szPipeName, hwndHV, eopt);

	// wait for the pipe-thread to finish reading our data
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	// did it work? 
	if(param.hMem != NULL)
	{
		if(OpenClipboard(hwndHV))
		{
			EmptyClipboard();
			SetClipboardData(CF_TEXT, param.hMem);
			CloseClipboard();
			success = TRUE;
		}
	}

	return success;
}

INT_PTR CALLBACK CopyAsDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndHV;

	switch (iMsg)
	{
	case WM_INITDIALOG:

		// add the export formats to the droplist
		AddComboStringList(GetDlgItem(hwnd, IDC_EXPORTFORMAT), szExportFormats, g_CopyOptions.format - 1);

		// add the datatypes
		AddSearchTypes(GetDlgItem(hwnd, IDC_DATATYPE), SEARCHTYPE_BYTE, SEARCHTYPE_DOUBLE, g_CopyOptions.basetype - SEARCHTYPE_BYTE);

		CheckDlgButton(hwnd, IDC_ENDIAN, g_CopyOptions.fBigEndian);

		EnableDlgItem(hwnd, IDC_DATATYPE, g_CopyOptions.format == FORMAT_CPP || g_CopyOptions.format == FORMAT_ASM);
		EnableDlgItem(hwnd, IDC_ENDIAN, g_CopyOptions.format == FORMAT_CPP || g_CopyOptions.format == FORMAT_ASM);

		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			g_CopyOptions.format     = SendDlgItemMessage(hwnd, IDC_EXPORTFORMAT, CB_GETCURSEL, 0, 0) + 1;
			g_CopyOptions.basetype   = ComboBox_GetSelData(GetDlgItem(hwnd, IDC_DATATYPE));
			g_CopyOptions.fBigEndian = IsDlgButtonChecked(hwnd, IDC_ENDIAN);

			hwndHV = GetActiveHexView(GetParent(hwnd));
			CopyAs(hwndHV, &g_CopyOptions);

			EndDialog(hwnd, TRUE);
			return 0;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		case IDC_EXPORTFORMAT:

			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				int idx = (int)SendDlgItemMessage(hwnd, IDC_EXPORTFORMAT, CB_GETCURSEL, 0, 0) + 1;
				BOOL fEnable = FALSE;

				if(idx == FORMAT_CPP || idx == FORMAT_ASM)
					fEnable = TRUE;

				EnableDlgItem(hwnd, IDC_DATATYPE, fEnable);
				EnableDlgItem(hwnd, IDC_ENDIAN, fEnable);
			}

			return TRUE;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_COPYAS);

	default:
		break;
	}
	return FALSE;

}

BOOL CopyAsDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_COPYAS), hwnd, CopyAsDlgProc);
}
