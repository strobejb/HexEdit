//
//  IDropTarget.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//


#define STRICT

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"

HRESULT GetDataObjDword(IDataObject *pDataObject, LPCTSTR szFormat, DWORD_PTR *pdwValue);
HRESULT GetDataObjBuf(IDataObject *pDataObject, LPCTSTR szFormat, PVOID pData, DWORD nLength);

static WORD g_cfFileDesc = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);


//
//	IDropTarget Interface
//
HRESULT WINAPI HexView::QueryInterface (REFIID iid, void ** ppvObject)
{
    if(iid == IID_IDropTarget || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

ULONG WINAPI HexView::AddRef (void)
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG WINAPI HexView::Release (void)
{
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
		delete this;

	return count;
}

HRESULT WINAPI HexView::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	// does the dataobject contain data we want?
    m_fAllowDrop = QueryDataObject(pDataObject);

	//if(m_nEditMode == HVMODE_READONLY)
	//	m_fAllowDrop = false;
	
	if(m_fAllowDrop)
    {
        // get the dropeffect based on keyboard state
        *pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);

        SetFocus(m_hWnd);

		m_nSelectionMode = SEL_DRAGDROP;

		// use a fake WM_MOUSEMOVE in order to position the caret
		ScreenToClient(m_hWnd, (POINT *)&pt);
		OnMouseMove(0, pt.x, pt.y);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;

}

HRESULT WINAPI HexView::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	if(m_fAllowDrop)
    {
        *pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);

		// Use a fake WM_MOUSEMOVE to position the caret
		ScreenToClient(m_hWnd, (POINT *)&pt);
		OnMouseMove(0, pt.x, pt.y);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

	return S_OK;
}

HRESULT WINAPI HexView::DragLeave(void)
{
	// restore selection/cursor state
	m_nCursorOffset  = m_nSelectionEnd;
	
	// fake a 
	OnLButtonUp(0, 0, 0);
	RepositionCaret();

	m_nSelectionMode = SEL_NONE;

	NotifyParent(HVN_SELECTION_CHANGE);
	NotifyParent(HVN_CURSOR_CHANGE);

	return S_OK;
}

HRESULT WINAPI HexView::Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	DWORD dwAllowed  = *pdwEffect;
	HRESULT hresult  = S_OK;

	size_w delstart = SelectionStart();
	size_w dellen   = SelectionSize();

	// prevent drop over existing selection
	if(m_nCursorOffset >= delstart && m_nCursorOffset < delstart+dellen)
	{
		m_fAllowDrop = FALSE;
		m_nSelectionStart = m_nSelectionEnd = m_nCursorOffset;
		RefreshWindow();
	}
	
	// if the data format is OK, and we're not over an existing selection
    if(m_fAllowDrop)
    {
		m_pDataSeq->group();

		// detect the source HWND of the drag-drop
		HWND hwndSource;
		GetDataObjDword(pDataObject, CFSTR_HEX_HWND, (DWORD_PTR *)&hwndSource);

		*pdwEffect = DropEffect(grfKeyState, pt, dwAllowed);

		// do an 'optimized' move if we are dragging+dropping within the same document
		if(*pdwEffect == DROPEFFECT_MOVE && hwndSource == m_hWnd)
		{
			*pdwEffect = DROPEFFECT_NONE;
			m_pDataSeq->erase(delstart, dellen);
		}

		// import the data
		if(!DropData(pDataObject, false, true))
		{
			*pdwEffect = DROPEFFECT_NONE;
			hresult = E_UNEXPECTED;
		}

		m_pDataSeq->ungroup();

    }


	OnLButtonUp(0, 0, 0);

//	*pdwEffect		 = DROPEFFECT_NONE;
	m_nSelectionMode = SEL_NONE;

	NotifyParent(HVN_SELECTION_CHANGE);
	NotifyParent(HVN_CURSOR_CHANGE);

    return hresult;	
}

//
//	
//
bool HexView::QueryDataObject(IDataObject *pDataObject)
{
    FORMATETC fmtetc[] = 
	{ 
		{	CF_TEXT,		0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
		{	CF_TEXT,		0, DVASPECT_CONTENT, -1, TYMED_ISTREAM },
		{	CF_HDROP,		0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
		{	g_cfFileDesc,	0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
		{	0 }
	};

	int i;

	for(i = 0; fmtetc[i].cfFormat; i++)
	{
        // does the data object support CF_TEXT using a HGLOBAL?
	   if(pDataObject->QueryGetData(&fmtetc[i]) == S_OK)
		   return true;
	}

	return false;
}


DWORD HexView::DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed)
{
	DWORD dwEffect = 0;

	// 1. check "pt" -> do we allow a drop at the specified coordinates?
	
	// 2. work out that the drop-effect should be based on grfKeyState
	if(grfKeyState & MK_CONTROL)
	{
		dwEffect = dwAllowed & DROPEFFECT_COPY;
	}
	else if(grfKeyState & MK_SHIFT)
	{
		dwEffect = dwAllowed & DROPEFFECT_MOVE;
	}
	
	// 3. no key-modifiers were specified (or drop effect not allowed), so
	//    base the effect on those allowed by the dropsource
	if(dwEffect == 0)
	{
		if(dwAllowed & DROPEFFECT_MOVE) dwEffect = DROPEFFECT_MOVE;
		if(dwAllowed & DROPEFFECT_COPY) dwEffect = DROPEFFECT_COPY;
		
	}
	
	return dwEffect;
}

bool GetStgMedium(IDataObject *pDataObject, UINT cfFormat, UINT tymed, STGMEDIUM *stgmed)
{
	FORMATETC fmtetc = { (CLIPFORMAT)cfFormat, 0, DVASPECT_CONTENT, -1, tymed };

	return pDataObject->GetData(&fmtetc, stgmed) == S_OK ? true : false;
}

//
//	Import the dataobject to the current cursor position
//
bool HexView::DropData(IDataObject *pDataObject, bool fReplaceSelection, bool fSelectData)
{
	// construct a FORMATETC object
	//FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed;
	HexSnapShot *hss;

	// Find the snapshot object
	if(S_OK == GetDataObjBuf(pDataObject, CFSTR_HEX_SNAPSHOTPTR, &hss, sizeof(HexSnapShot *)))
	{
		HWND hwndSource = 0;

		GetDataObjDword(pDataObject, CFSTR_HEX_HWND, (DWORD_PTR *)&hwndSource);

		// make sure that the source of the snapshot is *this* HexView!!!
		if(hwndSource == m_hWnd)
		{
			// only bother if it's a large amount of data
			if(hss->m_length >= 0x10000)
			{
				// enter a 'snapshot' rather than a databuffer -----v
				EnterData(NULL, 0, true, fReplaceSelection, fSelectData, hss);
				return true;
			}
		}
	}

	// Snapshot didn't work. See if the dataobject contains any TEXT stored as a HGLOBAL
	if(GetStgMedium(pDataObject, CF_TEXT, TYMED_HGLOBAL, &stgmed))
	{
		PVOID  data;
		DWORD_PTR  len;

		if((data = GlobalLock(stgmed.hGlobal)) != 0)
		{
			// see if there's a RLE object as well
			if(GetDataObjDword(pDataObject, CFSTR_HEX_DATALEN, &len) != S_OK)
			{
				// if not then manually calculate the length of data
				len = (DWORD)strlen((char *)data);
			}
			
			// input and select (highlight) the data-buffer
			EnterData((BYTE *)data, len, true, fReplaceSelection, fSelectData);
			
			GlobalUnlock(stgmed.hGlobal);
		}
		
		ReleaseStgMedium(&stgmed);
		return true;
	}
	
	
	if(GetStgMedium(pDataObject, CF_HDROP, TYMED_HGLOBAL, &stgmed))
	{
		//PVOID data;
		//DWORD len;

		TCHAR szFile[MAX_PATH];

		if(DragQueryFile((HDROP)stgmed.hGlobal, 0, szFile, MAX_PATH))
		{
			ImportFile(szFile, 0);
		}

		/*if((data = GlobalLock(stgmed.hGlobal)) != 0)
		{
	DROPFILES *df = (DROPFILES *)hDrop;
	
	
	{
		TCHAR tmp[MAX_PATH];
		
		if(ResolveShortcut(buf, tmp, MAX_PATH))
			lstrcpy(buf,tmp);

		HexeditOpenFile(hwnd, buf);
	}
	
	DragFinish(hDrop);*/
			
			
//			GlobalUnlock(stgmed.hGlobal);
//		}
		
		ReleaseStgMedium(&stgmed);
		return true;
	}

	return false;
}

void HexView::RegisterDropWindow()
{
	IDropTarget *pDropTarget = this;
	pDropTarget->AddRef();

	// acquire a strong lock
	CoLockObjectExternal(pDropTarget, TRUE, FALSE);

	// tell OLE that the window is a drop target
	RegisterDragDrop(m_hWnd, pDropTarget);
}

void HexView::UnregisterDropWindow()
{
	IDropTarget *pDropTarget = this;

	// remove drag+drop
	RevokeDragDrop(m_hWnd);

	// remove the strong lock
	CoLockObjectExternal(pDropTarget, FALSE, TRUE);

	// release our own reference
	pDropTarget->Release();
}
