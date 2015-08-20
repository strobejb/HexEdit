//
//  TypeViewGui.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS

#include <tchar.h>
#include <ctype.h>

#include "..\TypeLib\parser.h"

#include <windows.h>
#include <commctrl.h>

#include "..\HexEdit\HexUtils.h"
#include "..\HexEdit\resource.h"
#include "..\GridView\GridView.h"
#include "..\HexView\HexView.h"

#include "TypeView.h"

#include "trace.h"

bool Evaluate(HGRIDITEM hParent, ExprNode *expr, INUMTYPE *result, size_w baseOffset, HWND hwndHV, HWND hwndGV);
size_w SizeOf(Type *type, size_w offset, HWND hwndHV);

extern "C" HWND GetActiveHexView(HWND hwndMain);
extern "C" BOOL NotifyFileChange(TCHAR *szPathName, HWND hwndNotify, HANDLE hQuitEvent);

extern vector <FILE_DESC*>	globalFileHistory;

extern "C" BOOL g_fDisplayHex = FALSE;
extern "C" BOOL g_fDisplayBigEndian = FALSE;

extern "C" int Parse(char *file);
extern "C" HWND g_hwndMain;

size_w InsertTypeGV(HWND hwndGridView, HGRIDITEM hRoot, TypeDecl *typeDecl, size_w offset);

bool fShowFullType = true;

BOOL g_fShowGrid = FALSE;
BOOL g_fShowTreeLines = FALSE;
BOOL g_fShowFullSelection = FALSE;
BOOL g_fShowFocus = FALSE;

HGRIDITEM GridView2_GetItem(HWND hwnd, HGRIDITEM hItem, PGVITEM gvitem)
{
	return GridView_GetItem(hwnd, hItem, gvitem);
}

HGRIDITEM GridView2_FindChild(HWND hwnd, HGRIDITEM hItem, PGVITEM gvitem)
{
	return GridView_FindChild(hwnd, hItem, gvitem);
}

HGRIDITEM GridView2_InsertUniqueChild(HWND hwnd, HGRIDITEM hItem, PGVITEM gvitem)
{
	return GridView_InsertUniqueChild(hwnd, hItem, gvitem);
}

BOOL GridView2_SetItem(HWND hwnd, HGRIDITEM hItem, PGVITEM gvitem)
{
	return GridView_SetItem(hwnd, hItem, gvitem);
}

Type *TypeView_GetType(HWND hwndGV, HGRIDITEM hItem)
{
	GVITEM gvi = { 0 };

	gvi.mask = GVIF_PARAM;
	gvi.iSubItem  = COLIDX_NAME;
	GridView2_GetItem(hwndGV, hItem, &gvi);

	return (Type *)gvi.param;
}

ULONG64 TypeView_GetOffset(HWND hwndGV, HGRIDITEM hItem)
{
	GVITEM gvi = { 0 };

	gvi.mask = GVIF_PARAM;
	gvi.iSubItem  = COLIDX_OFFSET;
	GridView2_GetItem(hwndGV, hItem, &gvi);

	return gvi.param;
}

/*void Dump(FILE *fp)
{
	int i;

	for(i = 0; i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];

		DisplayWhitespace(&typeDecl->fileRef);
		printf("\n");
		DisplayTags(fp, typeDecl->tagList);
		DisplayType(fp, typeDecl->baseType);
		//DisplayType(globalTypeList[i], 0);
		printf(";\n\n");
	}
}*/

extern vector <TypeDecl *> globalTypeDeclList;

typedef struct
{
	HANDLE	hPipe;
	HGLOBAL hMem;
} PIPE_PARAMS;

extern "C" DWORD WINAPI PipeReadProc(PIPE_PARAMS *param);
extern "C" HANDLE CreatePipeThread(TCHAR *szPipeName, LPTHREAD_START_ROUTINE pThreadProc, PIPE_PARAMS *params);

//void ErrCallback(ERROR err, const char *str, void *param)
//{
//}

extern "C"
int Parse(char *file)
{
	PIPE_PARAMS param = { 0 };
	TCHAR	    szPipeName[MAX_PATH];
	HANDLE		hThread;

	// create a thread+pipe that will read+append data into a HGLOBAL memory object
	// the HGLOBAL will be returned into PIPE_PARAMS structure when done
	hThread = CreatePipeThread(szPipeName, (LPTHREAD_START_ROUTINE)PipeReadProc, &param);
	FILE *err = _tfopen(szPipeName, _T("wt"));


	Parser p;
	p.SetErrorStream(err);
	//p.SetErrorCallback(ErrCallback, hwndEdit);
	
	int r = 0;
	if(p.Init(file))
		r = p.Parse();

	fflush(err);
	fclose(err);

	// wait for the pipe-thread to finish reading our data
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	// did it work? 
	if(r == 0)
	{
		char *c = (char *)GlobalLock(param.hMem);
		HWND hwndEdit = CreateWindowEx(WS_EX_TOOLWINDOW, TEXT("EDIT"), 0, ES_MULTILINE|WS_VSCROLL|WS_HSCROLL|WS_POPUP|WS_OVERLAPPEDWINDOW ,
		100,100,900,400,g_hwndMain,0,0,0);
		
		HFONT hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0,_T("Consolas"));
		
		SendMessage(hwndEdit,WM_SETFONT,(WPARAM)hFont,0);
		CenterWindow(hwndEdit);
		
		SetWindowTextA(hwndEdit, c);
		ShowWindow(hwndEdit, SW_SHOW);
		
		return 0;
	}
	else
	{
		return 1;
	}
}

void FormatDataItem(HWND hwndGV, HGRIDITEM hItem, Type *type, size_w dwOffset)
{
	GVITEM gvitem = { 0 };
	TCHAR buf[200] = TEXT("");

	DWORD i, count = 20;
	BYTE HexData[14];

	HWND hwndHV = GetActiveHexView(g_hwndMain);
	DWORD numread;

	numread = HexView_GetData(hwndHV, dwOffset, (BYTE *)&HexData, sizeof(HexData));

	if(type->link->ty == typeARRAY)
		type = type->link->link;
	else
		return;

	switch(type->ty)
	{
	case typeCHAR:
		_stprintf(buf, TEXT("\"%hs\""),  (char *)HexData);
		gvitem.state	= 0;
		break;

	case typeDWORD:
		{
			DWORD *dwptr = (DWORD *)&HexData[0];
			TCHAR *sptr = buf;

			sptr += _stprintf(sptr, TEXT("{ "));
			
			for(i = 0; i < count; i++)
			{
				sptr += _stprintf(sptr, TEXT("%d, "), dwptr[i]);
			}

			sptr += _stprintf(sptr, TEXT("}"));
			gvitem.state	= GVIS_READONLY;
		}

	case typeWORD:
		{
			WORD *dwptr = (WORD *)&HexData[0];
			TCHAR *sptr = buf;

			sptr += _stprintf(sptr, TEXT("{ "));
			
			for(i = 0; i < count; i++)
			{
				sptr += _stprintf(sptr, TEXT("%d, "), dwptr[i]);
			}

			sptr += _stprintf(sptr, TEXT("}"));
			gvitem.state	= GVIS_READONLY;

		}
		break;

	default:
		gvitem.state	= 0;
		break;
	}

	gvitem.pszText		= buf;
	gvitem.iSubItem		= COLIDX_DATA;
	gvitem.param		= (ULONG_PTR)type;
	gvitem.color		= numread ? RGB(0,0,0) : RGB(128,128,128);
	gvitem.mask	= GVIF_PARAM | GVIF_TEXT | GVIF_STATE | GVIF_COLOR;

	GridView2_SetItem(hwndGV, hItem, &gvitem);
}

bool LocateComment(FILEREF *fileRef, char **s, char **cs, char **ce, char **e);

void FormatWhitespace(FILEREF *fileRef, TCHAR *buf)
{
	char *s, *cs, *ce, *e;
	if(LocateComment(fileRef, &s, &cs, &ce, &e))
	{
		_stprintf(buf, TEXT("%.*hs"), (int)(ce-cs), cs);
	}
	else
	{
		*buf = '\0';
	}

/*	if(fileRef->wspEnd > fileRef->wspStart)
	{
		FILE_DESC *fileDesc = fileRef->fileDesc;//fileHistory[fileRef->stackIdx];
		
		char *s1 = fileDesc->buf + fileRef->wspStart;
		char *s2 = fileDesc->buf + fileRef->wspEnd;

		// skip over space/tabs
		while(s1 < s2 && isspace(*s1))
			s1++;

		// skip over start of comment (// chars)
		//while(s1 < s2 && *s1 == '/')
		//	s1++;
		s1 += 2;

		// skip spaces again
		while(s1 < s2 && isspace(*s1))
			s1++;

		char *s0 = s1;
		while(s1 < s2 && *s1 != '\n')
			s1++;

		//while(s2 > s1 && isspace(*(s2-1)))
		//	s2--;

		_stprintf(buf, TEXT("%.*hs"), s1-s0, s0);

		//printf("%.*s", s2-s1, s1);
		
		//printf("%.*s",  fileRef->wspEnd - fileRef->wspStart - 1, 
		//				fileDesc->buf + fileRef->wspStart

		//);
	}*/
}

extern "C" void reverse(BYTE *buf, int len);

//#pragma hex[]

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;
	
	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime  = (DWORD)ll;
	pft->dwHighDateTime = (DWORD)(ll >> 32);
}

void FileTimeToUnixTime(LPFILETIME pft, time_t* t) 
{ 
  LONGLONG ll = ((LONGLONG)pft->dwHighDateTime) << 32; 
  ll = ll + pft->dwLowDateTime - 116444736000000000; 
  *t = (time_t)(ll/10000000);
} 

DWORD FmtData(TCHAR *buf, BYTE *HexData, TYPE baseType, bool displaySigned, bool displayHex, bool bigEndian)
{
	double f;

	DWORD  typeLenLookup[]			= { 0, 1, 2, 1, 2, 4, 8, 4, 8, 4, 2, 2, 4, 8 };
//	TCHAR *typeFmtLookupHex2[]		= { 0, TEXT("%#02x \'%c\'"), TEXT("%#04x '%lc\'"), TEXT("%#02x"), TEXT("%#04x"), TEXT("%#08x"), TEXT("%#0I64x"), TEXT("%#g"), TEXT("%#g") };
	TCHAR *typeFmtLookupHex[]		= { 0, TEXT("%02x \'%c\'"), TEXT("%04x '%lc\'"), TEXT("%02x"), TEXT("%04x"), TEXT("%08x"), TEXT("%0I64x"), TEXT("%#g"), TEXT("%#g") };
	TCHAR *typeFmtLookupSigned[]	= { 0, TEXT("%d \'%c\'"), TEXT("%d \'%lc\'"), TEXT("%d"), TEXT("%hd"), TEXT("%d"), TEXT("%I64d"), TEXT("%#g"), TEXT("%#g") };
	TCHAR *typeFmtLookupUnsigned[]	= { 0, TEXT("%u \'%c\'"), TEXT("%u \'%lc\'"), TEXT("%u"), TEXT("%hu"), TEXT("%u"), TEXT("%I64u"), TEXT("%#g"), TEXT("%#g") };

	TCHAR *fmt;

	if(displayHex)				fmt = typeFmtLookupHex[baseType];
	else if(displaySigned)		fmt = typeFmtLookupSigned[baseType];
	else						fmt = typeFmtLookupUnsigned[baseType];

	DWORD dwLength = typeLenLookup[baseType];

	if(bigEndian || g_fDisplayBigEndian)
		reverse(HexData, dwLength);

	struct DOSDATE
	{
		WORD day	: 5;
		WORD month	: 4;
		WORD year	: 7;
	} *dosdate = (DOSDATE *)HexData;

	struct DOSTIME
	{
		WORD sec	: 5;
		WORD min	: 6;
		WORD hour	: 5;
	} *dostime = (DOSTIME *)HexData;

	FILETIME ft;
	SYSTEMTIME st;

		switch(baseType)
		{
		case typeDOSTIME:
			_stprintf(buf, TEXT("%02d:%02d:%02d"), dostime->hour, dostime->min, dostime->sec);
			break;

		case typeDOSDATE:
			_stprintf(buf, TEXT("%02d/%02d/%04d"), dosdate->day, dosdate->month, dosdate->year);
			break;

		case typeTIMET:
			UnixTimeToFileTime(*(time_t *)HexData, &ft);
			FileTimeToSystemTime((FILETIME *)HexData, &st);
			_stprintf(buf, TEXT("%02d/%02d/%04d %02d:%02d:%02d"), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			break;

		case typeFILETIME:
			FileTimeToSystemTime((FILETIME *)HexData, &st);
			_stprintf(buf, TEXT("%02d/%02d/%04d %02d:%02d:%02d"), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			break;

		case typeCHAR:		
			_stprintf(buf, fmt, HexData[0], HexData[0] < ' ' ? '.' : HexData[0]);
			break;
			
		case typeWCHAR: 
			_stprintf(buf, fmt, *(WCHAR *)(&HexData[0]));
			break;
			
		case typeBYTE:		
			_stprintf(buf, fmt, HexData[0]);
			break;
			
		case typeWORD:
			_stprintf(buf, fmt, *(UINT16 *)(&HexData[0]));			
			break;
			
		case typeDWORD:		
			_stprintf(buf, fmt, *(UINT32 *)(&HexData[0]));
			break;
			
		case typeQWORD:
			_stprintf(buf, fmt, *(UINT64 *)(&HexData[0]));
			break;
			
		case typeFLOAT:
			f = *(float *)(&HexData[0]);
			_stprintf(buf, fmt, f);
			break;
			
		case typeDOUBLE:
			f = *(double*)(&HexData[0]);
			_stprintf(buf, fmt, f);
			break;

		}

	return 0;
}

extern "C"
DWORD FmtData(TCHAR *buf, BYTE *HexData, TOKEN baseType, int displaySigned, int displayHex, int bigEndian)
{
	return FmtData(buf, HexData, TokenToType(baseType), displaySigned ? true : false, displayHex ? true : false, bigEndian ? true : false);
}


//
//	offset - offset to read data from for this item
//
size_w FmtData(HWND hwndGV, HGRIDITEM hRoot, Type *type, size_w dwOffset, TypeDecl *typeDecl)
{
	GVITEM gvitem = { 0 };

	Enum *eptr;
	TCHAR buf[200];
	double f;
	size_t i;
	
	DWORD  typeLenLookup[]			= { 0, 1, 2, 1, 2, 4, 8, 4, 8, 4, 2, 2, 4, 8 };
//	TCHAR *typeFmtLookupHex2[]		= { 0, TEXT("%#02x \'%c\'"), TEXT("%#04x '%lc\'"), TEXT("%#02x"), TEXT("%#04x"), TEXT("%#08x"), TEXT("%#0I64x"), TEXT("%#g"), TEXT("%#g") };
	TCHAR *typeFmtLookupHex[]		= { 0, TEXT("%02x \'%c\'"), TEXT("%04x '%lc\'"), TEXT("%02x"), TEXT("%04x"), TEXT("%08x"), TEXT("%0I64x"), TEXT("%#g"), TEXT("%#g") };
	TCHAR *typeFmtLookupSigned[]	= { 0, TEXT("%d \'%c\'"), TEXT("%d \'%lc\'"), TEXT("%d"), TEXT("%hd"), TEXT("%d"), TEXT("%I64d"), TEXT("%#g"), TEXT("%#g") };
	TCHAR *typeFmtLookupUnsigned[]	= { 0, TEXT("%u \'%c\'"), TEXT("%u \'%lc\'"), TEXT("%u"), TEXT("%hu"), TEXT("%u"), TEXT("%I64u"), TEXT("%#g"), TEXT("%#g") };

	TCHAR *fmt;

	bool displaySigned		= false;
	BOOL displayHex			= g_fDisplayHex;
	bool bigEndian			= false;
	TYPE baseType			= BaseType(type);
	bool displayEnum		= false;
	Enum *baseEnum			= 0;

	displaySigned = FindType(type, typeSIGNED) ? true : false;

	if(typeDecl)
	{
		ExprNode *expr;
		if(FindTag(typeDecl->tagList, TOK_ENDIAN, &expr))
		{
			if(strcmp(expr->str, "big") == 0)
				bigEndian = true;
		}

		if(FindTag(typeDecl->tagList, TOK_ENUM, &expr))
		{
			baseEnum = FindEnum(expr->str);
			displayEnum = true;
		}
	}

	if(displayHex)				fmt = typeFmtLookupHex[baseType];
	else if(displaySigned)		fmt = typeFmtLookupSigned[baseType];
	else						fmt = typeFmtLookupUnsigned[baseType];

	BYTE HexData[8];
	HWND hwndHV = GetActiveHexView(g_hwndMain);
	DWORD numread;
	
	numread = HexView_GetData(hwndHV, dwOffset, (BYTE *)&HexData, sizeof(HexData));

	DWORD dwLength = typeLenLookup[type->ty];

	if(numread < dwLength)
	{
		gvitem.pszText		= TEXT("");
		gvitem.iSubItem		= COLIDX_DATA;
		gvitem.state		= 0;
		gvitem.mask			= GVIF_TEXT | GVIF_STATE;
		GridView2_SetItem(hwndGV, hRoot, &gvitem);
		return typeLenLookup[type->ty];
	}

	if(bigEndian || g_fDisplayBigEndian)
		reverse(HexData, dwLength);

	if(FindType(type, typeDOSDATE)) baseType = type->ty;
	if(FindType(type, typeDOSTIME))	baseType = type->ty;
	if(FindType(type, typeFILETIME))baseType = type->ty;
	if(FindType(type, typeTIMET))	baseType = type->ty;

	if(displayEnum)
	{
		if(BaseType(type) == typeENUM)
			eptr = BaseNode(type)->eptr;
		else
			eptr = baseEnum;
		
		WORD *wptr = (WORD *)(&HexData[0]);

		_stprintf(buf, TEXT("%04x"), *wptr);

		for(i = 0; i < eptr->fieldList.size(); i++)
		{
			if(Evaluate(eptr->fieldList[i]->expr) == *wptr)
			{
				_stprintf(buf, TEXT("%hs"), eptr->fieldList[i]->name->name);
				break;
			}
		}

		gvitem.pszText		= buf;
		gvitem.iSubItem		= COLIDX_DATA;
		gvitem.state		= GVIS_DROPDOWN;
		gvitem.mask	= GVIF_TEXT | GVIF_STATE;
		GridView2_SetItem(hwndGV, hRoot, &gvitem);

		return SizeOf(type, 0, 0);
	}
	else
	{
		struct DOSDATE
		{
			WORD day	: 5;
			WORD month	: 4;
			WORD year	: 7;
		} *dosdate = (DOSDATE *)HexData;

		struct DOSTIME
		{
			WORD sec	: 5;
			WORD min	: 6;
			WORD hour	: 5;
		} *dostime = (DOSTIME *)HexData;

		FILETIME ft;
		SYSTEMTIME st;

		switch(baseType)
		{
		case typeDOSTIME:
			_stprintf(buf, TEXT("%02d:%02d:%02d"), dostime->hour, dostime->min, dostime->sec);
			break;

		case typeDOSDATE:
			_stprintf(buf, TEXT("%02d/%02d/%04d"), dosdate->day, dosdate->month, dosdate->year);
			break;

		case typeTIMET:
			UnixTimeToFileTime(*(time_t *)HexData, &ft);
			FileTimeToSystemTime((FILETIME *)HexData, &st);
			_stprintf(buf, TEXT("%02d/%02d/%04d %02d:%02d:%02d"), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			break;

		case typeFILETIME:
			FileTimeToSystemTime((FILETIME *)HexData, &st);
			_stprintf(buf, TEXT("%02d/%02d/%04d %02d:%02d:%02d"), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			break;

		case typeCHAR:		
			_stprintf(buf, fmt, HexData[0], HexData[0] < ' ' ? '.' : HexData[0]);
			break;
			
		case typeWCHAR: 
			_stprintf(buf, fmt, *(WCHAR *)(&HexData[0]));
			break;
			
		case typeBYTE:		
			_stprintf(buf, fmt, HexData[0]);
			break;
			
		case typeWORD:
			_stprintf(buf, fmt, *(UINT16 *)(&HexData[0]));			
			break;
			
		case typeDWORD:		
			_stprintf(buf, fmt, *(UINT32 *)(&HexData[0]));
			break;
			
		case typeQWORD:
			_stprintf(buf, fmt, *(UINT64 *)(&HexData[0]));
			break;
			
		case typeFLOAT:
			f = *(float *)(&HexData[0]);
			_stprintf(buf, fmt, f);
			break;
			
		case typeDOUBLE:
			f = *(double*)(&HexData[0]);
			_stprintf(buf, fmt, f);
			break;
			
		case typeENUM:
			eptr = BaseNode(type)->eptr;
			
			{
				WORD *wptr = (WORD *)(&HexData[0]);
				
				_stprintf(buf, TEXT("%04x"), *wptr);
				
				for(i = 0; i < eptr->fieldList.size(); i++)
				{
					if(Evaluate(eptr->fieldList[i]->expr) == *wptr)
					{
						_stprintf(buf, TEXT("%hs"), eptr->fieldList[i]->name->name);
						break;
					}
				}
			}
			
			gvitem.pszText		= buf;
			gvitem.iSubItem		= COLIDX_DATA;
			gvitem.state		= GVIS_DROPDOWN;
			gvitem.mask	= GVIF_TEXT | GVIF_STATE;
			GridView2_SetItem(hwndGV, hRoot, &gvitem);
			
			return 4;
		}
	}

	gvitem.pszText		= buf;
	gvitem.iSubItem		= COLIDX_DATA;
	gvitem.state		= 0;
	gvitem.mask	= GVIF_TEXT | GVIF_STATE | GVIF_COLOR;
	gvitem.color		= numread ? RGB(0,0,0) : RGB(128,128,128);

	GridView2_SetItem(hwndGV, hRoot, &gvitem);

	return typeLenLookup[type->ty];
}

HGRIDITEM InsertIdentifier(HWND hwndGV, HGRIDITEM hRoot, Type *type, size_w dwOffset, TypeDecl *typeDecl)
{
	TCHAR buf[200];	
	GVITEM gvitem = { 0 };
	HGRIDITEM hItem;

	if(fShowFullType)
	{
		RenderType(buf, 200, type);
		gvitem.pszText = buf;
	}
	else
	{
		_stprintf(buf, TEXT("%hs"), type->sym->name);

		RenderType(buf, 200, type->link);
		gvitem.pszText = buf;//type->sym->name;
	}

	gvitem.state = 0;

	if(type->link && type->link->ty != typeARRAY)
		gvitem.state |= GVIS_EXPANDED;
		
	gvitem.iImage  = IsStruct(type) ? 1 : 0;
	//gvitem.iImage  = isstruct ? 3 : 2;

	if(typeDecl && typeDecl->tagList)
		gvitem.iImage+=2;


	gvitem.param = (ULONG_PTR)type;//typeDecl;
	gvitem.mask  = GVIF_TEXT|GVIF_PARAM |GVIF_STATE | GVIF_IMAGE;
	hItem = GridView2_InsertUniqueChild(hwndGV, hRoot, &gvitem);
				
	if(type->link && type->link->ty == typeARRAY)
	{
		FormatDataItem(hwndGV, hItem, type, dwOffset);
	}

	if(IsStruct(type))
	{
		gvitem.iSubItem = COLIDX_DATA;
		gvitem.state	= GVIS_READONLY;
		gvitem.pszText	= TEXT("{...}");
		gvitem.param	= 0;
		gvitem.mask = GVIF_PARAM | GVIF_TEXT | GVIF_STATE;
		GridView2_SetItem(hwndGV, hItem, &gvitem);
	}

		/*if(!fShowFullType)
		{
			RenderType(buf, 200, type->link);
			//_stprintf(buf, TEXT("%hs"), type->sym->name);
			
			gvitem.iSubItem = 0;//COLIDX_TYPE;
			gvitem.state	= GVIS_READONLY;
			gvitem.pszText	= buf;
			gvitem.param	= (UINT64)type;
			gvitem.mask = GVIF_PARAM | GVIF_TEXT | GVIF_STATE;
			GridView2_SetItem(hwndGV, hItem, &gvitem);

		}*/

	return hItem;
}

size_w InsertStructType(HWND hwndGV, HGRIDITEM hRoot, size_w dwOffset, Type *type)
{
	Structure *sptr = type->sptr;
	hRoot = InsertIdentifier(hwndGV, hRoot, type, dwOffset, 0);
	GridView_ExpandItem(hwndGV, hRoot, TRUE, FALSE);
	
	for(size_t i = 0; i < sptr->typeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = sptr->typeDeclList[i];
		dwOffset += InsertTypeGV(hwndGV, hRoot, typeDecl, dwOffset);
	}

	return dwOffset;
}

//
//	hwndGV		- handle to GridView control
//	hRoot		- root item-handle 
//	type		- node in the type-chain
//	dwOffset	- current offset
//
//	returns:	size of type
//
size_w RecurseType(HWND hwndGV, HGRIDITEM hRoot, Type *type, size_w dwOffset, TypeDecl *typeDecl)
{
	GVITEM gvitem = { 0 };
	Structure *sptr;
	size_t i;
	TCHAR buf[200];
	
	if(type == 0)
		return 0;

	size_w dwLength = 0;
	INUMTYPE switchVal = 0;//1;

	HWND hwndHV = GetActiveHexView(g_hwndMain);

	switch(type->ty)
	{
	case typeDOSTIME: case typeDOSDATE: case typeFILETIME: case typeTIMET:
		dwLength = FmtData(hwndGV, hRoot, type, dwOffset, typeDecl);
		break;

	case typeCHAR:  case typeWCHAR:  case typeBYTE: 
	case typeWORD:  case typeDWORD:  case typeQWORD:
	case typeFLOAT: case typeDOUBLE: case typeENUM:
		dwLength = FmtData(hwndGV, hRoot, type, dwOffset, typeDecl);
		break;

	case typeUNION:

		// evaluate the switch_is()
		ExprNode *switchExpr;
		if(FindTag(typeDecl->tagList, TOK_SWITCHIS, &switchExpr))
		{
			switchVal = Evaluate(switchExpr);
		}

		sptr = type->sptr;
		for(i = 0; i < sptr->typeDeclList.size(); i++)
		{
			TypeDecl *typeDecl = sptr->typeDeclList[i];
			ExprNode *caseExpr;
			size_w tmpLen = 0;

			if(FindTag(typeDecl->tagList, TOK_CASE, &caseExpr))
			{
				if(Evaluate(caseExpr) == switchVal)
					tmpLen = InsertTypeGV(hwndGV, hRoot, typeDecl, dwOffset);
			}
			else
			{
				tmpLen = InsertTypeGV(hwndGV, hRoot, typeDecl, dwOffset);
			}

			dwLength = max(dwLength, tmpLen);
		}
		
		break;
		
	case typeSTRUCT: 
		
		sptr = type->sptr;

		for(i = 0; i < sptr->typeDeclList.size(); i++)
		{
			TypeDecl *typeDecl = sptr->typeDeclList[i];
			dwLength += InsertTypeGV(hwndGV, hRoot, typeDecl, dwOffset + dwLength);
		}
				
		break;
			
	case typeIDENTIFIER:

		hRoot = InsertIdentifier(hwndGV, hRoot, type, dwOffset, typeDecl);
		dwLength = RecurseType(hwndGV, hRoot, type->link, dwOffset, typeDecl);

		break;
#if 0
		if(fShowFullType)
		{
			RenderType(buf, 200, type);
			gvitem.pszText = buf;
		}
		else
		{
			_stprintf(buf, TEXT("%hs"), type->sym->name);
			gvitem.pszText = buf;//type->sym->name;
		}

		gvitem.state = 0;

		if(type->link && type->link->ty != typeARRAY)
			gvitem.state |= GVIS_EXPANDED;
		
		gvitem.iImage  = IsStruct(type) ? 1 : 0;

		if(typeDecl->tagList)
			gvitem.iImage+=2;


		gvitem.param = (ULONG_PTR)type;//typeDecl;
		gvitem.mask = GVIF_PARAM | GVIF_TEXT | GVIF_STATE | GVIF_IMAGE;
		GridView2_SetItem(hwndGV, hRoot, &gvitem);

		if(type->link && type->link->ty == typeARRAY)
		{
			FormatDataItem(hwndGV, hRoot, type, dwOffset);
		}

		if(IsStruct(type))
		{
			gvitem.iSubItem = COLIDX_DATA;
			gvitem.state	= GVIS_READONLY;
			gvitem.pszText	= TEXT("{...}");
			gvitem.param	= 0;
			gvitem.mask = GVIF_PARAM | GVIF_TEXT | GVIF_STATE;
			GridView2_SetItem(hwndGV, hRoot, &gvitem);

		}

		if(!fShowFullType)
		{
			RenderType(buf, 200, type->link);
			//_stprintf(buf, TEXT("%hs"), type->sym->name);
			
			gvitem.iSubItem = 0;//COLIDX_TYPE;
			gvitem.state	= GVIS_READONLY;
			gvitem.pszText	= buf;
			gvitem.param	= (UINT64)type;
			gvitem.mask = GVIF_PARAM | GVIF_TEXT | GVIF_STATE;
			GridView2_SetItem(hwndGV, hRoot, &gvitem);

		}

		dwLength = RecurseType(hwndGV, hRoot, type->link, dwOffset, typeDecl);
		break;
#endif

	case typeTYPEDEF:
	case typeSIGNED:
	case typeUNSIGNED:
	case typeCONST:
	
	//	return 0;
	case typePOINTER:
		dwLength = RecurseType(hwndGV, hRoot, type->link, dwOffset, typeDecl);
		break;

	case typeARRAY:

		Symbol *sym;
		ExprNode *nameExpr;

		if(FindTag(typeDecl->tagList, TOK_NAME, &nameExpr))		
		{
			if((sym = LookupSymbol(globalTagSymbolList, nameExpr->str)) != 0)
			{
				if(sym->type && sym->type->ty == typeENUM)
					;
				else
					sym = 0;
			}
		}
		else
		{
			sym = 0;
		}

		UINT64 count;
		count = 0;

		Evaluate(GridView_GetParent(hwndGV, hRoot), type->elements, &count, dwOffset, hwndHV, hwndGV);
		
		count &= 0xffff;
		count = min(count,100);

		for(i = 0; i < count; i++)
		{
			HGRIDITEM hItem;
			TCHAR buf[164];

			int len = _stprintf(buf, TEXT("[%d]  "), (int)i);

			if(sym)
			{
				Enum *eptr = sym->type->eptr;
				char *s = 0;

				for(size_t x = 0; x < eptr->fieldList.size(); x++)
				{
					if(i == eptr->fieldList[x]->val)
					{ 
						s = eptr->fieldList[i]->name->name;
					}
				}

				_stprintf(buf + 5, TEXT("- %hs"), s);
			}


			gvitem.pszText	= buf;
			gvitem.state	= GVIS_READONLY;
			gvitem.iImage	= 0;//rand() % 2;
			gvitem.param	= (ULONG_PTR)type->link;
			gvitem.iSubItem		= COLIDX_NAME;
			gvitem.mask = GVIF_STATE|GVIF_PARAM|GVIF_TEXT;

			hItem = GridView2_InsertUniqueChild(hwndGV, hRoot, &gvitem);

			// used to pass '0' as typeDecl??? why?? because this is an array element we 
			// are recursing through!!!!
			dwLength += RecurseType(hwndGV, hItem, type->link, dwOffset + dwLength, 0);//typeDecl);
		}

		break;
		
	default:
		break;
	}
		
	// add the 'offset' column to all items
	if(hRoot)
	{
		_stprintf(buf, TEXT("%08x"), (DWORD)dwOffset);
		gvitem.pszText		= buf;
		gvitem.iSubItem		= COLIDX_OFFSET;
		gvitem.state		= 0;
		gvitem.param		= dwOffset;
		gvitem.mask	= GVIF_TEXT | GVIF_STATE | GVIF_PARAM;
		GridView2_SetItem(hwndGV, hRoot, &gvitem);

		// add the 'comment' column item - only do this for items that aren't array elements
		if(typeDecl)// && type->ty != typeARRAY)
		{
			FILEREF *ref;
			
			ref = &typeDecl->postRef;

			buf[0] = '\0';

			FormatWhitespace(ref, buf);
			
			gvitem.pszText		= buf;
			gvitem.iSubItem		= COLIDX_COMMENT;
			gvitem.state		= 0;
			gvitem.mask	= GVIF_TEXT | GVIF_STATE;
			
			GridView2_SetItem(hwndGV, hRoot, &gvitem);
		}
	}

	return dwLength;
}


/*DWORD head(HWND hwndGridView, TypeDecl *typeDecl, HGRIDITEM hRoot, DWORD dwOffset)
{
	Type *type;
	DWORD dwLength = 0;

	// unnamed struct?
	if(typeDecl->declList.size() == 0 && typeDecl->nested)
	{
		dwLength += RecurseType(hwndGridView, hRoot, typeDecl->baseType, dwOffset + dwLength, typeDecl);
		return dwLength;
	}

	// display each variable-decl
	for(int i = 0; i < typeDecl->declList.size(); i++)
	{
		type = typeDecl->declList[i];

		if(typeDecl->tagList)
		{
			Tag *tag = typeDecl->tagList;

			if(tag->tok == TOK_OFFSET)
			{
				dwOffset = Evaluate(tag->expr);
			}
		}

		GVITEM gvitem = { 0 };
		HGRIDITEM hChild = GridView_InsertzChild(hwndGridView, hRoot, &gvitem);
		dwLength += RecurseType(hwndGridView, hChild, type, dwOffset + dwLength, typeDecl);

	}

	return dwLength;
}*/

void GetModuleDirectory(HMODULE hModule, TCHAR *szPath, DWORD nSize)
{
	TCHAR *ptr;
	GetModuleFileName(hModule, szPath, nSize);

	if((ptr = _tcsrchr(szPath, '\\')) != 0)
		*ptr = '\0';
}


void InitTypeLibrary()
{
	TCHAR szPath[MAX_PATH];
	char path[MAX_PATH];

	globalIdentifierList.clear();
	globalTagSymbolList.clear();
	globalTypeDeclList.clear();

	GetModuleDirectory(0, szPath, MAX_PATH);
	lstrcat(szPath, TEXT("\\..\\..\\typelib"));

	if(GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
	{
		GetModuleDirectory(0, szPath, MAX_PATH);
		lstrcat(szPath, TEXT("\\typelib"));
	}

	size_t s = globalFileHistory.size();
	globalFileHistory.clear();
	globalIdentifierList.clear();
	globalTypeDeclList.clear();
	globalTagSymbolList.clear();
	s = globalFileHistory.size();

	//lstrcat(szPath, TEXT("\\typelib\\zip.txt"));
	//lstrcat(szPath, TEXT("\\test.txt"));

	WIN32_FIND_DATA w32fd;
	lstrcat(szPath, L"\\*.*");
	HANDLE hFind = FindFirstFile(szPath, &w32fd);

	if(hFind)
	{
		TCHAR *ptr = _tcsrchr(szPath, '\\'); *ptr = '\0';
		do 
		{
			if((w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				sprintf(path, "%ls\\%ls", szPath, w32fd.cFileName);
				Parser::Initialize();

				Parse(path);
			}
		} 
		while(FindNextFile(hFind, &w32fd));
	}
}

extern "C" void Initialize()
{
	InitTypeLibrary();

	for(size_t i = 0; i < globalFileHistory.size(); i++)
	{
		TCHAR szPath[MAX_PATH];
		FILE_DESC *fd = globalFileHistory[i];
		wsprintf(szPath, TEXT("%hs"), globalFileHistory[i]->filePath);
		NotifyFileChange(szPath, g_hwndMain, 0);
	}
}
/*
extern "C"
void FillTypeView(HWND hwndGV)
{
	//Parse("c:\\src\\hex\\typelib\\test.txt");
	size_t i;

	Initialize();

	GridView_DeleteAll(hwndGV);

//	IMAGE_FILE_HEADER

	size_w offset = 0;
	for(i = 0; i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];

		//DisplayTags(fp, typeDecl->tagList);

		//head(hwndGV, typeDecl, 0, 0);
		InsertTypeGV(hwndGV, 0, typeDecl, 0);

		//GVITEM gvitem = { 0 };
		//gvitem.pszText = "smeg";//typeDecl->baseType->
		//GridView_InsertTree(hwndGV, 0, &gvitem);
		//DisplayType(
	}

}
*/
extern "C" HWND PrepAllTypes(HWND hwndGridView, int widthArray[])
{
	GVCOLUMN gvcol = { 0 };
	HFONT hFont;

	GridView_SetStyle(hwndGridView, -1, GVS_SINGLE|GVS_FULLROWSELECT|GVS_GRIDLINES);

	hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0, TEXT("Verdana"));
	SendMessage(hwndGridView, WM_SETFONT, (WPARAM)hFont, 0);

	gvcol.xWidth = widthArray[COLIDX_TYPE];//300;
	gvcol.pszText = TEXT("Type");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 0, &gvcol);

	gvcol.xWidth = widthArray[COLIDX_TYPE];//300;
	gvcol.pszText = TEXT("Value");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 1, &gvcol);

	fShowFullType = false;

	return hwndGridView;

}

extern "C" HWND PrepGridView(HWND hwndGridView, int widthArray[], TCHAR *szTypeName)
{
	GVCOLUMN gvcol = { 0 };
	HFONT hFont, hBold, hItalic;
	HIMAGELIST hImgList ;
	bool fAllTypes = szTypeName && lstrcmp(szTypeName, TEXT("struct ALL")) == 0 ? true : false;

	GridView_SetStyle(hwndGridView, -1, GVS_FULLROWSELECT|GVS_GRIDLINES|GVS_TREELINES
		//|GVS_SHOWFOCUS
		);//,GVS_FULLROWSELECT|GVS_GRIDLINES);

	hBold = CreateFont(-14,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0, TEXT("Segoe UI"));
	//SendMessage(g_hWndGridView, WM_SETFONT, (WPARAM)hFont, 0);

	//hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0,"Segoe UI");
	hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0, TEXT("Verdana"));

	hItalic = CreateFont(-14,0,0,0,0,TRUE,0,0,0,0,0,0,0, TEXT("Verdana"));
	//SendMessage(GetWindow(g_hWndGridView, GW_CHILD), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(hwndGridView, WM_SETFONT, (WPARAM)hFont, 0);

//	gvcol.hFont = hBold;
	if(!fAllTypes)
	{
		fShowFullType = true;

		gvcol.xWidth = widthArray[COLIDX_NAME];//300;
		gvcol.pszText = TEXT("Name");
		gvcol.uState = GVCS_BLENDIMAGE;
		
		GridView_InsertColumn(hwndGridView, COLIDX_NAME, &gvcol);


		gvcol.hFont = hFont;
		gvcol.xWidth  = widthArray[COLIDX_DATA];//200;
		gvcol.pszText = TEXT("Value");
		gvcol.uState  = 0;//GVCS_ALIGN_RIGHT;
	
		GridView_InsertColumn(hwndGridView, COLIDX_DATA, &gvcol);

		gvcol.xWidth = widthArray[COLIDX_OFFSET];//100;
		gvcol.pszText = TEXT("Offset");
		gvcol.uState  = GVCS_READONLY;
		
		GridView_InsertColumn(hwndGridView, COLIDX_OFFSET, &gvcol);
		
		//GridView_InsertColumn(hwndGridView, 1, &gvcol);
		gvcol.xWidth = widthArray[COLIDX_COMMENT];//300;
		gvcol.pszText = TEXT("Comment");
		gvcol.uState  = 0;
		gvcol.hFont   = hItalic;

		GridView_InsertColumn(hwndGridView, COLIDX_COMMENT, &gvcol);
	}
	else
	{
		gvcol.xWidth = widthArray[COLIDX_TYPE];//300;
		gvcol.pszText = TEXT("Type");
		gvcol.uState  = 0;
	
		GridView_InsertColumn(hwndGridView, 0, &gvcol);

		gvcol.xWidth = widthArray[COLIDX_TYPE];//300;
		gvcol.pszText = TEXT("Value");
		gvcol.uState  = 0;

		GridView_InsertColumn(hwndGridView, 1, &gvcol);
	}

	hImgList = ImageList_LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP9), 16, 0, RGB(255,0,255));
	//hImgList = ImageList_LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP12), 16, 0, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION |LR_LOADTRANSPARENT);
	GridView_SetImageList(hwndGridView, hImgList);

	return hwndGridView;
}

extern "C"
size_w SetTypeGV(HWND hwndGridView, HGRIDITEM hItem, TypeDecl *typeDecl, size_w dwOffset)
{
	if(typeDecl->typeAlias == true)
		return dwOffset;

	if(typeDecl->declList.size() > 0)
	{
		Type *type = typeDecl->declList[0];
		ExprNode *offsetExpr;

		if(FindTag(typeDecl->tagList, TOK_OFFSET, &offsetExpr))
			dwOffset = Evaluate(offsetExpr);
			
		dwOffset += RecurseType(hwndGridView, hItem, type, dwOffset, typeDecl);
	}
	
	return dwOffset;
}



size_w InsertTypeGV(HWND hwndGridView, HGRIDITEM hRoot, TypeDecl *typeDecl, size_w dwOffset)
{
	GVITEM		gvitem = { 0 };
	size_w		dwLength = 0;
	ExprNode *	offsetExpr;

	// typedef statements are never displayed
	if(typeDecl->typeAlias == true)
		return dwOffset;

	if(typeDecl->declList.size() == 0 && typeDecl->nested)
	{
		if(FindTag(typeDecl->tagList, TOK_OFFSET, &offsetExpr))
		{
			UINT64 r;
			HWND hwndHV = GetActiveHexView(g_hwndMain);
			//dwOffset += Evaluate(offsetExpr);

			r = sizeof(IMAGE_DOS_HEADER);
			Evaluate(hRoot,	offsetExpr, &r, dwOffset, hwndHV, hwndGridView);
			dwOffset = r & 0xffff;
		}

		// embedded struct/union that has no variables (i.e. just a plain unnamed struct decl)
		// don't insert a gridview item for the struct/union name, this means that
		// the embedded struct members get added at the same level as the parent's members
		dwLength += RecurseType(hwndGridView, hRoot, typeDecl->baseType, dwOffset, typeDecl);
		return dwLength;
	}
	else
	{
		// display each variable-decl
		size_t i;

		for(i = 0; i < typeDecl->declList.size(); i++)
		{
			Type *type = typeDecl->declList[i];

			if(FindTag(typeDecl->tagList, TOK_OFFSET, &offsetExpr))
			{
				UINT64 r;
				HWND hwndHV = GetActiveHexView(g_hwndMain);
				Evaluate(hRoot,//typeDecl->parent->sptr, 
					offsetExpr, &r, dwOffset, hwndHV, hwndGridView);
			}
			
			// always an IDENTIFER at this point
			//HGRIDITEM hChild = GridView_InsertChild(hwndGridView, hRoot, &gvitem);
			//TRACEA("*** %d: %s\n", type->ty, type->sym->name);
			//dwLength += RecurseType(hwndGridView, hChild, type, dwOffset + dwLength, typeDecl);

			//TRACEA("*** %d: %s\n", type->ty, type->sym->name);
			dwLength += RecurseType(hwndGridView, hRoot, type, dwOffset + dwLength, typeDecl);
		}
	}

	return dwLength;
}

HGRIDITEM FindGridItem(HGRIDITEM hParent, ExprNode * expr, HWND hwndGridView)
{
	Symbol *sym;
	HGRIDITEM hItem = 0;
	Structure *parent;

	GVITEM gvitem = { 0, 0, 0, GVIF_PARAM };
	Type *t = TypeView_GetType(hwndGridView, hParent);

	if(IsStruct(t) == false)
		return 0;

	parent = BaseNode(t)->sptr;
	
	for(; expr; expr = expr->right)
	{
		// expr->left will always be EXPR_IDENTIFIER
		if((sym = LookupSymbol(parent->symbolTable, expr->left ? expr->left->str : expr->str)) == 0)
			return 0;	
		
		// look for the Type* node in the GridView
		gvitem.iSubItem = COLIDX_NAME;
		gvitem.mask     = GVIF_PARAM;
		gvitem.param    = (ULONG_PTR)sym->type;
		
		// this *should* succeed, but just in case...
		if((hItem = GridView2_FindChild(hwndGridView, hParent, &gvitem)) == 0)
			return 0;

		Type *base = BaseNode(sym->type);
		
		if(IsStruct(base) && expr->left)
		{
			//sptr   = base->sptr;
			parent	= base->sptr;
			hParent = hItem;
		}
		else if(expr->left == 0)
		{
			return hItem;
		}
		else
		{
			// lvalue is not a structure
			return 0;
		}
	}

	return 0;
}

//
//	Evaluate (flatten) the specified expression and
//	return it's numeric value
//
bool Evaluate(HGRIDITEM hParent, ExprNode *expr, INUMTYPE *result, size_w baseOffset, HWND hwndHV, HWND hwndGV)
{
	INUMTYPE left, right, cond;

	Symbol *sym;
	GVITEM gvitem	= { 0 };
	GVITEM gvi2		= { 0 };

	if(expr == 0)
		return false;

	switch(expr->type)
	{
	case EXPR_IDENTIFIER:
	case EXPR_FIELD:

		HGRIDITEM hItem;
		ExprNode *baseType; baseType = expr->type == EXPR_FIELD ? expr->right : expr;
		
		// look for the note in the GridView that corresponds to the field expression!
		if((hItem = FindGridItem(hParent, expr, hwndGV)) == 0)
		{
			// not a field. maybe it's an enum tag though?
			if(expr->type == EXPR_IDENTIFIER)
			{
				if((sym = LookupSymbol(globalIdentifierList, expr->str)) == 0)
					return false;
				
				if(sym->type->ty == typeENUMVALUE)
				{
					*result = sym->type->evptr->val;
					return true;
				}
				else
				{
					// lvalue is not a structure!
					return false;
				}
			}
			
			return false;
		}

		gvitem.iSubItem		= COLIDX_OFFSET;
		gvitem.mask			= GVIF_PARAM;
		GridView2_GetItem(hwndGV, hItem, &gvitem);

		gvi2.iSubItem		= COLIDX_OFFSET;
		gvi2.mask			= GVIF_PARAM;
		GridView2_GetItem(hwndGV, hParent, &gvi2);

		*result = 0;
		HexView_GetData(hwndHV, gvitem.param + gvi2.param, (BYTE *)result, sizeof(*result));
		return true;

	case EXPR_NUMBER:
		*result = (expr->tok == TOK_INUMBER) ? expr->val : (int)expr->fval;
		return true;

	case EXPR_UNARY:

		if(!Evaluate(hParent, expr->left, &left, baseOffset, hwndHV, hwndGV))
			return false;

		switch(expr->tok)
		{
		case '+':			*result = +left; break;
		case '-':			*result = /*-*/left; break;
		case '!':			*result = !left; break;
		case '~':			*result = ~left; break;
		default:			return false;
		}

		return true;
		
	case EXPR_BINARY:

		if(!Evaluate(hParent, expr->left, &left, baseOffset, hwndHV, hwndGV) || 
		   !Evaluate(hParent, expr->right, &right, baseOffset, hwndHV, hwndGV))
			return false;
			
		switch(expr->tok)
		{
		case '+':			*result = left +  right; break;
		case '-':			*result = left -  right;  break;
		case '*':			*result = left *  right; break;
		case '%':			*result = left %  right; break;
		case '/':			*result = left /  right; break;
		case '|':			*result = left |  right; break;
		case '&':			*result = left &  right; break;
		case '^':			*result = left ^  right; break;
		case TOK_ANDAND:	*result = left && right; break;
		case TOK_OROR:		*result = left || right; break;
		case TOK_SHR:		*result = left << right; break;
		case TOK_SHL:		*result = left >> right; break;
		case TOK_GE:		*result = left >= right; break;
		case TOK_LE:		*result = left <= right; break;
		default:			return false;
		}

	case EXPR_TERTIARY:

		if(!Evaluate(hParent, expr->cond, &cond, baseOffset, hwndHV, hwndGV))
			return false;

		if(cond)
		{
			if(Evaluate(hParent, expr->left, &left, baseOffset, hwndHV, hwndGV))
				*result = left;
			else
				return false;
		}
		else
		{
			if(Evaluate(hParent, expr->right, &right, baseOffset, hwndHV, hwndGV))
				*result = right;
			else
				return false;
		}

		return true;

	default:
		// don't understand anything else
		return false;
	}
}


size_w SizeOf(Type *type, size_w offset, HWND hwndHV)
{
	size_t i;
	size_w size = 0;

	if(type == 0)
		return 0;

	switch(type->ty)
	{
	default:			return SizeOf(type->link, offset, hwndHV);
	case typeARRAY:		return SizeOf(type->link, offset, hwndHV) * Evaluate(type->elements);
	case typeBYTE:		return 1;
	case typeWORD:		return 2;
	case typeDWORD:		return 4;
	case typeQWORD:		return 8;
	case typeFLOAT:		return 4;
	case typeDOUBLE:	return 8;
	case typeCHAR:		return 1;
	case typeWCHAR:		return 2;
	case typeENUM:		return 4;	
	case typeSTRUCT: 
	case typeUNION:	

		for(i = 0; i < type->sptr->fieldList.size(); i++)
		{
			Field *field = &type->sptr->fieldList[i];

			/*ExprNode *offsetExpr;
			if(FindTag(field->tagList, TOK_OFFSET, &offsetExpr))
			{
				unsigned r;
				offset = offset;
				//dwOffset += Evaluate(offsetExpr);

				if(Evaluate(type->sptr, offsetExpr, &r, 0, hwndHV, 0))
				{
					offset = r;
					size = 0;
				}

				//typeDecl->parent->sptr->Evaluate(offsetExpr, &r);
				//Evaluate(offsetExpr, &r);
			}*/

			if(type->ty == typeSTRUCT)
			{
				field->typeList->offset = offset+size;
				size += SizeOf(field->typeList, offset+size, hwndHV);
			}
			else
			{
				type->sptr->fieldList[i].typeList->offset = offset;
				size = max(size, SizeOf(type->sptr->fieldList[i].typeList, offset, hwndHV));
			}
		}
		return size;
	}
}

bool EvaluateUnary(UINT64 val, TOKEN op, UINT64 *result)
{
	switch(op)
	{
	case '+': *result = +val;	return true;
	case '-': *result = val;	return true;
	case '!': *result = !val;	return true;
	case '~': *result = ~val;	return true;
	default:					return false;
	}
}

bool EvaluateUnary(double val, TOKEN op, double *result)
{
	switch(op)
	{
	case '+': *result = +val;	return true;
	case '-': *result = -val;	return true;
	case '!': *result = !val;	return true;
	case '~':					return false;
	default:					return false;
	}
}

bool EvaluateBinary(UINT64 left, UINT64 right, TOKEN op, UINT64 *result)
{
	switch(op)
	{
	case '+':			*result = left +  right; break;
	case '-':			*result = left -  right; break;
	case '*':			*result = left *  right; break;
	case '%':			*result = left %  right; break;
	case '/':			*result = left /  right; break;
	case '|':			*result = left |  right; break;
	case '&':			*result = left &  right; break;
	case '^':			*result = left ^  right; break;
	case TOK_EQU:		*result = left == right; break;
	case TOK_NEQ:		*result = left != right; break;
	case TOK_ANDAND:	*result = left && right; break;
	case TOK_OROR:		*result = left || right; break;
	case TOK_SHR:		*result = left << right; break;
	case TOK_SHL:		*result = left >> right; break;
	case TOK_GE:		*result = left >= right; break;
	case TOK_LE:		*result = left <= right; break;
	default:			return false;
	}

	return true;
}

bool EvaluateBinary(double left, double right, TOKEN op, double *result)
{
	switch(op)
	{
	case '+':			*result = left +  right; break;
	case '-':			*result = left -  right; break;
	case '*':			*result = left *  right; break;
	case '/':			*result = left /  right; break;
	case TOK_EQU:		*result = left == right; break;
	case TOK_NEQ:		*result = left != right; break;
	case TOK_ANDAND:	*result = left && right; break;
	case TOK_OROR:		*result = left || right; break;
	case TOK_GE:		*result = left >= right; break;
	case TOK_LE:		*result = left <= right; break;
	default:			return false;
	}

	return true;
}

//
//	Evaluate (flatten) the specified expression and
//	return it's numeric value
//
bool Evaluate(HGRIDITEM hParent, ExprNode *expr, size_w baseOffset, HWND hwndHV, HWND hwndGV, ExprNode *result)
{
	//unsigned left, right, cond;
	ExprNode left;//(EXPR_NUMBER, TOK_INUMBER);
	ExprNode right;//(EXPR_NUMBER, TOK_INUMBER);
	ExprNode cond;//(/EXPR_NUMBER, TOK_INUMBER);

	Symbol *sym;
	GVITEM gvitem	= { 0 };
	GVITEM gvi2		= { 0 };

	if(expr == 0)
		return false;

	switch(expr->type)
	{
	case EXPR_IDENTIFIER:
	case EXPR_FIELD:

		HGRIDITEM hItem;
		
		// look for the note in the GridView that corresponds to the field expression!
		if((hItem = FindGridItem(hParent, expr, hwndGV)) == 0)
		{
			// not a field. maybe it's an enum tag though?
			if(expr->type == EXPR_IDENTIFIER)
			{
				if((sym = LookupSymbol(globalIdentifierList, expr->str)) == 0)
					return false;
				
				if(sym->type->ty == typeENUMVALUE)
				{
					result->val = sym->type->evptr->val;
					result->tok = TOK_INUMBER;
					return true;
					//Enum *e = sym->type->evptr->parent;
					//return e->Evaluate(sym->name, &result);
				}
				else
				{
					// lvalue is not a structure!
					return false;
				}
			}
			
			return false;
		}

		gvitem.iSubItem		= COLIDX_OFFSET;
		gvitem.mask	= GVIF_PARAM;
		GridView2_GetItem(hwndGV, hItem, &gvitem);

		gvi2.iSubItem		= COLIDX_OFFSET;
		gvi2.mask		= GVIF_PARAM;
		GridView2_GetItem(hwndGV, hParent, &gvi2);

		HexView_GetData(hwndHV, gvitem.param + gvi2.param, (BYTE *)result, sizeof(*result));
		return true;

	case EXPR_NUMBER:
		*result = *expr;
		//*result = (expr->tok == TOK_INUMBER) ? expr->val : (int)expr->fval;
		return true;

	//case EXPR_IDENTIFIER:
		// is it a structure member?

	case EXPR_UNARY:

		if(Evaluate(hParent, expr->left, baseOffset, hwndHV, hwndGV, &left))
		{
			if(left.tok == TOK_INUMBER)
			{
				result->tok = TOK_INUMBER;			
				return EvaluateUnary(left.val, expr->tok, &result->val);
			}
			else
			{
				result->tok = TOK_FNUMBER;			
				return EvaluateUnary(left.fval, expr->tok, &result->fval);
			}
		}
		else
		{
			return false;
		}
		
	case EXPR_BINARY:

		if(Evaluate(hParent, expr->left, baseOffset, hwndHV, hwndGV, &left) &&
		   Evaluate(hParent, expr->right, baseOffset, hwndHV, hwndGV, &right))
		{
			if(left.tok == TOK_INUMBER && right.tok == TOK_FNUMBER)
			{
				// we need to convert so they are the same base
				left.fval = (double)(signed __int64)left.val;
				left.tok  = TOK_FNUMBER;
			}
			else if(left.tok == TOK_FNUMBER && right.tok == TOK_INUMBER)
			{
				right.fval = (double)(signed __int64)right.val;
				right.tok  = TOK_FNUMBER;
			}

			if(left.tok == TOK_INUMBER)
			{
				result->tok = TOK_INUMBER;
				return EvaluateBinary(left.val, right.val, expr->tok, &result->val);
			}
			else
			{
				result->tok = TOK_FNUMBER;
				return EvaluateBinary(left.fval, right.fval, expr->tok, &result->fval);
			}
		}
		else
		{
			return false;
		}


	case EXPR_TERTIARY:

		if(Evaluate(hParent, expr->cond, baseOffset, hwndHV, hwndGV, &cond))
		{
			if(cond.val)
			{
				if(Evaluate(hParent, expr->left, baseOffset, hwndHV, hwndGV, &left))
					*result = left;
				else
					return false;
			}
			else
			{
				if(Evaluate(hParent, expr->right, baseOffset, hwndHV, hwndGV, &right))
					*result = right;
				else
					return false;
			}

			return true;
		}
		else
		{
			return false;
		}

	default:
		// don't understand anything else
		return false;
	}
}

