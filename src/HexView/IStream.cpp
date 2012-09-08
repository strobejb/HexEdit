//
//  IStream.cpp
//	Implementation of the IStream COM interface
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//
//
//	TODO: Implement IStream::Read
//

#define STRICT

#include <windows.h>
#include "HexViewInternal.h"

#define DEFNAME L"Catch22 IStream"

class CStream : public IStream
{
public:
	//
    // IUnknown members
	//
    HRESULT __stdcall QueryInterface	(REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef			(void);
    ULONG   __stdcall Release			(void);
		
    //
	// ISequentialStream members
	//
	HRESULT __stdcall Read				(void *pv, ULONG cb, ULONG *pcbRead);
	HRESULT __stdcall Write				(const void *pv, ULONG cb, ULONG *pcbWritten);

    //
	// IStream members
	//
	HRESULT __stdcall Seek				(LARGE_INTEGER  dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	HRESULT __stdcall SetSize			(ULARGE_INTEGER libNewSize);
	HRESULT __stdcall CopyTo			(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	HRESULT __stdcall Commit			(DWORD grfCommitFlags);
	HRESULT __stdcall Revert			(void);
	HRESULT __stdcall LockRegion		(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT __stdcall UnlockRegion		(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT __stdcall Stat				(STATSTG *pstatstg, DWORD grfStatFlag);
	HRESULT __stdcall Clone				(IStream **ppstm);

	//
    // Constructor / Destructor
	//
    CStream(HexView *pHexView, HexSnapShot *pSnapShot);
    ~CStream();
	
private:

    //
	// private members and functions
	//
    LONG			m_lRefCount;

	STATSTG			m_statstg;			// each IStream needs one of these
	ULARGE_INTEGER	m_nOffset;			// offset within the stream
	ULARGE_INTEGER	m_nLength;			// length of the stream
	
	HexView			* m_pHexView;		// HexView that we will stream data from
	HexSnapShot		* m_pSnapShot;		// the snapshot of data that we stream
};

//
//	Constructor
//
CStream::CStream(HexView *hvp, HexSnapShot *ss) 
{
	m_lRefCount					= 1;
	m_pHexView					= hvp;
	m_pSnapShot					= ss;
	
	// stream metrics
	m_nOffset.QuadPart			= 0;
	m_nLength.QuadPart			= 0;//len;

	// stream status
	m_statstg.type				= STGTY_STREAM;		// IStream object
	m_statstg.cbSize.QuadPart	= 0;//len;				// Set to the length of our stream object
	m_statstg.grfLocksSupported = 0;				// Region locking not supported
	m_statstg.grfMode			= 0;				// access mode
	m_statstg.clsid				= CLSID_NULL;		// not used for IStreams
	m_statstg.grfStateBits		= 0;				// not used for IStreams
	m_statstg.reserved			= 0;				// reserved for

	CoFileTimeNow(&m_statstg.ctime);				// creation time
	CoFileTimeNow(&m_statstg.atime);				// last access time
	CoFileTimeNow(&m_statstg.mtime);				// last modify time
}

//
//	Destructor
//
CStream::~CStream()
{
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CStream::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CStream::Release(void)
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
HRESULT __stdcall CStream::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IStream || iid == IID_IUnknown || iid == IID_ISequentialStream)
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

//
//	ISequentialStream::Read
//
HRESULT __stdcall CStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	ULONG available;

	if(pv == 0)
		return STG_E_INVALIDPOINTER;

	available = min(cb, (ULONG)(m_nLength.QuadPart - m_nOffset.QuadPart));

	// extract the data from HexView
	m_pSnapShot->Render((size_w)m_nOffset.QuadPart, (BYTE *)pv, available);

	m_nOffset.QuadPart += available;

	//m_pHexView->m_pDataSeq->rendersnapshot(
	//*pcbRead = m_pHexView->GetData((BYTE *)pv, m_nOffset.QuadPart, available);
	
	return S_OK;
}

//
//	ISequentialStream::Write
//
HRESULT __stdcall CStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	if(pv == 0)
		return STG_E_INVALIDPOINTER;

	return E_NOTIMPL;
}

//
//	IStream::Seek
//
HRESULT __stdcall CStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	switch(dwOrigin)
	{
	case STREAM_SEEK_SET:	m_nOffset.QuadPart = dlibMove.QuadPart;						 break;
	case STREAM_SEEK_CUR:	m_nOffset.QuadPart = m_nOffset.QuadPart + dlibMove.QuadPart; break;
	case STREAM_SEEK_END:	m_nOffset.QuadPart = m_nLength.QuadPart - dlibMove.QuadPart; break;
	}
	
	if(plibNewPosition)
		*plibNewPosition = m_nOffset;

	return S_OK;
}

//
//	IStream::SetSize
//
HRESULT __stdcall CStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return S_OK;
}

//
//	IStream::CopyTo
//
HRESULT __stdcall CStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	BYTE  buf[512];
	DWORD len, written;

	if(pcbRead)
		pcbRead->QuadPart = 0;

	if(pcbWritten)
		pcbWritten->QuadPart = 0;

	while(cb.QuadPart)
	{
		len = (ULONG)min(cb.QuadPart, 512);
		
		if(m_pHexView->GetData((size_w)m_nOffset.QuadPart, buf, len))
		{
			pstm->Write(buf, len, &written);

			if(pcbRead)
				pcbRead->QuadPart += len;

			if(pcbWritten)
				pcbWritten->QuadPart += written;
		}

		cb.QuadPart -= len;	
	}

	return S_OK;
}

//
//	IStream::Commit
//
HRESULT __stdcall CStream::Commit(DWORD grfCommitFlags)
{
	// Transacted mode is not supported
	return S_OK;
}

//
//	IStream::Revert
//
HRESULT __stdcall CStream::Revert()
{
	// Transacted mode is not supported
	return S_OK;
}

//
//	IStream::LockRegion
//
HRESULT __stdcall CStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	// locking is not supported
	return STG_E_INVALIDFUNCTION;
}

//
//	IStream::UnlockRegion
//
HRESULT __stdcall CStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	// locking is not supported
	return STG_E_INVALIDFUNCTION;
}

//
//	IStream::Stat
//
HRESULT __stdcall CStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	if(pstatstg == 0)
		return STG_E_INVALIDPOINTER;

	// return our STATSTG to the caller
	m_statstg.cbSize.QuadPart = m_pHexView->Size();
	*pstatstg = m_statstg;

	switch(grfStatFlag)
	{
	case STATFLAG_DEFAULT:

		// allocate a new buffer for the name
		if((pstatstg->pwcsName = (WCHAR *)CoTaskMemAlloc(sizeof(DEFNAME))) == 0)
			return STG_E_INSUFFICIENTMEMORY;

		lstrcpyW(pstatstg->pwcsName, DEFNAME);
		break;

	case STATFLAG_NONAME:
		pstatstg->pwcsName = 0;
		break;

	default:
		return STG_E_INVALIDFLAG;
	}

	return S_OK;
}

//
//	IStream::Clone
//
HRESULT __stdcall CStream::Clone(IStream **ppstm)
{
	if(ppstm == 0)
		return STG_E_INVALIDPOINTER;

	// duplicate this stream
	CStream *pStream	= new CStream(m_pHexView, m_pSnapShot);
	pStream->m_nOffset  = m_nOffset;

	*ppstm = pStream;

	return S_OK;
}

IStream *CreateStream(HexView * pHexView)
{
	return new CStream(pHexView, 0);
}
