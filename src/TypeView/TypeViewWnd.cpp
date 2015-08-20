//
//  TypeViewWnd.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#include "..\TypeLib\parser.h"

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>

#include "..\HexEdit\resource.h"
#include "..\HexEdit\FileChange.h"
#include "..\HexEdit\HexEdit.h"
#include "..\HexEdit\HexUtils.h"
#include "..\HexEdit\RegLib.h"
#include "..\DockLib\DockLib.h"
#include "..\HexEdit\TabView.h"
#include "..\HexEdit\ToolPanel.h"
#include "..\GridView\GridView.h"
#include "..\HexView\HexView.h"

#include "TypeView.h"

#include "trace.h"


#define IDC_TYPEVIEW_TOOLBAR	1
#define IDC_TYPEVIEW_REFRESH	3
#define IDC_TYPEVIEW_SAVE		4
#define IDC_TYPEVIEW_ADDRESS	5
#define IDC_TYPEVIEW_TYPECOMBO	6
#define IDC_TYPEVIEW_GRIDVIEW	7
#define IDC_TYPEVIEW_PIN		8

extern "C" void Initialize();
extern vector <TypeDecl *> globalTypeDeclList;
size_w FmtData(HWND hwndGV, HGRIDITEM hRoot, Type *type, size_w dwOffset, TypeDecl *typeDecl);
size_w InsertTypeGV(HWND hwndGridView, HGRIDITEM hRoot, TypeDecl *typeDecl, size_w dwOffset);
size_w InsertStructType(HWND hwndGV, HGRIDITEM hRoot, size_w dwOffset, Type *ty);

bool Evaluate(HGRIDITEM hParent, ExprNode *expr, INUMTYPE *result, DWORD baseOffset, HWND hwndHV, HWND hwndGV);
extern bool fShowFullType;
size_w SizeOf(Type *type, size_w offset, HWND hwndHV);
extern vector <FILE_DESC*>	globalFileHistory;
extern "C" {
HWND g_hwndMain;
extern BOOL g_fDisplayHex;
extern BOOL g_fDisplayBigEndian;

void Dump(FILE *fp);


HWND CreateEmptyToolbar(HWND hwndParent, int nBitmapIdx, int nBitmapWidth, int nCtrlId, DWORD dwExtraStyle);
void AddButton(HWND hwndTB, UINT uCmdId, UINT uImageIdx, UINT uStyle, TCHAR *szText);
int  ResizeToolbar(HWND hwndTB);
//HWND CreateChild(DWORD dwStyleEx, DWORD dwStyle, TCHAR *szClass, HWND hwndParent);
HWND CreateChild(DWORD dwStyleEx, DWORD dwStyle, TCHAR *szClass, HWND hwndParent, UINT nCommandId);

HWND PrepGridView(HWND hwndGridView, int widthArray[], TCHAR *szTypeName);
HWND PrepAllTypes(HWND hwndGridView, int widthArray[]);
void UpdateAllTypesGridView(HWND hwndGV, size_w baseOffset);
HWND GetActiveHexView(HWND hwndMain);
void UpdateTypeGridView(HWND hwndGridView, size_w baseOffset, char *typeName);
void UpdateTypeView();

DWORD SetTypeGV(HWND hwndGridView, HGRIDITEM hItem, TypeDecl *typeDecl, DWORD offset);
}

void FragEdit(HWND hwndEdit)
{
	RECT rect = { 2,2,2,2 };
	SendMessage(hwndEdit, EM_GETRECT, 0, (LPARAM)&rect);
	InflateRect(&rect, 1,1);
	SendMessage(hwndEdit, EM_SETRECT, 0, (LPARAM)&rect);
}

int GetEditFontHeight(HWND hwndEdit)
{
	HFONT		hFont;
	HDC			hdc;
	HANDLE		hOld;
	TEXTMETRIC	tm;
	int			height;

	// get the font for the edit control
	hFont	= (HFONT)SendMessage(hwndEdit, WM_GETFONT, 0, 0);

	hdc		= GetDC(hwndEdit);
	hOld	= SelectObject(hdc, hFont);

	GetTextMetrics(hdc, &tm);

	height = tm.tmHeight + tm.tmExternalLeading + GetSystemMetrics(SM_CYEDGE) * 2 + 3;
	//SetWindowSize(hwndEdit, GetWindowWidth(hwndEdit), tm.tmHeight + tm.tmExternalLeading + GetSystemMetrics(SM_CYEDGE) * 2;

	SelectObject(hdc, hOld);
	ReleaseDC(hwndEdit, hdc);

	return height;
}

INT_PTR CALLBACK TypeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		CenterWindow(hwnd);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return 0;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;

		}
		return 0;
	}
	return FALSE;
}

HBOOKMARK SetTypeViewBookmark(HWND hwndHexView, size_w offset, size_w length, COLORREF fgcol, COLORREF bgcol)
{
	HBOOKMARK hbm = (HBOOKMARK)GetProp(hwndHexView, TEXT("TypeViewHBOOKMARK"));

	BOOKMARK bm = 
	{ 
		HVBF_NOENUM | HVBF_NOPERSIST | HVBF_NOBOOKNOTE,
		offset, 
		length,
		fgcol,
		bgcol
	};

	if(hbm == 0)
	{
		hbm = HexView_AddBookmark(hwndHexView, &bm);
		SetProp(hwndHexView, TEXT("TypeViewHBOOKMARK"), (HANDLE)hbm);
	}
	else
	{
		HexView_SetBookmark(hwndHexView, hbm, &bm);
	}
	
	return hbm;
}

LONG OnGridViewSelChanged(HWND hwnd, UINT msg, NMGRIDVIEW *nmgv)
{
	HWND hwndHV = GetActiveHexView(g_hwndMain);
	
	GVITEM gvi = { nmgv->iItem, 2 };
	size_w offset = 0;

		// get the offset of this item from the gridview item
	gvi.iSubItem	= COLIDX_OFFSET;
	gvi.mask		= GVIF_PARAM;
	GridView_GetParentItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
	offset			= gvi.param;
	
	gvi.iSubItem	= COLIDX_NAME;
	GridView_GetParentItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
	
	SetTypeViewBookmark(hwndHV, offset, SizeOf((Type *)gvi.param, offset, 0), RGB(0,0,0), 0xFCD2B4);
	
	gvi.iSubItem = COLIDX_OFFSET;

	GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
	offset			= gvi.param;
	
	gvi.iSubItem = COLIDX_NAME;
	GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);

	size_w s = SizeOf((Type *)gvi.param, 0, 0);
	HexView_SetCurSel(hwndHV, offset, offset+s);
	
	return 0;
}

bool Evaluate(HGRIDITEM hParent, ExprNode *expr, size_w baseOffset, HWND hwndHV, HWND hwndGV, ExprNode *result);

LONG OnGridViewDataChanged(HWND hwnd, UINT msg, NMGRIDVIEW *nmgv)
{
	TCHAR szTextBuf[200];
	GVITEM gvi		 = { nmgv->iItem, 0 };

	size_w offset;
	Type *type;

	HWND hwndGridView = nmgv->hdr.hwndFrom;
	
	// get the text that was entered into the data field
	gvi.pszText			= szTextBuf;
	gvi.cchTextMax		= 200;
	gvi.mask		= GVIF_TEXT;
	gvi.iSubItem		= COLIDX_DATA;
	GridView_GetItem(hwndGridView, nmgv->hItem, &gvi);

	// get the offset that we will insert the data at
	gvi.mask		= GVIF_PARAM;
	gvi.iSubItem		= COLIDX_OFFSET;
	GridView_GetItem(hwndGridView, nmgv->hItem, &gvi);
	offset = gvi.param;

	// get the type-decl thing
	gvi.iSubItem		= COLIDX_NAME;
	GridView_GetItem(hwndGridView, nmgv->hItem, &gvi);
	type = (Type *)gvi.param;


	Parser p;
	ExprNode *expr;

	if(!p.Init(szTextBuf, lstrlen(szTextBuf)))
		return 1;

	if((expr = p.ParseExpression()) != 0)
	{
		INUMTYPE result;
		Type *baseType = BaseNode(type);
		result = Evaluate(expr);
		
		HWND hwndHexView = GetActiveHexView(g_hwndMain);

		// probably want the parent's offset here
		ExprNode result2;
		
		if(!Evaluate(nmgv->hItem, expr, offset, hwndHexView, hwndGridView, &result2))
			return 1;
		
		// set the data into the HexView, this will select the data as well
		if(IsFloat(baseType->ty))
		{
			if(result2.tok == TOK_INUMBER)
				result2.fval = (double)(signed __int64)result2.val;

			if(baseType->ty == typeFLOAT)
			{
				float f = (float)result2.fval;
				HexView_SetData(hwndHexView, offset, (BYTE*)&f, sizeof(float));
			}
			else
			{
				double d = result2.fval;
				HexView_SetData(hwndHexView, offset, (BYTE*)&d, sizeof(double));
			}
		}
		else
		{
			if(result2.tok == TOK_FNUMBER)
				result2.val = (unsigned __int64)result2.fval;

			size_w sz = SizeOf(baseType, 0, 0);
			HexView_SetData(hwndHexView, offset, (BYTE*)&result2.val, (ULONG)sz);
		}

		// force an update on the GridView item to refresh 
		// the expression to it's basic form 
		FmtData(hwndGridView, nmgv->hItem, type, offset, type->parent);

		//Evaluate(nmgv->hItem, expr, &res, 0, 
		delete expr;
		return 0;
	}

	return 1;
}

Type * bollocks(Type *bn, int &i, int &idx)
{
	if(IsStruct(bn) == false)
		return 0;

	TypeDeclList &tdl = bn->sptr->typeDeclList;
	
	for(size_t j = 0; j < tdl.size(); j++)
	{
		TypeDecl *td = tdl[j];

		// unnamed/nested struct?
		if(IsStruct(td->baseType) && (td->declList.size() == 0 && td->nested))
		{
			Type *t = bollocks(BaseNode(td->baseType), i, idx);

			if(t && i == idx)
				return t;
		}
		else
		{
			for(size_t k = 0 ; k < td->declList.size(); k++)
			{
				if(i == idx)
				{
					i = (int)(j+k);
					return bn;
				}

				i++;
			}
		}
	}

	i = -1;
	return 0;
}

LONG OnGridViewChanged(HWND hwnd, UINT msg, NMGRIDVIEW *nmgv)
{
	TCHAR szTextBuf[200];

	if(nmgv->iSubItem == COLIDX_DATA)
		return OnGridViewDataChanged(hwnd, msg, nmgv);

	GVITEM gvi		 = { nmgv->iItem, 0 };
	GVITEM gviParent = { nmgv->iItem, 0 };
	
	gvi.pszText			= szTextBuf;
	gvi.cchTextMax		= 200;
	gvi.mask		= GVIF_TEXT|GVIF_PARAM;
	GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
	
	if(nmgv->iSubItem == COLIDX_COMMENT)
	{
		Type *type = (Type *)gvi.param;

		gvi.iSubItem  = COLIDX_COMMENT;
		gvi.mask = GVIF_TEXT;
		GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);

		type->parent->comment = new char[200];
		sprintf(type->parent->comment, "%ls", szTextBuf);
		return 0;
	}

	if(nmgv->iSubItem != COLIDX_NAME)
		return 0;
	
	gviParent.mask = GVIF_PARAM;
	GridView_GetParentItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gviParent);
	
	// delete the existing item
	
	// parse the new one!
	Parser p;
	SymbolTable tmpSymTable;
	p.Init(gvi.pszText, lstrlen(gvi.pszText));
	
	extern SymbolTable globalIdentifierList;
	TypeDecl *td = p.ParseTypeDecl(0, tmpSymTable, true, false);


	if(td != 0 && td->declList.size())
	{
		Type *type = td->declList[0];
		Type *typeParent = (Type *)gviParent.param;
		TypeDecl *typeDeclParent = typeParent->parent;//(TypeDecl *)gviParent.param;
		
		// if there is a parent then this new typedecl must be
		// a structure field
		if(typeDeclParent)
		{
			Type *bn = BaseNode(typeDeclParent->baseType);
			
			if(IsStruct(bn))
			{
				int idx = GridView_GetChildIndex(nmgv->hdr.hwndFrom, nmgv->hItem);
				int i = 0;

				bn = bollocks(bn, i, idx);
				
				if(!AppendTypeDecl(bn, td, idx))
				{
					return 1;
				}
			}
		}
		// if there's no parent then we're inserting at root
		else
		{
		}
	}
	else
	{
		TCHAR errstr[200];

		MultiByteToWideChar(CP_ACP, 0, p.LastErrStr(), -1, errstr, 200);
		SendMessageA(g_hwndStatusBar, SB_SIMPLE, TRUE, 0);
		SendMessage(g_hwndStatusBar, SB_SETTEXT, SB_SIMPLEID, (LPARAM)errstr);		

		return 1;
	}
	
	GridView_DeleteChildren(nmgv->hdr.hwndFrom, nmgv->hItem);
	
	SetTypeGV(nmgv->hdr.hwndFrom, nmgv->hItem, td, 0);
	return 0;
}

Enum * IsEnum(Type *type)
{
	ExprNode *expr;
	
	if(BaseType(type) == typeENUM)
		return BaseNode(type)->eptr;
	else if(type->parent && FindTag(type->parent->tagList, TOK_ENUM, &expr) && expr)
		return FindEnum(expr->str);
	else 
		return 0;
}

Enum * IsBitflag(Type *type)
{
	ExprNode *expr;

	if(type->parent && FindTag(type->parent->tagList, TOK_BITFLAG, &expr) && expr)
		return FindEnum(expr->str);
	else
		return 0;
}

LONG OnGridViewDropDown(HWND hwnd, UINT msg, NMGRIDVIEW *nmgv)
{
	GVITEM gvi = { 0 };//nmgv->nColumn };
	
	gvi.mask  = GVIF_PARAM;
	GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);
	Type *t = (Type *)gvi.param;
	TypeDecl *td = t->parent;//(TypeDecl *)gvi.param;

	Enum *eptr = IsEnum(t);	
	Enum *bptr = IsBitflag(t);

	gvi.iSubItem = COLIDX_OFFSET;
	GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);

	if(eptr == 0) eptr = bptr;
	if(eptr || bptr)//td && BaseType(td->baseType) == typeENUM || )
	{
		//Enum *eptr = BaseNode(td->baseType)->eptr;
		int maxitemwidth = 0;
		RECT rect;
		HDC hdc = GetDC(nmgv->hwndDropList);
		HFONT hFont = (HFONT)SendMessage(nmgv->hwndDropList, WM_GETFONT, 0, 0);
		HANDLE hOldFont = SelectObject(hdc, hFont);
		
		for(size_t i = 0; i < eptr->fieldList.size(); i++)
		{
			TCHAR ach[100];
			SIZE  sz;
			//wsprintf(ach, TEXT("%04X - %hs"), eptr->fieldList[i]->val, eptr->fieldList[i]->name->name);
			wsprintf(ach, TEXT("%hs"), eptr->fieldList[i]->name->name);
			int idx = (int)SendMessage(nmgv->hwndDropList, LB_ADDSTRING, 0, (LPARAM)ach);
			
			if(bptr)
				SendMessage(nmgv->hwndDropList, LB_SETSEL, TRUE, idx);

			GetTextExtentPoint32(hdc, ach, lstrlen(ach), &sz);
			
			//SendMessage(nmgv->hwndDropList, LB_GETITEMRECT, idx, (LPARAM)&rect);
			maxitemwidth = max(maxitemwidth, sz.cx);
		}
		
		SelectObject(hdc, hOldFont);
		ReleaseDC(nmgv->hwndDropList, hdc);
		
		GetClientRect(nmgv->hwndDropList, &rect);
		if(rect.right - rect.left < maxitemwidth)
		{
			SetWindowPos(nmgv->hwndDropList, 0, 0, 0, maxitemwidth + 32, rect.bottom-rect.top, 
				SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
		
	}

	return 0;
}

//
//	Process WM_NOTIFY and WM_COMMAND messages sent to the ToolPanel by it's childen
//
LRESULT CALLBACK TypeViewCommandHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INITCOMMONCONTROLSEX ice = { sizeof(ice), (DWORD)-1 };
	TCHAR szTextBuf[200];

	HWND hwndGridView = GetDlgItem(hwnd, IDC_TYPEVIEW_GRIDVIEW);
	GVITEM gvi = { 0 };

	if(msg == WM_INITMENU || msg == WM_INITMENUPOPUP)
	{
		return 0;
	}
	if(msg == WM_COMMAND)
	{
		FILE *fp;
		switch(LOWORD(wParam))
		{
		case IDC_TYPEVIEW_SAVE:
			fp = fopen("out.txt", "wt");
			Dump(fp);
			fclose(fp);
			return 0;

		case IDC_TYPEVIEW_REFRESH:
			//FillGridView(hwnd, hwndGridView);
			UpdateTypeView();
			return 0;

		case IDC_TYPEVIEW_TYPECOMBO:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				char typeName[256];
				HWND hwndCombo = GetDlgItem(hwnd, IDC_TYPEVIEW_TYPECOMBO);
				//GetDlgItemTextA(hwnd, IDC_TYPEVIEW_TYPECOMBO, typeName, 100);

				int idx = (int)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
				(int)SendMessageA(hwndCombo, CB_GETLBTEXT, (idx), (LPARAM)typeName);

				HWND hwndGridView = GetDlgItem(hwnd, IDC_TYPEVIEW_GRIDVIEW);
				UpdateTypeGridView(hwndGridView, 0, typeName);
				//OpenTypeView(hwnd, buf);
			}
			return 0;

		case IDM_TYPEVIEW_PROPERTIIES:
			InitCommonControlsEx(&ice);
			DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_TYPEDLG), g_hwndMain, TypeDlgProc, 0);
			return 0;

		case IDM_TYPEVIEW_HEX:
			g_fDisplayHex = !g_fDisplayHex;
			UpdateTypeView();
			return 0;

		case IDM_TYPEVIEW_BIGENDIAN:
			g_fDisplayBigEndian = !g_fDisplayBigEndian;
			UpdateTypeView();
			return 0;

		case IDM_TYPEVIEW_LOCATE:
			
			gvi.iItem		= GridView_GetCurSel(hwndGridView);
			gvi.mask	= GVIF_PARAM;

			if(GridView_GetItem(hwndGridView, 0, &gvi) && gvi.param)
			{
				Type *type = (Type *)gvi.param;
				STARTUPINFO si = { sizeof(si) };
				PROCESS_INFORMATION pi;
				TCHAR szPath[MAX_PATH+20];
				wsprintf(szPath, TEXT("notepad.exe \"%hs\""), type->parent->fileRef.fileDesc->filePath);

				if(CreateProcess(0, szPath, 0, 0, 0, 0, 0, 0, &si, &pi))
				{
					GUITHREADINFO gti = { sizeof(gti) };
					int count = 10;

					while(count)
					{
						WaitForInputIdle(pi.hProcess, 3000);

						GetGUIThreadInfo(pi.dwThreadId, &gti);

						if(gti.hwndActive)
							break;

						Sleep(100);
						count--;
					}
					
					HWND hwndEdit = GetWindow(gti.hwndActive, GW_CHILD);
					int line = (int)SendMessage(hwndEdit, EM_LINEFROMCHAR, type->parent->fileRef.wspEnd, 0);
					int s    = (int)SendMessage(hwndEdit, EM_LINEINDEX, line, 0);
					int len  = (int)SendMessage(hwndEdit, EM_LINELENGTH, line, 0);

					SendMessage(hwndEdit, EM_SETSEL, s, s+len);
					SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
					
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
			}

			return 0;
		}
	}
	else if(msg == WM_NOTIFY)
	{
		NMGRIDVIEW *nmgv = (NMGRIDVIEW *)lParam;
		NMFILECHANGE *nmfc = (NMFILECHANGE *)lParam;

		if(nmfc->hdr.code == FCN_FILECHANGE)
		{
			TCHAR szMessage[MAX_PATH+100];
			wsprintf(szMessage, TEXT("%s\r\n\r\nThis file has changed outside of the TypeView editor.\r\nDo you want to reload the changes?"), nmfc->pszFile);

			UINT ret = MessageBox(hwnd, szMessage, TEXT("HexEdit"), MB_ICONQUESTION|MB_YESNO);

			if(ret == IDYES)
				UpdateTypeView();
			return 0;
		}

		// 
		if(nmgv->hdr.code == GVN_CANINSERT || nmgv->hdr.code == GVN_CANDELETE)
		{
			GVITEM gvi = { nmgv->iItem, 0 };
			gvi.mask		= GVIF_TEXT;
			gvi.pszText		= szTextBuf;
			gvi.cchTextMax	= 200;

			GridView_GetItem(nmgv->hdr.hwndFrom, nmgv->hItem, &gvi);

			// prevent array elements from being deleted/inserted
			return (gvi.pszText[0] == '[') ? 1 : 0;
		}
		// new item was inserted. if it's a structure then add the child elements
		else if(nmgv->hdr.code == GVN_INSERTED || nmgv->hdr.code == GVN_CHANGED)
		{
			return OnGridViewChanged(hwnd, msg, (NMGRIDVIEW *)lParam);
		}
		else if(nmgv->hdr.code == GVN_SELCHANGED)
		{
			return OnGridViewSelChanged(hwnd, msg, (NMGRIDVIEW *)lParam);
		}
		// is the droplist about to appear?
		else if(nmgv->hdr.code == GVN_DROPDOWN)
		{
			return OnGridViewDropDown(hwnd, msg, (NMGRIDVIEW *)lParam);
		}	
	}

	return 0;
}

extern "C"
void SaveTypeView(HWND hwndPanel, HKEY hKey)
{
	int orderArray[10];
	int widthArray[10];
	
	HWND hwndGridView	= GetDlgItem(hwndPanel, IDC_TYPEVIEW_GRIDVIEW);
	HWND hwndHeader		= GridView_GetHeader(hwndGridView);
	HWND hwndTB			= GetDlgItem(hwndPanel, IDC_TYPEVIEW_TOOLBAR);
	int  count = Header_GetItemCount(hwndHeader), i;
	UINT_PTR state;

	state = SendMessage(hwndTB, TB_GETSTATE, IDC_TYPEVIEW_PIN, 0);

	WriteSettingInt(hKey, TEXT("pin"), (state & TBSTATE_CHECKED) ? TRUE : FALSE);

	Header_GetOrderArray(hwndHeader, count, orderArray);

	for(i = 0; i < count; i++)
	{
		RECT rect;
		Header_GetItemRect(hwndHeader, i, &rect);
		widthArray[i] = rect.right-rect.left;
	}

	if(count > 0)
	{
		WriteSettingBin(hKey, TEXT("order"), orderArray, count * sizeof(int));
		WriteSettingBin(hKey, TEXT("width"), widthArray, count * sizeof(int));
	}
}

void FillTypeList(HWND hwndCombo)
{
	for(size_t i = 0; i < globalTagSymbolList.size(); i++)
	{
		Symbol *sym = globalTagSymbolList[i];

		if(IsExportedStruct(sym->type) && sym->anonymous == false)
		{
			SendMessageA(hwndCombo, CB_ADDSTRING, 0, (LPARAM)sym->name);
		}
	}

	SendMessageA(hwndCombo, CB_SETCURSEL, 0, 0);
}
 
void SetDefaultType(HWND hwndTypeView)
{
	TCHAR buf[MAX_PATH], *ptr;
	HexView_GetFileName(g_hwndHexView, buf, MAX_PATH);
	ptr = _tcsrchr(buf, '.');
	char ext[MAX_PATH];
	if(ptr) sprintf(ext, "%ls", ptr);
	
	for(size_t i = 0; ptr && i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];
		ExprNode *commaExpr;

		if(typeDecl->baseType->ty == typeSTRUCT &&
			FindTag(typeDecl->tagList, TOK_ASSOC, &commaExpr))
		{
			for( ; commaExpr; commaExpr = commaExpr->right)
			{
				if(commaExpr->left->type == EXPR_STRINGBUF && strcmp(commaExpr->left->str, ext) == 0)
				{
					char *typeName = typeDecl->baseType->sptr->symbol->name;
					SendDlgItemMessageA(hwndTypeView, IDC_TYPEVIEW_TYPECOMBO, CB_SELECTSTRING, -1, (LPARAM)typeName);
					UpdateTypeGridView(GetDlgItem(hwndTypeView, IDC_TYPEVIEW_GRIDVIEW), 0, typeName);
					return;
				}
			}
		}
	}

}

HWND CreateTypeViewPanel(HWND hwndParent, HKEY hSettingsKey)
{
	HWND hwndPanel;
	HWND hwndTB1;
	HWND hwndEdit1;
	HWND hwndEdit2;
	HWND hwndGridView;

	InitGridView();

	//
	//	Create the container window and give it a sizing grip
	//
	hwndPanel = ToolPanel_Create(hwndParent, TypeViewCommandHandler);
	ToolPanel_AddGripper(hwndPanel);

	//
	//	Create the 1st toolbar (the "Goto" button)
	//
	hwndTB1   = CreateEmptyToolbar(hwndPanel, IDB_BITMAP6, 15, IDC_TYPEVIEW_TOOLBAR, 0);

	GetWindowWidth(hwndTB1);

	AddButton(hwndTB1, IDC_TYPEVIEW_SAVE,		4, TBSTYLE_BUTTON, 0);
	AddButton(hwndTB1, IDC_TYPEVIEW_REFRESH,	1, TBSTYLE_BUTTON, 0);
	AddButton(hwndTB1, 0,						2, TBSTYLE_BUTTON, 0);
	AddButton(hwndTB1, IDC_TYPEVIEW_PIN,		3, TBSTYLE_BUTTON|TBSTYLE_CHECK, 0);

	ResizeToolbar(hwndTB1);
	ToolPanel_AddItem(hwndPanel, hwndTB1, 0);

	//
	//	Create the goto edit-field
	//
	hwndEdit1 = CreateChild(WS_EX_CLIENTEDGE, 
		WS_TABSTOP|WS_CHILD|WS_VISIBLE|ES_MULTILINE|WS_VSCROLL, 
		TEXT("EDIT"), hwndPanel, IDC_TYPEVIEW_ADDRESS);

	SetWindowSize(hwndEdit1, 150, GetEditFontHeight(hwndEdit1), 0);

	FragEdit(hwndEdit1);
	ToolPanel_AddItem(hwndPanel, hwndEdit1, 0);

	//
	//	Create the search combobox
	//
	hwndEdit2 = CreateChild(WS_EX_CLIENTEDGE, 
		WS_TABSTOP|WS_CHILD|WS_VISIBLE|CBS_DROPDOWN,
		TEXT("COMBOBOX"), hwndPanel, IDC_TYPEVIEW_TYPECOMBO);

	SetWindowSize(hwndEdit2, 200, GetEditFontHeight(hwndEdit1), 0);

	ToolPanel_AddItem(hwndPanel, hwndEdit2, 0);

	//
	//	Create the gridview. Leave it empty for now
	//
	hwndGridView = CreateGridView(hwndPanel, IDC_TYPEVIEW_GRIDVIEW, 0, WS_EX_CLIENTEDGE);
	ToolPanel_AddNewLine(hwndPanel, 4);
	ToolPanel_AddItem(hwndPanel, hwndGridView, 0);
	ToolPanel_AddAnchor(hwndPanel, 0, 0);//32);

	ToolPanel_AutoSize(hwndPanel);
	SetWindowHeight(hwndPanel, 200, NULL);

	ShowWindow(hwndPanel, SW_SHOW);

	return hwndPanel;
}

extern "C"
HWND CreateTypeView(HWND hwndParent, HKEY hKey, BOOL fAllTypes)
{
	HWND hwndPanel;
	HWND hwndGridView;
	HWND hwndTB;

	hwndPanel	 = CreateTypeViewPanel(hwndParent, hKey);
	hwndGridView = GetDlgItem(hwndPanel, IDC_TYPEVIEW_GRIDVIEW);
	hwndTB		 = GetDlgItem(hwndPanel, IDC_TYPEVIEW_TOOLBAR);

	//
	//	Initialize the gridview
	//
	
	int orderArray[10] = { 0, 1, 2, 3, 4 };
	int widthArray[10] = { 300,200,100,300,200};
	LONG fPin;
	
	GetSettingBin(hKey, TEXT("order"), orderArray, sizeof(orderArray));
	GetSettingBin(hKey, TEXT("width"), widthArray, sizeof(widthArray));
	GetSettingInt(hKey, TEXT("pin"),   &fPin, TRUE);

	SendMessage(hwndTB, TB_CHECKBUTTON, IDC_TYPEVIEW_PIN, fPin);

	Initialize();

	if(fAllTypes)
	{
		fShowFullType = true;
		PrepAllTypes(hwndGridView, widthArray);		
		//FillAllTypes(hwndPanel, hwndGridView);
		UpdateAllTypesGridView(hwndGridView, 0);
	}
	else
	{
		fShowFullType = false;
		PrepGridView(hwndGridView, widthArray, 0);		
		UpdateTypeGridView(hwndGridView, 0, "");
	}

	HWND hwndHeader = GridView_GetHeader(hwndGridView);
	Header_SetOrderArray(hwndHeader, Header_GetItemCount(hwndHeader), orderArray);
	
	// populate the combo dropdown
	FillTypeList(GetDlgItem(hwndPanel, IDC_TYPEVIEW_TYPECOMBO));

	// choose a default structure
	if(!fAllTypes)
		SetDefaultType(hwndPanel);

	//RegCloseKey(hKey);
	return hwndPanel;
}

extern "C" void UpdateAllTypesGridView(HWND hwndGV, size_w baseOffset)
{
	Symbol *sym = LookupSymbol(globalTagSymbolList, "ALL");

	fShowFullType = false;

	SendMessage(hwndGV, WM_SETREDRAW, FALSE, 0);

	if(sym && sym->type->ty == typeSTRUCT)
	{	
		Structure *sptr = sym->type->sptr;

		InsertTypeGV(hwndGV, 0, sptr->typeDeclList[0], baseOffset);
	}

	SendMessage(hwndGV, WM_SETREDRAW, TRUE, 0);
}
/*
BOOL UpdateAllTypes()
{
	HWND hwndAllTypes = DockWnd_GetContents(g_hwndMain, DWID_ALLTYPES);

	if(hwndAllTypes)
	{
		HWND hwndGridView = GetDlgItem(hwndAllTypes, IDC_TYPEVIEW_GRIDVIEW);	
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
*/
extern "C"
void UpdateTypeGridView(HWND hwndGridView, size_w baseOffset, char *typeName)
{
	fShowFullType = true;

	SendMessage(hwndGridView, WM_SETREDRAW, FALSE, 0);

	GridView_DeleteAll(hwndGridView);

	/*for(size_t i = 0; i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];
		InsertTypeGV(hwndGridView, 0, typeDecl, baseOffset);
	}*/

	//TypeDecl *typeDecl = LookupTypeDecl(typeName);
	Symbol *sym = LookupSymbol(globalTagSymbolList, typeName);

	if(sym && sym->type->ty == typeSTRUCT)
	{	
		InsertStructType(hwndGridView, 0, baseOffset, sym->type);
	}

	SendMessage(hwndGridView, WM_SETREDRAW, TRUE, 0);
}

extern "C"
void UpdateTypeView()
{
	size_w offset;
	int i;
	int id;
	int l[3] = { DWID_TYPEVIEW, DWID_ALLTYPES, 0 };

	HexView_GetCurPos(GetActiveHexView(g_hwndMain), &offset);

	//LARGE_INTEGER tmstart, tmend;
	//QueryPerformanceCounter(&tmstart);

	//for(i = 0; (id = DockWnd_EnumVisible(g_hwndMain, DWID_TYPEVIEW, i)) != 0; i++)
	for(i = 0; l[i] != 0; i++)
	{
		id = l[i];
		HWND hwndTypeView  = DockWnd_GetContents(g_hwndMain, id);
		HWND hwndGridView  = GetDlgItem(hwndTypeView, IDC_TYPEVIEW_GRIDVIEW);

		SetDlgItemBaseInt(hwndTypeView, IDC_TYPEVIEW_ADDRESS, offset, 16, TRUE);

		if(id == DWID_ALLTYPES)
		{
			UpdateAllTypesGridView(hwndGridView, offset);
		}
		else
		{
			char typeName[100];
			GetDlgItemTextA(hwndTypeView, IDC_TYPEVIEW_TYPECOMBO, typeName, 100);
			UpdateTypeGridView(hwndGridView, offset, typeName);
		}
	}

/**	LARGE_INTEGER freq;
	QueryPerformanceCounter(&tmend);
	QueryPerformanceFrequency(&freq);
	UINT64 s = (tmend.QuadPart - tmstart.QuadPart);
	UINT64 f = freq.QuadPart;
	TRACEA("********* time: %I64d %I64d (%I64d)\n", s, f,  f/s);*/
}


void ShowTypeViewDlg()
{
	

}

