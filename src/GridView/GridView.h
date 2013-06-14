//
//  GridView.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef GRIDVIEW_INCLUDED
#define GRIDVIEW_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif 

#define WC_GRIDVIEW _T("GridView32")

//
//	GVITEM - represents one cell of the GridView
//
typedef struct
{
	UINT		mask;				// which items to set/get
	int			iItem;				// item index (row number)
	int			iSubItem;			// sub-item index (column number)
	UINT		state;				// state of the item
	//UINT		stateMask;			// which states are valid
	LPTSTR		pszText;			// the text
	int			cchTextMax;			// length of text
	int			iIndent;			// indentation in device units
	int			iImage;				// image-list index
	int			iFont;				// font-index
	COLORREF	color;				// rgb foreground color 
	ULONG64		param;				// user-defined parameter

} GVITEM, *PGVITEM;

//
//	Flags to indicate which fields of GVITEM are valid
//
#define GVIF_TEXT	0x01
#define GVIF_INDENT 0x02
#define GVIF_IMAGE	0x04
#define GVIF_PARAM	0x08
#define GVIF_STATE	0x10
#define GVIF_FONT	0x20
#define GVIF_COLOR	0x40

//
//
//
#define GVI_ROOT	0
#define GVI_CHILD	1
#define GVI_SIBLING	2
#define GVI_FIRST	3
#define GVI_LAST	GVI_SIBLING
#define GVI_BEFORE	4
#define GVI_AFTER	5

typedef PVOID HGRIDITEM;

// strings are a minimum of 32bytes long, and grow
// in chunks of 32?
#define GVSTRSIZE 32

typedef UINT (CALLBACK* GridViewProc)(GVITEM *);

typedef struct 
{
	UINT	uState;
	int		xWidth;				// width of this item
	HFONT	hFont;				// font to use for each item in the column
	
	//int		nVariants;			//number of possible values, 0 for any
//	int		nDefaultValue;		//default value if no data entered
	
	//GridViewProc gvpCallback;

	//TCHAR	szDefault[256];		//default string for this column if no data entered
	
	TCHAR	*pszText;			//the text of the Header item
	int		 cchTextMax;

//	int		cchInitialText;		//intialize length of data for new items

} GVCOLUMN, *PGVCOLUMN;

//
//	GridView control styles:
//
#define GVS_SINGLE				0x00000			// single selection
#define GVS_MULTIPLE			0x00001			// multiple selecton
#define GVS_EXTENDED			0x00002			// extended selection (any)
#define GVS_SELMASK				0x00003
//#define GVS_GRIDLINES			0x00004			// show grid lines
#define GVS_FULLROWSELECT		0x00008			// show selection for all items
#define GVS_SHOWFULLFOCUS		0x00010			// show focus rectangle for all items
#define GVS_SHOWFOCUS			0x00010
#define GVS_NOCOLUMNHEADER		0x00020			// don't display header control
#define GVS_NOHIDESELECTION		0x00040			// always show selection (even when not focussed)
#define GVS_CANINSERT			0x00080			// allow manual item insertions
#define GVS_CANDELETE			0x00100			// allow manual item deletions
#define GVS_READONLY			0x00200			// no user-input at all
#define GVS_DISABLENOSCROLL		0x00400			// always keep scrollbar visible
#define GVS_NOHORZSCROLL		0x00800			// don't show horz scrollbar
#define GVS_TREEMODE			0x01000			// hierarchical display
#define GVS_HASBUTTONS			0x02000			// show expansion buttons [+], [-]
#define GVS_HASLINES			0x04000			// show tree lines
#define GVS_TREELINES			GVS_HASLINES
#define GVS_ICONS				0x10000			// per-item icons
#define GVS_CHECKBOXES			0x20000			// per-item checkboxes
#define GVS_VERTGRIDLINES		0x40000
#define GVS_HORZGRIDLINES		0x80000
#define GVS_GRIDLINES			(GVS_HORZGRIDLINES|GVS_VERTGRIDLINES)
#define GVS_SMEGHEAD			0x100000


//
//	Gridview Item styles
//
#define GVIS_EXPANDED			1				// item is expanded
#define GVIS_UNDERLINE			2
#define GVIS_READONLY			4
#define GVIS_IMAGEIDX			0x10			// make expanded item-image ++
#define GVIS_DROPLIST			0x0100
#define GVIS_DROPDOWN			0x0200

//
//	GridView Column styles
#define GVCS_ALIGN_LEFT			0
#define GVCS_ALIGN_RIGHT		1
#define GVCS_ALIGN_CENTER		2
#define GVCS_ALIGN_MASK			3
#define GVCS_IMAGE				0x0004			// all items show an image
#define GVCS_CHECKBOX			0x0008			// all items show a check-box
#define GVCS_SELECTIMAGE		0x0100			// selection covers images
#define GVCS_BLENDIMAGE			0x0200			// selection covers images
#define GVCS_ELLIPSIS			0x0400			// truncated text shows ellipsis (...)
#define GVCS_READONLY			0x0800


//

//
//	Colour indices
//
#define GV_SYS_COLOR 0x80000000
#define GV_GET_COLOR 0x00ffffff

#define SYSCOL(COLOR_IDX)					   ( 0x80000000 |                     COLOR_IDX  )
#define MIXED_SYSCOL(COLOR_IDX1, COLOR_IDX2)   ( 0xC0000000 | (COLOR_IDX2 << 8) | COLOR_IDX1 )
#define MIXED_SYSCOL2(COLOR_IDX1, COLOR_IDX2)  ( 0xE0000000 | (COLOR_IDX2 << 8) | COLOR_IDX1 )
#define SYSCOLIDX(COLREF)   ( 0x00FFFFFF & COLREF )
#define REALIZE_SYSCOL(col) (RealizeColour(col))

#define GVC_FOREGROUND		0	
#define GVC_BACKGROUND		1
#define GVC_HIGHLIGHTTEXT	2
#define GVC_HIGHLIGHT		3
#define GVC_GRIDLINES		4
#define GVC_TREELINES		5
#define GVC_TREEBUT			6
#define GVC_TREEBUT_BORDER	7
#define GVC_TREEBUT_GLYPH	8
#define GVC_HIGHLIGHTTEXT2	9
#define GVC_HIGHLIGHT2		10
#define GVC_GRIDLINES2		11

#define GVC_MAX_COLOURS 12

//
//	GridView messages:
//
#define GVM_FIRST					WM_USER
#define GVM_ADDCOLUMN				(GVM_FIRST + 0)
#define GVM_ADDITEM					(GVM_FIRST + 1)
#define GVM_CLEAR					(GVM_FIRST + 2)
#define GVM_DELETEALL				(GVM_FIRST + 3)
#define GVM_DELETECOLUMN			(GVM_FIRST + 4)
#define GVM_DELETEITEM				(GVM_FIRST + 5)
#define GVM_EDITITEM				(GVM_FIRST + 6)
#define GVM_ENSUREVISIBLE			(GVM_FIRST + 7)
//#define GVM_FINDITEM				(GVM_FIRST + 8)
#define GVM_GETBKCOLOR				(GVM_FIRST + 10)
#define GVM_GETCOLUMN				(GVM_FIRST + 20)
#define GVM_GETCOLUMNORDERARRAY		(GVM_FIRST + 21)
#define GVM_GETCOLUMNWIDTH			(GVM_FIRST + 22)
#define GVM_GETEDITCONTROL			(GVM_FIRST + 23)
#define GVM_GETHEADER				(GVM_FIRST + 25)
#define GVM_GETIMAGELIST			(GVM_FIRST + 26)
#define GVM_GETCURSEL				(GVM_FIRST + 27)
#define GVM_GETITEMHANDLE			(GVM_FIRST + 28)
#define GVM_GETITEM					(GVM_FIRST + 30)
#define GVM_GETITEMCOUNT			(GVM_FIRST + 31)
#define GVM_GETITEMPOSITION			(GVM_FIRST + 32)
#define GVM_GETITEMRECT				(GVM_FIRST + 33)
#define GVM_GETITEMSPACING			(GVM_FIRST + 34)
#define GVM_GETITEMSTATE			(GVM_FIRST + 35)
#define GVM_GETITEMTEXT				(GVM_FIRST + 36)
#define GVM_GETLISTCONTROL			(GVM_FIRST + 37)
#define GVM_GETSELCOUNT				(GVM_FIRST + 38)
#define GVM_GETSELBKCOLOR			(GVM_FIRST + 39)
#define GVM_GETSELTEXTCOLOR			(GVM_FIRST + 40)
#define GVM_GETTEXTCOLOR			(GVM_FIRST + 41)
#define GVM_GETTOPINDEX				(GVM_FIRST + 42)
#define GVM_GETCHILDINDEX			(GVM_FIRST + 43)
#define GVM_HITTEST					(GVM_FIRST + 50)
#define GVM_INSERTCOLUMN			(GVM_FIRST + 51)
//#define GVM_INSERTITEM				(GVM_FIRST + 52)
#define GVM_REDRAWITEMS				(GVM_FIRST + 53)
#define GVM_SETBKCOLOR				(GVM_FIRST + 54)
#define GVM_SETCOLUMN				(GVM_FIRST + 60)
#define GVM_SETCOLUMNWIDTH			(GVM_FIRST + 61)
#define GVM_SETCOLUMNORDERARRAY		(GVM_FIRST + 62)
#define GVM_SETIMAGELIST			(GVM_FIRST + 63)
#define GVM_SETITEM					(GVM_FIRST + 64)
#define GVM_SETITEMCOUNT			(GVM_FIRST + 65)
#define GVM_SETITEMSPACING			(GVM_FIRST + 66)
#define GVM_SETITEMSTATE			(GVM_FIRST + 67)
#define GVM_SETITEMTEXT				(GVM_FIRST + 68)
#define GVM_SETSELBKCOLOR			(GVM_FIRST + 69)
#define GVM_SETSELTEXTCOLOR			(GVM_FIRST + 70)
#define GVM_SETTEXTCOLOR			(GVM_FIRST + 71)
#define GVM_INSERTCHILD				(GVM_FIRST + 72)
#define GVM_INSERTAFTER				(GVM_FIRST + 73)
#define GVM_INSERTBEFORE			(GVM_FIRST + 74)
#define GVM_SETSTYLE				(GVM_FIRST + 75)
#define GVM_UPDATE					(GVM_FIRST + 76)
#define GVM_DELETECHILDREN			(GVM_FIRST + 77)
#define GVM_GETPARENTITEM			(GVM_FIRST + 78)
#define GVM_FINDCHILD				(GVM_FIRST + 79)
#define GVM_GETPARENT				(GVM_FIRST + 80)
#define GVM_GETFIRSTCHILD			(GVM_FIRST + 81)
#define GVM_GETNEXTSIBLING			(GVM_FIRST + 82)
#define GVM_INSERTUNIQUECHILD		(GVM_FIRST + 83)
#define GVM_FINDCHILDR				(GVM_FIRST + 84)
#define GVM_EXPANDITEM				(GVM_FIRST + 85)

//
//	Notification messages
//
#define GVN_FIRST					(WM_USER)
#define GVN_SELCHANGED				(GVN_FIRST + 1)
#define GVN_ITEMEXPANDED			(GVN_FIRST + 2)
#define GVN_CANINSERT				(GVN_FIRST + 3)
#define GVN_INSERTED				(GVN_FIRST + 4)
#define GVN_CANDELETE				(GVN_FIRST + 5)
#define GVN_DELETED					(GVN_FIRST + 6)
#define GVN_CANCHANGE				(GVN_FIRST + 7)
#define GVN_CHANGED					(GVN_FIRST + 8)
#define GVN_DROPDOWN				(GVN_FIRST + 9)
#define GVN_DBLCLK					(GVN_FIRST + 10)

typedef struct
{
	NMHDR		hdr;
	UINT		action;
	//GVITEM	itemOld;
	//GVITEM	itemNew;
	HGRIDITEM	hItem;
	POINT		ptDrag;

	UINT		iItem;
	UINT		iSubItem;

	HWND		hwndDropList;

} NMGRIDVIEW, *PNMGRIDVIEW;

//
//	Message helper macros
//
#ifdef  __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif

//#define GridView_InsertTree(hwnd, hitem, pitem) \
//	(HGRIDITEM)SNDMSG((hwnd), GVM_INSERTITEM, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)


//
//	Set/Insert
//

//
//	Insert new row into the gridview
//
#define GridView_InsertChild(hwnd, hitem, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_INSERTCHILD, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

//
//	Insert new row only if it doesn't already exist
//	Returns handle to existing row/item otherwise
//
#define GridView_InsertUniqueChild(hwnd, hitem, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_INSERTUNIQUECHILD, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

#define GridView_InsertAfter(hwnd, hitem, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_INSERTAFTER, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

#define GridView_InsertBefore(hwnd, hitem, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_INSERTBEFORE, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

#define GridView_SetImageList(hwnd, hImageList) \
	(BOOL)SNDMSG((hwnd), GVM_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)(hImageList))

#define GridView_SetItem(hwnd, hitem, pitem) \
	(BOOL)SNDMSG((hwnd), GVM_SETITEM, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

#define GridView_GetItem(hwnd, hitem, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_GETITEM, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

#define GridView_GetParentItem(hwnd, hitem, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_GETPARENTITEM, (WPARAM)(HGRIDITEM)(hitem), (LPARAM)(const GVITEM *)pitem)

#define GridView_InsertColumn(hwnd, index, pcol) \
	(BOOL)SNDMSG((hwnd), GVM_INSERTCOLUMN, (WPARAM)(index), (LPARAM)(const GVCOLUMN *)pcol)

#define GridView_SetStyle(hwnd, mask, style) \
	(BOOL)SNDMSG((hwnd), GVM_SETSTYLE, (WPARAM)(mask), (LPARAM)(style))

#define GridView_DeleteAll(hwnd) \
	(BOOL)SNDMSG((hwnd), GVM_DELETEALL, 0, 0)

#define GridView_DeleteItem(hwnd, hitem) \
	(BOOL)SNDMSG((hwnd), GVM_DELETEITEM, (WPARAM)(HGRIDITEM)(hitem), 0)

#define GridView_DeleteChildren(hwnd, hitem) \
	(BOOL)SNDMSG((hwnd), GVM_DELETECHILDREN, (WPARAM)(HGRIDITEM)(hitem), 0)

#define GridView_Update(hwnd) \
	(BOOL)SNDMSG((hwnd), GVM_UPDATE, 0, 0)

#define GridView_FindChild(hwnd, hParent, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_FINDCHILD, (WPARAM)(HGRIDITEM)(hParent), (LPARAM)(const GVITEM *)pitem)

#define GridView_FindChildRecurse(hwnd, hParent, pitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_FINDCHILDR, (WPARAM)(HGRIDITEM)(hParent), (LPARAM)(const GVITEM *)pitem)

#define GridView_GetParent(hwnd, hitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_GETPARENT, (WPARAM)(HGRIDITEM)(hitem), 0)

#define GridView_GetFirstChild(hwnd, hparent) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_GETFIRSTCHILD, (WPARAM)(HGRIDITEM)(hparent), 0)

#define GridView_GetNextSibling(hwnd, hitem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_GETNEXTSIBLING, (WPARAM)(HGRIDITEM)(hitem), 0)

#define GridView_GetCurSel(hwnd) \
	(UINT)SNDMSG((hwnd), GVM_GETCURSEL, 0, 0)

#define GridView_GetItemHandle(hwnd, iItem) \
	(HGRIDITEM)SNDMSG((hwnd), GVM_GETITEMHANDLE, (WPARAM)(UINT)(iItem), 0)

#define GridView_GetHeader(hwnd) \
	(HWND)SNDMSG((hwnd), GVM_GETHEADER, 0, 0)

#define GridView_GetChildIndex(hwnd, hparent) \
	(UINT)SNDMSG((hwnd), GVM_GETCHILDINDEX, (WPARAM)(HGRIDITEM)(hparent), 0)

#define GridView_SetFont(hwnd, hFont, nFontIdx) \
	(UINT)SNDMSG((hwnd), WM_SETFONT, (WPARAM)(HFONT)(hFont), (LPARAM)(int)(nFontIdx))

#define GridView_ExpandItem(hwnd, hItem, expanded, recurse) \
	(UINT)SNDMSG((hwnd), GVM_EXPANDITEM, (WPARAM)(HGRIDITEM)(hItem), MAKELPARAM((BOOL)(expanded), (BOOL)(recurse)))


//
//	GridView API
//
HWND CreateGridView(HWND hwndParent, int id, UINT uStyle, UINT uExStyle);
void InitGridView();


#ifdef __cplusplus
}
#endif 

#endif
