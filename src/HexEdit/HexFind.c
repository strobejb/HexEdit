//
//  HexFind.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <commctrl.h>
#include "resource.h"
#include "trace.h"
#include "..\HexView\HexView.h"
#include "HexEdit.h"
#include "HexUtils.h"

#include "dlgfont.h"

#include "endian.h"

BYTE  searchData[100];
DWORD searchLen = 0;//sizeof(searchData);
BOOL  searchValid = FALSE;

BYTE  replaceData[100];
DWORD replaceLen = 0;//sizeof(searchData);
BOOL  replaceValid = FALSE;

BOOL ShowFindDialog(HWND hwndMain, int idx);
BOOL UpdateProgress(MAINWND *mainWnd, BOOL fVisible, size_w pos, size_w len);

/*struct SEARCH_PARAMS
{
	BOOL fNumeric;
};


TCHAR *szNumberNames[] = 
{ 
	TEXT("8bit  Byte"),  //TEXT("Signed Byte"), 
	TEXT("16bit Word"),  //TEXT("Signed Word"), 
	TEXT("32bit Dword"), //TEXT("Signed Dword"),
	TEXT("64bit Qword"), //TEXT("Signed Dword"),
	TEXT("Float (32bit IEEE)"), 
	TEXT("Double (64bit IEEE)"),
	NULL
};

TCHAR *szTextNames[] = 
{ 
	TEXT("Ascii String"), 
	TEXT("Unicode (UTF-8)"),
	TEXT("Unicode (UTF-16)"),
	TEXT("Unicode (UTF-32)"),
	NULL
};


TCHAR *szHexNames[] = 
{
	TEXT("Hex Bytes"),
	//TEXT("Ascii String"),
	//TEXT("Unicode String"),
	NULL
};*/

HWND g_hwndSearch;

HDWP RightJustifyWindow(HWND hwndCtrl, HWND hwndParent, int margin, HDWP hdwp);
BOOL EnableDialogTheme(HWND hwnd);


#define MAX_FIND_PANES 4
#define IDC_PIN_TOOLBAR IDC_KEEPVISIBLE
#define TOOLBAR_PIN_STYLES  (TBSTYLE_FLAT |	WS_CHILD | WS_VISIBLE | \
						CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER)

#define MAX_SEARCH_TEXT 256		// max length of text in find-combos
#define HISTORY_LEN MAX_SEARCH_TEXT
#define HISTORY_MAX 100

//#define MAX_SEARCH_DATALEN 256	// max bytes can search for
//BYTE g_SearchData[MAX_SEARCH_DATALEN];

typedef struct
{
	int		nDataTypeIdx;
	TCHAR	szText[HISTORY_LEN];

} SEARCH_HISTORY;
typedef struct 
{
	UINT  uDialogId;
	SEARCH_HISTORY history1[HISTORY_MAX];
	//TCHAR szHistory1[HISTORY_MAX][HISTORY_LEN];
	//TCHAR szHistory2[HISTORY_MAX][HISTORY_LEN];
	int   nHistoryCount1;
	int   nHistoryCount2;
	int	  nLastDataIdx;

} SEARCH_PANE_STATE;


SEARCH_PANE_STATE g_SearchState[4];


HWND g_hwndFindPane[MAX_FIND_PANES];

#define FINDBORDER 6

BOOL	g_fKeepVisible		= TRUE;
BOOL	g_fMatchCase		= TRUE;
BOOL	g_fSearchBackwards  = FALSE;
BOOL	g_fFindInSelection  = FALSE;
BOOL	g_fBigEndian		= FALSE;

void ShiftWindow(HWND hwnd, HWND hwndHV)
{
	POINT pt;
	RECT rect;

	// get the hexview's caret position in screen-coordinates
	HexView_GetCurCoord(hwndHV, &pt);
	ClientToScreen(hwndHV, &pt);

	GetWindowRect(hwnd, &rect);
	
	// see if the specified window overlaps the point
	if(PtInRect(&rect, pt))
	{
		// which direction do we shift the window - it must be:
		// the least distance (up/down/left/right), that also
		// keeps the window contained within the same display
		int dx = 0, dy = 0;

		// get current monitor's display
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi);
		
		/*
		if(pt.x - rect.left < rect.right-pt.x && 
			rect.right + (pt.x - rect.left) +32 < mi.rcWork.right)
			dx = pt.x - rect.left + 32;
		else
			dx = pt.x - rect.right - 32;*/

		if(pt.y - rect.top < rect.bottom - pt.y && 
			rect.bottom + (pt.y - rect.top) + 32 < mi.rcWork.bottom)
			dy = pt.y - rect.top + 32;
		else
			dy = pt.y - rect.bottom - 32;

		
		//if(pt.y - rect.top < rect.right-pt.x)
		//	;

		SetWindowXY(hwnd, rect.left + dx, rect.top + dy, NULL);
	}
}

int hex2dec(int ch)
{
	if(isdigit(ch))
		return ch - '0';
	else if(ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if(ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}


void UpdatePin(HWND hwndDlg, UINT uId, BOOL fChecked)
{
	HWND hwndPin = GetDlgItem(hwndDlg, uId);
	SendMessage(hwndPin, TB_CHANGEBITMAP, uId, MAKELPARAM(fChecked, 0)); 
	SendMessage(hwndPin, TB_CHECKBUTTON, uId, MAKELPARAM(fChecked, 0)); 
}

void UpdateFindGui(HWND hwndPanel, SEARCH_PANE_STATE *sps)
{
	//CheckRadioButton(hwndPanel, IDC_SCOPE_CURSOR, IDC_SCOPE_SEL, g_fFindInSelection ? IDC_SCOPE_SEL : IDC_SCOPE_CURSOR);
	CheckDlgButton(hwndPanel, IDC_SCOPE_CURSOR, !g_fFindInSelection);
	CheckDlgButton(hwndPanel, IDC_SCOPE_SEL, g_fFindInSelection);
	CheckDlgButton(hwndPanel, IDC_SEARCHBACK, g_fSearchBackwards);
	CheckDlgButton(hwndPanel, IDC_MATCHCASE, g_fMatchCase);
	CheckDlgButton(hwndPanel, IDC_SEARCH_ENDIAN, g_fBigEndian);
	EnableDlgItem(hwndPanel, IDOK, searchValid);

	//CheckDlgButton(hwndPanel, IDC_KEEPVISIBLE, g_fKeepVisible);

	UpdatePin(hwndPanel, IDC_KEEPVISIBLE, g_fKeepVisible);
}

BOOL IsToolbarButtonChecked(HWND hwndTB, UINT nCtrlId)
{
	return (TBSTATE_CHECKED & SendMessage(hwndTB, TB_GETSTATE, nCtrlId, 0)) ? TRUE : FALSE;
}



void DumpHex(HWND hwndEdit, BYTE *data, UINT len)
{
	TCHAR szBuf[200], *ptr = szBuf;
	UINT i;

	szBuf[0] = '\0';
	for(i = 0; i < len && ptr-szBuf+4<200; i++)
	{
		ptr += wsprintf(ptr, TEXT("%02x "), data[i]);
	}

	SetWindowText(hwndEdit, szBuf);
}

int hex2dec(int ch);
BOOL Hex2Binary(TCHAR *szText, BYTE *buf, int *len, BOOL fBigEndian)
{
	TCHAR *ptr = szText;
	
	int i, j;
	BYTE val = 0;
	int count = 0;

	for(i = 0, j = 0; ptr[i] && j < *len; i++)
	{
		if(isxdigit(ptr[i]))
		{
			if(count) val <<= 4;
			else      val = 0;
			val |= hex2dec(ptr[i]);

			 buf[j] = val;

			//TRACEA("%x ", val);
						
			count = !count;
			if(count == 0)
				j++;
		}
		else if(isspace(ptr[i]))
		{
			if(count)
				j++;
			count = 0;
		}
		else
		{
			return FALSE;
		}
	}

	*len = j+count;
	return TRUE;//j+count;
}

BOOL Num2Binary(TCHAR *szText, BYTE *buf, int *buflen, int width, BOOL fInteger, BOOL fBigEndian)
{
	TCHAR tmp[21];
	int i = 0;

	int len = *buflen;
	BOOL fSuccess = TRUE;
	TCHAR *ot = szText;

	do
	{
		if(*szText == '\0' || *szText == ',' || _istspace(*szText))
		{
			if(i > 0 && len >= width)
			{
				UINT64 inum;
				double fnum;
				const  TCHAR * fmt;

				tmp[i] = '\0';

				if(tmp[0] == '0' && tmp[1] == 'x')
					fmt = TEXT("0x%I64x");
				else
					fmt = TEXT("%I64d");
				
				// integer number
				if(fInteger)
				{
					if(_stscanf(tmp, fmt, &inum))
					{
						switch(width)
						{
						case 1: *((UINT8 *)buf) = (UINT8)inum;  break;
						case 2: *((UINT16*)buf) = (UINT16)inum; break;
						case 4: *((UINT32*)buf) = (UINT32)inum; break;
						case 8: *((UINT64*)buf) = (UINT64)inum; break;
						}
						
						if(fBigEndian)
							reverse(buf,width);
						
						buf += width;
						len -= width;					
					}
					else
					{
						*buflen = (int)(szText-ot);
						return FALSE;//fSuccess = FALSE;
						break;
					}
				}
				// floating point number
				else if(!fInteger)// && _stscanf(tmp, TEXT("%f"), &fnum))
				{
					TCHAR *ep;
					fnum = _tcstod(tmp, &ep);
					switch(width)
					{
					case 4: *((float *)buf) = (float)fnum; break;
					case 8: *((double *)buf) = (double)fnum; break;
					}
					
					if(fBigEndian)
						reverse(buf,width);				

					buf += width;
					len -= width;
				}

				i = 0;
			}
		}
		else if(i < 20)
		{
			tmp[i++] = *szText;
		}
	} while(*szText++);

	*buflen = *buflen - len;
	return fSuccess;
}

//
//	Convert the specified UTF-16 text to the given format
//
BOOL Text2Binary(WCHAR *szText, BYTE *buf, int *buflen, int codepage, BOOL fBigEndian)
{
	int len = *buflen;

	if(codepage == -1)	// stay as UTF16
	{
		int i;
		UINT16 *ptr16 = (UINT16 *)buf;

		for(i = 0; szText[i] && i < len/2; i++)
		{
			ptr16[i] = szText[i];

			if(fBigEndian)
				reverse((BYTE *)&ptr16[i], sizeof(UINT16));
		}

		*buflen = i * sizeof(UINT16);
		return TRUE;
	}
	else if(codepage == -2)	// convert to UTF32
	{
		int i;
		UINT32 *ptr32 = (UINT32 *)buf;

		for(i = 0; szText[i] && i < len/4; i++)
		{
			ptr32[i] = szText[i];// + (szText[i+1] << 16);

			if(fBigEndian)
				reverse((BYTE *)&ptr32[i], sizeof(UINT32));
		}

		*buflen = i * sizeof(UINT32);
		return TRUE;
	}
	else	// standard conversion
	{
		len = WideCharToMultiByte(codepage, 0, szText, -1, buf, len, 0, 0);
		*buflen =  len > 0 ? len - 1 : 0;
		return TRUE;
	}
}

//
//	Create a toolbar with one button in it, for
//  the pin-button
//
HWND CreatePinToolbar(HWND hwndDlg, UINT nCtrlId, BOOL fRightAligned)
{
	RECT    rect;
	RECT	rc1, rc2;
	HWND	hwndPin;
	
	static TBBUTTON tbbPin[] = 
	{	
		{	0,	0, TBSTATE_ENABLED, TBSTYLE_CHECK,  {0}	},
	};	

	tbbPin[0].idCommand = nCtrlId;

	// Create the toolbar to hold pin bitmap
	hwndPin = CreateToolbarEx(
			hwndDlg,	
			TOOLBAR_PIN_STYLES,				//,
			nCtrlId,						// toolbar ID (don't need)
			2,								// number of button images
			g_hInstance,					// where the bitmap is
			IDB_PIN_BITMAP,					// bitmap resource name
			tbbPin,							// TBBUTTON structure
			sizeof(tbbPin) / sizeof(tbbPin[0]),
			15,14,15,14,					// 
			sizeof(TBBUTTON) );


	// Find out how big the button is, so we can resize the
	// toolbar to fit perfectly
	SendMessage(hwndPin, TB_GETITEMRECT, 0, (LPARAM)&rect);
	
	SetWindowPos(hwndPin, HWND_TOP, 0,0, 
		rect.right-rect.left, 
		rect.bottom-rect.top, SWP_NOMOVE);

	// Setup the bitmap image
	SendMessage(hwndPin, TB_CHANGEBITMAP, nCtrlId, (LPARAM)MAKELPARAM(g_fKeepVisible, 0)); 

	// Checked / Unchecked
	SendMessage(hwndPin, TB_CHECKBUTTON, nCtrlId, MAKELONG(g_fKeepVisible, 0));

	GetClientRect(hwndDlg, &rc1);
	GetClientRect(hwndPin, &rc2);
	
	if(fRightAligned)
		SetWindowPos(hwndPin, 0, rc1.right - rc2.right-10, rc1.bottom-rc2.bottom-8,0,0,SWP_NOSIZE);
	else
		SetWindowPos(hwndPin, 0, rc1.left+10, rc1.bottom-rc2.bottom-8,0,0,SWP_NOSIZE);

	return hwndPin;
}

BOOL UpdateSearchData(HWND hwndSource, SEARCHTYPE searchType, BOOL fBigEndian, BYTE *searchData, int *searchLen)
{
	// these need updating if the SEARCHTYPE_xxx values ever change
	const int codepage[] = { 0, CP_ACP, CP_UTF8, -1, -2, 0, 0,0,0,0,0 };
	const int width[]    = { 0, 0, 0, 0, 0,  1,2,4,8,4,8};
	const int isint[]    = { 0, 0, 0, 0, 0,  1,1,1,1,0,0};

	TCHAR szText[MAX_SEARCH_TEXT];
	BOOL  fSuccess = FALSE;
	
	GetWindowText(hwndSource, szText, MAX_SEARCH_TEXT);
		
	switch(searchType)
	{
	// raw hex 
	case SEARCHTYPE_HEX:
		fSuccess = Hex2Binary(szText, searchData, searchLen, fBigEndian);
		break;
		
	// text formats
	case SEARCHTYPE_ASCII: case SEARCHTYPE_UTF8: 
	case SEARCHTYPE_UTF16: case SEARCHTYPE_UTF32:
		fSuccess = Text2Binary(szText, searchData, searchLen, codepage[searchType], fBigEndian);
		break;
		
	// number formats
	case SEARCHTYPE_BYTE:  case SEARCHTYPE_WORD: 
	case SEARCHTYPE_DWORD: case SEARCHTYPE_QWORD:  
	case SEARCHTYPE_FLOAT: case SEARCHTYPE_DOUBLE:  
		fSuccess = Num2Binary(szText, searchData, searchLen, width[searchType], isint[searchType], fBigEndian);
		break;
		
	default:
		*searchLen  = 0;
	}

/*	if(fSuccess)
	{
		DumpHex(GetDlgItem(hwnd, IDC_EDIT_PREVIEW), searchData, *searchLen);
	}
	else
	{
		SetDlgItemText(hwnd, IDC_EDIT_PREVIEW, TEXT("error"));
		//SendDlgItemMessage(hwnd, IDC_COMBO1, CB_SETEDITSEL, len, 0);
	}*/

	return fSuccess;
}

int GetSearchType(HWND hwndCombo)
{
	int idx	= (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
	return    (int)SendMessage(hwndCombo, CB_GETITEMDATA, idx, 0);
}

BOOL UpdateSearchDataDlg(HWND hwndDlg, int sourceId, BOOL fBigEndian, BYTE *searchData, int *searchLen)
{
	int searchType;

	// get the searchType from the datatype dropdown
	searchType = GetSearchType(GetDlgItem(hwndDlg, IDC_COMBO_DATATYPE));

	if(UpdateSearchData(GetDlgItem(hwndDlg, sourceId), searchType, fBigEndian, searchData, searchLen))
	{
		DumpHex(GetDlgItem(hwndDlg, IDC_EDIT_PREVIEW), searchData, *searchLen);
		return TRUE;
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_EDIT_PREVIEW, TEXT("error"));
		return FALSE;
	}
}
/*
BOOL Update(HWND hwnd, SEARCH_PANE_STATE *sps, BYTE *searchData, int *searchLen)
{
	TCHAR szText[MAX_SEARCH_TEXT];
	//BYTE oof[100] = "";
	//int len = sizeof(oof);
	int idx, searchType;
	int codepage[] = { CP_ACP, CP_UTF8, -1, -2, 0 };
	int width[] = { 1,2,4,8,4,8};
	int isi[]   = { 1,1,1,1,0,0};
	BOOL fSuccess = FALSE;
	
	GetDlgItemText(hwnd, IDC_COMBO1, szText, MAX_SEARCH_TEXT);
	
	idx = SendDlgItemMessage(hwnd, IDC_COMBO_DATATYPE, CB_GETCURSEL, 0, 0);
	searchType = SendDlgItemMessage(hwnd, IDC_COMBO_DATATYPE, CB_GETITEMDATA, idx, 0);
	
	switch(sps->uDialogId)//GetWindowLongPtr(hwnd, GWL_USERDATA))
	{
	case IDD_FINDHEX:	
		fSuccess = Hex2Binary(szText, searchData, searchLen, g_fBigEndian);
		break;
		
	case IDD_FINDASC:	
		fSuccess = Text2Binary(szText, searchData, searchLen, codepage[idx], g_fBigEndian);
		break;
		
	case IDD_FINDNUM:
		fSuccess = Num2Binary(szText, searchData, searchLen, width[idx], isi[idx], g_fBigEndian);
		break;
		
	default:
		*searchLen  = 0;
	}

	if(fSuccess)
	{
		DumpHex(GetDlgItem(hwnd, IDC_EDIT_PREVIEW), searchData, *searchLen);
	}
	else
	{
		SetDlgItemText(hwnd, IDC_EDIT_PREVIEW, TEXT("error"));
		//SendDlgItemMessage(hwnd, IDC_COMBO1, CB_SETEDITSEL, len, 0);
	}

	return fSuccess;
}*/


BOOL FindNext()
{
	HWND hwndHV = g_hwndHexView;
	size_w result;
	UINT options = 0;

	if(searchLen == 0 || searchValid == FALSE)
	{
		ShowFindDialog(g_hwndMain, -2);
		return FALSE;
	}

	if(g_fFindInSelection)
		options |= HVFF_SCOPE_SELECTION;

	if(g_fMatchCase == FALSE)
		options |= HVFF_CASE_INSENSITIVE;

	if(HexView_FindNext(hwndHV, &result, options))
	{
		HexView_SetSelStart(hwndHV, result);
		HexView_SetSelEnd(hwndHV, result+searchLen);
		HexView_SetCurPos(hwndHV, result+searchLen);

		ShiftWindow(g_hwndSearch, hwndHV);

		InvalidateRect(hwndHV, 0, 0);
		return TRUE;
	}
	else
	{
		MAINWND *mainWnd = (MAINWND *)GetWindowLongPtr(g_hwndMain, 0);
		UpdateProgress(mainWnd, FALSE, 0, 0);
		MessageBox(g_hwndMain, TEXT("Could not find any more data"), TEXT("HexEdit"), MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}
}

BOOL Find(HWND hwnd, HWND hwndHV)
{
	TCHAR szText[MAX_SEARCH_TEXT];
	int i;

	SEARCH_PANE_STATE *sps = (SEARCH_PANE_STATE *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	GetDlgItemText(hwnd, IDC_COMBO1, szText, MAX_SEARCH_TEXT);
			
			for(i = 0; i < sps->nHistoryCount1; i++)
			{
				if(lstrcmp(sps->history1[i].szText, szText) == 0)
					break;
			}

			if(i == sps->nHistoryCount1 && i < HISTORY_MAX)
			{
				int idx;
				
				lstrcpyn(sps->history1[i].szText, szText, HISTORY_LEN);
				sps->history1[i].nDataTypeIdx = (int)SendDlgItemMessage(hwnd, IDC_COMBO_DATATYPE, CB_GETCURSEL, 0, 0);
				sps->nHistoryCount1++;
				
				idx = (int)SendDlgItemMessage(hwnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)szText);
				SendDlgItemMessage(hwnd, IDC_COMBO1, CB_SETITEMDATA, idx, sps->history1[i].nDataTypeIdx);
			}

			// get the search data!
			searchLen = sizeof(searchData);
			//Update(hwnd, sps, searchData, &searchLen);
			searchValid = UpdateSearchDataDlg(hwnd, IDC_COMBO1, g_fBigEndian, searchData, &searchLen);

			if(searchValid == FALSE)
			{
				HexErrorBox(TEXT("%s"), TEXT("Invalid search data - select Data Type"));
				SetDlgItemFocus(hwnd, IDC_COMBO1);
				return FALSE;
			}

			HexView_FindInit(hwndHV, searchData, searchLen);
			HexView_SetSearchPattern(hwndHV, searchData, searchLen);
			
			FindNext();

			if(!g_fKeepVisible)
				DestroyWindow(hwnd);

	return TRUE;
}

BOOL Replace(HWND hwndDlg, HWND hwndHV)
{
	int searchType;
	size_w selsize;

	// get the searchType from the datatype dropdown
	searchType = GetSearchType(GetDlgItem(hwndDlg, IDC_COMBO_DATATYPE));

	replaceLen = sizeof(replaceData);
	UpdateSearchData(GetDlgItem(hwndDlg, IDC_COMBO2), searchType, g_fBigEndian, replaceData, &replaceLen);

	HexView_GetSelSize(hwndHV, &selsize);

	if(selsize != replaceLen && HexView_GetEditMode(hwndHV) != HVMODE_INSERT)
	{
		MessageBox(hwndDlg, TEXT("Replace data must be same length in Overwrite mode"), TEXT("error"), MB_OK|MB_ICONWARNING);
		return FALSE;
	}
	else
	{
		HexView_SetDataCur(hwndHV, replaceData, replaceLen);
		return FindNext();
	}
}

void ReplaceAll(HWND hwndDlg, HWND hwndHV)
{
	HexView_SetRedraw(hwndHV, FALSE);
	
	for( ;; )
	{
		NMHVPROGRESS nmhvp;
		size_w len, pos;
		
		if(!Replace(hwndDlg, hwndHV))
			break;

		HexView_GetCurPos(hwndHV, &pos);
		HexView_GetFileSize(hwndHV, &len);

		nmhvp.hdr.code     = HVN_PROGRESS;
		nmhvp.hdr.hwndFrom = hwndHV;
		nmhvp.hdr.idFrom   = GetWindowLongPtr(hwndHV, GWL_ID);
	
		nmhvp.len = len;
		nmhvp.pos = pos;

		if(SendMessage(GetParent(hwndHV), WM_NOTIFY, 0, (LPARAM)&nmhvp))
			break;
	}

	HexView_SetRedraw(hwndHV, TRUE);
}

static DWORD GetClipboardDataBuf(UINT uFormat, PVOID pData, DWORD nLength)
{
	HANDLE	hMem;
	PVOID	ptr;

	if((hMem = GetClipboardData(uFormat)) == 0)
		return FALSE;

	if((ptr = GlobalLock(hMem)) == 0)
		return FALSE;

	nLength = (DWORD)min(nLength, GlobalSize(hMem));

	memcpy(pData, ptr, nLength);

	GlobalUnlock(hMem);
	return nLength;
}

static int BinToHex(BYTE *buf, int len, char *text)
{
	int i;
	for(i = 0; i < len; i++)
	{
		text += wsprintfA(text, "%02X ", buf[i]);
	}
	return i*3;
}

BOOL IsHexString(char *str, int len)
{
	int i;
	for(i = 0; i < len; i++)
	{
		if(!(isxdigit(str[i]) || str[i] == ' '))
			return FALSE;
	}

	return TRUE;
}

static INT_PTR CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC oldProc = (WNDPROC)GetProp(hwnd, TEXT("oldProc"));
	DWORD len;
	char  buf[64];

	switch(msg)
	{
	case WM_PASTE:
		OpenClipboard(hwnd);

		if((len = GetClipboardDataBuf(CF_TEXT, buf, sizeof(buf))) > 1)
		{
			HWND hwndCombo = GetParent(hwnd);
			HWND hwndDlg   = GetParent(hwndCombo);

			int searchType = GetSearchType(GetDlgItem(hwndDlg, IDC_COMBO_DATATYPE));

			if(searchType != SEARCHTYPE_HEX || IsHexString(buf, len-1))
			{
				SetWindowTextA(hwnd, buf);
			}
			else
			{
				char buf2[256];
				BinToHex((BYTE *)buf, len-1, buf2);
				SetWindowTextA(hwnd, buf2);				
			}

			SendMessage(hwndCombo, CB_SETEDITSEL, 0, MAKELONG(0,-1));//-1, -1);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwndCombo), CBN_EDITUPDATE), (LPARAM)hwndCombo);
		}

		CloseClipboard();
		return 0;
	}

	return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
}

void HookEdit(HWND hwndEdit)
{
	WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
	SetProp(hwndEdit, TEXT("oldProc"), (WNDPROC)oldProc);
}

INT_PTR CALLBACK FindHexDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL g_nLastTextIdx = 0;
	static BOOL g_nLastNumIdx = 0;
	static BOOL g_nLastRepIdx = 0;

	//static SEARCH_PANE_STATE state[4];

	SEARCH_PANE_STATE *sps = (SEARCH_PANE_STATE *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
//	TCHAR szText[MAX_SEARCH_TEXT];
	int i;
	HWND hwndPin;

	HWND hwndHV = g_hwndHexView;

	switch(msg)
	{
	case WM_INITDIALOG:

		sps = (SEARCH_PANE_STATE *)lParam;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)sps);

		EnableDialogTheme(hwnd);

		UpdateFindGui(hwnd, sps);

		// hex or replace dialogs
		if(sps->uDialogId == IDD_FINDHEX+0 || sps->uDialogId == IDD_FINDHEX+3)
		{
			HookEdit(GetWindow(GetDlgItem(hwnd, IDC_COMBO1), GW_CHILD));
			HookEdit(GetWindow(GetDlgItem(hwnd, IDC_COMBO2), GW_CHILD));
		}

		hwndPin = CreatePinToolbar(hwnd, IDC_KEEPVISIBLE, FALSE);//TRUE);

		AlignWindow(hwndPin, GetDlgItem(hwnd, IDC_GROUP), ALIGN_BOTTOM);
		AlignWindow(hwndPin, GetDlgItem(hwnd, IDCANCEL),  ALIGN_LEFT);


		// add the history!
		for(i = 0; i < sps->nHistoryCount1; i++)
		{
			int idx = (int)SendDlgItemMessage(hwnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)sps->history1[i].szText);
			SendDlgItemMessage(hwnd, IDC_COMBO1, CB_SETITEMDATA, idx, sps->history1[i].nDataTypeIdx);
		}

		return FALSE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			DestroyWindow(GetParent(hwnd));
			return TRUE;

		case IDC_KEEPVISIBLE:

			g_fKeepVisible = IsToolbarButtonChecked(GetDlgItem(hwnd, IDC_KEEPVISIBLE), IDC_KEEPVISIBLE);
			UpdatePin(hwnd, IDC_KEEPVISIBLE, g_fKeepVisible);
			return TRUE;

		case IDC_REPLACE:
			Replace(hwnd, hwndHV);
			return TRUE;

		case IDC_REPLACEALL:
			ReplaceAll(hwnd, hwndHV);
			return TRUE;

		case IDOK:
			Find(hwnd, hwndHV);
			return TRUE;
		}

		// one of the buttons/checks/radios was clicked. update all state
		if(HIWORD(wParam) == BN_CLICKED)
		{
			g_fFindInSelection	= IsDlgButtonChecked(hwnd, IDC_SCOPE_SEL);
			g_fSearchBackwards	= IsDlgButtonChecked(hwnd, IDC_SEARCHBACK);
			g_fBigEndian		= IsDlgButtonChecked(hwnd, IDC_SEARCH_ENDIAN);
			g_fMatchCase		= IsDlgButtonChecked(hwnd, IDC_MATCHCASE);
			searchLen = sizeof(searchData);
			//Update(hwnd, sps, searchData, &searchLen);
			searchValid = UpdateSearchDataDlg(hwnd, IDC_COMBO1, g_fBigEndian, searchData, &searchLen);
			return TRUE;
		}

		if(HIWORD(wParam) == CBN_EDITUPDATE || HIWORD(wParam) == CBN_SELCHANGE)
		{
			if(LOWORD(wParam) == IDC_COMBO1)
			{
				searchLen = sizeof(searchData);
				searchValid = UpdateSearchDataDlg(hwnd, IDC_COMBO1, g_fBigEndian, searchData, &searchLen);

				EnableDlgItem(hwnd, IDOK, searchValid);
			}

			if(LOWORD(wParam) == IDC_COMBO2)
			{
				replaceLen   = sizeof(replaceData);
				replaceValid = UpdateSearchDataDlg(hwnd, IDC_COMBO2, g_fBigEndian, replaceData, &replaceLen);

				EnableDlgItem(hwnd, IDC_REPLACE,    replaceValid);
				EnableDlgItem(hwnd, IDC_REPLACEALL, replaceValid);
			}

			if(LOWORD(wParam) == IDC_COMBO_DATATYPE)
			{
				searchLen = sizeof(searchData);
				searchValid = UpdateSearchDataDlg(hwnd, IDC_COMBO1, g_fBigEndian, searchData, &searchLen);

				EnableDlgItem(hwnd, IDOK, searchValid);
			}

			/*if(LOWORD(wParam) == IDC_COMBO1 && HIWORD(wParam) == CBN_SELCHANGE)
			{
				int idx = SendDlgItemMessage(hwnd, IDC_COMBO1, CB_GETCURSEL, 0, 0);
				idx = SendDlgItemMessage(hwnd, IDC_COMBO1, CB_GETITEMDATA, idx, 0);
				SendDlgItemMessage(hwnd, IDC_COMBO_DATATYPE, CB_SETCURSEL, sps->history1[idx].nDataTypeIdx, 0);
			}*/
			
			return TRUE;
		}

		return FALSE;

	case WM_CLOSE:
		DestroyWindow(GetParent(hwnd));
		return TRUE;
	}
	return FALSE;
}


void AddSearchTabs(HWND hwnd)
{
	TCITEM tcitem;
	RECT rect;
	int i;

	HWND hwndTab = GetDlgItem(hwnd, IDC_TAB1);

	tcitem.mask = TCIF_TEXT;
	tcitem.pszText = TEXT("Hex");
	TabCtrl_InsertItem(hwndTab, 0, &tcitem);

	tcitem.mask = TCIF_TEXT;
	tcitem.pszText = TEXT("Text");
	TabCtrl_InsertItem(hwndTab, 1, &tcitem);

	tcitem.mask = TCIF_TEXT;
	tcitem.pszText = TEXT("Numbers");
	TabCtrl_InsertItem(hwndTab, 2, &tcitem);

	tcitem.mask = TCIF_TEXT;
	tcitem.pszText = TEXT("Replace");
	TabCtrl_InsertItem(hwndTab, 3, &tcitem);

	//	GetClient
//	TabCtrl_GetItemRect(hwndTab, 0, &rect);

	for(i = MAX_FIND_PANES-1; i >= 0; i--)
	{
		SEARCH_PANE_STATE *sps = &g_SearchState[i];

		g_SearchState[i].uDialogId = IDD_FINDHEX+i;

		g_hwndFindPane[i] = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_FINDHEX+i), hwnd, FindHexDlg, (LPARAM)sps);
		//CreateDialogWithFont(g_hInstance, MAKEINTRESOURCE(IDD_FINDHEX+i), hwnd, FindHexDlg, (LPARAM)sps, 
			//TEXT("Segoe UI"), 8);
	}

	// work out how big tab control needs to be to hold the pane
	//for(i = 0; i < MAX_FIND_PANES; i++)
	i = 0;
	{
		GetClientRect(g_hwndFindPane[i], &rect);
		MapWindowPoints(g_hwndFindPane[i], hwnd, (POINT *)&rect, 2);
		TabCtrl_AdjustRect(hwndTab, TRUE, &rect);
		
	//	break;
	}

	// move tab control into position
	MoveWindow(hwndTab, FINDBORDER, FINDBORDER, rect.right-rect.left, rect.bottom-rect.top, FALSE);

	// adjust the find dialog size
	AdjustWindowRectEx(&rect, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
	InflateRect(&rect, FINDBORDER, FINDBORDER);
	//SetWindowPos(hwnd, 0, 0, 0, rect.right-rect.left, rect.bottom-rect.top-2, SWP_SIZEONLY);

	// now find out the tab control's client display area
	GetWindowRect(hwndTab, &rect);
	MapWindowPoints(0, hwnd, (POINT *)&rect, 2);
	TabCtrl_AdjustRect(hwndTab, FALSE, &rect);

	// move find pane into position
	for(i = 0; i < MAX_FIND_PANES; i++)
	{
		MoveWindow(g_hwndFindPane[i], rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, FALSE);
	}
	
//	ShowWindow(g_hwndFindPane[0], SW_SHOW);
}

/*void ResizeTab(HWND hwnd)
{
	RECT rect;
	HWND hwndTab = GetDlgItem(hwnd, IDC_TAB1);

	HDWP hdwp = BeginDeferWindowPos(2);

	GetClientRect(hwnd, &rect);
	InflateRect(&rect, -FINDBORDER, -FINDBORDER);

	MoveWindow(hwndTab, FINDBORDER, FINDBORDER, rect.right-rect.left, rect.bottom-rect.top, FALSE);

	TabCtrl_AdjustRect(hwndTab, FALSE, &rect);

	// now find out the tab control's client display area
	GetWindowRect(hwndTab, &rect);
	MapWindowPoints(0, hwnd, (POINT *)&rect, 2);
	TabCtrl_AdjustRect(hwndTab, FALSE, &rect);

	// move find pane into position
	MoveWindow(g_hwndFindHex, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, FALSE);

	EndDeferWindowPos(hdwp);
}*/

void AddComboStrings(HWND hwndDlg, UINT uComboId, TCHAR *szStrings[])
{
	int i;

	for(i = 0; szStrings[i]; i++)
		SendDlgItemMessage(hwndDlg, uComboId, CB_ADDSTRING, 0, (LPARAM)szStrings[i]);

	SendDlgItemMessage(hwndDlg, uComboId, CB_SETCURSEL, 0, 0);
}

HWND GetCurFindTab(HWND hwndFindDlg)
{
	int i = TabCtrl_GetCurSel(GetDlgItem(hwndFindDlg, IDC_TAB1));

	return g_hwndFindPane[i];
}

void SetFindTabData(HWND hwndFindDlg, int idx, BYTE *buf, int len)
{
	HWND hwndPanel = g_hwndFindPane[idx];
	HWND hwndCtrl = GetDlgItem(hwndPanel, IDC_COMBO1);
	char buf2[256];

	if(len == 0) return;

	if(idx == 0)
	{
		BinToHex(buf, len, buf2);
	}
	else
	{
		sprintf_s(buf2, sizeof(buf2), "%.*s", len, buf);
	}

	SetWindowTextA(hwndCtrl, buf2);
}

void SetFindTab(HWND hwndFindDlg, int idx, BOOL fMouseActivated)
{
	int i;
	HWND hwndPanel;
	SEARCH_PANE_STATE *sps;

	TabCtrl_SetCurSel(GetDlgItem(hwndFindDlg, IDC_TAB1), idx);

	// find the current (new) pane and update is state
	hwndPanel = g_hwndFindPane[idx];
	sps = (SEARCH_PANE_STATE *)GetWindowLongPtr(hwndPanel, GWLP_USERDATA);
	UpdateFindGui(hwndPanel, sps);
	
	// display!
	SetWindowPos(g_hwndFindPane[idx], HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
	
	// was the mouse used to activate this tab?
	if(fMouseActivated)
	{
		SetDlgItemFocus(hwndPanel, IDC_COMBO1);
		PostMessage(hwndPanel, WM_NEXTDLGCTL, IDC_COMBO1, TRUE);
	}
	
	// hide any other search-pane
	for(i = 0; i < MAX_FIND_PANES; i++)				
	{
		if(i != idx)
		{
			DelStyle(g_hwndFindPane[i], WS_VISIBLE);
			//ShowWindow(g_hwndFindPane[i], SW_HIDE);
		}
	}

	// hide the replace/replaceall buttons if necessary
	EnableDlgItem(hwndFindDlg, IDC_REPLACE, (BOOL)(idx == 3));
	EnableDlgItem(hwndFindDlg, IDC_REPLACEALL, (BOOL)(idx == 3));

	SetDlgItemFocus(g_hwndFindPane[idx], IDC_COMBO1);
	PostMessage(g_hwndFindPane[idx], WM_NEXTDLGCTL, IDC_COMBO1, TRUE);

	PostMessage(g_hwndFindPane[idx], WM_COMMAND, MAKEWPARAM(IDC_COMBO1, CBN_EDITUPDATE), 0);
}

int nCurrentFindTab = -1;


INT_PTR CALLBACK SearchDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NMHDR *nmhdr;
	static BOOL fMouseDown = FALSE;
	static POINT ptLastPos;
	static BOOL  fFirstTime = TRUE;
	HWND hwndCombo;
	HWND hwndHV = g_hwndHexView;

	switch(msg)
	{
	case WM_INITDIALOG:
		AddSearchTabs(hwnd);

		// text (UTF8/UTF16/UTF32)
		hwndCombo = GetDlgItem(g_hwndFindPane[1], IDC_COMBO_DATATYPE);
		AddSearchTypes(hwndCombo, SEARCHTYPE_ASCII, SEARCHTYPE_UTF32, 0);
		//AddComboStrings(g_hwndFindPane[1], IDC_COMBO_DATATYPE, szTextNames);

		// numbers (byte/word/dword/float/double)
		hwndCombo = GetDlgItem(g_hwndFindPane[2], IDC_COMBO_DATATYPE);
		AddSearchTypes(hwndCombo, SEARCHTYPE_BYTE, SEARCHTYPE_DOUBLE, 0);
		
		//AddComboStrings(g_hwndFindPane[2], IDC_COMBO_DATATYPE, szNumberNames);

		// replace (everything)
		hwndCombo = GetDlgItem(g_hwndFindPane[3], IDC_COMBO_DATATYPE);
		AddSearchTypes(hwndCombo, SEARCHTYPE_HEX, SEARCHTYPE_HEX, 0);
		AddSearchTypes(hwndCombo, SEARCHTYPE_ASCII, SEARCHTYPE_UTF32, 0);
		AddSearchTypes(hwndCombo, SEARCHTYPE_BYTE, SEARCHTYPE_DOUBLE, 0);
		//AddComboStrings(g_hwndFindPane[3], IDC_COMBO3, szHexNames);
		//AddComboStrings(g_hwndFindPane[3], IDC_COMBO3, szTextNames);
		//AddComboStrings(g_hwndFindPane[3], IDC_COMBO3, szNumberNames);

		if(fFirstTime)
		{
			CenterWindow(hwnd);
			fFirstTime = FALSE;
		}
		else
		{
			SetWindowXY(hwnd, ptLastPos.x, ptLastPos.y, NULL);
		}

		return FALSE;

	//case WM_SETFOC

	case WM_NOTIFY:
		nmhdr = (NMHDR *)lParam;

		if(nmhdr->code == TCN_SELCHANGE)
		{
			nCurrentFindTab = TabCtrl_GetCurSel(nmhdr->hwndFrom);
			SetFindTab(hwnd, nCurrentFindTab, fMouseDown);

			return TRUE;
		}
		else if(nmhdr->code == TCN_SELCHANGING)
		{
			fMouseDown = (GetKeyState(VK_LBUTTON) & 0x80000000) ? TRUE : FALSE;
		}
		else if(nmhdr->code == NM_RELEASEDCAPTURE)
		{
			fMouseDown = FALSE;
		}
		break;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			DestroyWindow(hwnd);
			return TRUE;

		case IDC_REPLACE:
			Replace(GetCurFindTab(hwnd), hwndHV);
			return TRUE;

		case IDC_REPLACEALL:
			ReplaceAll(GetCurFindTab(hwnd), hwndHV);
			return TRUE;

		case IDOK:
			Find(GetCurFindTab(hwnd), hwndHV);
			return TRUE;
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

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;

	case WM_DESTROY:
		g_hwndSearch = 0;
		return TRUE;
	}
	return FALSE;
}

BOOL ShowFindDialog(HWND hwndMain, int nCurrentFindTab)
{
	BYTE initial[64];
	size_w sellen = 0;
	size_w offset;

	if(HexView_GetSelSize(g_hwndHexView, &sellen))
	{
		sellen = min(64, sellen);
		HexView_GetSelStart(g_hwndHexView, &offset);
		HexView_GetData(g_hwndHexView, offset, initial, (ULONG)sellen);
	}

	if(nCurrentFindTab == -1)
	{
		nCurrentFindTab = HexView_GetCurPane(g_hwndHexView);
	}
	else if(nCurrentFindTab == -2 && g_hwndSearch)
	{
		nCurrentFindTab = TabCtrl_GetCurSel(GetDlgItem(g_hwndSearch, IDC_TAB1));
	}

	if(g_hwndSearch == 0)
	{
		g_hwndSearch = CreateDialog(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SEARCH), hwndMain, SearchDlg);
	
		SetFindTabData(g_hwndSearch, nCurrentFindTab, initial, (int)sellen);
		ShowWindow(g_hwndSearch, SW_SHOW);
	}

	SetFindTab(g_hwndSearch, nCurrentFindTab, FALSE);

	return TRUE;
}