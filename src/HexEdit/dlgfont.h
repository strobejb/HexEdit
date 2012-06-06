//
//  dlgfont.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef DLGFONT_INCLUDED
#define DLGFONT_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// 
//	Replacement APIs for DialogBox and CreateDialog, allows the specification of a new font/fontsize
//
BOOL	DialogBoxWithFont	 (HINSTANCE hInst, LPCTSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam, LPCTSTR szFontName, WORD nPointSize);
HWND	CreateDialogWithFont (HINSTANCE hInst, LPCTSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam, LPCTSTR szFontName, WORD nPointSize);
HGLOBAL LoadDialogTemplate	 (HINSTANCE hInst, LPCTSTR lpTemplateName, LPCTSTR szFontName, WORD nPointSize);

//
//	Process-wide interception of all dialog-creation APIs
//
VOID	Init_DialogFont(TCHAR *szFontName, WORD nPointSize);
VOID	DeInit_DialogFont();

#ifdef __cplusplus
}
#endif

#endif


