//
//  ComboUtil.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef COMBOUTIL_INCLUDED
#define COMBOUTIL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <uxtheme.h>

typedef BOOL (WINAPI * DrawItemProc)(HTHEME hTheme, DRAWITEMSTRUCT *dis, BOOL fDrawBackground);
typedef BOOL (WINAPI * MeasureItemProc)(MEASUREITEMSTRUCT *mis);

typedef struct
{
	UINT_PTR	 ctrlId;
	DrawItemProc pfnDrawItemProc;
	WNDPROC		 oldWndProc;
	BOOL		 fMouseOver;
	HTHEME		 hTheme;

} CTRLITEM;

typedef struct
{
	/*UINT				idList[100];
	DrawItemProc 		drawProcList[100];
	MeasureItemProc 	measureProcList[100];*/
	CTRLITEM			itemList[100];
	LONG				itemCount;
	WNDPROC				oldProc;

} DLGCTRL;

CTRLITEM *SubclassCombo(HWND hwndCombo, DrawItemProc drawProc);

void DrawItem_DefaultColours(DRAWITEMSTRUCT *dis);
void DrawThemedComboBackground(CTRLITEM *cip, HWND hwndCombo, HDC hdc, RECT *rect);

void SetComboItemHeight(HWND hwndCombo, int nMinHeight);

void AutoOwnerDraw(HTHEME hTheme, HWND hwndCtrl, DrawItemProc drawProc, MeasureItemProc measureProc);

#ifdef __cplusplus
}
#endif

#endif