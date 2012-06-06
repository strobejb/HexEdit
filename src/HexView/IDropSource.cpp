//
//  IDropSource.cpp
//	Implementation of the IDropSource COM interface
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//


#define STRICT

#include <windows.h>

class CDropSource : public IDropSource
{
public:
	//
    // IUnknown members
	//
    HRESULT __stdcall QueryInterface	(REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef			(void);
    ULONG   __stdcall Release			(void);
		
    //
	// IDropSource members
	//
    HRESULT __stdcall QueryContinueDrag	(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT __stdcall GiveFeedback		(DWORD dwEffect);
	
	//
    // Constructor / Destructor
	//
    CDropSource();
    ~CDropSource();
	
private:

    //
	// private members and functions
	//
    LONG	   m_lRefCount;
};

//
//	Constructor
//
CDropSource::CDropSource() 
{
	m_lRefCount = 1;
}

//
//	Destructor
//
CDropSource::~CDropSource()
{
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDropSource::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDropSource::Release(void)
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
HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IDropSource || iid == IID_IUnknown)
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
//	CDropSource::QueryContinueDrag
//
//	Called by OLE whenever Escape/Control/Shift/Mouse buttons have changed
//
HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if(fEscapePressed == TRUE)
		return DRAGDROP_S_CANCEL;	

	// if the <LeftMouse> button has been released, then do the drop!
	if((grfKeyState & MK_LBUTTON) == 0)
		return DRAGDROP_S_DROP;

	// continue with the drag-drop
	return S_OK;
}

//
//	CDropSource::GiveFeedback
//
//	Return either S_OK, or DRAGDROP_S_USEDEFAULTCURSORS to instruct OLE to use the
//  default mouse cursor images
//
HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//
//	Helper routine to create an IDropSource object
//	
HRESULT CreateDropSource(IDropSource **ppDropSource)
{
	if(ppDropSource == 0)
		return E_INVALIDARG;

	*ppDropSource = new CDropSource();

	return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;

}