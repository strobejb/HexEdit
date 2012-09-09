//
//	WTL C++ interface to the HexView win32 control
//
//	www.catch22.net
//
//	Released under the MIT licence
//
#pragma once

#include "HexView.h"

template <class TBase>
class CHexViewCtrlT : public TBase
{
public:
	// Constructors
	CHexViewCtrlT(HWND hWnd = NULL) : TBase(hWnd)
	{ }

	CHexViewCtrlT< TBase >& operator =(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		pMsg;
		return FALSE;
	}

	static ATOM InitHexView()
	{
		return ::InitHexView();
	}

	HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			ATL::_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		InitHexView();
		return TBase::Create(GetWndClassName(), hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, MenuOrID.m_hMenu, lpCreateParam);
	}

// Attributes
	static LPCTSTR GetWndClassName()
	{
		return WC_HEXVIEW;
	}

  // uIndex - one of the HVC_* values
  UINT SetColor(UINT uIndex, COLORREF color)
  {
    return HexView_SetColor(m_hWnd, uIndex, color);
  }

  COLORREF GetColor(UINT uIndex)
  {
    return HexView_GetColor(m_hWnd, uIndex);
  }

  UINT SetFontSpacing(int xspace, int yspace)
  {
    return HexView_SetFontSpacing(m_hWnd, xspace, yspace);
  }

  VOID SetFont(HFONT hFont)
  {
    return HexView_SetFont(m_hWnd, hFont);
  }

  // uMask, uStyle - one or more of the HVS_* styles
	UINT SetStyle(UINT uMask, UINT uStyle)
	{
		return HexView_SetStyle(m_hWnd, uMask, uStyle);
	}

	UINT SetGrouping(UINT nBytes)
	{
		return HexView_SetGrouping(m_hWnd, nBytes);
	}

  BOOL SetPadding(int nPaddingLeft, int nPaddingRight)
  {
    return HexView_SetPadding(m_hWnd, nPaddingLeft, nPaddingRight);
  }

  VOID SetAddressOffset(size_w offset)
  {
    HexView_SetAddressOffset(m_hWnd, offset);
  }

	UINT GetStyle()
	{
		return HexView_GetStyle(m_hWnd);
	}

	UINT GetStyleMask(UINT uMask)
	{
		return HexView_GetStyleMask(m_hWnd, uMask);
	}

	UINT GetGrouping()
	{
		return HexView_GetGrouping(m_hWnd);
	}

	HBOOKMARK AddBookmark(PBOOKMARK param)
	{
		return HexView_AddBookmark(m_hWnd, param);
	}

	UINT DelBookmark(PBOOKMARK param)
	{
		return HexView_DelBookmark(m_hWnd, param);
	}

	UINT SetSearchPattern(PBYTE pData, ULONG nLength)
	{
		return HexView_SetSearchPattern(m_hWnd, pData, nLength);
	}

	size_w GetCurPos()
	{
		size_w pos = 0;
		HexView_GetCurPos(m_hWnd, &pos);
		return pos;
	}

	size_w GetSelStart()
	{
		size_w pos = 0;
		HexView_GetSelStart(m_hWnd, &pos);
		return pos;
	}

	size_w GetSelEnd()
	{
		size_w pos = 0;
		HexView_GetSelEnd(m_hWnd, &pos);
		return pos;
	}

	size_w GetSelSize()
	{
		size_w len = 0;
		HexView_GetSelSize(m_hWnd, &len)
		return len;
	}

	size_w GetFileSize()
	{
		size_w len = 0;
		HexView_GetFileSize(m_hWnd, &len);
		return len;
	}

	UINT GetDataCur(PBYTE pData, ULONG nLength)
	{
		return HexView_GetDataCur(m_hWnd, pData, nLength);
	}

	UINT GetDataAdv(PBYTE pData, ULONG nLength)
	{
		return HexView_GetDataAdv(m_hWnd, pData, nLength);
	}

	UINT SetDataCur(PBYTE pData, ULONG nLength)
	{
		return HexView_SetDataCur(m_hWnd, pData, nLength);
	}

	UINT SetDataAdv(PBYTE pData, ULONG nLength)
	{
		return HexView_SetDataAdv(m_hWnd, pData, nLength);
	}

	UINT Undo()
	{
		return HexView_Undo(m_hWnd);
	}

	UINT Redo()
	{
		return HexView_Redo(m_hWnd);
	}

	UINT Cut()
	{
		return HexView_Cut(m_hWnd);
	}

	UINT Copy()
	{
		return HexView_Copy(m_hWnd);
	}

	UINT Paste()
	{
		return HexView_Paste(m_hWnd);
	}

	UINT Delete()
	{
		return HexView_Delete(m_hWnd);
	}

	UINT SetEditMode(UINT nEditMode)
	{
		return HexView_SetEditMode(m_hWnd, nEditMode);
	}

	UINT GetEditMode()
	{
		return HexView_GetEditMode(m_hWnd);
	}

	UINT OpenFile(LPCTSTR szFileName, UINT uMethod)
	{
		return HexView_OpenFile(m_hWnd, szFileName, uMethod);
	}

	UINT SaveFile(LPCTSTR szFileName, UINT uMethod)
	{
		return HexView_SaveFile(m_hWnd, szFileName, uMethod);
	}

  UINT InitBuf(const BYTE *buf, size_w len)
  {
    return HexView_InitBuf(m_hWnd, buf, len);
  }

  UINT InitBufShared(const BYTE *buf, size_w len)
  {
    return HexView_InitBufShared(m_hWnd, buf, len);
  }

	BOOL CanUndo()
	{
		return HexView_CanUndo(m_hWnd);
	}

	BOOL CanRedo()
	{
		return HexView_CanRedo(m_hWnd);
	}

	HMENU SetContextMenu(HMENU hMenu)
	{
		return HexView_SetContextMenu(m_hWnd, hMenu);
	}

	BOOL Clear()
	{
		return HexView_Clear(m_hWnd);
	}

	BOOL ClearBookmarks()
	{
		return HexView_ClearBookmarks(m_hWnd);
	}

	BOOL GetBookmark(HBOOKMARK hBookmark, PBOOKMARK bookm)
	{
		return HexView_GetBookmark(m_hWnd, hBookmark, bookm);
	}

	HBOOKMARK EnumBookmark(HBOOKMARK hBookmark, PBOOKMARK bookm)
	{
		return HexView_EnumBook(m_hWnd, hBookmark, bookm)
	}

	BOOL SetBookmark(HBOOKMARK hBookmark, PBOOKMARK bookm)
	{
		return HexView_SetBookmark(m_hWnd, hBookmark, bookm);
	}

	UINT SetCurPos(size_w pos)
	{
		return HexView_SetCurPos(m_hWnd, pos);
	}

	UINT SetSelStart(size_w pos)
	{
		return HexView_SetSelStart(m_hWnd, pos);
	}

	UINT SetSelEnd(size_w pos)
	{
		return HexView_SetSelEnd(m_hWnd, pos);
	}

	UINT ScrollTo(size_w pos)
	{
		return HexView_ScrollTo(m_hWnd, pos);
	}

	UINT ScrollTop(size_w pos)
	{
		return HexView_ScrollTop(m_hWnd, pos);
	}

  int FormatData(HEXFMT_PARAMS *fmtparam)
	{
		return HexView_FormatData(m_hWnd, fmtparam);
	}

	UINT GetLineChars()
	{
		return HexView_GetLineChars(m_hWnd);
	}

  UINT GetLineLen()
	{
		return HexView_GetLineLen(m_hWnd);
	}

	UINT SetLineLen(UINT len)
	{
		return HexView_SetLineLen(m_hWnd, len);
	}

	BOOL IsDragLoop()
	{
		return HexView_IsDragLoop(m_hWnd);
	}

	UINT SelectAll()
	{
		return HexView_SelectAll(m_hWnd);
	}

	BOOL FindInit(PBYTE data, UINT len)
	{
		return HexView_FindInit(m_hWnd, data, len);
	}

	BOOL FindNext(size_w *pos, UINT options)
	{
		return HexView_FindNext(m_hWnd, pos, options);
	}

	BOOL FindPrev(size_w *pos, UINT options)
	{
		return HexView_FindPrev(m_hWnd, pos, options);
	}

	BOOL FindCancel()
	{
		return HexView_FindCancel(m_hWnd);
	}

	HANDLE GetFileHandle()
	{
		return HexView_GetFileHandle(m_hWnd);
	}

	UINT GetCurPane()
	{
		return HexView_GetCurPane(m_hWnd);
	}

	UINT SetCurPane(UINT pane)
	{
		return HexView_SetCurPane(m_hWnd, pane);
	}

	UINT GetFileName(LPTSTR szName, UINT len)
	{
		return HexView_GetFileName(hwnd, szName, len);
	}

	BOOL IsReadOnly()
	{
		return HexView_IsReadOnly(m_hWnd);
	}

	BOOL GetCurCoord(POINT *coord)
	{
		return HexView_GetCurCoord(m_hWnd, coord);
	}

	BOOL Revert()
	{
		return HexView_Revert(m_hWnd);
	}

	UINT ImportFile(LPCTSTR szFileName, UINT uMethod)
	{
		return HexView_ImportFile(m_hWnd, szFileName, uMethod);
	}

	ULONG SetCurSel(size_w selStart, size_w selEnd)
	{
		return HexView_SetCurSel(m_hWnd, selStart, selEnd);
	}

	ULONG SetData(size_w offset, BYTE *buf, ULONG len)
	{
		return HexView_SetData(m_hWnd, offset, buf, len);
	}

	ULONG GetData(size_w offset, BYTE *buf, ULONG len)
	{
		return HexView_GetData(m_hWnd, offset, buf, len);
	}

	ULONG FillData(BYTE *buf, ULONG buflen, size_w len)
	{
		return HexView_FillData(m_hWnd, buf, buflen, len);
	}

};

typedef CHexViewCtrlT<ATL::CWindow>   CHexViewCtrl;
