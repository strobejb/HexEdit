//
//  HexView.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef HEXVIEW_INCLUDED
#define HEXVIEW_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SEQUENCE64
typedef unsigned __int64  size_w;
#else
typedef unsigned long	  size_w;
#endif

#define MAKE_SIZEW(wParam, lParam) ((size_w)(wParam) | ((size_w)(lParam) << 32))
#define WPARAM_SIZEW(sw) ((WPARAM)((size_w)(sw) & 0xffffffff))
#define LPARAM_SIZEW(sw) ((LPARAM)((size_w)(sw) >> 32))

//
//	HexView window class name
//
#define WC_HEXVIEW _T("HexView32")

typedef struct
{
	COLORREF colFG;
	COLORREF colBG;

} HEXCOL, ATTR;

typedef struct _HEXFMT_PARAMS
{
	size_w	offset;
	size_t	length;
	TCHAR	szText[400];
	ATTR	attrList[200];

} HEXFMT_PARAMS, *PHEXFMT_PARAMS;

typedef struct _BOOKMARK
{
	DWORD		flags;

	size_w		offset;
	size_w		length;
	COLORREF	col;
	COLORREF	backcol;

	TCHAR    *	pszText;
	ULONG		nMaxText;
	TCHAR    *	pszTitle;
	ULONG		nMaxTitle;

} BOOKMARK, *PBOOKMARK;

typedef void * HBOOKMARK;

//
//	HexView internal window styles
//	Be sure to update the HVS_xxx_MASK values if you change anything!!
//

// Hex column styles
#define HVS_FORMAT_HEX		0x0000		// hex format is default
#define HVS_FORMAT_DEC		0x0001		// decimal formatting
#define HVS_FORMAT_OCT		0x0002		// octal formatting
#define HVS_FORMAT_BIN		0x0003		// binary formatting
#define HVS_FORMAT_MASK		0x0003

// Address column styles
#define HVS_ADDR_HEX		0x0000		// hex address (default)
#define HVS_ADDR_VISIBLE	0x0000		// visible (default)
#define HVS_ADDR_DEC		0x0004		// decimal address 
#define HVS_ADDR_RESIZING	0x0008		// auto resize address
#define HVS_ADDR_ENDCOLON	0x0010		// place a colon : at the end of the address
#define HVS_ADDR_MIDCOLON	0x0020		// place a colon in middle (hex address only)
#define HVS_ADDR_INVISIBLE	0x0040		// hide the address
#define HVS_ADDR_MASK		0x007C

/*#define HVS_WIDTH_BYTE		0x0000		// single-byte grouping is default
#define HVS_WIDTH_WORD		0x0100		// word (16bit) grouping of bytes
#define HVS_WIDTH_DWORD		0x0200		// dword (32bit) grouping
#define HVS_WIDTH_MASK		0x0300*/

#define HVS_ENDIAN_LITTLE	0x0000		// little endian is default
#define HVS_ENDIAN_BIG		0x0400		// big endian (only when WORD/DWORD grouping)
#define HVS_ENDIAN_MASK		0x0400

// Ascii column styles
#define HVS_ASCII_VISIBLE	0x0000		// visible (default)
#define HVS_ASCII_SHOWCTRLS	0x0800		// show control (0-31) chars
#define HVS_ASCII_SHOWEXTD	0x1000		// show extended chars (128-255)
#define HVS_ASCII_INSIVIBLE 0x2000		// invisible
#define HVS_ASCII_MASK		0x3800

// Control styles
#define HVS_ALWAYSDELETE	0x4000		// backspace/delete work even in OVR mode
#define HVS_UPPERCASEHEX	0x000000	// hex characters (A-F) are upper-case (default)
#define HVS_LOWERCASEHEX	0x010000	// hex characters (A-F) are lower-case
#define HVS_FITTOWINDOW		0x020000	// adjust columns to fit in window		
#define HVS_SHOWMODS		0x040000	// show modifications to the file
#define HVS_REPLACESEL		0x080000	// typing replaces selection
#define HVS_ENABLEDRAGDROP	0x100000	// enable drag-and-drop
#define HVS_INSERTMODEDEF	0x200000	// insert mode is default (replace is usually)
#define HVS_LINKDROPFILES	0x400000	// link any files dropped from explorer (default)
#define HVS_BRINGTOTOP		0x800000	// bring window to foreground during drag+drop

// Editing styles
#define HVS_OVERSTRIKE		0x0000000	// overstrike (default)
#define HVS_INSERT			0x1000000	// insertion
#define HVS_READONLY		0x2000000	// readonly (no editing)
#define HVS_QUICKSAVE		0x4000000	// quicksave

#define HVS_HEXPANE			0x0000000	// cursor in hex pane (default)
#define HVS_ASCPANE			0x8000000	// cursor in ascii pane

#define HVS_ALWAYSVSCROLL   0x10000000
#define HVS_ALWAYSHSCROLL   0x20000000
#define HVS_RESIZEBAR	    0x40000000
#define HVS_INVERTSELECTION 0x80000000

// Editing mode
#define HVMODE_READONLY			0			// readonly (no editing)
#define HVMODE_INSERT			1			// insertion
#define HVMODE_OVERWRITE		2			// overstrike (default)
#define HVMODE_QUICKSAVE		0x4000000	// quicksave

//
//	HexView_OpenFile flags
//
#define HVOF_DEFAULT			0
#define HVOF_READONLY			1
#define HVOF_QUICKLOAD			2
#define HVOF_QUICKSAVE			4
#define HVOF_AUTOQUICKLOAD		(8 | HVOF_QUICKLOAD)

//
//	HexView_Bookmark flags
//
#define HVBF_DEFAULT			0
#define HVBF_NOPERSIST			1	// mark as non-persistant
#define HVBF_NOBOOKNOTE			2	// don't display the note-tab
#define HVBF_NOENUM				4	// don't return in bookmark enumerations

//
//	HexView_Find flags
//
#define HVFF_SCOPE_ALL			0
#define HVFF_SCOPE_SELECTION	1
#define HVFF_CASE_INSENSITIVE	2

//
//	Colour support
//
#define HVC_BACKGROUND		0			// normal background colour
#define HVC_SELECTION       1			// selection background colour
#define HVC_SELECTION2      2			// secondary selection background colour
#define HVC_ADDRESS			3			// address text
#define HVC_HEXODD			4			// odd column text
#define HVC_HEXODDSEL		5			// odd selected text
#define HVC_HEXODDSEL2		6			// odd selected text (secondary)
#define HVC_HEXEVEN			7			// even column text
#define HVC_HEXEVENSEL		8			// even selected text
#define HVC_HEXEVENSEL2		9			// even selected text (secondary
#define HVC_ASCII			10			// ascii text
#define HVC_ASCIISEL		11			// ascii selection text
#define HVC_ASCIISEL2		12			// ascii selection text
#define HVC_MODIFY			13			// modified bytes colour
#define HVC_MODIFYSEL		14			// modified bytes selected
#define HVC_MODIFYSEL2		15			// modified bytes selected
#define HVC_BOOKMARK_FG		16			// bookmarks
#define HVC_BOOKMARK_BG		17			// bookmarks
#define HVC_BOOKSEL			18			// selected bookmark
#define HVC_RESIZEBAR		19			// resizing bar
#define HVC_SELECTION3      20			// secondary selection background colour
#define HVC_SELECTION4		21
#define HVC_MATCHED			22			// matched pattern background
#define HVC_MATCHEDSEL		23			// selected pattern match			
#define HVC_MATCHEDSEL2		24			// selected pattern match (secondary)
#define HVC_MAX_COLOURS		25

#define HEX_GET_COLOR		0x00ffffff
#define HEX_SYS_COLOR		0x80000000
	
HWND CreateHexView(HWND hwndParent);
ATOM InitHexView();

//
//	HexView user-messages
//
#define HVM_FIRST WM_USER



//#define HVM_HIGHLIGHT		(HVM_FIRST + 2)
#define HVM_GETCURPOS		(HVM_FIRST + 3)
#define HVM_GETSELSTART		(HVM_FIRST + 4)
#define HVM_GETSELEND		(HVM_FIRST + 5)
#define HVM_GETSELSIZE		(HVM_FIRST + 6)
#define HVM_SETSEARCHPAT	(HVM_FIRST + 7)
#define HVM_GETDATACUR		(HVM_FIRST + 8)
#define HVM_SETDATACUR		(HVM_FIRST + 9)
//#define HVM_INSERTDATA		(HVM_FIRST + 10)
//#define HVM_REPLACEDATA		(HVM_FIRST + 11)
#define HVM_ERASEDATA		(HVM_FIRST + 12)
#define HVM_UNDO			(HVM_FIRST + 13)
#define HVM_REDO			(HVM_FIRST + 14)

#define HVM_SETSTYLE		(HVM_FIRST + 0)
#define HVM_SETGROUPING		(HVM_FIRST + 1)

#define HVM_SETEDITMODE		(HVM_FIRST + 15)
#define HVM_GETEDITMODE		(HVM_FIRST + 16)
#define HVM_OPENFILE		(HVM_FIRST + 17)
#define HVM_SAVEFILE		(HVM_FIRST + 18)
#define HVM_SELECTALL		(HVM_FIRST + 19)
#define HVM_CANUNDO			(HVM_FIRST + 20)
#define HVM_CANREDO			(HVM_FIRST + 21)
#define HVM_SETCONTEXTMENU  (HVM_FIRST + 22)
#define HVM_CLEAR			(HVM_FIRST + 23)
#define HVM_ADDBOOKMARK		(HVM_FIRST + 24)
#define HVM_CLEARBOOKMARKS	(HVM_FIRST + 25)
#define HVM_GETBOOKMARK		(HVM_FIRST + 26)
#define HVM_SETBOOKMARK		(HVM_FIRST + 27)
#define HVM_SETCURPOS		(HVM_FIRST + 28)
#define HVM_SETSELSTART		(HVM_FIRST + 29)
#define HVM_SETSELEND		(HVM_FIRST + 30)
#define HVM_SCROLLTO		(HVM_FIRST + 31)
#define HVM_GETSTYLE		(HVM_FIRST + 32)
#define HVM_GETGROUPING		(HVM_FIRST + 33)
#define HVM_FORMATDATA		(HVM_FIRST + 34)
#define HVM_GETLINELEN		(HVM_FIRST + 35)
#define HVM_SETLINELEN		(HVM_FIRST + 36)
#define HVM_ISDRAGLOOP		(HVM_FIRST + 37)
#define HVM_GETDATAADV		(HVM_FIRST + 38)
#define HVM_SETDATAADV		(HVM_FIRST + 39)
#define HVM_GETFILESIZE		(HVM_FIRST + 40)
#define HVM_FINDINIT		(HVM_FIRST + 41)
#define HVM_FINDNEXT		(HVM_FIRST + 42)
#define HVM_FINDPREV		(HVM_FIRST + 43)
#define HVM_FINDCANCEL		(HVM_FIRST + 44)
#define	HVM_GETFILEHANDLE	(HVM_FIRST + 45)
#define HVM_GETCURPANE		(HVM_FIRST + 46)
#define HVM_SETCURPANE		(HVM_FIRST + 47)
#define HVM_GETFILENAME		(HVM_FIRST + 48)
#define HVM_ISREADONLY		(HVM_FIRST + 49)
#define HVM_GETCURCOORD		(HVM_FIRST + 50)
#define HVM_REVERT			(HVM_FIRST + 51)
#define HVM_FILLDATA		(HVM_FIRST + 52)
#define HVM_IMPORTFILE		(HVM_FIRST + 53)
#define HVM_DELBOOKMARK		(HVM_FIRST + 54)
#define HVM_ENUMBOOKMARK	(HVM_FIRST + 55)


//
//	HexView notifications
//	sent via the WM_NOTIFY message
//
#define HVN_BASE				(WM_USER)
#define HVN_CURSOR_CHANGE		(HVN_BASE + 0)
#define HVN_SELECTION_CHANGE	(HVN_BASE + 1)
#define HVN_EDITMODE_CHANGE		(HVN_BASE + 2)
#define HVN_CHANGED				(HVN_BASE + 3)
#define HVN_ESCAPE				(HVN_BASE + 4)
#define HVN_PROGRESS			(HVN_BASE + 5)
#define HVN_BOOKCLOSE			(HVN_BASE + 6)
#define HVN_CONTEXTMENU			(HVN_BASE + 7)

typedef struct _NMHVPROGRESS
{
	NMHDR  hdr;
	size_w pos;
	size_w len;
} NMHVPROGRESS;

typedef struct _NMHVBOOKMARK
{
	NMHDR		hdr;
	HBOOKMARK	hbm;

} NMHVBOOKMARK;

#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif

#define HexView_SetStyle(hwnd, uMask, uStyle) \
	(UINT)SNDMSG((hwnd), HVM_SETSTYLE, (WPARAM)(UINT)(uMask), (LPARAM)(UINT)(uStyle))

#define HexView_SetGrouping(hwnd, nBytes) \
	(UINT)SNDMSG((hwnd), HVM_SETGROUPING, (WPARAM)(UINT)(nBytes), 0)

#define HexView_GetStyle(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETSTYLE, (WPARAM)(UINT)(-1), 0)

#define HexView_GetStyleMask(hwnd, uMask) \
	(UINT)SNDMSG((hwnd), HVM_GETSTYLE, (WPARAM)(UINT)(uMask), 0)

#define HexView_GetGrouping(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETGROUPING, 0, 0)

//#define HexView_Highlight(hwnd, fEnable) \
//	(UINT)SNDMSG((hwnd), HVM_HIGHLIGHT, (WPARAM)(BOOL)(fEnable), 0)

#define HexView_AddBookmark(hwnd, param) \
	(HBOOKMARK)SNDMSG((hwnd), HVM_ADDBOOKMARK, 0, (LPARAM)(PBOOKMARK)(param))

#define HexView_DelBookmark(hwnd, param) \
	(UINT)SNDMSG((hwnd), HVM_DELBOOKMARK, (WPARAM)(HBOOKMARK)(param), 0)

#define HexView_SetSearchPattern(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_SETSEARCHPAT, (WPARAM)(ULONG)(nLength), (LPARAM)(PBYTE)(pData))

#define HexView_GetCurPos(hwnd, pCurPos) \
	(UINT)SNDMSG((hwnd), HVM_GETCURPOS, 0, (LPARAM)(size_w *)(pCurPos))

#define HexView_GetSelStart(hwnd, pSelStart) \
	(UINT)SNDMSG((hwnd), HVM_GETSELSTART, 0, (LPARAM)(size_w *)(pSelStart))

#define HexView_GetSelEnd(hwnd, pSelEnd) \
	(UINT)SNDMSG((hwnd), HVM_GETSELEND, 0, (LPARAM)(size_w *)(pSelEnd))

#define HexView_GetSelSize(hwnd, pSelSize) \
	(UINT)SNDMSG((hwnd), HVM_GETSELSIZE, 0, (LPARAM)(size_w *)(pSelSize))

#define HexView_GetFileSize(hwnd, pFileSize) \
	(UINT)SNDMSG((hwnd), HVM_GETFILESIZE, 0, (LPARAM)(size_w *)(pFileSize))


#define HexView_GetDataCur(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_GETDATACUR, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

#define HexView_GetDataAdv(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_GETDATAADV, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

#define HexView_SetDataCur(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_SETDATACUR, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

#define HexView_SetDataAdv(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_SETDATAADV, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

#define HexView_Undo(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_UNDO, 0, 0)

#define HexView_Redo(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_REDO, 0, 0)

#define HexView_Cut(hwnd) \
	(UINT)SNDMSG((hwnd), WM_CUT, 0, 0)

#define HexView_Copy(hwnd) \
	(UINT)SNDMSG((hwnd), WM_COPY, 0, 0)

#define HexView_Paste(hwnd) \
	(UINT)SNDMSG((hwnd), WM_PASTE, 0, 0)

#define HexView_Delete(hwnd) \
	(UINT)SNDMSG((hwnd), WM_CLEAR, 0, 0)

#define HexView_SetEditMode(hwnd, nEditMode) \
	(UINT)SendMessage((hwnd), HVM_SETEDITMODE, (WPARAM)(nEditMode), 0)

#define HexView_GetEditMode(hwnd) \
	(UINT)SendMessage((hwnd), HVM_GETEDITMODE, 0, 0)

//
//	HexView_OpenFile
//
//	szFileName - full or partial path to file
//	uMethod    - specify one of more of the HVOF_xxx flags:
//					HVOF_READONLY, HVOF_QUICKLOAD, HVOF_QUICKSAVE
//
#define HexView_OpenFile(hwnd, szFileName, uMethod) \
	(UINT)SNDMSG((hwnd), HVM_OPENFILE, (WPARAM)(UINT)(uMethod), (LPARAM)(LPCTSTR)(szFileName))

//
//	HexView_SaveFile
//
//	szFileName - full or partial path to file
//	uMethod    - specify one of more of the HVOF_xxx flags:
//					HVOF_READONLY, HVOF_QUICKLOAD, HVOF_QUICKSAVE
//
#define HexView_SaveFile(hwnd, szFileName, uMethod) \
	(UINT)SNDMSG((hwnd), HVM_SAVEFILE, (WPARAM)(UINT)(uMethod), (LPARAM)(LPCTSTR)(szFileName))

#define HexView_CanUndo(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_CANUNDO, 0, 0)

#define HexView_CanRedo(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_CANREDO, 0, 0)

#define HexView_SetContextMenu(hwnd, hMenu) \
	(HMENU)SNDMSG((hwnd), HVM_SETCONTEXTMENU, (WPARAM)(HMENU)(hMenu), 0)

#define HexView_Clear(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_CLEAR, 0, 0)

#define HexView_ClearBookmarks(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_CLEARBOOKMARKS, 0, 0)

#define HexView_GetBookmark(hwnd, hBookmark, bookm) \
	(BOOL)SNDMSG((hwnd), HVM_GETBOOKMARK, (WPARAM)(HBOOKMARK)(hBookmark), (LPARAM)(PBOOKMARK)(bookm))

#define HexView_EnumBookmark(hwnd, hBookmark, bookm) \
	(HBOOKMARK)SNDMSG((hwnd), HVM_ENUMBOOKMARK, (WPARAM)(HBOOKMARK)(hBookmark), (LPARAM)(PBOOKMARK)(bookm))

#define HexView_SetBookmark(hwnd, hBookmark, bookm) \
	(BOOL)SNDMSG((hwnd), HVM_SETBOOKMARK, (WPARAM)(HBOOKMARK)(hBookmark), (LPARAM)(PBOOKMARK)(bookm))

#define HexView_SetCurPos(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SETCURPOS, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

#define HexView_SetSelStart(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SETSELSTART, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

#define HexView_SetSelEnd(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SETSELEND, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

#define HexView_ScrollTo(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SCROLLTO, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

#define HexView_FormatData(hwnd, fmtparam) \
	(int)SNDMSG((hwnd), HVM_FORMATDATA, 0, (LPARAM)(HEXFMT_PARAMS *)(fmtparam))

#define HexView_GetLineLen(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETLINELEN, 0, 0)

#define HexView_SetLineLen(hwnd, len) \
	(UINT)SNDMSG((hwnd), HVM_SETSELEND, (WPARAM)(UINT)(len), 0)

#define HexView_IsDragLoop(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_ISDRAGLOOP, 0, 0)

#define HexView_SelectAll(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_SELECTALL, 0, 0)

#define HexView_FindInit(hwnd, data, len) \
	(BOOL)SNDMSG((hwnd), HVM_FINDINIT, (WPARAM)(UINT)(len), (LPARAM)(PBYTE)(data))

#define HexView_FindNext(hwnd, pdwResult, options) \
	(BOOL)SNDMSG((hwnd), HVM_FINDNEXT, (WPARAM)(UINT)(options), (LPARAM)(size_w *)(pdwResult))

#define HexView_FindPrev(hwnd, pdwResult, options) \
	(BOOL)SNDMSG((hwnd), HVM_FINDPREV, (WPARAM)(UINT)(options), (LPARAM)(size_w *)(pdwResult))

#define HexView_FindCancel(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_FINDCANCEL, 0, 0)

#define HexView_GetFileHandle(hwnd) \
	(HANDLE)SNDMSG((hwnd), HVM_GETFILEHANDLE, 0, 0)

#define HexView_GetCurPane(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETCURPANE, 0, 0)

#define HexView_SetCurPane(hwnd, pane) \
	(UINT)SNDMSG((hwnd), HVM_SETCURPANE, (WPARAM)(UINT)(pane), 0)

#define HexView_GetFileName(hwnd, buf, len) \
	(UINT)SNDMSG((hwnd), HVM_GETFILENAME, (WPARAM)(UINT)(len), (LPARAM)(LPCTSTR)(buf))

#define HexView_IsReadOnly(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_ISREADONLY, 0, 0)

#define HexView_GetCurCoord(hwnd, coord) \
	(BOOL)SNDMSG((hwnd), HVM_GETCURCOORD, 0, (LPARAM)(POINT *)(coord))

#define HexView_Revert(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_REVERT, 0, 0)

#define HexView_ImportFile(hwnd, szFileName, uMethod) \
	(UINT)SNDMSG((hwnd), HVM_IMPORTFILE, (WPARAM)(UINT)(uMethod), (LPARAM)(LPCTSTR)(szFileName))


//
//	Functions exported from HexLib
//
ULONG WINAPI HexView_SetCurSel(HWND hwnd, size_w selStart, size_w selEnd);
ULONG WINAPI HexView_SetData(HWND hwnd, size_w offset, BYTE *buf, ULONG len);
ULONG WINAPI HexView_GetData(HWND hwnd, size_w offset, BYTE *buf, ULONG len);
ULONG WINAPI HexView_FillData(HWND hwnd, BYTE *buf, ULONG buflen, size_w len);

#ifdef __cplusplus
}
#endif

#endif
