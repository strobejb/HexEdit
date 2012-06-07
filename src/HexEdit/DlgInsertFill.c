//
//  DlgInsertFill.c
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

static BOOL  g_fBigEndian = FALSE;
static BYTE  fillData[100];
static int   fillLen = 0;//sizeof(searchData);
static TCHAR fillText[100];
static int   fillType = SEARCHTYPE_HEX;

BOOL UpdateSearchData(HWND hwndSource, int searchType, BYTE *searchData, int *searchLen);
BOOL UpdateSearchDataDlg(HWND hwndDlg, int sourceId, BOOL fBigEndian, BYTE *searchData, int *searchLen);

extern TCHAR *szNumberNames[];
extern TCHAR *szTextNames[];
extern TCHAR *szHexNames[];

typedef struct 
{
	TCHAR *str;
	DWORD  val;
} STRINGVAL;

/*
LPCWSTR FindStringResourceEx(HINSTANCE hInst, UINT uId, UINT langId)
{
	LPCWSTR wstr = NULL;
	UINT    i;

	// Convert the string ID into a bundle number
	WORD    resId = (WORD)(uId / 16 + 1);
	
	HRSRC hResInfo = FindResourceEx(hInst, RT_STRING, MAKEINTRESOURCE(resId), (WORD)langId);
 
	if(hResInfo) 
	{
		HGLOBAL hResData = LoadResource(hInst, hResInfo);
  
		if(hResData)
		{
			wstr = (LPCWSTR)LockResource(hResData);
   
			if(wstr) 
			{
				// okay now walk the string table
				for(i = 0; i < (uId & 0xF); i++) 
				{
					wstr += 1 + (UINT)*wstr;
				}
		
				UnlockResource(wstr);
			}
	
			FreeResource(hResData);
		}
	}

	return wstr;
}

LPCWSTR FindStringResource(HINSTANCE hInst, UINT uId)
{
	return FindStringResourceEx(hInst, uId, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
}
*/

//
// input: one of the SEARCHTYPE_xxx values
//
const TCHAR * SearchTypeStr(int nId)
{
	switch(nId)
	{
	case SEARCHTYPE_BYTE:   return TEXT("8bit  Byte");
	case SEARCHTYPE_WORD:   return TEXT("16bit Word");
	case SEARCHTYPE_DWORD:  return TEXT("32bit Dword");
	case SEARCHTYPE_QWORD:  return TEXT("64bit Qword");
	case SEARCHTYPE_FLOAT:  return TEXT("Float (32bit IEEE)");
	case SEARCHTYPE_DOUBLE: return TEXT("Double (64bit IEEE)");
	case SEARCHTYPE_ASCII:  return TEXT("Ascii String");
	case SEARCHTYPE_UTF8:   return TEXT("Unicode (UTF-8)");
	case SEARCHTYPE_UTF16:  return TEXT("Unicode (UTF-16)");
	case SEARCHTYPE_UTF32:  return TEXT("Unicode (UTF-32)");
	case SEARCHTYPE_HEX:    return TEXT("Hex Bytes");
	default:				return NULL;
	}
}

void AddComboStringVal(HWND hwndCombo, TCHAR *szString, DWORD nValue)
{
	int idx = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)szString);
	SendMessage(hwndCombo, CB_SETITEMDATA, idx, nValue);
}

void AddSearchTypes(HWND hwndCombo, int startType, int endType, int initial)
{
	int i;

	for(i = startType; i <= endType; i++)
	{
		const TCHAR *wstr = SearchTypeStr(i);
		int idx = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)wstr);
		SendMessage(hwndCombo, CB_SETITEMDATA, idx, i);
	}

	SendMessage(hwndCombo, CB_SETCURSEL, initial, 0);
}

INT_PTR CALLBACK InsertDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static size_w len;
	static BOOL   fHex;

	HWND hwndHV = g_hwndHexView;
	HWND hwndCombo;

	switch (iMsg)
	{
	case WM_INITDIALOG:

	//	FindStringResource(GetModuleHandle(0), IDS_HEXTYPES);

		hwndCombo = GetDlgItem(hwnd, IDC_COMBO_DATATYPE);

		AddSearchTypes(hwndCombo, SEARCHTYPE_HEX, SEARCHTYPE_DOUBLE, fillType);

		//AddComboStrings(hwnd, IDC_INSERT_DATATYPE, szHexNames);
		//AddComboStrings(hwnd, IDC_INSERT_DATATYPE, szTextNames);
		//AddComboStrings(hwnd, IDC_INSERT_DATATYPE, szNumberNames);
		
		if(HexView_GetSelSize(hwndHV, 0) != 0)
		{
			HexView_GetSelSize(hwndHV, &len);
		}

		CheckDlgButton(hwnd, IDC_HEX, fHex);

		CheckDlgButton(hwnd, IDC_ENDIAN, g_fBigEndian);

		SetDlgItemBaseInt(hwnd, IDC_INSERT_NUMBYTES, len, fHex ? 16 : 10, FALSE);
		SetDlgItemText(hwnd, IDC_INSERT_FILLWITH, fillText);
		
		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_ENDIAN:
			g_fBigEndian = IsDlgButtonChecked(hwnd, IDC_ENDIAN);
			fillLen = sizeof(fillData);
			UpdateSearchDataDlg(hwnd, IDC_INSERT_FILLWITH, g_fBigEndian, fillData, &fillLen);
			return 0;

		case IDC_HEX:
			len  = GetDlgItemBaseInt(hwnd, IDC_INSERT_NUMBYTES, fHex ? 16 : 10);
			fHex = IsDlgButtonChecked(hwnd, IDC_HEX);
			SetDlgItemBaseInt(hwnd, IDC_INSERT_NUMBYTES, len, fHex ? 16 : 10, FALSE);

			SendDlgItemMessage(hwnd, IDC_INSERT_NUMBYTES, EM_SETSEL, 0, -1);
			SetDlgItemFocus(hwnd, IDC_INSERT_NUMBYTES);

			return 0;

		case IDC_INSERT_FILLWITH:
			fillLen = sizeof(fillData);
			UpdateSearchDataDlg(hwnd, IDC_INSERT_FILLWITH, g_fBigEndian, fillData, &fillLen);
			return 0;


		case IDC_COMBO_DATATYPE:
			fillLen = sizeof(fillData);
			UpdateSearchDataDlg(hwnd, IDC_INSERT_FILLWITH, g_fBigEndian, fillData, &fillLen);
			//UpdateSearchData(GetDlgItem(hwnd, IDC_INSERT_TEXT),
			//	fill
			return 0;

		case IDOK:
			len = GetDlgItemBaseInt(hwnd, IDC_INSERT_NUMBYTES, fHex ? 16 : 10);
			fillType = (int)SendDlgItemMessage(hwnd, IDC_COMBO_DATATYPE, CB_GETCURSEL, 0, 0);

			GetDlgItemText(hwnd, IDC_INSERT_FILLWITH, fillText, 100);
			fillLen = sizeof(fillData);
			UpdateSearchDataDlg(hwnd, IDC_INSERT_FILLWITH, g_fBigEndian, fillData, &fillLen);

			if(len == 0 || fillLen == 0)
			{
				MessageBox(hwnd, TEXT("No data to fill"), TEXT("HexEdit"), MB_ICONWARNING|MB_OK);
				return 0;
			}
			HexView_FillData(hwndHV, fillData, fillLen, len);

			EndDialog(hwnd, TRUE);
			return 0;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return 0;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_INSERT);

	default:
		break;
	}
	return FALSE;

}


BOOL ShowInsertDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_INSERT), hwnd, InsertDlgProc);
}
