//
//  HexViewFile.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define STRICT
#define _WIN32_WINNT 0x501
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "HexView.h"
#include "HexViewInternal.h"


BOOL HexView::OpenFile(LPCTSTR szFileName, UINT uOpenFlags)
{
	bool fReadonly  = (uOpenFlags & HVOF_READONLY) ? true : false;
	bool fQuickload = (uOpenFlags & HVOF_QUICKLOAD) ? true : false;
	bool fQuicksave = (uOpenFlags & HVOF_QUICKSAVE) ? true : false;

	// try to open as the caller requests us
	if(m_pDataSeq->open(szFileName, fReadonly, fQuickload))
	{
		TCHAR *fp;
		DWORD e = GetLastError();
		GetFullPathName(szFileName, MAX_PATH, m_szFilePath, &fp);

		RecalcPositions();

		SetCaretPos((m_nAddressWidth + m_nHexPaddingLeft) * m_nFontHeight, 0);

		UpdateMetrics();

		SetLastError(e);

		if(m_pDataSeq->isreadonly())
			m_nEditMode = HVMODE_READONLY;
		else
			m_nEditMode = HVMODE_OVERWRITE;

		m_nSearchLen = 0;
		return TRUE;
	}

	return FALSE;
}
BOOL HexView::InitBuf(const BYTE *buffer, size_t len, bool copybuf, bool readonly)
{
	if(m_pDataSeq->init(buffer, len, copybuf))
	{
		m_szFilePath[0] = '\0';
		
		m_nEditMode = readonly ? HVMODE_READONLY : HVMODE_OVERWRITE;

		RecalcPositions();
		SetCaretPos((m_nAddressWidth + m_nHexPaddingLeft) * m_nFontHeight, 0);
		UpdateMetrics();

		m_nSearchLen = 0;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL HexView::CloseFile()
{
	ClipboardShutdown();
	m_szFilePath[0]		= '\0';
	m_nSearchLen = 0;
	return TRUE;
}

BOOL HexView::SaveFile(LPCTSTR szFileName, UINT uMethod)
{
	// filename might be NULL, in which case we overwrite current file
	if(m_pDataSeq->save(szFileName))
	{
		if(szFileName)
		{
			TCHAR *fp;
			GetFullPathName(szFileName, MAX_PATH, m_szFilePath, &fp);
		}

		UpdateMetrics();
		m_nSearchLen = 0;

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL HexView::ClearFile()
{
	if(m_pDataSeq)
	{
		m_pDataSeq->clear();
	}

	m_nVScrollPos		= 0;
	m_nHScrollPos		= 0;

	m_nSelectionStart	= 0;
	m_nSelectionEnd		= 0;
	m_nCursorOffset		= 0;
	m_nSearchLen		= 0;
	m_szFilePath[0]		= '\0';

	UpdateMetrics();

	return TRUE;
}

BOOL HexView::RevertFile()
{
	if(m_pDataSeq)
	{
		m_pDataSeq->clear();
	}

	m_nSearchLen = 0;

	if(m_szFilePath[0])
		return OpenFile(m_szFilePath, false);
	else
		return TRUE;
}

BOOL HexView::ImportFile(LPCTSTR szFileName, UINT uImportFlags)
{
	size_w len;

	bool fReadonly  = (uImportFlags & HVOF_READONLY) ? true : false;
	bool fQuickload = (uImportFlags & HVOF_QUICKLOAD) ? true : false;

	if(m_nEditMode == HVMODE_READONLY || m_pDataSeq == 0)
		return 0;

	m_nSelectionStart = m_nCursorOffset;

	if(m_nEditMode == HVMODE_OVERWRITE)
	{
		len = m_pDataSeq->replace_file(szFileName, m_nCursorOffset, fQuickload);
	}
	else if(m_nEditMode == HVMODE_INSERT)
	{
		len = m_pDataSeq->insert_file(szFileName, m_nCursorOffset, fQuickload);
	}

	m_nSelectionEnd = m_nCursorOffset + len;
	m_nCursorOffset = m_nCursorOffset + len;

	UpdateMetrics();
	ScrollToCaret();
	//SetCursor(

	return len ? TRUE : FALSE;
}
