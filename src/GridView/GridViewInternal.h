//
//  GridViewInternal.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef GRIDVIEW_INTERNAL_INCLUDED
#define GRIDVIEW_INTERNAL_INCLUDED

#include <uxtheme.h>
#include "GridView.h"
#include "GridViewData.h"

#define MAX_GVCOLUMNS 32

#define IMAGEWIDTH  16
#define IMAGEPADX	3

#define LEVEL_WIDTH (IMAGEWIDTH + IMAGEPADX)
#define LINEOFFX	(IMAGEWIDTH / 2)

#define MAX_GRIDVIEW_FONTS 16

class GridView
{
public:
	GridView(HWND hwnd);
	~GridView();

	// message dispatcher
	LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

private:
	
	// message handlers
	LRESULT OnSize(int width, int height);
	LRESULT OnPaint();
	LRESULT OnNcPaint(HRGN hrgnUpdate);
	LRESULT OnSetFont(HFONT hFont, int nFontIdx);
	LRESULT OnHeaderNotify(int nCtrlId, NMHEADER *nmheader);
	LRESULT OnCommand(UINT nCtrlId, UINT uNotifyCode, HWND hwndFrom);
	LRESULT OnTooltipNotify(int nCtrlId, NMHDR *hdr);
	LRESULT OnRButtonDown(int x, int y);
	LRESULT OnLButtonDown(int x, int y);
	LRESULT OnLButtonDblClick(int x, int y);
	LRESULT OnLButtonUp(int x, int y);
	LRESULT OnMouseMove(int x, int y);
	LRESULT OnKeyDown(UINT nKeyCode, UINT nFlags);
	LRESULT OnChar(UINT nChar);
	LRESULT OnSetFocus();
	LRESULT OnCopy(GVRow *gvrow);
	LRESULT OnCut(GVRow *gvrow);
	LRESULT OnPaste(GVRow *gvrow, TCHAR *szText);
	LRESULT OnClear(GVRow *gvrow);
	LRESULT OnSetRedraw(BOOL fRedraw);

	LRESULT OnKillFocus();
	LRESULT OnDrawItem(UINT uId, DRAWITEMSTRUCT *dis);
	LRESULT OnColorEdit(HDC hdc, HWND hwndCtrl);
	LRESULT SetImageList(HIMAGELIST hImgList);
	LRESULT OnVScroll(UINT uSBCode, UINT uPos);
	LRESULT OnHScroll(UINT uSBCode, UINT uPos);
	LRESULT	OnMouseWheel(int nDelta);
	LRESULT	OnTimer(UINT_PTR nTimer);
	bool	PinToBottomCorner();
	void	Scroll(int dx, int dy);
	HRGN	ScrollRgn(int dx, int dy, bool fReturnUpdateRgn);
	void	ScrollToLine(ULONG lineNo);
	void	SetupScrollbars();

	LRESULT	NotifyParent(UINT nCode, HGRIDITEM hItem);

	void	GetActiveClientRect(RECT *rect);
	BOOL	GetItemRect(ULONG column, ULONG line, RECT *rect);

	BOOL	InsertColumn(int index, GVCOLUMN *gvcol);
	
	BOOL	DeleteColumn(int index);
	HGRIDITEM	InsertItem(HGRIDITEM hItem, UINT gviWhere, GVITEM *gvitem);
	HGRIDITEM	InsertUniqueChild(HGRIDITEM hItem, GVITEM *gvitem);
	GVRow * 	FindChild(GVRow *gvrow, GVITEM *gvitem, int depth);
	BOOL		SetRowItem(HGRIDITEM hItem, GVITEM *gvitem);
	BOOL		DeleteItem(GVITEM *gvitem);
	BOOL		DeleteRow(GVRow *gvrow);
	BOOL		SetItem(GVITEM *gvitem);
	GVRow *		MouseToItem(int x, int y, ULONG *line, ULONG *col, ULONG *uPortion, RECT *rect, GVITEM **gvitem = 0);
	BOOL		ToggleRow(GVRow *gvrow, ULONG lineNo);
	GVRow *		GetFirstVisibleRow();
	GVRow *		GetRowItem(ULONG nLine, ULONG nColumn, GVITEM **gvitem);
	GVRow *		GetCurRow();
	BOOL		ExpandItem(GVRow *gvrow, BOOL expand, BOOL recurse);

	GVCOLUMN *	GetColumn(ULONG nColumn, int *nHeaderIndex);

	ULONG		L2V(ULONG lidx);
	ULONG		V2L(ULONG vidx);

	bool	CheckStyle(ULONG uStyle);
	UINT	SetStyle(ULONG uMask, ULONG uStyle);
	
	void	RefreshWindow();
	void	UpdateMetrics();
	int		DrawRow(HDC hdc, GVRow *rowptr, int x, int y, int height, ULONG colidx, ULONG rowidx);
	int		DrawItem(HDC hdc, GVRow *rowptr, GVITEM *gvitem, GVCOLUMN *gvcol, int x, int y, int width, int height, bool selected, bool active, bool focussed);
	int		DrawTree(HDC hdc, GVRow *rowptr, int x, int y, int height);
	void	DrawTreeBox(HDC hdc, RECT *rect, bool plus);
	void	RedrawLine(ULONG lineNo);
	COLORREF GetColour(int idx);
	HFONT	GetFont(GVITEM *gvitem, GVCOLUMN *gvcol);

	bool	EnterEditMode();
	bool	ExitEditMode(BOOL fAcceptChange);
	HWND	CreateComboLBox(HWND hwndOwner);
	void	ShowComboLBox(int x, int y, int width, int height);
	HWND	m_hwndEdit;
	WNDPROC m_oldEditProc;
	HWND	m_hwndComboLBox;
	WNDPROC m_oldComboProc;

	static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ComboLBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void	ClientRect(RECT *rect);

	HWND		m_hWnd;
	HTHEME		m_hComboTheme;
	HTHEME		m_hGridTheme;
	HWND		m_hWndHeader;
	//HWND		m_hWndEdit;
	HWND		m_hWndTooltip;
	HIMAGELIST	m_hImageList;
	HFONT		m_hFont[MAX_GRIDVIEW_FONTS];
	ULONG		m_uState;

	GVData		m_gvData;
	ULONG		m_nLineHeight;
	int			m_nHeaderHeight;
	ULONG		m_nCurrentLine;
	ULONG		m_nCurrentColumn;
	ULONG		m_nNumLines;

	COLORREF	m_rgbColourList[GVC_MAX_COLOURS];
	//GVCOLUMN	m_gvColumn[MAX_GVCOLUMNS];
	ULONG		m_nNumColumns;

	ULONG		m_nVScrollPos;
	ULONG		m_nVScrollMax;
	int			m_nWindowLines;
	int			m_nHorzScrollUnits;
	int			m_nTotalWidth;
	int			m_nHScrollPos;
	int			m_nHScrollMax;
	UINT_PTR	m_nScrollTimer;
	UINT		m_nScrollCounter;
	LONG		m_nScrollMouseRemainder;
	BOOL		m_fMouseDown;
	HGRIDITEM   m_pTempInsertItem;
	bool		m_fInNotify;
	bool		m_fEditError;
	bool		m_fRedrawChanges;

};



#endif
