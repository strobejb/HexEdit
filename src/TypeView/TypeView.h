//
//  TypeView.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef TYPEVIEW_INCLUDED
#define TYPEVIEW_INCLUDED



#define COLIDX_NAME		0
#define COLIDX_DATA		1
#define COLIDX_OFFSET	2
#define COLIDX_COMMENT	3
#define COLIDX_TYPE		4

#ifdef __cplusplus

Type	*	TypeView_GetType   (HWND hwndGV, HGRIDITEM hItem);
ULONG64		TypeView_GetOffset (HWND hwndGV, HGRIDITEM hItem);

extern "C"{
#endif

void SaveTypeView(HWND hwndPanel, HKEY hKey);
HWND CreateTypeView(HWND hwndParent, HKEY hKey, BOOL fAllTypes);
void UpdateTypeView();

#ifdef __cplusplus
}
#endif

#endif