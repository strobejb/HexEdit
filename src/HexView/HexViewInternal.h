//
//  HexViewInternal.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef HEXVIEW_INTERNAL_INCLUDED
#define HEXVIEW_INTERNAL_INCLUDED
#include "sequence.h"
#undef HEXVIEW_64
//#define HEXVIEW_64

#ifdef HEXVIEW_64
//typedef UINT64 size_w;
//#define SIZEW_BITS 64
#pragma warning(disable : 4244)
#else
//typedef UINT32 size_w;
//#define SIZEW_BITS 32
#endif

#include <uxtheme.h>

#include "HexView.h"

#define HV_MAX_COLS 64

//typedef sequence<BYTE> byte_seq;

struct _HIGHLIGHT_LIST;

// hex-hittest regions
#define HVHT_NONE		0x00
#define HVHT_MAIN		0x01
//#define HVHT_ADDRESS	(HVHT_MAIN | 0x02)
//#define HVHT_HEXCOL		(HVHT_MAIN | 0x04)
//#define HVHT_ASCCOL		(HVHT_MAIN | 0x08)
#define HVHT_SELECTION	0x02
#define HVHT_RESIZE		0x10
#define HVHT_RESIZE0    (0x20 | HVHT_RESIZE)
#define HVHT_BOOKMARK	0x100
#define HVHT_BOOKCLOSE	(HVHT_BOOKMARK | 0x200)
#define HVHT_BOOKSIZE	(HVHT_BOOKMARK | 0x400)
#define HVHT_BOOKGRIP	(HVHT_BOOKMARK | 0x800)
#define HVHT_BOOKEDIT	(HVHT_BOOKMARK | 0x1000)
#define HVHT_BOOKEDIT1	(HVHT_BOOKEDIT | 0x2000)
#define HVHT_BOOKEDIT2	(HVHT_BOOKEDIT | 0x4000)
#define HVHT_BOOKEDIT3	(HVHT_BOOKEDIT | 0x8000)


#define BOOKMARK_XOFFSET 30
#define BOOKMARK_GRIPWIDTH 10

class HexView;
class HexSnapShot;

typedef struct _BOOKNODE
{
	_BOOKNODE() : prev(0), next(0)
	{
		memset(&bookmark, 0, sizeof(bookmark));
	}
	
	BOOKMARK bookmark;
	_BOOKNODE *prev;
	_BOOKNODE *next;

} BOOKNODE;


enum SELMODE { SEL_NONE, SEL_NORMAL, SEL_MARGIN, SEL_DRAGDROP };

// HexEdit private clipboard formats
#define CFSTR_HEX_DATALEN		_T("HexDataLength")
#define CFSTR_HEX_HWND			_T("HexHWND")
#define CFSTR_HEX_SNAPSHOTPTR	_T("HexSnapshotPtr")
//#define CF_PRI_RLE32 _T("RLE32HexBinary")

//
//	Define our custom HexView class. Inherit from the IDropTarget interface
//	to enable this window to become an OLE drop-target
//
class HexView : public IDropTarget
{
	friend class HexSnapShot;

public:
	HexView(HWND hwnd);//, byte_seq *seq);
	~HexView();

	LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);



	//
	//	IDropTarget Interface
	//
	STDMETHODIMP QueryInterface (REFIID iid, void ** ppvObject);
    STDMETHODIMP_(ULONG) AddRef (void);
    STDMETHODIMP_(ULONG) Release (void);

    // IDropTarget implementation
    STDMETHODIMP DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

	BOOL SetCurSel(size_w selStart, size_w selEnd);

	//
	//	
	//
	LRESULT OnPaint();
	LRESULT OnNcPaint(HRGN hrgnUpdate);
	LRESULT OnSetFont(HFONT hFont);
	UINT HitTest(int x, int y, RECT *phirc = 0, BOOKNODE **pbn = 0);
	LRESULT OnRButtonDown(UINT nFlags, int x, int y);
	LRESULT OnLButtonDown(UINT nFlags, int x, int y);
	LRESULT OnLButtonDblClick(UINT nFlags, int x, int y);
	LRESULT OnLButtonUp(UINT nFlags, int x, int y);
	LRESULT OnContextMenu(HWND hwndParam, int x, int y);
	LRESULT OnMouseMove(UINT nFlags, int x, int y);
	LRESULT OnMouseWheel(int nDelta);
	LRESULT OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage);
	LRESULT OnTimer(UINT_PTR Id);
	LRESULT OnSetFocus();
	LRESULT OnKillFocus();
	LRESULT OnSetCursor(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(UINT nFlags, int width, int height);
	LRESULT OnKeyDown(UINT nVirtualKey, UINT nRepeatCount, UINT nFlags);
	LRESULT OnChar(UINT nChar);

	LRESULT OnSetCurPos(size_w pos);
	LRESULT OnSetSelStart(size_w pos);
	LRESULT OnSetSelEnd(size_w pos);
	LRESULT OnSelectAll();

	BOOL OnCopy();
	BOOL OnCut();
	BOOL OnPaste();
	BOOL OnClear();

	BOOL FindInit(BYTE *pattern, size_t length, BOOL searchback, BOOL matchcase);
	BOOL FindNext(size_w *result, UINT options);

	// internal
	int		SearchBlock(BYTE *block, int start, int length, int *partial, bool matchcase = true);
	bool	SearchCompile(BYTE *pat, size_t length);
	LRESULT QueryProgressNotify(UINT code, size_w pos, size_w len);



	LRESULT NotifyParent(UINT nNotifyCode, NMHDR *optional = 0);
	VOID FakeSize();

	LRESULT OnHScroll(UINT nSBCode, UINT nPos);
	LRESULT OnVScroll(UINT nSBCode, UINT nPos);

	VOID OnLengthChange(size_w nNewLength);

	HMENU SetContextMenu(HMENU hMenu);
	VOID Scroll(int dx, int dy);
	HRGN ScrollRgn(int dx, int dy, bool fReturnUpdateRgn);
	VOID SetupScrollbars();
	VOID RecalcPositions();
	VOID UpdateResizeBarPos();
	VOID UpdateMetrics();

	HMENU CreateContextMenu();

	int  GetLogicalX(int nScreenX, int *pane, int *subitem = 0);
	int  GetLogicalY(int nScreenY);
	void PositionCaret(int x, int y, int pane);
	int  LogToPhyXCoord(int x, int pane);
	void CaretPosFromOffset(size_w offset, int *x, int *y);

	size_w OffsetFromPhysCoord(int x, int y, int *pane = 0, int *lx = 0, int *ly = 0, int *subitem = 0);
	void   RepositionCaret();
	VOID   ScrollToCaret();
	BOOL   ScrollTo(size_w offset);
	BOOL   ScrollTop(size_w offset);
	bool   PinToBottomCorner();
	void   PinToOffset(size_w offset);
	void   UpdatePinnedOffset();

	bool   Undo();
	bool   Redo();
	bool   CanUndo();
	bool   CanRedo();
	//
	//
	//
	VOID	InvalidateRange(size_w start, size_w finish);
	void	RefreshWindow();
	int		PaintLine(HDC hdc, size_w nLineNo, BYTE *data, size_t datalen, seqchar_info *bufinfo);
	size_t	FormatAddress(size_w addr, TCHAR *buf, size_t buflen);
	size_t	FormatHexUnit(BYTE *data, TCHAR *buf, size_t buflen);
	size_t	FormatLine(BYTE *data, size_t length, size_w offset, TCHAR *buf, size_t buflen, ATTR *attrList, seqchar_info *infobuf, bool fIncSelection);

	size_t	FormatData(HEXFMT_PARAMS *fmtParams);


	VOID IdentifySearchPatterns(BYTE *data, size_t len, seqchar_info *infobuf);
	
	LONG Highlight(BOOL fEnable);

	BOOL GetHighlightCol(size_w offset, int pane, BOOKNODE * itemStart, HEXCOL *col1, HEXCOL *col2, bool fModified, bool fMatched, bool fIncSelection = true);

	BOOKNODE * AddBookmark(BOOKMARK * bookm);
	BOOL DelBookmark(BOOKNODE *);
	BOOL BookmarkRect(BOOKMARK *bm, RECT *rect);
	BOOL	   GetBookmark(BOOKNODE *, BOOKMARK *param);
	BOOKNODE * EnumBookmark(BOOKNODE *, BOOKMARK *param);
	BOOL SetBookmark(BOOKNODE *, BOOKMARK *param);
	BOOL ClearBookmarks();
	BOOKNODE * FindBookmark(size_w startoff, size_w endoff);
	void DrawNoteStrip(HDC hdc, int nx, int ny, BOOKNODE *pbn);

	static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	WNDPROC m_oldEditProc;

	BOOL EditBookmark(BOOKNODE *pbn, RECT *rect, bool fTitle);
	BOOL ExitBookmark();

	BOOL	 SetHexColour(UINT uIndex, COLORREF col);
	COLORREF GetHexColour(UINT uIndex);
	COLORREF RealiseColour(COLORREF cr);

	bool	CheckStyle(UINT uStyleFlag);
	UINT	SetStyle(UINT uMask, UINT uStyles);
	UINT	GetStyle(UINT uMask);
	UINT	GetStyleMask(UINT uStyleFlag);
	UINT	SetGrouping(UINT nBytes);
	UINT	GetGrouping();
	UINT	GetLineLen();
	UINT	SetLineLen(UINT nLineLen);
	BOOL	SetPadding(int left, int right);

	size_w  NumFileLines(size_w length);
	bool	IsOverResizeBar(int x);
	int		UnitWidth();

	size_w  SelectionSize();
	size_w  SelectionStart();
	size_w  SelectionEnd();
	size_w	Size();

	size_w EnterData(BYTE *pDest, size_w nLength, bool fAdvanceCaret, bool fReplaceSelection, bool fSelectData, HexSnapShot *hss = 0);
	ULONG SetData(size_w offset, BYTE *buf, ULONG nLength);
	ULONG GetData(size_w offset, BYTE *pBuf, ULONG nLength);
	ULONG FillData(BYTE *buf, ULONG buflen, size_w len);

	bool ForwardDelete();
	bool BackDelete();

	bool AllowChange(size_w offset, size_w length, UINT method, BYTE *data = 0, UINT mask = 0);
	void ContentChanged(size_w offset, size_w length, UINT method);
	//size_w nStartOffset, size_w nLength

	BOOL OpenFile(LPCTSTR szFileName, UINT uMethod);
	BOOL SaveFile(LPCTSTR szFileName, UINT uMethod);
	BOOL ImportFile(LPCTSTR szFileName, UINT uMethod);
	BOOL InitBuf(const BYTE *pBuffer, size_t nLength, bool copybuf, bool readonly);
	BOOL CloseFile();
	BOOL ClearFile();
	BOOL RevertFile();

	BOOL SetRedraw(BOOL fRedraw);


private:

	//
	//	private helper functions
	//
	DWORD	DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed);
	bool	QueryDataObject(IDataObject *pDataObject);
	void	RegisterDropWindow();
	void	UnregisterDropWindow();
	bool	DropData(IDataObject *pDataObject, bool fReplaceSelection, bool fSelectData);
	void	StartDrag();
	bool	CreateDataObject (size_w offset, size_w length, IDataObject **ppDataObject);
	HGLOBAL BuildHGlobal(size_w offset, size_w length);
	HexSnapShot *CreateSnapshot(size_w offset, size_w length);
	VOID	ClipboardShutdown();
	int		CalcTotalWidth();
	BOOL	SetFontSpacing(int x, int y);

	
	HWND		m_hWnd;
	HWND		m_hwndEdit;
	HTHEME		m_hTheme;


	TCHAR		m_szFilePath[MAX_PATH];

	//byte_seq *m_sequence;

	sequence	*m_pDataSeq;

	UINT	m_nControlStyles;

	// cursor
	size_w  m_nCursorOffset;
	int		m_nCaretX;
	int		m_nCaretY;
	int		m_nWhichPane;
	size_w  m_nAddressOffset;

	int     m_nDataShift;			// range from 0 to (m_nBytesPerLine-1)
	size_w  m_nLastEditOffset;
	bool	m_fCursorMoved;
	//bool	

	size_w  m_nSelectionStart;
	size_w  m_nSelectionEnd;


	// Font specific
	HFONT	m_hFont;
	int		m_nFontHeight;
	int		m_nFontWidth;


	// View dimensions
	int		m_nWindowLines;
	int		m_nWindowColumns;
	int		m_nBytesPerLine;
	int		m_nBytesPerColumn;

	// Scrollbar dimensions
	size_w	m_nVScrollPos;
	size_w  m_nVScrollMax;
	int		m_nHScrollPos;
	int		m_nHScrollMax;

	size_w  m_nVScrollPinned;

	// Drag+Drop support
	long   m_lRefCount;
    bool   m_fAllowDrop;
	bool   m_fStartDrag;
	bool   m_fDigDragDrop;


	// 
	int		m_nAddressWidth;
	int		m_nHexWidth;
	int		m_nAddressDigits;
	int		m_nHexPaddingLeft;
	int		m_nHexPaddingRight;
	int		m_nTotalWidth;
	int		m_nResizeBarPos;

	BOOKNODE * m_BookHead;
	BOOKNODE * m_BookTail;

	// hexview base colours
	COLORREF m_ColourList[HV_MAX_COLS];

	//
	// runtime flags
	//
	//BOOL	m_fMouseDown;
	bool		m_fRedrawChanges;
	SELMODE		m_nSelectionMode;
	UINT_PTR	m_nScrollTimer;
	LONG		m_nScrollCounter;
	LONG		m_nScrollMouseRemainder;
	BOOL		m_fCursorAdjustment;
	bool		m_fResizeBar;
	bool		m_fResizeAddr;
	UINT		m_nEditMode;
	int			m_nSubItem;
	HMENU		m_hUserMenu;
	
	BOOKNODE	*m_HighlightCurrent;
	UINT		m_HitTestCurrent;
	BOOKNODE	*m_HighlightHot;
	UINT		m_HitTestHot;
	BOOKNODE	m_HighlightGhost;

	IDataObject * m_pLastDataObject;

	BYTE	m_pSearchPat[64];
	ULONG	m_nSearchLen;

};	

class HexSnapShot : public IUnknown
{
	friend class HexView;

public:
	HexSnapShot(HexView *hvp)
	{
		m_lRefCount = 1;
		m_pHexView  = hvp;
	}

	~HexSnapShot()
	{
		delete[] m_desclist;
	}


    HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject)
	{
		if(iid == IID_IUnknown)
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		else
		{
			return E_NOINTERFACE;
		}
	}

    ULONG __stdcall AddRef(void)
	{
		return InterlockedIncrement(&m_lRefCount);
	}

    ULONG __stdcall Release(void)
	{
		LONG count = InterlockedDecrement(&m_lRefCount);
		
		if(count == 0)
			delete this;

		return count;
	}

	size_w Render(size_w offset, BYTE *buffer, size_w length)
	{
		return m_pHexView->m_pDataSeq->rendersnapshot(m_count, m_desclist, offset, buffer, (size_t)length);
	}

	HGLOBAL RenderAsHGlobal()
	{
		BYTE *bptr;
		
		// allocate space for the buffer, +1 for string-terminator
		if((bptr = (BYTE *)GlobalAlloc(GPTR, (DWORD)(m_length+1))) == 0)
			return 0;

		// render data and null-terminate
		Render(0, bptr, m_length);
		bptr[m_length] = 0;
		return bptr;
	}

private:

	LONG					m_lRefCount;

	HexView				*	m_pHexView;
	size_t					m_count;
	size_w					m_length;
	sequence::span_desc *	m_desclist;
};

#endif