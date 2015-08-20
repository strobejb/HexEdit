//
//  IDataObject.cpp
//	Implementation of the IDataObject COM interface
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//


#define STRICT

#include <windows.h>
#include <tchar.h>
#include "trace.h"
#include "HexViewInternal.h"

// defined in ienumformat.cpp
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);

// defined in istream.cpp
IStream *CreateStream(HexView * pHexView);


class CDataObject : public IDataObject
{
	struct DataItem
	{
		FORMATETC FormatEtc;
		STGMEDIUM StgMedium;
		BOOL	  fOwnMedium;
		BOOL	  fDelayRender;
	};

public:
	//
    // IUnknown members
	//
    HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef (void);
    ULONG   __stdcall Release (void);
		
    //
	// IDataObject members
	//
    HRESULT __stdcall GetData				(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
    HRESULT __stdcall GetDataHere			(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
    HRESULT __stdcall QueryGetData			(FORMATETC *pFormatEtc);
	HRESULT __stdcall GetCanonicalFormatEtc (FORMATETC *pFormatEct,  FORMATETC *pFormatEtcOut);
    HRESULT __stdcall SetData				(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease);
	HRESULT __stdcall EnumFormatEtc			(DWORD      dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
	HRESULT __stdcall DAdvise				(FORMATETC *pFormatEtc,  DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
	HRESULT __stdcall DUnadvise				(DWORD      dwConnection);
	HRESULT __stdcall EnumDAdvise			(IEnumSTATDATA **ppEnumAdvise);
	
	//
    // Constructor / Destructor
	//
    CDataObject(int max);
    ~CDataObject();
	
	HRESULT SetData(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease, BOOL fDelayed);
	void SetSnapShot(HexSnapShot *hss);
	
private:

	int     LookupFormatEtc(FORMATETC *pFormatEtc);
	HRESULT DeepCopyStgMedium(FORMATETC *pFormatEtc, STGMEDIUM *pSrcMed, STGMEDIUM *pDestMed);

    //
	// any private members and functions
	//
    LONG			m_lRefCount;

	FORMATETC	  *	m_pFormatEtc;
	STGMEDIUM	  *	m_pStgMedium;
	BOOL		  *	m_pOwnMedium;
	BOOL		  *	m_pDelayed;
	LONG			m_nNumFormats;
	LONG			m_nMaxFormats;
	
	HexSnapShot   * m_pSnapShot;
};

//
//	Constructor
//
CDataObject::CDataObject(int max)
{
	m_lRefCount   = 1;
	m_nNumFormats = 0;
	m_nMaxFormats = max;
	m_pSnapShot   = 0;
	
	m_pFormatEtc  = new FORMATETC[m_nMaxFormats];
	m_pStgMedium  = new STGMEDIUM[m_nMaxFormats];
	m_pOwnMedium  = new BOOL[m_nMaxFormats];
	m_pDelayed    = new BOOL[m_nMaxFormats];
}

//
//	Destructor
//
CDataObject::~CDataObject()
{
	LONG i;

	// check our BOOL array to see which STGMEDIUMs need releasing
	for(i = 0; i < m_nNumFormats; i++)
	{
		if(m_pOwnMedium[i])
			ReleaseStgMedium(&m_pStgMedium[i]);
	}

	// cleanup
	if(m_pFormatEtc) delete[] m_pFormatEtc;
	if(m_pStgMedium) delete[] m_pStgMedium;
	if(m_pOwnMedium) delete[] m_pOwnMedium;
	if(m_pDelayed)   delete[] m_pDelayed;
}

void CDataObject::SetSnapShot(HexSnapShot *hss)
{
	m_pSnapShot = hss;
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDataObject::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDataObject::Release(void)
{
    // decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IDataObject || iid == IID_IUnknown)
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

HGLOBAL DupMem(HGLOBAL hMem)
{
	// lock the source memory object
	size_t  len    = GlobalSize(hMem);
	PVOID	source = GlobalLock(hMem);
	
	// create a fixed "global" block - i.e. just
	// a regular lump of our process heap
	PVOID   dest   = GlobalAlloc(GMEM_FIXED, len);

	memcpy(dest, source, len);

	GlobalUnlock(hMem);

	return dest;
}

//
//	Duplicate specified buffer as a HGLOBAL
//
HANDLE CopyBuffer(PVOID pmem, ULONG plen)
{
    void  *ptr;
	
    // allocate and lock a global memory buffer. Make it fixed
    // data so we don't have to use GlobalLock
    ptr = (void *)GlobalAlloc(GMEM_FIXED, plen);
	
    // copy the string into the buffer
    memcpy(ptr, pmem, plen);
	
    return ptr;
}


//
//	'Deep-copy' the STGMEDIUM structure from source to destination, 
//  calling OleDuplicateData to copy the HGLOBAL/GDI contents
//
HRESULT CDataObject::DeepCopyStgMedium(FORMATETC *pFormatEtc, STGMEDIUM *pSrcMed, STGMEDIUM *pDestMed)
{
	// duplicate the contents of the storage-medium
	switch(pSrcMed->tymed)
	{
	case TYMED_ISTREAM:

		// Bump the reference-count and copy the pointer across
		pDestMed->pstm = pSrcMed->pstm;
		pDestMed->pstm->AddRef();
		break;

	case TYMED_ISTORAGE:

		// Bump the reference-count and copy the pointer across
		pDestMed->pstg = pSrcMed->pstg;
		pDestMed->pstg->AddRef();
		break;

	case TYMED_HGLOBAL: 
	case TYMED_GDI: 
	case TYMED_MFPICT: 
	case TYMED_ENHMF:
	case TYMED_FILE:

		// duplicate the HGLOBAL/HBITMAP/whatever
		pDestMed->hGlobal = OleDuplicateData(pSrcMed->hGlobal, pFormatEtc->cfFormat, 0);
		break;

	default:
		return DV_E_FORMATETC;
	}

	// copy the remaining structure-members across
	pDestMed->tymed			 = pSrcMed->tymed;
	pDestMed->pUnkForRelease = NULL;

	// copy the IUnknownForRelease if present
	if(pSrcMed->pUnkForRelease)
	{
		pDestMed->pUnkForRelease = pSrcMed->pUnkForRelease;
		pDestMed->pUnkForRelease->AddRef();
	}

	return S_OK;
}

//
// Private routine. See if we can match the requested FORMATETC
//
int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc)
{
	for(int i = 0; i < m_nNumFormats; i++)
	{
		bool fMatchedLindex = true;
		
		// if the 'lindex' is >= 0 then we need to check this matches also
		if(pFormatEtc->lindex != -1 && pFormatEtc->lindex != m_pFormatEtc[i].lindex)
			fMatchedLindex = false;

		if((pFormatEtc->tymed    &  m_pFormatEtc[i].tymed)   &&
			pFormatEtc->cfFormat == m_pFormatEtc[i].cfFormat && 
			pFormatEtc->dwAspect == m_pFormatEtc[i].dwAspect &&
			fMatchedLindex
			)
		{
			return i;
		}
	}

	return -1;
}

//
//	IDataObject::GetData
//
HRESULT __stdcall CDataObject::GetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	int idx;

	//
	// try to match the requested FORMATETC with one of our supported formats
	//
	if((idx = LookupFormatEtc(pFormatEtc)) == -1)
	{
		return DV_E_FORMATETC;
	}

	// check if we can do a delay-render of CF_TEXT
	if(pFormatEtc->cfFormat == CF_TEXT && m_pSnapShot != 0)
	{
		if((pFormatEtc->tymed & TYMED_HGLOBAL) && m_pStgMedium[0].hGlobal == 0)
		{
			pMedium->tymed			= TYMED_HGLOBAL;
			pMedium->pUnkForRelease = 0;

			// render snapshot as a HGLOBAL
			if((pMedium->hGlobal = m_pSnapShot->RenderAsHGlobal()) == 0)
				return E_OUTOFMEMORY;

			TRACEA("Delay rendering HGLOBAL!\n");
			return S_OK;
		}
	}

	// found a match! transfer the data into the supplied storage-medium
	return DeepCopyStgMedium(&m_pFormatEtc[idx], &m_pStgMedium[idx], pMedium);
}

//
//	IDataObject::GetDataHere
//
HRESULT __stdcall CDataObject::GetDataHere (FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	// GetDataHere is only required for IStream and IStorage mediums
	// It is an error to call GetDataHere for things like HGLOBAL and other clipboard formats
	//
	//	OleFlushClipboard 
	//
	return DATA_E_FORMATETC;
}

//
//	IDataObject::QueryGetData
//
//	Called to see if the IDataObject supports the specified format of data
//
HRESULT __stdcall CDataObject::QueryGetData (FORMATETC *pFormatEtc)
{
	TRACEA("%d %d\n", LookupFormatEtc(pFormatEtc), pFormatEtc->cfFormat);

	return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

//
//	IDataObject::GetCanonicalFormatEtc
//
HRESULT __stdcall CDataObject::GetCanonicalFormatEtc (FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
	// Apparently we have to set this field to NULL even though we don't do anything else
	pFormatEtcOut->ptd = NULL;
	return E_NOTIMPL;
}

//
//	IDataObject::SetData
//
//	Private routine
//
HRESULT CDataObject::SetData(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease, BOOL fDelayed)
{
	int idx;
	
	if(pFormatEtc->tymed != pMedium->tymed)
		return DV_E_TYMED;

	// see if we already have this format
	if((idx = LookupFormatEtc(pFormatEtc)) != -1)
	{
		// make sure we have enough room
		if(m_nNumFormats == m_nMaxFormats)
			return E_FAIL;

		// free the existing data
		if(m_pOwnMedium[idx])
			ReleaseStgMedium(&m_pStgMedium[idx]);
	}
	else
	{
		// add to the end 
		idx = m_nNumFormats++;
	}

	// copy the format across, and remember who owns it
	m_pFormatEtc[idx] = *pFormatEtc;
	m_pStgMedium[idx] = *pMedium;
	m_pOwnMedium[idx] =  fRelease;		
	m_pDelayed[idx]   =  fDelayed;

	return S_OK;
}

//
//	IDataObject::SetData
//
HRESULT __stdcall CDataObject::SetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium,  BOOL fRelease)
{
	return SetData(pFormatEtc, pMedium, fRelease, FALSE);
}

//
//	IDataObject::EnumFormatEtc
//
HRESULT __stdcall CDataObject::EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	if(dwDirection == DATADIR_GET)
	{
		// for Win2k+ you can use the SHCreateStdEnumFmtEtc API call, however
		// to support all Windows platforms we need to implement IEnumFormatEtc ourselves.
		return CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);
	}
	else
	{
		// the direction specified is not support for drag+drop
		return E_NOTIMPL;
	}
}

//
//	IDataObject::DAdvise
//
HRESULT __stdcall CDataObject::DAdvise (FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::DUnadvise
//
HRESULT __stdcall CDataObject::DUnadvise (DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::EnumDAdvise
//
HRESULT __stdcall CDataObject::EnumDAdvise (IEnumSTATDATA **ppEnumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	Helper function to set the specified DWORD as a HGLOBAL
//
HRESULT SetDataObjBuf(IDataObject *pDataObject, LPCTSTR szFormat, PVOID pData, DWORD nLength)
{
	// register the specified clipboard-format
	CLIPFORMAT  cfFormat = RegisterClipboardFormat(szFormat);
	
	FORMATETC   fmtetc   = { cfFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM   stgmed   = { TYMED_HGLOBAL, NULL, NULL };
	
	// allocate a HGLOBAL to store the data
	if((stgmed.hGlobal = GlobalAlloc(GPTR, nLength)) == 0)
		return E_FAIL;

	// copy the data across
	memcpy(stgmed.hGlobal, pData, nLength);

	// pass the HGLOBAL to the data-object
	return pDataObject->SetData(&fmtetc, &stgmed, TRUE);
}

//
//	Helper function to retrieve the specified DWORD as a HGLOBAL
//
HRESULT GetDataObjBuf(IDataObject *pDataObject, LPCTSTR szFormat, PVOID pData, DWORD nLength)
{
	// register the specified clipboard-format
	CLIPFORMAT  cfFormat = RegisterClipboardFormat(szFormat);
	
	FORMATETC   fmtetc   = { cfFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM   stgmed   = { TYMED_HGLOBAL, NULL, NULL };

	PVOID   ptmp;
	HRESULT success;

	// ask for the specified format
	if((success = pDataObject->GetData(&fmtetc, &stgmed)) == S_OK)
	{
		success = E_FAIL;

		// extract the data stored inside the HGLOBAL
		if((ptmp = GlobalLock(stgmed.hGlobal)) != 0)
		{
			memcpy(pData, ptmp, min(nLength, GlobalSize(stgmed.hGlobal)));
			GlobalUnlock(stgmed.hGlobal);

			success = S_OK;
		}

		ReleaseStgMedium(&stgmed);
	}

	return success;
}

HRESULT SetDataObjDword(IDataObject *pDataObject, LPCTSTR szFormat, DWORD_PTR dwValue)
{
	return SetDataObjBuf(pDataObject, szFormat, &dwValue, sizeof(dwValue));
}

HRESULT GetDataObjDword(IDataObject *pDataObject, LPCTSTR szFormat, DWORD_PTR *pdwValue)
{	
	return GetDataObjBuf(pDataObject, szFormat, pdwValue, sizeof(*pdwValue));
}


HGLOBAL HexView::BuildHGlobal(size_w offset, size_w length)
{
	HGLOBAL hMem;
	PBYTE	ptr;

	if((hMem = GlobalAlloc(GPTR, (ULONG)(length + 1))) == 0)
		return 0;

	if((ptr = (BYTE *)GlobalLock(hMem)) != 0)
	{
		GetData(offset, ptr, (ULONG)length);
		ptr[length] = '\0';
	}

	GlobalUnlock(hMem);
	return hMem;
}

HexSnapShot * HexView::CreateSnapshot(size_w offset, size_w length)
{
	HexSnapShot *hss;
	size_t desclen = 0;

	// anything less than 64kb and we won't bother
	//if(length < 0x10000)
	//	return NULL;

	// calculate how many snapshot items there are
	m_pDataSeq->takesnapshot(offset, length, 0, &desclen);
	
	// allocate a new snapshot object
	hss = new HexSnapShot(this);
	hss->m_desclist = new sequence::span_desc[desclen+1];
	hss->m_count    = desclen;
	hss->m_length	= length;
	
	// take the snapshot again, this time fill in the data
	m_pDataSeq->takesnapshot(offset, length, hss->m_desclist, &desclen);

	return hss;
}

//
//	HexView helper function for creating DataObjects
//
bool HexView::CreateDataObject (size_w offset, size_w length, IDataObject **ppDataObject)
{
	if(ppDataObject == 0)
		return false;

	// create the data-object
	CDataObject *pDataObject = new CDataObject(10);

	// load it with data!
	FORMATETC fmtetc = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed = { TYMED_HGLOBAL };

	// set the CF_TEXT content as a HGLOBAL
	// The handle will be NULL - we will delay-render it only when necessary
	stgmed.hGlobal = NULL;	//BuildHGlobal(offset, length);
	pDataObject->SetData(&fmtetc, &stgmed, TRUE);

	// set the CF_TEXT content as an IStream
	/*fmtetc.tymed = TYMED_ISTREAM;
	stgmed.tymed = TYMED_ISTREAM;
	stgmed.pstm  = CreateStream(this);
	pDataObject->SetData(&fmtetc, &stgmed, TRUE);*/

	// set the RLE32HexBinary
	SetDataObjDword(pDataObject, CFSTR_HEX_DATALEN, (DWORD_PTR)SelectionSize());

	// set the HexHWND
	SetDataObjDword(pDataObject, CFSTR_HEX_HWND, (DWORD_PTR)m_hWnd);

	// create a snapshot of the specified range of data, but
	// store the *address* of it on the clipboard. This is fine
	// because the data is only valid within the context of *this* process
	HexSnapShot *hss;
		
	if((hss = CreateSnapshot(offset, length)) != 0)
	{
		SetDataObjBuf(pDataObject, CFSTR_HEX_SNAPSHOTPTR, &hss, sizeof(HexSnapShot *));
		pDataObject->SetSnapShot(hss);
	}
	
	// return the data-object back to the caller as a pure IDataObject interface
	*ppDataObject = pDataObject;
	return pDataObject ? true : false;
}