/**
 * @file
 * @author  James Brown
 * @version 1.0
 *
 * @section LICENSE
 *
 * Copyright (C) 2012 James Brown
 * Please refer to the file LICENCE.TXT for copying permission
 *
 * @section DESCRIPTION
 *
 * HexView win32 control
 * www.catch22.net
 *
 */
#ifndef HEXVIEW_INCLUDED
#define HEXVIEW_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the HexView control library
 *
 * This function registers the HexView window class - must be called 
 * prior to creating any HexView controls
 *
 */
ATOM InitHexView();
		
/**
 * Create a HexView control. Just a convenience function -
 * use CreateWindowEx specifying WC_HEXVIEW as an alternative
 */
HWND CreateHexView(HWND hwndParent);

// HexView window class name
#define WC_HEXVIEW _T("HexView32")

//
// Define SEQUENCE64 at the project-level in order to
// support 64bit filesizes (i.e. files over 4gb in size)
//
// Recommened for all projects - this method works in 
// both win32 and win64 projects
//
#ifdef SEQUENCE64
typedef unsigned __int64  size_w;
#else
typedef unsigned long	  size_w;
#endif


typedef struct
{
	COLORREF colFG;
	COLORREF colBG;

} HEXCOL, ATTR;

//
//	Input to HVM_FORMATDATA
//  Caller must allocate szText and attrList to the
//  size returned from HVM_GETLINECHARS
//
typedef struct _HEXFMT_PARAMS
{
	size_w	offset;
	size_t	length;

	size_t   bufferSize;
	TCHAR	*szText;
	ATTR	*attrList;

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

/**
 * HexView styles
 *
 * Set using the HexView_SetStyle message
 * 
 *	Be sure to update the HVS_xxx_MASK values if you change anything!!
 */
//! Hex column styles
#define HVS_FORMAT_HEX		0x0000		// hex format is the default
#define HVS_FORMAT_DEC		0x0001		// decimal formatting
#define HVS_FORMAT_OCT		0x0002		// octal formatting
#define HVS_FORMAT_BIN		0x0003		// binary formatting
#define HVS_FORMAT_MASK		0x0003

//! Address column styles
#define HVS_ADDR_HEX		0x0000		// hex address (default)
#define HVS_ADDR_VISIBLE	0x0000		// visible (default)
#define HVS_ADDR_DEC		0x0004		// decimal address 
#define HVS_ADDR_RESIZING	0x0008		// auto resize address
#define HVS_ADDR_ENDCOLON	0x0010		// place a colon : at the end of the address
#define HVS_ADDR_MIDCOLON	0x0020		// place a colon in middle (hex address only)
#define HVS_ADDR_INVISIBLE	0x0040		// hide the address
#define HVS_ADDR_MASK		0x007C

/* // grouping now specified via HVM_SETGROUPING message
#define HVS_WIDTH_BYTE		0x0000		// single-byte grouping is default
#define HVS_WIDTH_WORD		0x0100		// word (16bit) grouping of bytes
#define HVS_WIDTH_DWORD		0x0200		// dword (32bit) grouping
#define HVS_WIDTH_MASK		0x0300*/

// Endian display mode (for hex column, with grouping enabled)
#define HVS_ENDIAN_LITTLE	0x0000		// little endian is default
#define HVS_ENDIAN_BIG		0x0400		// big endian (only when WORD/DWORD grouping)
#define HVS_ENDIAN_MASK		0x0400

// Ascii column styles
#define HVS_ASCII_VISIBLE	0x0000		// visible (default)
#define HVS_ASCII_SHOWCTRLS	0x0800		// show control (0-31) chars
#define HVS_ASCII_SHOWEXTD	0x1000		// show extended chars (128-255)
#define HVS_ASCII_INVISIBLE 0x2000		// hide the ascii display
#define HVS_ASCII_MASK		0x3800

// Basic HexView control styles
#define HVS_FORCE_FIXEDCOLS 0x80		// force whole-sized hex columns
#define HVS_FIXED_EDITMODE  0x0100		// prevent user from using INSERT to change edit mode
#define HVS_DISABLE_UNDO	0x0200		// prevent undo/redo functionality
#define HVS_SUBPOSITIONING  0x0200      // use keyboard & mouse to allow inter-byte (bit/nibble) editing & positioning

#define HVS_ALWAYSDELETE	0x4000		// backspace/delete work even in OVR mode
#define HVS_HEX_INVISIBLE   0x8000		// hide the hex display
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
/*#define HVS_OVERSTRIKE		0x0000000	// overstrike (default)
#define HVS_INSERT			0x1000000	// insertion
#define HVS_READONLY		0x2000000	// readonly (no editing)
#define HVS_QUICKSAVE		0x4000000	// quicksave
*/

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
#define HVOF_READONLY			1			// open the file as read-only
#define HVOF_QUICKLOAD			2			// enable quickload - keep file on disk + use minimal memory
#define HVOF_QUICKSAVE			4			// enable quicksave - only writes modified portions of a file to disk
#define HVOF_AUTOQUICKLOAD		(8 | HVOF_QUICKLOAD) // always quickload for larger files

//
//	HexView_Bookmark flags
//
#define HVBF_DEFAULT			0
#define HVBF_NOPERSIST			1	// mark as non-persistant
#define HVBF_NOBOOKNOTE			2	// don't display the note-tab
#define HVBF_NOENUM				4	// don't return in bookmark enumerations

/*
 *	HexView_Find flags
 */
#define HVFF_SCOPE_ALL			0
#define HVFF_SCOPE_SELECTION	1
#define HVFF_CASE_INSENSITIVE	2

/*
 *	Colour support
 *  Modify the colour palete using the HexView_SetColor message
 */
#define HVC_BACKGROUND		0			///< normal background colour
#define HVC_SELECTION       1			///< selection background colour
#define HVC_SELECTION2      2			//! secondary selection background colour
#define HVC_ADDRESS			3			//! address text
#define HVC_HEXODD			4			//! odd column text
#define HVC_HEXODDSEL		5			//! odd selected text
#define HVC_HEXODDSEL2		6			//! odd selected text (secondary)
#define HVC_HEXEVEN			7			//! even column text
#define HVC_HEXEVENSEL		8			//! even selected text
#define HVC_HEXEVENSEL2		9			//! even selected text (secondary
#define HVC_ASCII			10			//! ascii text
#define HVC_ASCIISEL		11			//! ascii selection text
#define HVC_ASCIISEL2		12			//! ascii selection text
#define HVC_MODIFY			13			//! modified bytes colour
#define HVC_MODIFYSEL		14			//! modified bytes selected
#define HVC_MODIFYSEL2		15			//! modified bytes selected
#define HVC_BOOKMARK_FG		16			//! bookmarks
#define HVC_BOOKMARK_BG		17			//! bookmarks
#define HVC_BOOKSEL			18			//! selected bookmark
#define HVC_RESIZEBAR		19			//! resizing bar
#define HVC_SELECTION3      20			//! secondary selection background colour
#define HVC_SELECTION4		21
#define HVC_MATCHED			22			//! matched pattern background
#define HVC_MATCHEDSEL		23			//! selected pattern match			
#define HVC_MATCHEDSEL2		24			//! selected pattern match (secondary)
#define HVC_MAX_COLOURS		25

#define HEX_GET_COLOR		0x00ffffff	// mask to 
#define HEX_SYS_COLOR		0x80000000	// top bit in a COLORREF indicates that it is a COLOR_xx index 

// create a HexView color based on one of the GetSysColor COLOR_* system color codes
// the resulting
#define HEX_SYSCOLOR(code) (HEX_SYS_COLOR | (code))	

// Find (realise) the real colour
// an RGB value will not have the top byte set to anything,
// so we can tell if we have a specific colour or not.
// we can set the top byte to a meaningful value to indicate
// that we are using a system colour - just mask the low triple
// bytes to get the COLOR_* value
#define HexView_RealiseColour(cr) (((cr) & HEX_SYS_COLOR) ? GetSysColor((cr) & HEX_GET_COLOR) : (cr))


/*
 *	HexView-specific messages that can be sent to a HexView control
 *  using the SendMessage API
 *
 *  Refer to the corresponding HexView_xxx message macros at the bottom of
 *  this file for details about what parameters each message takes.
 *
 */
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
#define HVM_INITBUF			(HVM_FIRST + 56)
#define HVM_INITBUF_SHARED  (HVM_FIRST + 57)
#define HVM_SETFONTSPACING  (HVM_FIRST + 58)
#define HVM_SETCOLOR		(HVM_FIRST + 59)
#define HVM_GETCOLOR		(HVM_FIRST + 60)
#define HVM_SETPADDING		(HVM_FIRST + 61)
//#define HVM_GETPADDING		(HVM_FIRST + 62)
#define HVM_SETADDROFFSET   (HVM_FIRST + 63)
#define HVM_SCROLLTOP       (HVM_FIRST + 64)
#define HVM_GETLINECHARS	(HVM_FIRST + 65)
#define HVM_SETDATASHIFT    (HVM_FIRST + 66)


//
//	HexView notifications
//	sent via the WM_NOTIFY message
//
#define HVN_BASE				(WM_USER)
#define HVN_CURSOR_CHANGE		(HVN_BASE + 0)  // the cursor has changed location
#define HVN_SELECTION_CHANGE	(HVN_BASE + 1)  // the selection has changed
#define HVN_EDITMODE_CHANGE		(HVN_BASE + 2)  // the edit mode (insert/delete/readonly) has changed
#define HVN_CHANGED				(HVN_BASE + 3)  // the file content has changed (see NMHVCHANGED structure)
#define HVN_ESCAPE				(HVN_BASE + 4)  // the escape key was pressed
#define HVN_PROGRESS			(HVN_BASE + 5)  // NMHVPROGRESS
#define HVN_BOOKCLOSE			(HVN_BASE + 6)  // a bookmark has been closed
#define HVN_CONTEXTMENU			(HVN_BASE + 7)  // the context menu is being displayed
#define HVN_CHANGING            (HVN_BASE + 8)  // the file content is about to change (return TRUE to prevent edits)

/**
 * NMHVCHANGED
 *
 * Sent via the WM_NOTIFY message (with HVN_CHANGE and HVN_CHANGING)
 * Indicates that a range of bytes has changed due to 
 * an action by the user
 *
 * HVN_CHANGING - return TRUE to prevent the edit from occuring
 * HVN_CHANGED  - sent AFTER the edit has occured
*/
typedef struct _NMHVCHANGED
{
#define HVMETHOD_INSERT    1
#define HVMETHOD_OVERWRITE 2
#define HVMETHOD_DELETE    3

	NMHDR	hdr;
	UINT	method;		// how the data changed - one of the HVMETHOD_xxx values
	UINT    bitmask;    // bitmask that indicates which bit(s) have changed

	size_w  offset;     // starting offset of where data changed
	size_w  length;     // number of bytes that were affected
	BYTE *  data;		// optional pointer to data that is being inserted

} NMHVCHANGED;

/**
 * NMHVPROGRESS
 *
 * Sent via the WM_NOTIFY message (with HVN_PROGRESS).
 * Indicates the progress of the current operation (typically searching)
*/
typedef struct _NMHVPROGRESS
{
	NMHDR  hdr;
	size_w pos;  // current progress
	size_w len;  // when pos equals this value, the operation is complete
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

//
// macros to allow passing of 64bit values to the 32bit version of SendMessage
// this works by passing the low 32bits in WPARAM, and the top 32bits in LPARAM
//
#ifdef SEQUENCE64
#define MAKE_SIZEW(wParam, lParam) ((size_w)(wParam) | ((size_w)(lParam) << 32))
#define WPARAM_SIZEW(sw) ((WPARAM)((size_w)(sw) & 0xffffffff))
#define LPARAM_SIZEW(sw) ((LPARAM)((size_w)(sw) >> 32))
#else
#define MAKE_SIZEW(wParam, lParam) ((size_w)(wParam))
#define WPARAM_SIZEW(sw) ((WPARAM)((size_w)(sw) & 0xffffffff))
#define LPARAM_SIZEW(sw) ((LPARAM)0)
#endif

/**
 * Apply one or more of the HexView-specific HVS_xxx styles. 
 *
 * @param  UINT uMask  - bitmask to control which bits to modify
 * @param  UINT uStyle - one or more of the HVS_xxx style values
 *
 */
#define HexView_SetStyle(hwnd, uMask, uStyle) \
	(UINT)SNDMSG((hwnd), HVM_SETSTYLE, (WPARAM)(UINT_PTR)(uMask), (LPARAM)(UINT_PTR)(uStyle))

/**
 * Specify the byte-grouping arrangement in the hex-pane. This allows
 * the hexpane to group bytes into columns. Typical values are 1, 2, 4 and 8
 *
 * @param UINT nBytes - the number of bytes in each column grouping
 */
#define HexView_SetGrouping(hwnd, nBytes) \
	(UINT)SNDMSG((hwnd), HVM_SETGROUPING, (WPARAM)(UINT_PTR)(nBytes), 0)

/**
 * Return the current styles selected into the hexview control
 * The styles are returned as a single UINT value
 *
 * @param
 */
#define HexView_GetStyle(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETSTYLE, (WPARAM)(UINT_PTR)(-1), 0)

/**
 * Return the style value(s) specified by the mask. The uMask
 * parameter must be one of the HVS_xx style values
 *
 * @param UINT uMask - bitmask that specifies which style(s) to return
 */
#define HexView_GetStyleMask(hwnd, uMask) \
	(UINT)SNDMSG((hwnd), HVM_GETSTYLE, (WPARAM)(UINT)(uMask), 0)

/**
 * Return the current byte-grouping arrange in the hex pane
 *
 */
#define HexView_GetGrouping(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETGROUPING, 0, 0)

//#define HexView_Highlight(hwnd, fEnable) \
//	(UINT)SNDMSG((hwnd), HVM_HIGHLIGHT, (WPARAM)(BOOL)(fEnable), 0)

#define HexView_AddBookmark(hwnd, param) \
	(HBOOKMARK)SNDMSG((hwnd), HVM_ADDBOOKMARK, 0, (LPARAM)(PBOOKMARK)(param))

#define HexView_DelBookmark(hwnd, param) \
	(UINT)SNDMSG((hwnd), HVM_DELBOOKMARK, (WPARAM)(HBOOKMARK)(param), 0)

/**
 * Specify a sequence of bytes that will have its every occurance
 * highlighted in the current file
 *
 * @param
 */
#define HexView_SetSearchPattern(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_SETSEARCHPAT, (WPARAM)(ULONG_PTR)(nLength), (LPARAM)(PBYTE)(pData))

/**
 * Return the current position of the cursor, as an offset
 * in bytes, relative to the start of the file
 *
 * @param size_w *pCurPos - address of variable that will receive the value
 */
#define HexView_GetCurPos(hwnd, pCurPos) \
	(UINT)SNDMSG((hwnd), HVM_GETCURPOS, 0, (LPARAM)(size_w *)(pCurPos))

/**
 * Return the starting position of the active selection
 *
 * @param size_w *pSelStart - address of variable that will receive the value
 */
#define HexView_GetSelStart(hwnd, pSelStart) \
	(UINT)SNDMSG((hwnd), HVM_GETSELSTART, 0, (LPARAM)(size_w *)(pSelStart))

/**
 * Return the ending position of the active selection
 *
 * @param size_w *pSelEnd  - address of variable that will receive the value
 */
#define HexView_GetSelEnd(hwnd, pSelEnd) \
	(UINT)SNDMSG((hwnd), HVM_GETSELEND, 0, (LPARAM)(size_w *)(pSelEnd))

/**
 * Return the size of the active selection, in bytes
 *
 * @param size_w *pSelSize - 
 */
#define HexView_GetSelSize(hwnd, pSelSize) \
	(UINT)SNDMSG((hwnd), HVM_GETSELSIZE, 0, (LPARAM)(size_w *)(pSelSize))

/**
 * Return the size of the current file, in bytes
 *
 * @param size_w *pFileSize - pointer to a size_w variable, into which the filesize is returned
 */
#define HexView_GetFileSize(hwnd, pFileSize) \
	(UINT)SNDMSG((hwnd), HVM_GETFILESIZE, 0, (LPARAM)(size_w *)(pFileSize))

/**
 * Retrieve data from the active cursor location
 *
 * @param  BYTE *pData   - buffer which will receive the data being read
 * @param  ULONG nLength - size of pData, in bytes
 */
#define HexView_GetDataCur(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_GETDATACUR, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

/**
 * Retrieve data from the active cursor location, and 
 * advance the cursor offset to the last position read from the file
 *
 * @param BYTE *pData - buffer which will receive the data being read
 * @param ULONG nLength - sizeof of pData, in bytes
 */
#define HexView_GetDataAdv(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_GETDATAADV, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

/**
 * Insert/Overwrite data into the current cursor location,
 * but do not adjust the cursor offset
 *
 * @param BYTE *pData - buffer containing the data to insert/overwrite
 * @param ULONG nLength - size of pData, in bytes
 */
#define HexView_SetDataCur(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_SETDATACUR, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

/**
 * Insert/Overwrite data into the current cursor location,
 * and advance the cursor offset to the end of the newly inserted data
 *
 * @param BYTE *pData - buffer containing the data to insert/overwrite
 * @param ULONG nLength - size of pData, in bytes
 */
#define HexView_SetDataAdv(hwnd, pData, nLength) \
	(UINT)SNDMSG((hwnd), HVM_SETDATAADV, (WPARAM)(PBYTE)pData, (LPARAM)(ULONG)nLength)

/**
 * Undo the last edit operation
 *
 * @param
 */
#define HexView_Undo(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_UNDO, 0, 0)

/**
 * Repeat the last Undo operation
 *
 */
#define HexView_Redo(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_REDO, 0, 0)

/**
 * Remove data within the active selection, and copy it to the clipboard
 *
 * @param
 */
#define HexView_Cut(hwnd) \
	(UINT)SNDMSG((hwnd), WM_CUT, 0, 0)

/**
 * Copy the active selection to the clipboard as CF_TEXT
 *
 * @param
 */
#define HexView_Copy(hwnd) \
	(UINT)SNDMSG((hwnd), WM_COPY, 0, 0)

/**
 * Paste the current CF_TEXT clipboard contents into HexView
 *
 * @param
 */
#define HexView_Paste(hwnd) \
	(UINT)SNDMSG((hwnd), WM_PASTE, 0, 0)

/**
 * Delete the current file content, but do not
 * reset the active file - allowing this operation to be undone
 *
 */
#define HexView_Delete(hwnd) \
	(UINT)SNDMSG((hwnd), WM_CLEAR, 0, 0)

/**
 * Set the current edit mode (insert/overwrite/readonly)
 *
 * @param UINT nEditMode - one of the 
 */
#define HexView_SetEditMode(hwnd, nEditMode) \
	(UINT)SNDMSG((hwnd), HVM_SETEDITMODE, (WPARAM)(nEditMode), 0)

/**
 * Return the current edit mode (one of the HVS_ values)
 *
 * @param
 */
#define HexView_GetEditMode(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETEDITMODE, 0, 0)

//!
//!	HexView_OpenFile
//!
//!	@param szFileName - full or partial path to file
//!	@param uMethod    - specify one of more of the HVOF_xxx flags:
//!					HVOF_READONLY, HVOF_QUICKLOAD, HVOF_QUICKSAVE
//!
#define HexView_OpenFile(hwnd, szFileName, uMethod) \
	(UINT)SNDMSG((hwnd), HVM_OPENFILE, (WPARAM)(UINT_PTR)(uMethod), (LPARAM)(LPCTSTR)(szFileName))

//!
//!	HexView_SaveFile
//!
//!	szFileName - full or partial path to file
//!	uMethod    - specify one of more of the HVOF_xxx flags:
//!					HVOF_READONLY, HVOF_QUICKLOAD, HVOF_QUICKSAVE
//!
#define HexView_SaveFile(hwnd, szFileName, uMethod) \
	(UINT)SNDMSG((hwnd), HVM_SAVEFILE, (WPARAM)(UINT_PTR)(uMethod), (LPARAM)(LPCTSTR)(szFileName))

#define HexView_InitBuf(hwnd, pBuffer, nLength) \
	(UINT)SNDMSG((hwnd), HVM_INITBUF, (WPARAM)(const PBYTE)(pBuffer), (LPARAM)(UINT_PTR)(nLength))

#define HexView_InitBufShared(hwnd, pBuffer, nLength) \
	(UINT)SNDMSG((hwnd), HVM_INITBUF_SHARED, (WPARAM)(const PBYTE)(pBuffer), (LPARAM)(UINT_PTR)(nLength))

/**
 * Return a boolean value indicating whether there are
 * any undo operations available
 *
 */
#define HexView_CanUndo(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_CANUNDO, 0, 0)

/**
 * Return a boolean value indicating whether there are any 
 * redo operations available
 *
 */
#define HexView_CanRedo(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_CANREDO, 0, 0)

/**
 * Specify a new menu to be used for the context-menu
 *
 * @param HMENU hMenu - win32 handle to the menu resource
 */
#define HexView_SetContextMenu(hwnd, hMenu) \
	(HMENU)SNDMSG((hwnd), HVM_SETCONTEXTMENU, (WPARAM)(HMENU)(hMenu), 0)

/**
 * Clear (delete) the current file content
 *
 */
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

/**
 * Set the active cursor location (in bytes, offset from the start of the file)
 *
 * @param size_w pos - new offset of the cursor 
 */
#define HexView_SetCurPos(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SETCURPOS, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

/**
 * Set the starting position of the current selection
 *
 * @param size_w pos - offset of the selection-start, in bytes
 */
#define HexView_SetSelStart(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SETSELSTART, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

/**
 * Set the ending position of the current selection
 *
 * @param  size_w pos - offset of selection-end, in bytes
 */
#define HexView_SetSelEnd(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SETSELEND, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

/**
 * Adjust the viewport so that the specified file-offset
 * is scrolled into view, if that offset is outside of the viewport
 *
 * @param  size_w pos  - offset within the file which will be scrolled into view
 */
#define HexView_ScrollTo(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SCROLLTO, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

/**
 * Adjust the viewport so that the specified file-offset
 * is scrolled to the top of the viewport
 *
 * @param  size_w pos  - offset within the file which will be scrolled into view
 */
#define HexView_ScrollTop(hwnd, pos) \
	(UINT)SNDMSG((hwnd), HVM_SCROLLTOP, WPARAM_SIZEW(pos), LPARAM_SIZEW(pos))

#define HexView_FormatData(hwnd, fmtparam) \
	(int)SNDMSG((hwnd), HVM_FORMATDATA, 0, (LPARAM)(HEXFMT_PARAMS *)(fmtparam))

/**
 * Return the current line-length, in bytes
 *
 */
#define HexView_GetLineLen(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETLINELEN, 0, 0)

/**
 * Set the current line-length, in bytes
 *
 * @param UINT len - length of line, in bytes
 */
#define HexView_SetLineLen(hwnd, len) \
	(UINT)SNDMSG((hwnd), HVM_SETLINELEN, (WPARAM)(UINT)(len), 0)

/**
 * Returns a boolean value indicating if the HexView is in a 
 * drag-and-drop operation
 *
 * @param
 */
#define HexView_IsDragLoop(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_ISDRAGLOOP, 0, 0)

/**
 * Select all data in the HexView control
 *
 */
#define HexView_SelectAll(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_SELECTALL, 0, 0)

/**
 * Initialize a search with a new search term
 *
 * @param  BYTE *data  - address of buffer containing the search term
 * @param  UINT  len   - lengh of [data], in bytes
 */
#define HexView_FindInit(hwnd, data, len) \
	(BOOL)SNDMSG((hwnd), HVM_FINDINIT, (WPARAM)(UINT)(len), (LPARAM)(PBYTE)(data))

/**
 * Find the next occurance of the current search term
 *
 * @param size_w *pdwResult  - address of variable to receive location of matched data
 * @param UINT    options    - one of the HVFF_xxx values
 */
#define HexView_FindNext(hwnd, pdwResult, options) \
	(BOOL)SNDMSG((hwnd), HVM_FINDNEXT, (WPARAM)(UINT)(options), (LPARAM)(size_w *)(pdwResult))

/**
 * Find the previous occurance of the current search term
 *
 * @param size_w *pdwResult  - address of size_w variable to receive location of matched data
 * @param UINT    options    - one of the HVFF_xxx values
 *
 * @returns BOOL - boolean value indicating success or failure
 */
#define HexView_FindPrev(hwnd, pdwResult, options) \
	(BOOL)SNDMSG((hwnd), HVM_FINDPREV, (WPARAM)(UINT)(options), (LPARAM)(size_w *)(pdwResult))

/**
 * Cancel the current Find operation, if one is active
 *
 */
#define HexView_FindCancel(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_FINDCANCEL, 0, 0)

/**
 * Return the Win32 File HANDLE of the current file being edited
 *
 */
#define HexView_GetFileHandle(hwnd) \
	(HANDLE)SNDMSG((hwnd), HVM_GETFILEHANDLE, 0, 0)

/**
 * Return the current editing pane - either HVS_HEXPANE or HCS_ASCPANE
 * 
 */
#define HexView_GetCurPane(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETCURPANE, 0, 0)

/**
 * Set the current editing pane - either Hex or Ascii
 *
 * @param UINT pane  - either HVS_HEXPANE or HVS_ASCPANE values
 */
#define HexView_SetCurPane(hwnd, pane) \
	(UINT)SNDMSG((hwnd), HVM_SETCURPANE, (WPARAM)(UINT)(pane), 0)

/**
 * Return the path to the current file being edited
 *
 * @param TCHAR *buf  - pointer to buffer that will receive the filename
 * @param UINT   len  - length of [buf], in TCHARs
 */
#define HexView_GetFileName(hwnd, buf, len) \
	(UINT)SNDMSG((hwnd), HVM_GETFILENAME, (WPARAM)(UINT)(len), (LPARAM)(LPCTSTR)(buf))

/**
 * Returns whether the current file is being edited as readonly
 *
 */
#define HexView_IsReadOnly(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_ISREADONLY, 0, 0)

/**
 * Return the logical cursor position within the current
 * viewport, as an x,y coordinate
 *
 * @param POINT *coord - address of a POINT structure that receives the cursor position
 */
#define HexView_GetCurCoord(hwnd, coord) \
	(BOOL)SNDMSG((hwnd), HVM_GETCURCOORD, 0, (LPARAM)(POINT *)(coord))

/**
 * Restore the current file in the hexview to it's original state
 *
 */
#define HexView_Revert(hwnd) \
	(BOOL)SNDMSG((hwnd), HVM_REVERT, 0, 0)

/**
 * Import the specified file at the current cursor location,
 * using the current editing mode (insert/overwrite)
 *
 * @param TCHAR *szFileName - Path to the file to import
 * @param UINT   uMethod    - One of the HVOF_xxx flags
 */
#define HexView_ImportFile(hwnd, szFileName, uMethod) \
	(UINT)SNDMSG((hwnd), HVM_IMPORTFILE, (WPARAM)(UINT)(uMethod), (LPARAM)(LPCTSTR)(szFileName))

/**
 * Adjust the horizontal and vertical spacing between characters in
 * the current font selected into the HexView. Send this message after 
 * setting the active font.
 *
 * @param SHORT xspace   - Horizontal spacing (in pixels)
 * @param SHORT yspace   - Vertical spacing (in pixels)
 */
#define HexView_SetFontSpacing(hwnd, xspace, yspace) \
	(UINT)SNDMSG((hwnd), HVM_SETFONTSPACING, 0, (LPARAM)MAKELONG((short)(xspace), (short)(yspace)))

/**
 * Set the font for the specified HexView control
 *
 * @param HFONT hFont - win32 font handle
 *
 */
#define HexView_SetFont(hwnd, hFont) \
	(VOID)SNDMSG((hwnd), WM_SETFONT, (WPARAM)(HFONT)(hFont), 0)

/**
 * Set a new color for the specified HexView display element
 *
 * @param UINT index     - one of the HVC_* color indices
 * @param COLORREF color - a RGB value, or a HEX_SYSCOLOR value
 */
#define HexView_SetColor(hwnd, index, color) \
	(UINT)SNDMSG((hwnd), HVM_SETCOLOR, (WPARAM)(UINT_PTR)(index), (LPARAM)(COLORREF)(color))

/**
 * Get the color-value for the specified HexView display element
 *
 * @param UINT index     - one of the HVC_* color indices
 */
#define HexView_GetColor(hwnd, index) \
	(COLORREF)SNDMSG((hwnd), HVM_GETCOLOR, (WPARAM)(UINT)(index), 0)

/**
 * Specify the left & right padding (in character units) 
 * either side of the main hex display. i.e. in-between the address&hex display,
 * and the hex&ascii display, respectively
 *
 * @param int left - number of characters between the address column & hex display
 * @param int right - number of characters between the hex display & ascii display
 */
#define HexView_SetPadding(hwnd, left, right) \
	(VOID)SNDMSG((hwnd), HVM_SETPADDING, 0, (LPARAM)MAKEWPARAM((USHORT)(int)(left), (USHORT)(int)(right)))

/**
 * Specify offset that the address display begins at (default is 0)
 * This is purely a cosmetic change - actual offsets (such as cursor/selection)
 * within the hexview do not change
 *
 * @param size_w offset - value by which to offset the address column
 */
#define HexView_SetAddressOffset(hwnd, offset) \
	(VOID)SNDMSG((hwnd), HVM_SETADDROFFSET, WPARAM_SIZEW(offset), LPARAM_SIZEW(offset))

/**
 * Return the number of characters required to render a line in the hexview,
 * includes the space required for the null character
 */
#define HexView_GetLineChars(hwnd) \
	(UINT)SNDMSG((hwnd), HVM_GETLINECHARS, 0, 0)

/**
 * Offset the hex&ascii display by the given amount 
 *
 * @param size_w offset  - number of bytes to offset the display by
 */
#define HexView_SetDataShift(hwnd, offset) \
	(UINT)SNDMSG((hwnd), HVM_SETDATASHIFT, WPARAM_SIZEW(offset), LPARAM_SIZEW(offset))


//
//	Functions exported from HexLib (i.e not message macros)
//

/**
 * Set the starting and ending positions for the active selection
 *
 * @param size_w selStart - starting offset, in bytes
 * @param size_w selEnd   - ending offset, in bytes
 */
ULONG WINAPI HexView_SetCurSel(HWND hwnd, size_w selStart, size_w selEnd);

/**
 * Insert or Overwrite (depending on the current edit mode) the specified
 * buffer of data, into the specified offset within the file
 *
 * @param  size_w offset - position into which to insert/overwrite the data
 * @param  BYTE *buf     - buffer containing the data to insert
 * @param  ULONG len     - length of data to insert, in bytes
 */
ULONG WINAPI HexView_SetData(HWND hwnd, size_w offset, BYTE *buf, ULONG len);

/**
 * Retrieve data in the current file from the specified offset
 *
 * @param  size_w offset - position from which to retrieve the data
 * @param  BYTE *buf     - buffer that will receive the data
 * @param  ULONG len     - length of the [buf] buffer, in bytes
 */
ULONG WINAPI HexView_GetData(HWND hwnd, size_w offset, BYTE *buf, ULONG len);

/**
 * Fill using the specified buffer of data, at the current cursor offset, using the 
 * current edit mode (insert/overwrite). The data will be repeated to ensure the 
 * specified range of data is updated
 *
 * @param  BYTE *buf    - buffer containing the Fill data
 * @param  ULONG buflen - size of [buf], in bytes
 * @param  size_w len   - total length of data to insert/overwrite
 */
ULONG WINAPI HexView_FillData(HWND hwnd, BYTE *buf, ULONG buflen, size_w len);

#define HexView_SetRedraw(hwnd, fRedraw) \
	(UINT)SNDMSG((hwnd), WM_SETREDRAW, (WPARAM)(BOOL)(fRedraw), 0)

#ifdef __cplusplus
}
#endif

#endif
