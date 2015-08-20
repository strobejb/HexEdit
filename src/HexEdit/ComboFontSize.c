//
//  ComboFontSize.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>

//
//	Convert from logical-units to point-sizes
//
static int LogicalToPoints(int nLogicalSize)
{
	HDC hdc      = GetDC(0);
	int size     = MulDiv(nLogicalSize, 72, GetDeviceCaps(hdc, LOGPIXELSY));

	ReleaseDC(0, hdc);

	return size;
}

static int CALLBACK EnumFontSizes(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	static const int ttsizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
	TCHAR ach[100];

	BOOL fTrueType = (lpelfe->elfLogFont.lfOutPrecision == OUT_STROKE_PRECIS) ? TRUE : FALSE;

	HWND hwndCombo = (HWND)lParam;
	int  i, count, idx;

	if(fTrueType)
	{
		for(i = 0; i < (sizeof(ttsizes) / sizeof(ttsizes[0])); i++)
		{
			wsprintf(ach, TEXT("%d"), ttsizes[i]);
			idx = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)ach);
			SendMessage(hwndCombo, CB_SETITEMDATA, idx, ttsizes[i]);
			//nFontSizes[i] = ttsizes[i];
		}
		//nNumFontSizes = i;
		return 0;
	}
	else
	{
		int size = LogicalToPoints(lpntme->ntmTm.tmHeight);
		wsprintf(ach, TEXT("%d"), size);

		count = (int)SendMessage(hwndCombo, CB_GETCOUNT, 0, 0);

		for(i = 0; i < count; i++)
		{
			if(SendMessage(hwndCombo, CB_GETITEMDATA, 0, 0) == size)
				break;
		}
		
		if(i >= count)
		{
			idx = (int)SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)ach);
			SendMessage(hwndCombo, CB_SETITEMDATA, idx, size);
		}

		return 1;	
	}

	return 1;
}

void InitSizeList(HWND hwndCombo, TCHAR *szFontName)
{
	LOGFONT lf = { 0 };
	HDC hdc = GetDC(hwndCombo);
	
	// get current font size
	int cursize = 10;//GetDlgItemInt(hwnd, IDC_SIZELIST, 0, 0);
		
	//int item   = SendDlgItemMessage(hwnd, IDC_FONTLIST, CB_GETCURSEL, 0, 0);
	int i, count, nearest = 0;

	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfPitchAndFamily = 0;
	lstrcpy(lf.lfFaceName, szFontName);
	//SendMessage(hwndCombo, IDC_FONTLIST, CB_GETLBTEXT, item, (LPARAM)lf.lfFaceName);

	// empty list
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

	// enumerate font sizes
	EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontSizes, (LPARAM)hwndCombo, 0);

	count = (int)SendMessage(hwndCombo, CB_GETCOUNT, 0, 0);
	
	// find the nearest item to what it was previously
	for(i = 0; i < count; i++)
	{
		int n = (int)SendMessage(hwndCombo, CB_GETITEMDATA, i, 0);

		if(n <= cursize) 
			nearest = i;
	}

	SendMessage(hwndCombo, CB_SETCURSEL, nearest, 0);

	ReleaseDC(hwndCombo, hdc);
}
