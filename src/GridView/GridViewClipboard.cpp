//
//  GridViewClipboard.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "GridViewInternal.h"

#define CF_PRI_GRIDVIEW TEXT("GridViewRow")

static DWORD GetClipboardDataBuf(TCHAR *szFormatName, PVOID pData, DWORD nLength)
{
	UINT	uFormat;
	HANDLE	hMem;
	PVOID	ptr;
	
	if((uFormat = RegisterClipboardFormat(szFormatName)) == 0)
		return FALSE;

	if((hMem = GetClipboardData(uFormat)) == 0)
		return FALSE;

	if((ptr = GlobalLock(hMem)) == 0)
		return FALSE;

	nLength = (DWORD)min(nLength, GlobalSize(hMem));

	memcpy(pData, ptr, nLength);

	GlobalUnlock(hMem);
	return nLength;
}

static BOOL SetClipboardDataBuf(TCHAR *szFormatName, PVOID pData, ULONG nLength)
{
	UINT	uFormat;
	PVOID   ptr;
	
	if((uFormat = RegisterClipboardFormat(szFormatName)) == 0)
		return FALSE;

	if((ptr = GlobalAlloc(GPTR, nLength)) == 0)
		return FALSE;

	memcpy(ptr, pData, nLength);
	SetClipboardData(uFormat, ptr);
	return TRUE;
}

static DWORD GetClipboardDword(TCHAR *szFormatName)
{
	DWORD d = 0;
	GetClipboardDataBuf(szFormatName, &d, sizeof(DWORD));
	return d;
}

static BOOL SetClipboardDword(TCHAR *szFormatName, DWORD d)
{
	return SetClipboardDataBuf(szFormatName, &d, sizeof(DWORD));
}



LRESULT GridView::OnCopy(GVRow *gvrow)
{
	if(OpenClipboard(m_hWnd))
	{
		TCHAR *str = (TCHAR *)GlobalAlloc(GPTR, 400);
		TCHAR *ptr = str;
		ULONG i;

		for(i = 0, ptr = str; i < m_nNumColumns; i++)
		{
			lstrcpy(ptr, gvrow->items[i].pszText);
			ptr += lstrlen(ptr) + 1;
		}

		*ptr = '\0';

		SetClipboardData(CF_UNICODETEXT, (HANDLE)str);
		SetClipboardDword(CF_PRI_GRIDVIEW, 1);
		CloseClipboard();
	}

	return 0;
}

LRESULT GridView::OnCut(GVRow *gvrow)
{
	OnCopy(gvrow);
	OnClear(gvrow);
	return 0;
}

LRESULT GridView::OnPaste(GVRow *gvrow, TCHAR *szText)
{
	TCHAR *ptr;
	TCHAR *newstr = _T("");
	ULONG i = 0; 
	GVITEM gvitem = { 0 };
	HANDLE hData = 0;

	if(szText == NULL && OpenClipboard(m_hWnd))
	{
		hData	= GetClipboardData(CF_UNICODETEXT);
		szText	= (TCHAR *)GlobalLock(hData);
	}
			
	gvitem.iImage		= 0;
	//gvitem.state		= GVIS_IMAGE;//|GVIF_TEXT;
	gvitem.mask			= GVIF_IMAGE;//|GVIF_STATE;
	m_pTempInsertItem	= InsertItem(gvrow, GVI_BEFORE, &gvitem);
	InvalidateRect(m_hWnd, 0, 0);
		
	if(GetClipboardDword(CF_PRI_GRIDVIEW))
	{
		for(ptr = szText; *ptr && i < m_nNumColumns; ptr += lstrlen(ptr) + 1)
		{
			GVRow *gvrow = (GVRow *)m_pTempInsertItem;
			gvrow->items[i++].pszText = _wcsdup(ptr);
		}
	}
	else
	{
		GVRow *gvrow = (GVRow *)m_pTempInsertItem;
		gvrow->items[0].pszText = _wcsdup(szText);
	}

	if(hData)
	{
		GlobalUnlock(hData);
		CloseClipboard();
	}

	return 0;
}

LRESULT GridView::OnClear(GVRow *gvrow)
{
	DeleteRow(gvrow);
	InvalidateRect(m_hWnd, 0, 0);

	if(m_nCurrentLine >= m_gvData.VisibleRows())
		m_nCurrentLine--;

	return 0;
}

