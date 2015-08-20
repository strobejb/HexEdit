//
//  HexViewClipboard.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include "HexView.h"
#include "HexViewInternal.h"
#include "trace.h"

HRESULT GetDataObjBuf(IDataObject *pDataObject, LPCTSTR szFormat, PVOID pData, DWORD nLength);
HRESULT GetDataObjDword(IDataObject *pDataObject, LPCTSTR szFormat, DWORD_PTR *pdwValue);

/*DWORD GetClipboardDataPtr(TCHAR *szFormatName, PVOID pData, DWORD nLength)
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

	nLength = min(nLength, GlobalSize(hMem));

	memcpy(pData, ptr, nLength);

	GlobalUnlock(hMem);
	return nLength;
}

BOOL SetClipboardDataPtr(TCHAR *szFormatName, PVOID pData, ULONG nLength)
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
}*/

//
//	Paste any CF_TEXT/CF_UNICODE text from the clipboard
//
BOOL HexView::OnPaste()
{
	BOOL success = FALSE;
	IDataObject *pDataObject;

	if(m_nEditMode == HVMODE_READONLY)
		return FALSE;

	// Retrieve IDataObject from the clipboard
	if(OleGetClipboard(&pDataObject) == S_OK)
	{
		// extract the data!
		DropData(pDataObject, true, false);
		pDataObject->Release();

		m_pDataSeq->breakopt();
		return TRUE;
	}

	return FALSE;

/*	if(OpenClipboard(m_hWnd))
	{
		DWORD  len;
		HANDLE hMem		= GetClipboardData(CF_TEXT);
		BYTE *pData		= (BYTE *)GlobalLock(hMem);

		if(pData)
		{
			if(GetClipboardDataPtr(CF_PRI_RLE32, &len, sizeof(DWORD)) == 0)
			{
				len = strlen((char *)pData);
			}

			EnterData(pData, len, false);

			m_pDataSeq->breakopt();

			//if(textlen > 1)
			//	m_pTextDoc->m_seq.breakopt();	

			GlobalUnlock(hMem);
			success = TRUE;
		}

		CloseClipboard();
	}

	return success;*/
}

//
//	Retrieve the specified range of text and copy it to supplied buffer
//	szDest must be big enough to hold nLength characters
//	nLength includes the terminating NULL
//
ULONG HexView::GetData(size_w nStartOffset, BYTE *pBuf, ULONG nLength)
{
	return (ULONG)m_pDataSeq->render(nStartOffset, pBuf, (ULONG)nLength);
}

//
//	Retrieve the specified range of text and copy it to supplied buffer
//	szDest must be big enough to hold nLength characters
//	nLength includes the terminating NULL
//
ULONG HexView::SetData(size_w nStartOffset, BYTE *pBuf, ULONG nLength)
{
	InvalidateRange(nStartOffset, nStartOffset + nLength); 
	return (ULONG)m_pDataSeq->replace(nStartOffset, pBuf, (ULONG)nLength);
	//return m_pDataSeq->render(nStartOffset, pDest, nLength);
}

ULONG HexView::FillData(BYTE *buf, ULONG buflen, size_w len)
{
	m_pDataSeq->group();

	BOOL fRedraw = SetRedraw(FALSE);
	
	while(len > 0)
	{
		buflen = (ULONG)min(buflen, len);

		EnterData(buf, buflen, true, true, false, NULL);

		len -= buflen;
	}

	SetRedraw(fRedraw);

	size_w offset = min(SelectionStart(), m_nCursorOffset);
	ContentChanged(offset, len, HVMETHOD_OVERWRITE);

	m_pDataSeq->ungroup();
	return 0;
}



//
//	Enter a buffer of data into the HexView at the current
//  cursor position. Optional HexSnapShot can be used instead of buffer
//
size_w HexView::EnterData(BYTE *pSource, size_w nLength, bool fAdvanceCaret, bool fReplaceSelection, bool fSelectData, HexSnapShot *hss /*= 0*/)
{
	size_w offset = m_nCursorOffset;
	bool fAllowChange = true;

	if(m_nEditMode == HVMODE_READONLY)
		return 0;

	if(pSource == NULL && hss == NULL)
		return 0;

	if(m_fRedrawChanges)
		fAllowChange = AllowChange(m_nCursorOffset, nLength, m_nEditMode == HVMODE_OVERWRITE ? HVMETHOD_OVERWRITE:HVMETHOD_INSERT, pSource);

	if(m_nEditMode != HVMODE_INSERT)
		fReplaceSelection = false;

	if(fReplaceSelection && SelectionSize() == 0)
		fReplaceSelection = false;

	if(fReplaceSelection && fAllowChange)
	{
		// group this erase with the insert/replace operation
		m_pDataSeq->group();
		m_pDataSeq->erase(SelectionStart(), SelectionSize());
	}

	if(SelectionSize() > 0 && (m_nCursorOffset == m_nSelectionStart || m_nCursorOffset == m_nSelectionEnd))
	{
		m_nCursorOffset = SelectionStart();
	}

	// are we entering a snapshot?
	if(hss)
	{
		if(fAllowChange)
		{
			if(m_nEditMode == HVMODE_OVERWRITE)
				m_pDataSeq->replace_snapshot(m_nCursorOffset, hss->m_length, hss->m_desclist, hss->m_count);
			else			
				m_pDataSeq->insert_snapshot(m_nCursorOffset, hss->m_length, hss->m_desclist, hss->m_count);
		}

		nLength = hss->m_length;
	}
	// regular data entry
	else
	{
		if(fAllowChange)
		{
			if(m_nEditMode == HVMODE_OVERWRITE)
				m_pDataSeq->replace(m_nCursorOffset, pSource, nLength);
			else
				m_pDataSeq->insert(m_nCursorOffset, pSource, nLength);
		}
	}

	if(fSelectData)
	{
		m_nSelectionStart = m_nCursorOffset;
		m_nCursorOffset  += nLength;
		m_nSelectionEnd   = m_nCursorOffset;
	}
	else if(fAllowChange)
	{
		if(fAdvanceCaret)
			m_nCursorOffset  += nLength;

		m_nSelectionStart = m_nCursorOffset;
		m_nSelectionEnd   = m_nCursorOffset;
	}

	if(fReplaceSelection && fAllowChange)
		m_pDataSeq->ungroup();

	if(m_fRedrawChanges)
		ContentChanged(offset, nLength, m_nEditMode == HVMODE_INSERT ? HVMETHOD_INSERT : HVMETHOD_OVERWRITE);

	return nLength;
}

//
//	Copy the currently selected text to the clipboard as CF_TEXT/CF_UNICODE
//
BOOL HexView::OnCopy()
{
	IDataObject *pDataObject;

	if(SelectionSize() == 0)
		return FALSE;

	// Create an IDataObject representing the range of selected data
	if(!CreateDataObject(SelectionStart(), SelectionSize(), &pDataObject))
		return FALSE;

	if(OleSetClipboard(pDataObject) == S_OK)
	{
		m_pLastDataObject = pDataObject;
		//pDataObject->Release();
		return TRUE;
	}
	else
	{
		pDataObject->Release();
		return FALSE;
	}

/*	if(OpenClipboard(m_hWnd))
	{
		HANDLE hMem;
		BYTE  *ptr;
		
		if((hMem = GlobalAlloc(GPTR, sellen + 1)) != 0)
		{
			if((ptr = (BYTE *)GlobalLock(hMem)) != 0)
			{
				EmptyClipboard();

				GetData(ptr, selstart, sellen);
				ptr[sellen] = '\0';

				SetClipboardData(CF_TEXT, hMem);
				success = TRUE;

				GlobalUnlock(hMem);

				SetClipboardDataPtr(CF_PRI_RLE32, &sellen, sizeof(DWORD));
			}
		}

		CloseClipboard();
	}*/

//	return success;
}

//
//	Remove current selection and copy to the clipboard
//
BOOL HexView::OnCut()
{
	BOOL success = FALSE;

	// can only 'cut' when in Insert mode
	if(m_nEditMode != HVMODE_INSERT)
		return FALSE;

	if(SelectionSize() > 0)
	{
		// copy selected text to clipboard then erase current selection
		success = OnCopy() && OnClear();
	}

	return success;
}

//
//	Remove the current selection
//
BOOL HexView::OnClear()
{
	BOOL success = FALSE;

	// can only 'delete' when in Insert mode
	switch(m_nEditMode)
	{
	case HVMODE_READONLY:
		break;

	case HVMODE_INSERT:

		if(SelectionSize() > 0)
		{
			ForwardDelete();
		}

		break;

	case HVMODE_OVERWRITE:

		if(SelectionSize() > 0)
		{
			BYTE b[] = { 0 };
			success = FillData(b, 1, SelectionSize());
			//ContentChanged(m_nCursorOffset, SelectionSize(), HVMETHOD_OVERWRITE);
		}

		break;
	}

	return success;
}

//
//
//
VOID HexView::ClipboardShutdown()
{
	// is our dataobject still on the clipboard?
	if(S_OK == OleIsCurrentClipboard(m_pLastDataObject))
	{
		DWORD_PTR len;

		// see how much data we put there
		if(GetDataObjDword(m_pLastDataObject, CFSTR_HEX_DATALEN, &len) == S_OK)
		{
			// ask the user to empty the clipboard if the data's quite large
			if(len > 1024)
			{
				UINT answer;
				TCHAR szMessage[200];

				wsprintf(szMessage, _T("There is currently %dkb of data on the Clipboard.\r\n")
								    _T("Would you like to leave this data for other applications?"),
									len/1024);
				
				answer = MessageBox(m_hWnd, szMessage, _T("HexEdit"), MB_YESNO | MB_ICONQUESTION);

				if(answer != IDYES)
					OleSetClipboard(NULL);
			}
		}

		// 'flush' our dataobject
		OleFlushClipboard();
	}
}