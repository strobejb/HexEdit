//
//  DlgProperties.c
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

void MakeStaticSplitter(HWND hwnd);

//extern TCHAR		g_szFileName[MAX_PATH];
//extern TCHAR		g_szFileTitle[MAX_PATH];

//
//	Set the date-time pickers from the specified file
//
void UpdateFileTime(HWND hwnd, HANDLE hFile)
{
	SYSTEMTIME st;
	FILETIME ft, ftc, fta, ftm;

	GetFileTime(hFile, &ftc, &fta, &ftm);
	
	FileTimeToLocalFileTime(&ftc, &ft);
	FileTimeToSystemTime(&ft, &st);			//created
	SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER1, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
	SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER2, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
		
	FileTimeToLocalFileTime(&ftm, &ft);
	FileTimeToSystemTime(&ft, &st);			//modified
	SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER3, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
	SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER4, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);

	FileTimeToLocalFileTime(&fta, &ft);
	FileTimeToSystemTime(&ft, &st);			//accessed
	SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER5, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
	SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER6, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
}

BOOL ApplyFileTime(HWND hwndDlg, HWND hwndHV)
{
	SYSTEMTIME st1, st2;
	FILETIME ft, ftc, fta, ftm;
	HANDLE hFile;

	// need to combine the date and time portions to give the
	// appropriate time
	
	// creation time
	SendDlgItemMessage(hwndDlg, IDC_DATETIMEPICKER1, DTM_GETSYSTEMTIME, 0, (LPARAM)&st1);
	SendDlgItemMessage(hwndDlg, IDC_DATETIMEPICKER2, DTM_GETSYSTEMTIME, 0, (LPARAM)&st2);
	st1.wHour			= st2.wHour;
	st1.wMilliseconds	= st2.wMilliseconds;
	st1.wMinute			= st2.wMinute;
	st1.wSecond			= st2.wSecond;
	st1.wDayOfWeek		= 0;
	SystemTimeToFileTime(&st1, &ft);		//created
	LocalFileTimeToFileTime(&ft, &ftc);

	// modify time
	SendDlgItemMessage(hwndDlg, IDC_DATETIMEPICKER3, DTM_GETSYSTEMTIME, 0, (LPARAM)&st1);
	SendDlgItemMessage(hwndDlg, IDC_DATETIMEPICKER4, DTM_GETSYSTEMTIME, 0, (LPARAM)&st2);
	st1.wHour			= st2.wHour;
	st1.wMilliseconds	= st2.wMilliseconds;
	st1.wMinute			= st2.wMinute;
	st1.wSecond			= st2.wSecond;
	st1.wDayOfWeek		= 0;
	SystemTimeToFileTime(&st1, &ft);		//modified
	LocalFileTimeToFileTime(&ft, &ftm);

	// access time
	SendDlgItemMessage(hwndDlg, IDC_DATETIMEPICKER5, DTM_GETSYSTEMTIME, 0, (LPARAM)&st1);
	SendDlgItemMessage(hwndDlg, IDC_DATETIMEPICKER6, DTM_GETSYSTEMTIME, 0, (LPARAM)&st2);
	st1.wHour			= st2.wHour;
	st1.wMilliseconds	= st2.wMilliseconds;
	st1.wMinute			= st2.wMinute;
	st1.wSecond			= st2.wSecond;
	st1.wDayOfWeek		= 0;
	SystemTimeToFileTime(&st1, &ft);		//accessed
	LocalFileTimeToFileTime(&ft, &fta);

	//Finally set the file time
	hFile = HexView_GetFileHandle(hwndHV);

	return SetFileTime(hFile, &ftc, &fta, &ftm);
}

BOOL ShowProperties(HWND hwndDlg, HWND hwndHV)
{
	UINT64 nFileSize;
	TCHAR  buf[60];
	DWORD  dwAttr;
	HANDLE hFile;

	TCHAR  szFilePath[MAX_PATH], *name;
	SHFILEINFO shfi = { 0 };

	HexView_GetFileSize(hwndHV, &nFileSize);

	HexView_GetFileName(hwndHV, szFilePath, MAX_PATH);
	name = _tcsrchr(szFilePath, '\\') + 1;

	SHGetFileInfo(szFilePath, 0, &shfi, sizeof(shfi), SHGFI_ICON);

	SendDlgItemMessage(hwndDlg, IDC_FILEICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)shfi.hIcon);
	//
	//	File name/path
	//
	SetDlgItemText(hwndDlg, IDC_FILENAME, name);
	SetDlgItemText(hwndDlg, IDC_FILEPATH, szFilePath);
	
	//
	//	File size
	//
	if(nFileSize < 1024)
		wsprintf(buf, TEXT("%I64u bytes (%I64u bytes)"), nFileSize, nFileSize);
	else if(nFileSize >= 1024 && nFileSize < 1024*1024)
		wsprintf(buf, TEXT("%I64u KB (%I64u bytes)"), (nFileSize + 1024) / 1024, nFileSize);
	else
		wsprintf(buf, TEXT("%I64u MB (%I64u bytes)"), (nFileSize + 1024*1024) / (1024*1024), nFileSize);

	SetDlgItemText(hwndDlg, IDC_FILESIZE, buf);

	//
	//	File attributes
	//
	dwAttr = GetFileAttributes(szFilePath);

	CheckDlgButton(hwndDlg, IDC_ATTR_A, dwAttr & FILE_ATTRIBUTE_ARCHIVE);
	CheckDlgButton(hwndDlg, IDC_ATTR_R, dwAttr & FILE_ATTRIBUTE_READONLY);
	CheckDlgButton(hwndDlg, IDC_ATTR_S, dwAttr & FILE_ATTRIBUTE_SYSTEM);
	CheckDlgButton(hwndDlg, IDC_ATTR_H, dwAttr & FILE_ATTRIBUTE_HIDDEN);

	//
	//	File timestamps
	//
	hFile = HexView_GetFileHandle(hwndHV);
	
	if(hFile && hFile != INVALID_HANDLE_VALUE)
	{		
		UpdateFileTime(hwndDlg, hFile);		
		//CloseHandle(hFile);
	}

	return TRUE;
}

BOOL ApplyFileAttr(HWND hwndDlg, HWND hwndHV)
{
	TCHAR szFilePath[MAX_PATH];
	DWORD dwAttr;
	DWORD dwAttrNew = 0;
	
	HexView_GetFileName(hwndHV, szFilePath, MAX_PATH);

	// get the file's current attributes
	dwAttr = GetFileAttributes(szFilePath);

	// clear the R/A/S/H bits
	dwAttr &= ~ (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | 
				 FILE_ATTRIBUTE_SYSTEM |  FILE_ATTRIBUTE_ARCHIVE );
	
	// now reset them based on what the tick-boxes are set to
	if(IsDlgButtonChecked(hwndDlg, IDC_ATTR_R))
		dwAttr |= FILE_ATTRIBUTE_READONLY;
	
	if(IsDlgButtonChecked(hwndDlg, IDC_ATTR_H))
		dwAttr |= FILE_ATTRIBUTE_HIDDEN;
	
	if(IsDlgButtonChecked(hwndDlg, IDC_ATTR_A))
		dwAttr |= FILE_ATTRIBUTE_ARCHIVE;

	if(IsDlgButtonChecked(hwndDlg, IDC_ATTR_S))
		dwAttr |= FILE_ATTRIBUTE_SYSTEM;
	
	return SetFileAttributes(szFilePath, dwAttr);
}

INT_PTR CALLBACK FilePropertiesDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	FILETIME   ft;
	FILETIME   lt;
	SYSTEMTIME st;
	NMHDR	 * nmhdr;
	
	HWND hwndHexView;
	HWND hwndMain;

	static BOOL fTimeChanged = FALSE;
	static BOOL fAttrChanged = FALSE;

	
	switch(iMsg)
	{
	case WM_INITDIALOG:

		hwndMain = GetParent(GetParent(hwnd));
		hwndHexView = GetActiveHexView(hwndMain);

		CenterRelative(GetParent(hwnd), hwndMain, NULL);

		MakeStaticSplitter(GetDlgItem(hwnd, IDC_LINE1));
		MakeStaticSplitter(GetDlgItem(hwnd, IDC_LINE2));

		ShowProperties(hwnd, hwndHexView);

		fTimeChanged = FALSE;
		fAttrChanged = FALSE;
		return TRUE;

	case WM_COMMAND:
		
		switch(LOWORD(wParam))
		{
		// if one of the file-attributes is toggled then enable the APPLY button
		case IDC_ATTR_R: case IDC_ATTR_H: case IDC_ATTR_A: case IDC_ATTR_S:
			PostMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
			fAttrChanged = TRUE;
			return 0;

		// 'Touch' button - update the date-time pickers but
		// don't modify the file's timestamps
		case IDC_PROPERTY_TOUCH:

			GetSystemTimeAsFileTime(&ft);
			FileTimeToLocalFileTime(&ft, &lt);
			FileTimeToSystemTime(&lt, &st);
			
			SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER3, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
			SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER4, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
			SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER5, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
			SendDlgItemMessage(hwnd, IDC_DATETIMEPICKER6, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
				
			PostMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
			fTimeChanged = TRUE;
			return FALSE;
			
		}

		return FALSE;

	case WM_NOTIFY:
		nmhdr = (NMHDR *)lParam;
		if(nmhdr->code == DTN_DATETIMECHANGE)
		{
			fTimeChanged = TRUE;
			PostMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
		}
		else if(nmhdr->code == PSN_APPLY)
		{
			hwndMain = GetParent(GetParent(hwnd));
			hwndHexView = GetActiveHexView(hwndMain);

			// Apply button - if there are any changes then ask the user!
			if(fTimeChanged || fAttrChanged)
			{
				TCHAR szFilePath[MAX_PATH];
				HexView_GetFileName(hwndHexView, szFilePath, MAX_PATH);
				//if(IDYES != HexErrorBoxIdExt(MB_ICONEXCLAMATION | MB_YESNO, IDS_MSG_QUERYMODTS) ))
				//	return TRUE;
				//if(MessageBox(hwnd, L"Change file?", L"HexEdit", MB_YESNO|MB_ICONEXCLAMATION) != IDYES)
				//	return TRUE;

				if(fTimeChanged)
				{
					if(ApplyFileTime(hwnd, hwndHexView))
					{
						fTimeChanged = FALSE;
						UpdateFileTime(hwnd, hwndHexView);
					}
					else
					{
						HexWinErrorBox(GetLastError(), TEXT("%s\r\n\r\nYou do not have permission to modify this file,\r\n")
													   TEXT("or the file may already be in use by another application."),
													   szFilePath);
						return TRUE;
					}
				}

				if(fAttrChanged)
				{
					fAttrChanged = FALSE;
					if(!ApplyFileAttr(hwnd, hwndHexView))
					{
						HexWinErrorBox(GetLastError(), TEXT("%s\r\n\r\nYou do not have permission to modify this file,\r\n")
													   TEXT("or the file may already be in use by another application."),
													   szFilePath);
						return TRUE;
					}
				}
			}

		}


		return TRUE;

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_PROPERTIES);

	}

	return FALSE;
}


BOOL FileProperties(HWND hwndMain)
{
	PROPSHEETHEADER psh = { sizeof(psh) };
	PROPSHEETPAGE psp[1] = { { sizeof(psp[0]) } };

	//ZeroMemory(&pspage, nNumPropSheetPages * sizeof(PROPSHEETPAGE));

	psp[0].dwSize			= sizeof(PROPSHEETPAGE);
	psp[0].dwFlags			= PSP_USETITLE;
	psp[0].hInstance		= GetModuleHandle(0);
	psp[0].pszTemplate		= MAKEINTRESOURCE(IDD_PROPERTIES);
	psp[0].pszIcon			= 0;
	psp[0].pfnDlgProc		= FilePropertiesDlgProc;
	psp[0].lParam			= 0;
	psp[0].pfnCallback		= 0;
	psp[0].pszTitle			= TEXT("General");

	psh.dwFlags			= PSH_PROPSHEETPAGE;	//set so can use array of propsheetpages
	psh.hwndParent		= hwndMain;
	psh.hInstance		= GetModuleHandle(0);
	psh.pszCaption		= TEXT("Properties");
	psh.nPages			= 1;
	psh.ppsp			= psp;
	psh.pfnCallback		= 0;

	return (BOOL)PropertySheet(&psh);
}