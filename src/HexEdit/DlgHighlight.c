//
//  DlgHighlight.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "ToolPanel.h"
#include "..\HexView\HexView.h"
#include "..\ConfigLib\ConfigLib.h"
#include "..\GridView\GridView.h"
#include "..\DockLib\DockLib.h"

#include "HexUtils.h"
#include "HexEdit.h"
#include "HexFile.h"
#include "trace.h"

#define IDC_HIGHLIGHT_GRIDVIEW	5678

#define IDC_HIGHLIGHT_ADD		1
#define IDC_HIGHLIGHT_EDIT		2
#define IDC_HIGHLIGHT_DELETE	3
#define IDC_HIGHLIGHT_SHOWALL	4
#define IDC_HIGHLIGHT_REPORT	5

HWND CreateEmptyToolbar(HWND hwndParent, int nBitmapIdx, int nBitmapWidth, int nCtrlId, DWORD dwExtraStyle);
void AddButton(HWND hwndTB, UINT uCmdId, UINT uImageIdx, UINT uStyle, TCHAR *szText);
int  ResizeToolbar(HWND hwndTB);
HWND HexIsOpen(HWND hwndMain, LPCTSTR szFileName, int *idx);
BOOL HexSetCurFileName(HWND hwndMain, LPCTSTR szFileName);
BOOL HexSetCurFileHwnd(HWND hwndMain, HWND);
BOOL UpdateHighlights(BOOL fAlways);

COLORREF bgcollist[] = 
{
/*	 RGB(255,255,255),	
	 RGB(0,0,0),		
	 RGB(255,255,255),	
	 RGB(128, 0, 0),	
	 RGB(0, 128,0),	
	 RGB(128,128,0),	
	 RGB(0,0,128),		
	 RGB(128,0,128),	
	 RGB(0,128,128),	
	 RGB(196,196,196),	
	 RGB(128,128,128),	
	 RGB(255,0,0),		
	 RGB(0,255,0),		
	 RGB(255,255,0),	
	 RGB(0,0,255),		
	 RGB(255,0,255),	
	 RGB(0,255,255),	*/
0xddeaee,
	0xc6d8dd,
	0x96e2fb,
	0x71dafd,
	0xcef3d2,
	0xa3e5ab,
	0xcfe0c3,
	0xb5caa6,
	
	0xfceede,
	0xfce2c7,
	0xf6dcdb,
	0xf2bfbe,
	0xfbd9ee,
	0xfcc0e5,
	0xe8e6f9,

	0xf4e6cc,
	0xe6d5b5,
	0xd4eeeb,
	0xb8dbd7,
	0xd5e2ec,
	0xbbcdda,
	0xe4e5c3,
	0xced0a4
//	{ RGB(255,255,255),	"Custom..." },
};

TCHAR *bgtextlist[] = 
{
TEXT("Automatic"),
TEXT("Black"),
TEXT("White"),
TEXT("Maroon"),
TEXT("Dark Green"),
TEXT("Olive"),
TEXT("Dark Blue"),
TEXT("Purple"),
TEXT("Aquamarine"),
TEXT("Light Grey"),
TEXT("Dark Grey"),
TEXT("Red"),
TEXT("Green"),
TEXT("Yellow"),
TEXT("Blue"),
TEXT("Magenta"),
TEXT("Cyan"),
};

BOOL MakeColourCombo(HWND hwndCombo, COLORREF crList[], TCHAR *szTextList[], int nCount);
COLORREF GetColourComboRGB(HWND hwndCombo);

void GetProgramDataPath(TCHAR *szPath, DWORD nLength)
{
	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, szPath);

	lstrcat(szPath, TEXT("\\Catch22\\HexEdit"));
}

typedef VOID (WINAPI * ENUMBOOKMARK_CALLBACK)(BOOKMARK *bm, PVOID param);
BOOL EnumHighlights(TCHAR *szBookPath, ENUMBOOKMARK_CALLBACK callback, PVOID param);

INT_PTR CALLBACK HighlightDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL fMask = FALSE;

	static HWND	hwndHV;
	size_w	offset, length;
	BOOKMARK bm = {0};

	static TCHAR title[100];
	static TCHAR text[200];
	static int count;
	static HBOOKMARK hbm;

	switch (iMsg)
	{
	case WM_INITDIALOG:

		hwndHV = GetActiveHexView(GetParent(hwnd));
		//	GetOwner(hwnd);//

		SendDlgItemMessage(hwnd, IDC_NAME, EM_SETCUEBANNER, TRUE, (LPARAM)TEXT("Enter some descriptive text here"));
		SendDlgItemMessage(hwnd, IDC_ANNOTATION, EM_SETCUEBANNER, 0, (LPARAM)TEXT("Enter some descriptive text here"));

		//MakeColourCombo(GetDlgItem(hwnd, IDC_COMBO1), fgcollist, fgtextlist, 16);
		MakeColourCombo(GetDlgItem(hwnd, IDC_COMBO4), bgcollist, bgtextlist, 16);

		hbm = (HBOOKMARK)lParam;

		if(hbm == 0)
		{
			HexView_GetSelStart(hwndHV, &offset);
			HexView_GetSelSize(hwndHV, &length);

			wsprintf(title, TEXT("bookmark-%03d"), ++count);
			//lstrcpy(title, TEXT(""));//TEXT("Enter some descriptive text here"));
			lstrcpy(text, TEXT(""));//TEXT("Enter some descriptive text here"));
		}
		else
		{
			HBOOKMARK hbm = (HBOOKMARK)lParam;
			BOOKMARK bm;

			HexView_GetBookmark(hwndHV, hbm, &bm);

			offset = bm.offset;
			length = bm.length;
			lstrcpy(title, bm.pszTitle);
			lstrcpy(text, bm.pszText);

			// Set the selected color
			for (int i = 0; i < 16; i++)
			{
				if (bm.backcol == bgcollist[i])
				{
					SendMessage(GetDlgItem(hwnd, IDC_COMBO4), CB_SETCURSEL, i, 0);
					break;
				}
			}
		}

		SetDlgItemBaseInt(hwnd, IDC_OFFSET, offset, 16, 1);
		SetDlgItemBaseInt(hwnd, IDC_LENGTH, length, 16, 1);
		SetDlgItemText(hwnd, IDC_NAME, title);
		SetDlgItemText(hwnd, IDC_ANNOTATION, text);
	
	//ClientToScreen(hwndParent, &pt);
		//GetCursorPos(&pt);
		//SetWindowPos(hwnd, 0, pt.x, pt.y, 0, 0, SWP_NOSIZE);
		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
	
			GetDlgItemText(hwnd, IDC_ANNOTATION, text, 200);
			GetDlgItemText(hwnd, IDC_NAME, title, 100);

			bm.col		= 0;
			bm.backcol	= GetColourComboRGB(GetDlgItem(hwnd, IDC_COMBO4));//RGB(rand()+128, rand()+128, rand()+128);//0xffffff;
			bm.pszText	= text;
			bm.pszTitle = title;
			bm.offset	= GetDlgItemBaseInt(hwnd, IDC_OFFSET, 16);
			bm.length	= GetDlgItemBaseInt(hwnd, IDC_LENGTH, 16);
		
			if(hbm == 0)
			{
				HexView_AddBookmark(hwndHV, &bm);
			}
			else
			{
				HexView_SetBookmark(hwndHV, hbm, &bm);
			}
//
			DockWnd_UpdateContent(g_hwndMain, DWID_HIGHLIGHT);
			///UpdateHighlights(hwndHV, hwndGridView);

			EndDialog(hwnd, TRUE);
			return 0;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return 0;

		default:
			return FALSE;
		}

	default:
		break;
	}
	return FALSE;

}


int HighlightDlg(HWND hwndHV, HBOOKMARK hBookMark)
{
	return (BOOL)DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_HIGHLIGHT), g_hwndMain, HighlightDlgProc, (LPARAM)hBookMark);
}

HWND PrepGridView2(HWND hwndParent, int ctrlId)
{
	GVCOLUMN gvcol = { 0 };
	HFONT hFont, hBold;
	HIMAGELIST hImgList;

	HWND hwndGridView;

	hwndGridView = CreateGridView(hwndParent, ctrlId, 0, WS_EX_CLIENTEDGE);


	hBold = CreateFont(-14,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0, TEXT("Segoe UI"));

	hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0, TEXT("Verdana"));
	//SendMessage(hwndGridView, WM_SETFONT, (WPARAM)hFont, 0);

	GridView_SetFont(hwndGridView, hFont, 0);
	GridView_SetFont(hwndGridView, hBold, 1);

	gvcol.xWidth = 300;
	gvcol.pszText = TEXT("Name");
	gvcol.uState = 0;//GVCS_BLENDIMAGE;
	GridView_InsertColumn(hwndGridView, 0, &gvcol);

	gvcol.hFont = hFont;
	gvcol.xWidth  = 300;
	gvcol.pszText = TEXT("Offset");
	gvcol.uState  = 0;//GVISGVCS_ALIGN_RIGHT;
	GridView_InsertColumn(hwndGridView, 1, &gvcol);

	gvcol.xWidth = 400;
	gvcol.pszText = TEXT("Annotation");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 2, &gvcol);

	//hImgList = ImageList_LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP15), 16, 0, RGB(255,0,255));
	hImgList = ImageList_LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP15), 16, 0, 
		CLR_DEFAULT, //RGB(255,0,255), 
		IMAGE_BITMAP, 
		
		LR_CREATEDIBSECTION );//
		//|		LR_LOADTRANSPARENT);
	GridView_SetImageList(hwndGridView, hImgList);

	return hwndGridView;
}

void FillGridView2(HWND hwndGV, HGRIDITEM hGridItem, HBOOKMARK hbm, BOOKMARK *bm, COLORREF col)
{
	GVITEM gvitem = { 0 };
	HGRIDITEM hItem;

	TCHAR szOffset[32];
	_stprintf(szOffset, TEXT("%08I64X  (%d bytes)"), bm->offset, (int)bm->length);

	gvitem.pszText	= bm->pszTitle;
	gvitem.iSubItem = 0;
	gvitem.iImage	= 0;
	gvitem.color	= col;
	gvitem.state	= GVIS_READONLY;
	gvitem.mask		= GVIF_PARAM|GVIF_TEXT|GVIF_STATE|GVIF_IMAGE|GVIF_COLOR;
	gvitem.param	= (UINT64)hbm;

	
	hItem = GridView_InsertChild(hwndGV, hGridItem, &gvitem);

	gvitem.pszText	= szOffset;
	gvitem.iSubItem = 1;
	gvitem.state	= GVIS_READONLY;
	gvitem.mask		= GVIF_PARAM|GVIF_TEXT|GVIF_STATE|GVIF_COLOR;
	GridView_SetItem(hwndGV, hItem, &gvitem);

	gvitem.pszText	= bm->pszText;
	gvitem.iSubItem = 2;
	gvitem.state	= GVIS_READONLY;
	gvitem.mask		= GVIF_PARAM|GVIF_TEXT|GVIF_STATE|GVIF_COLOR;
	GridView_SetItem(hwndGV, hItem, &gvitem);
}


BOOL GetBookmarkFileName(HWND hwndHexView, LPCTSTR szFileName, TCHAR *szBookPath)
{
	TCHAR szBase[MAX_PATH] = {0};
	TCHAR szAppData[MAX_PATH];
	TCHAR *pszName;
	UINT64 bookid;
	
	if(szFileName == 0)
	{
		szFileName = szBase;

		if(!HexView_GetFileName(hwndHexView, szBase, MAX_PATH))
			return FALSE;
	}

	// has this file got a bookmark id already??
	if(LoadFileData(szFileName, TEXT("HexEdit.bookmark"), &bookid, sizeof(UINT64)) != sizeof(UINT64))
	{
		//BY_HANDLE_FILE_INFORMATION bhfi;
		//HANDLE hFile = CreateFile(szBase, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

		//if(hFile == INVALID_HANDLE_VALUE)
		//	return FALSE;

		// generate one from the base NTFS fileid
		//GetFileInformationByHandle(hFile, &bhfi);
		//CloseHandle(hFile);

		bookid = GetTickCount();//(UINT64)bhfi.nFileIndexLow | ((UINT64)bhfi.nFileIndexHigh << 32);

		// only save if the file exists
		if(GetFileAttributes(szFileName) != INVALID_FILE_ATTRIBUTES)
		{
			SaveFileData(szFileName, TEXT("HexEdit.bookmark"), &bookid, sizeof(UINT64));
		}
	}

	if((pszName = _tcsrchr(szFileName, '\\')) == 0)
		return FALSE;

	// get location of application data

	//
	// get/create the following directory:
	//
	//	{ApplicationData}\Local\Catch22\Bookmarks 
	//
	GetProgramDataPath(szAppData, MAX_PATH);
	lstrcat(szAppData, TEXT("\\Bookmarks"));
	SHCreateDirectory(NULL, szAppData);

	//
	//	Formulate the bookmark filename
	//
	wsprintf(szBookPath, TEXT("%s\\%s-%08x-bookmark.xml"), szAppData, pszName+1, bookid);
	//lstrcat(szBookPath, pszName+1);
	//lstrcat(szBookPath, TEXT(".bookmark"));

	return TRUE;
}

BOOL GetConfigBookmark(HCONFIG hBookmark, BOOKMARK *bm)
{
	UINT64 a, b;

	GetConfigStr(hBookmark, TEXT("name"), bm->pszTitle, 100, TEXT(""));
	GetConfigStr(hBookmark, TEXT("desc"), bm->pszText, 100, TEXT(""));
	GetConfigI64(hBookmark, TEXT("offset"), &a, 0);
	GetConfigI64(hBookmark, TEXT("length"), &b, 0);
	GetConfigH32(hBookmark, TEXT("fgcol"), &bm->col, 0);
	GetConfigH32(hBookmark, TEXT("bgcol"), &bm->backcol, 0);
				
	bm->offset	= a;
	bm->length	= b;
	return TRUE;
}

BOOL EnumHighlights(TCHAR *szBookPath, ENUMBOOKMARK_CALLBACK callback, PVOID param)
{
	HCONFIG hConfig, hFileData, hBookmarks, bookmark;
	int i;

	if((hConfig = OpenConfig(szBookPath)) == 0)
	{
		return FALSE;
	}

	hFileData = OpenConfigSection(hConfig, TEXT("hexFileData"));

	hBookmarks = OpenConfigSection(hFileData, TEXT("bookmarks"));

	for(i = 0; bookmark = EnumConfigSection(hBookmarks, TEXT("bookmark"), i); i++)
	{
		TCHAR name[100], desc[100];
		BOOKMARK bm = { 0, 0, 0, 0, 0, name, 100, desc, 100 };

		GetConfigBookmark(bookmark, &bm);

		callback(&bm, param);	
	}

	return TRUE;
}

VOID WINAPI AddBookmarkCallback(BOOKMARK *bm, PVOID hwndHexView)
{
	HexView_AddBookmark((HWND)hwndHexView, bm);
}

BOOL LoadHighlights(HWND hwndHexView)
{
	TCHAR szBookPath[MAX_PATH];

	if(!GetBookmarkFileName(hwndHexView, NULL, szBookPath))
		return FALSE;

	HexView_ClearBookmarks(hwndHexView);
	EnumHighlights(szBookPath, AddBookmarkCallback, hwndHexView);

	return TRUE;
}

BOOL SaveHighlights(HWND hwndHexView)
{
	TCHAR szBookPath[MAX_PATH];
	TCHAR szFilePath[MAX_PATH];
	BOOKMARK bm;
	HBOOKMARK hbm;
	HCONFIG config, filedata, bookmarks;

	GetBookmarkFileName(hwndHexView, NULL, szBookPath);

	config	  = CreateConfig();
	filedata  = CreateConfigSection(config, TEXT("hexFileData"));

	HexView_GetFileName(hwndHexView, szFilePath, MAX_PATH);

	// only save if the file exists
	if(GetFileAttributes(szFilePath) != INVALID_FILE_ATTRIBUTES)
	{
		SetConfigStr(filedata, TEXT("filePath"), szFilePath);
	}

	bookmarks = CreateConfigSection(filedata, TEXT("bookmarks"));

	for(hbm = 0; (hbm = HexView_EnumBookmark(hwndHexView, hbm, &bm)) != 0; )
	{
		if((bm.flags & HVBF_NOPERSIST) == 0)
		{
			HCONFIG bookmark = CreateConfigSection(bookmarks, TEXT("bookmark"));

			SetConfigStr(bookmark, TEXT("name"), bm.pszTitle);
			SetConfigStr(bookmark, TEXT("desc"), bm.pszText);
			SetConfigI64(bookmark, TEXT("offset"), bm.offset);
			SetConfigI64(bookmark, TEXT("length"), bm.length);
			SetConfigH32(bookmark, TEXT("fgcol"), bm.col);
			SetConfigH32(bookmark, TEXT("bgcol"), bm.backcol);
		}
	}

	SaveConfig(szBookPath, config);

	return TRUE;
}

void DeleteBookmarkFromConfig(LPCTSTR szFilename, LPCTSTR szBookmarkName)
{
	WIN32_FIND_DATA win32fd;
	HANDLE hFind;
	TCHAR szBookPath[MAX_PATH];
	TCHAR szFilePath[MAX_PATH];
	TCHAR szRefFilePath[MAX_PATH] = { 0 };

	GetProgramDataPath(szBookPath, MAX_PATH);
	lstrcat(szBookPath, TEXT("\\Bookmarks\\*"));

	// enumerate all files in the bookmarks directory
	if ((hFind = FindFirstFile(szBookPath, &win32fd)) != 0)
	{
		do
		{
			if ((win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				GetProgramDataPath(szBookPath, MAX_PATH);
				wsprintf(szFilePath, TEXT("%s\\Bookmarks\\%s"), szBookPath, win32fd.cFileName);

				HCONFIG configFile = OpenConfig(szFilePath);
				if (configFile)
				{
					if (GetConfigStr(configFile, TEXT("hexFileData\\filePath"), szRefFilePath, MAX_PATH, 0) && _tcscmp(szRefFilePath, szFilename) == 0)
					{
						HCONFIG bookmarksSection = OpenConfigSection(configFile, TEXT("hexFileData\\bookmarks"));
						DWORD idx = 0;
						HCONFIG bookmark = 0;
						DWORD countedBookmarks = 0;
						while (bookmark = EnumConfigSection(bookmarksSection, TEXT("bookmark"), idx++))
						{
							TCHAR title[100];
							BOOKMARK bm = { 0 };
							bm.pszTitle = title;
							bm.nMaxTitle = ARRAYSIZE(title);

							if (GetConfigBookmark(bookmark, &bm) && _tcscmp(title, szBookmarkName) == 0)
							{
								DeleteConfigSection(bookmark);
							}
							else
							{
								countedBookmarks++;
							}
						}

						SaveConfig(szFilePath, configFile);
						CloseConfig(configFile);
						if (countedBookmarks == 0)
						{
							DeleteFile(szFilePath);
						}
						break;
					}
					CloseConfig(configFile);
				}
			}

		} while (FindNextFile(hFind, &win32fd));

		FindClose(hFind);
	}
}

LRESULT CALLBACK HighlightViewCommandHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern HWND g_hwndMain;

	HWND hwndHV = GetActiveHexView(g_hwndMain);

	if (msg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case IDC_HIGHLIGHT_ADD:
			HighlightDlg(NULL, NULL);
			break;
		case IDC_HIGHLIGHT_EDIT:
		{
			HWND hwndGridView = GetDlgItem(hwnd, IDC_HIGHLIGHT_GRIDVIEW);
			ULONG curSelItem = GridView_GetCurSel(hwndGridView);
			HGRIDITEM hgCurSelItem = GridView_GetItemHandle(hwndGridView, curSelItem);
			GVITEM curItem = { GVIF_PARAM, curSelItem, 0 };

			// Get the current item - param contains the HBOOKMARK for this item
			if (GridView_GetItem(hwndGridView, hgCurSelItem, &curItem) && curItem.param)
			{
				HBOOKMARK hbm = (HBOOKMARK)curItem.param;

				// Get the parent item - param contains the HWND for the hexview
				if (GridView_GetParentItem(hwndGridView, hgCurSelItem, &curItem) && curItem.param)
				{
					HWND hwndHexView = (HWND)curItem.param;

					HighlightDlg(hwndHexView, hbm);
				}
			}

			break;
		}
		case IDC_HIGHLIGHT_DELETE:
		{
			TCHAR itemText[100] = { 0 };
			HWND hwndGridView = GetDlgItem(hwnd, IDC_HIGHLIGHT_GRIDVIEW);
			ULONG curSelItem = GridView_GetCurSel(hwndGridView);
			HGRIDITEM hgCurSelItem = GridView_GetItemHandle(hwndGridView, curSelItem);
			GVITEM curItem = { GVIF_PARAM | GVIF_TEXT, curSelItem, 0 };
			curItem.pszText = itemText;
			curItem.cchTextMax = ARRAYSIZE(itemText);

			// Get the current item - param contains the HBOOKMARK for this item
			if (GridView_GetItem(hwndGridView, hgCurSelItem, &curItem))
			{
				if (curItem.param)
				{
					HBOOKMARK hbm = (HBOOKMARK)curItem.param;

					// Get the parent item - param contains the HWND for the hexview
					if (GridView_GetParentItem(hwndGridView, hgCurSelItem, &curItem) && curItem.param)
					{
						HWND hwndHexView = (HWND)curItem.param;

						GridView_DeleteItem(hwndGridView, hgCurSelItem);
						HexView_DelBookmark(hwndHexView, hbm);
						GridView_Update(hwndGridView);
					}
				}
				else
				{
					// This wasn't a live bookmark - it exists in a config file
					TCHAR parentText[100] = { 0 };
					curItem.pszText = parentText;
					if (GridView_GetParentItem(hwndGridView, hgCurSelItem, &curItem))
					{
						DeleteBookmarkFromConfig(parentText, itemText);
						GridView_DeleteAll(hwndGridView);
						UpdateHighlights(TRUE);
					}
				}
			}

			break;
		}
		}
		return 0;
	}

	if(msg == WM_NOTIFY)
	{
		NMGRIDVIEW *nmgv = (NMGRIDVIEW *)lParam;

		if(nmgv->hdr.code == GVN_DBLCLK)
		{
			GVITEM gvi = { GVIF_PARAM, nmgv->iItem, 0 };//nmgv->nColumn };

			if(GridView_GetParent(nmgv->hdr.hwndFrom, nmgv->hItem))
			{
				if(GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi))
				{
					// file is not open yet!
					if(gvi.param == 0)
					{
						TCHAR szFileName[MAX_PATH];
						gvi.mask |= GVIF_TEXT;
						gvi.pszText = szFileName;
						gvi.cchTextMax = MAX_PATH;
						
						if(GridView_GetParentItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi))
						{
							HexOpenFile(g_hwndMain, szFileName, DefaultFileMode());
						}
					}
					else
					{
						HighlightDlg(hwndHV, (HBOOKMARK)gvi.param);
					}
				}
			}
		}
		else if(nmgv->hdr.code == GVN_DELETED)
		{
			GVITEM gvi = { GVIF_PARAM, nmgv->iItem, 0 };//nmgv->nColumn };
			//HIGHLIGHT_PARAM hp;

			// get current item in gridview
			GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);

			HexView_DelBookmark(hwndHV, (HBOOKMARK)gvi.param);
			return 0;
		}
		else if(nmgv->hdr.code == GVN_SELCHANGED)
		{
			GVITEM gvi = { GVIF_PARAM, nmgv->iItem, 0 };//nmgv->nColumn };
			BOOKMARK bm;

			// get the gvi.param - contains the HBOOKMARK for this bookmark entry
			if(GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi) && gvi.param)
			{
				HBOOKMARK hbm = (HBOOKMARK)gvi.param;

				// get parent item's param - contains HWND to the hexview
				if(GridView_GetParentItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi) && gvi.param)
				{
					// make sure the current file is active
					hwndHV = (HWND)gvi.param;
					HexSetCurFileHwnd(g_hwndMain, hwndHV);

					if(HexView_GetBookmark(hwndHV, hbm, &bm))
					{
						HexView_SetCurPos(hwndHV, bm.offset + bm.length);
						HexView_SetSelStart(hwndHV, bm.offset);
						HexView_SetSelEnd(hwndHV, bm.offset + bm.length);
						HexView_ScrollTo(hwndHV,  bm.offset + bm.length);
						InvalidateRect(hwndHV,0,0);
					}
				}
			}
			return 0;
		}
	}

	return 0;
}

HWND CreateHighlightView(HWND hwndParent)
{
	HWND hwndPanel;
	HWND hwndGridView;
	HWND hwndTB1;

//	RegisterGridView();

	InitGridView();

	//
	//	Create the base tool panel
	//
	hwndPanel = ToolPanel_Create(hwndParent, HighlightViewCommandHandler);
	//ToolPanel_AddVSpace(hwndPanel, 4);


	ToolPanel_AddGripper(hwndPanel);

	
	//
	//	Create the 1st toolbar (the "Goto" button)
	//
	hwndTB1   = CreateEmptyToolbar(hwndPanel, IDB_BITMAP11, 16, 666, TBSTYLE_LIST);
	SendMessage(hwndTB1, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

	GetWindowWidth(hwndTB1);
	AddButton(hwndTB1, IDC_HIGHLIGHT_ADD, 0, TBSTYLE_BUTTON, _T("Add"));
	AddButton(hwndTB1, IDC_HIGHLIGHT_EDIT, 3, TBSTYLE_BUTTON, _T("Edit"));
	AddButton(hwndTB1, IDC_HIGHLIGHT_DELETE, 1, TBSTYLE_BUTTON, _T("Delete"));
	AddButton(hwndTB1, -0, 0, TBSTYLE_SEP, 0);
	AddButton(hwndTB1, IDC_HIGHLIGHT_SHOWALL, 2, TBSTYLE_BUTTON|TBSTYLE_CHECK, _T("Show All"));
	AddButton(hwndTB1, IDC_HIGHLIGHT_REPORT, 4, TBSTYLE_BUTTON, _T("Create Report"));
	ResizeToolbar(hwndTB1);
	ToolPanel_AddItem(hwndPanel, hwndTB1, 0);
	ToolPanel_AddNewLine(hwndPanel, 4);

	
	

	//
	//	Create the gridview!!
	//
	hwndGridView = PrepGridView2(hwndPanel, IDC_HIGHLIGHT_GRIDVIEW);
	
	GridView_SetStyle(hwndGridView, -1, GVS_READONLY|GVS_FULLROWSELECT|GVS_VERTGRIDLINES//|GVS_TREELINES
		//|GVS_SHOWFOCUS
		);//,GVS_FULLROWSELECT|GVS_GRIDLINES);

	
	
	//UpdateHighlights(g_hwndHexView, hwndGridView);
	//UpdateHighlights((HWND)-1, hwndGridView);

	ToolPanel_AddItem(hwndPanel, hwndGridView, 0);
	ToolPanel_AddAnchor(hwndPanel, 0, 2);

	ToolPanel_AutoSize(hwndPanel);
	SetWindowHeight(hwndPanel, 200, NULL);

	ShowWindow(hwndPanel, SW_SHOW);
	return hwndPanel;
}

HWND GetBookmarkWnd()
{
	HWND hwndBookMark = //GetWindow(hwndBookMark, GW_CHILD);
		DockWnd_GetContents(g_hwndMain, DWID_HIGHLIGHT);
			
//				(HWND)-1,//hwndHV, 
//				GetDlgItem(hwndBookMark, IDC_HIGHLIGHT_GRIDVIEW));
	return hwndBookMark;

}

//
//	Add hexview's bookmarks to the gridview
//
void FillHexViewHighlights(HGRIDITEM hRoot, HWND hwndHexView, HWND hwndGridView, COLORREF col)
{
	BOOKMARK bm;
	HBOOKMARK hbm;

	for(hbm = 0; (hbm = HexView_EnumBookmark(hwndHexView, hbm, &bm)) != 0; )
	{
		FillGridView2(hwndGridView, hRoot, hbm, &bm, col);
	}
}

void MkRootGVITEM(GVITEM *gvitem, LPCTSTR szText, BOOL fBold, PVOID param)
{
	ZeroMemory(gvitem, sizeof(GVITEM));
	gvitem->pszText		= (WCHAR *)szText;
	gvitem->iSubItem	= 0;
	gvitem->iImage		= 1;
	gvitem->state		= GVIS_READONLY|GVIS_IMAGEIDX|GVS_SMEGHEAD;
	gvitem->mask		= GVIF_PARAM|GVIF_TEXT|GVIF_STATE|GVIF_IMAGE|GVIF_FONT;
	gvitem->iFont		= fBold ? 1 : 0;
	gvitem->param		= (UINT64)param;
}

//
//	Add hexview's filename + bookmarks to the gridview
//
void AddHexViewHighlights(HGRIDITEM hRoot, HWND hwndHexView, HWND hwndGridView, COLORREF col)
{
	GVITEM gvitem = { 0 };
	HGRIDITEM hItem;
	TCHAR szFileName[MAX_PATH];

	HexView_GetFileName(hwndHexView, szFileName, MAX_PATH);

	MkRootGVITEM(&gvitem, szFileName, TRUE, 0);	
	hItem = GridView_InsertChild(hwndGridView, hRoot, &gvitem);
	
	FillHexViewHighlights(hItem, hwndHexView, hwndGridView, col);
}

HGRIDITEM CreateBookmarkFileRoot(HGRIDITEM hRoot, LPCTSTR szFileName, HWND hwndGridView, BOOL fBold)
{
	GVITEM gvitem = { 0 };
	HGRIDITEM hItem;

	MkRootGVITEM(&gvitem, szFileName, fBold, 0);	
	hItem = GridView_InsertChild(hwndGridView, hRoot, &gvitem);

	return hItem;
}

BOOL UpdateHighlightsFromConfig(HGRIDITEM hRoot, HWND hwndGridView, TCHAR *szBookPath, COLORREF col)
{
	HCONFIG hConf, hBookmarks;
	HBOOKMARK bookmark;
	size_t i;
	TCHAR szFilePath[MAX_PATH];
	GVITEM gvitem = { 0 };

	hConf = OpenConfig(szBookPath);

	if(hConf)
	{
		if(GetConfigStr(hConf, TEXT("hexFileData\\filePath"), szFilePath, MAX_PATH, 0))
		{
			hBookmarks = OpenConfigSection(hConf, TEXT("hexFileData\\bookmarks"));

			for(i = 0; bookmark = EnumConfigSection(hBookmarks, TEXT("bookmark"), (int)i); i++)
			{
				TCHAR name[100], desc[100];
				BOOKMARK bm = { 0, 0, 0, 0, 0, name, 100, desc, 100 };

				// do we need to add a root item?
				if(hRoot == 0)
				{
					GVITEM gvitem = { GVIF_TEXT };
					gvitem.pszText = szFilePath;
					hRoot = GridView_FindChild(hwndGridView, GVI_ROOT, &gvitem);

					if (!hRoot)
						hRoot = CreateBookmarkFileRoot(GVI_ROOT, szFilePath, hwndGridView, FALSE);

					gvitem.mask  = GVIF_PARAM|GVIF_FONT;
					gvitem.iFont = 0; // regular
					gvitem.param = 0;
					GridView_SetItem(hwndGridView, hRoot, &gvitem);
					GridView_DeleteChildren(hwndGridView, hRoot);	
			
					MkRootGVITEM(&gvitem, szFilePath, FALSE, 0);	
					GridView_SetItem(hwndGridView, hRoot, &gvitem);
				}

				GetConfigBookmark(bookmark, &bm);

				FillGridView2(hwndGridView, hRoot, NULL, &bm, col);
						//callback(&bm, param);	
			}

		}

		CloseConfig(hConf);
	}

	return TRUE;
}

VOID WINAPI addgv(BOOKMARK *bm, PVOID hwndGridView)
{
//	FillGridView2(hwndGridView, 
}

BOOL UpdateHighlights2(HWND hwndHexView, HWND hwndGridView)
{

	HGRIDITEM hGridItem;
	hGridItem = GVI_ROOT;//GridView_InsertChild(hwndGridView, GVI_ROOT, &gvitem);

	SendMessage(hwndGridView, WM_SETREDRAW, FALSE, 0);
	GridView_DeleteAll(hwndGridView);

	if(hwndHexView == 0)
	{
		// just add bookmarks for specified hexview
		FillHexViewHighlights(GVI_ROOT, hwndHexView, hwndGridView, 0);
	}
	else if(hwndHexView != (HWND)-1)
	{
		int i;

		// add bookmarks for all open files
		for(i = 0; (hwndHexView = EnumHexView(g_hwndMain, i)) != 0; i++)
		{
			AddHexViewHighlights(GVI_ROOT, hwndHexView, hwndGridView, 0);
		}
	}
	else 
	{
		WIN32_FIND_DATA win32fd;
		HANDLE hFind;
		TCHAR szBookPath[MAX_PATH];
		TCHAR szFilePath[MAX_PATH];
		HCONFIG hConf ;

		GetProgramDataPath(szBookPath, MAX_PATH);
		lstrcat(szBookPath, TEXT("\\Bookmarks\\*"));

		// enumerate all files in the bookmarks directory
		if((hFind = FindFirstFile(szBookPath, &win32fd)) != 0)
		{
			do 
			{
				if((win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					HWND hwndHV; 
					int i;
					HCONFIG hBookmarks, bookmark;
					HGRIDITEM hItem;

					GetProgramDataPath(szBookPath, MAX_PATH);
					wsprintf(szFilePath, TEXT("%s\\Bookmarks\\%s"), szBookPath, win32fd.cFileName);				

					hConf = OpenConfig(szFilePath);
					GetConfigStr(hConf, TEXT("hexFileData\\filePath"), szFilePath, MAX_PATH, 0);

					if((hwndHV = HexIsOpen(g_hwndMain, szFilePath, 0)) != 0)
					{
						AddHexViewHighlights(GVI_ROOT, hwndHV, hwndGridView, 0);
					}
					else
					{
						hItem = CreateBookmarkFileRoot(GVI_ROOT, szFilePath, hwndGridView, FALSE);
					//EnumHighlights(szBookPath, addgv, hwndGridView);

						hBookmarks = OpenConfigSection(hConf, TEXT("hexFileData\\bookmarks"));

						for(i = 0; bookmark = EnumConfigSection(hBookmarks, TEXT("bookmark"), i); i++)
						{
							TCHAR name[100], desc[100];
							BOOKMARK bm = { 0, 0, 0, 0, 0, name, 100, desc, 100 };

							GetConfigBookmark(bookmark, &bm);

							FillGridView2(hwndGridView, hItem, NULL, &bm, 0);
						//callback(&bm, param);	
						}
					}

					CloseConfig(hConf);
				}

			} while(FindNextFile(hFind, &win32fd));

			FindClose(hFind);
		}


	}

/*	for(hbm = 0; (hbm = HexView_EnumBookmark(hwndHexView, hbm, &bm)) != 0; )
	{
		TCHAR szOffset[32];
		TCHAR szTitle[32];

		_stprintf(szTitle, TEXT("%s"), bm.pszTitle);
		_stprintf(szOffset, TEXT("%08I64X  (%d bytes)"), bm.offset, bm.length);

		FillGridView2(hwndGridView, hGridItem, hbm, szTitle, szOffset, bm.pszText);
	}*/

	SendMessage(hwndGridView, WM_SETREDRAW, TRUE, 0);
	GridView_Update(hwndGridView);

	//ForceClientResize(hwndGridView);
	//InvalidateRect(hwndGridView, 0, 0);

	return TRUE;
}

BOOL UpdateHighlight(LPCTSTR pszFilePath, TCHAR * pszBookPath, BOOL fAlways)
{
	HWND hwndGV;
	HWND hwndHV;

	HGRIDITEM hItem  = 0;  
	GVITEM    gvitem = { GVIF_TEXT };

	// get the bookmark window's gridview
	if ((hwndGV = GetDlgItem(GetBookmarkWnd(), IDC_HIGHLIGHT_GRIDVIEW)) == 0)
		return FALSE;

	// find the gridview item for specified filename
	// (search for the matching filename)
	if(pszFilePath)
	{
		gvitem.pszText = (WCHAR *)pszFilePath;
		hItem = GridView_FindChild(hwndGV, NULL, &gvitem);
	}

	// file already open?
	if((hwndHV = HexIsOpen(g_hwndMain, pszFilePath, 0)) != 0)
	{
		// yes - update bookmarks from the hexview
		if(hItem == 0)
			hItem = CreateBookmarkFileRoot(GVI_ROOT, pszFilePath, hwndGV, TRUE);

		gvitem.mask  = GVIF_PARAM|GVIF_FONT;
		gvitem.iFont = 1; // bold
		gvitem.param = (UINT64)hwndHV;
		GridView_SetItem(hwndGV, hItem, &gvitem);

		GridView_DeleteChildren(hwndGV, hItem);
		FillHexViewHighlights(hItem, hwndHV, hwndGV, RGB(0,0,0));
		//		UpdateHighlights(
	}
	else if(fAlways)
	{
		// no - update bookmarks from the bookmark file
		TCHAR szBookPath[MAX_PATH] = { 0 };

		if(pszBookPath == 0)
		{
			pszBookPath = szBookPath;
			GetBookmarkFileName(0, pszFilePath, pszBookPath);
		}

		//if(hItem == 0 && pszFilePath)
		//	hItem = CreateBookmarkFileRoot(GVI_ROOT, pszFilePath, hwndGV, FALSE);

		UpdateHighlightsFromConfig(hItem, hwndGV, pszBookPath, RGB(128, 128, 128));

		InvalidateRect(hwndGV, 0, 0);
	}
	else 
	{
		if(hItem)
			GridView_DeleteItem(hwndHV, hItem);
	}
	//UpdateHighlights(

	return TRUE;
}


BOOL UpdateHighlights(BOOL fAlways)
{
	HWND hwndGridView;

	if ((hwndGridView = GetDlgItem(GetBookmarkWnd(), IDC_HIGHLIGHT_GRIDVIEW)) == 0)
		return FALSE;

	SendMessage(hwndGridView, WM_SETREDRAW, FALSE, 0);

	if(fAlways)
	{
		WIN32_FIND_DATA win32fd;
		HANDLE hFind;
		TCHAR szBookPath[MAX_PATH];
		TCHAR szFilePath[MAX_PATH];

		GetProgramDataPath(szBookPath, MAX_PATH);
		lstrcat(szBookPath, TEXT("\\Bookmarks\\*"));

		// enumerate all files in the bookmarks directory
		if((hFind = FindFirstFile(szBookPath, &win32fd)) != 0)
		{
			do 
			{
				if((win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					GetProgramDataPath(szBookPath, MAX_PATH);
					wsprintf(szFilePath, TEXT("%s\\Bookmarks\\%s"), szBookPath, win32fd.cFileName);				

					UpdateHighlight(0, szFilePath, TRUE);
				}

			} while(FindNextFile(hFind, &win32fd));

			FindClose(hFind);
		}
	}
	//else
	{
		int i;
		HWND hwndHexView;

		// add bookmarks for all open files
		for(i = 0; (hwndHexView = EnumHexView(g_hwndMain, i)) != 0; i++)
		{
			TCHAR szFilePath[MAX_PATH];
			HexView_GetFileName(hwndHexView, szFilePath, MAX_PATH);
			UpdateHighlight(szFilePath, 0, FALSE);
		}
	}

	SendMessage(hwndGridView, WM_SETREDRAW, TRUE, 0);
	return TRUE;
}