//
//  TypeViewGui - Copy.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <tchar.h>
#include <ctype.h>
//#include "..\TypeLib\lexer.h"
//#include "..\TypeLib\error.h"
//#include "..\TypeLib\expr.h"
//#include "..\TypeLib\types.h"

#include "..\TypeLib\parser.h"

#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "..\TypeView2\GridView\GridView.h"


extern "C" int Parse(char *file);
TOKEN TypeToToken(TYPE ty);
Type *InvertType(Type *type);
void  AppendType(Type *type, Type *append);
Type * MakeFullType(Type *base, Type *decl);
Type * MakeDisplayType(Type *decl);
#include "file.h"

Type *BreakLink(Type *type, Type *term);
void RestoreLink(Type *type, Type *term);

void InsertType(HWND hwndGridView, HGRIDITEM hRoot, TypeDecl *typeDecl);

bool fShowFullType = true;
//BYTE file[0x1000]

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

//PIMAGE_DOS_HEADER

DWORD head(HWND hwndGV, TypeDecl *typeDecl, HGRIDITEM hRoot, DWORD dwOffset);

struct string
{
	int   len;
	int   maxlen;
	TCHAR *ptr;
//	char  buf[100];
	bool  brackets;
};

int smeg(string *sbuf, Type *type)
{
	GVITEM gvitem = { 0 };
	Structure *sptr;

	if(type == 0)
		return 0;

	switch(type->ty)
	{
	case typeSTRUCT: case typeUNION:
		
		sptr = type->sptr;
		
		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("%hs "), Parser::inenglish(TypeToToken(type->ty)));

		if(!sptr->symbol->anonymous)
			sbuf->ptr += _stprintf(sbuf->ptr, TEXT("%hs "), sptr->symbol->name); 
			//sbuf->ptr += sprintf(sbuf->ptr, " %s ", sptr->symbol->anonymous ? "<anon>" : sptr->symbol->name); 

		smeg(sbuf, type->link);
		
		break;
		
	case typeCHAR:		case typeWCHAR: 
	case typeBYTE:		case typeWORD:
	case typeDWORD:		case typeQWORD:
		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("%hs "), Parser::inenglish(TypeToToken(type->ty))); 
		smeg(sbuf, type->link);

		break;

	case typeTYPEDEF:		
		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("%hs "), type->sym->name);
		smeg(sbuf, type->link);
		break;
		
	case typeIDENTIFIER:
		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("%hs"), type->sym->name);
		smeg(sbuf, type->link);
		break;

	case typePOINTER:

		if(sbuf->brackets)
			sbuf->ptr += _stprintf(sbuf->ptr, TEXT("("));

		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("*"));
		smeg(sbuf, type->link);

		if(sbuf->brackets)
			sbuf->ptr += _stprintf(sbuf->ptr, TEXT(")"));

		break;

	case typeENUM:
		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("enum %hs "), type->eptr->symbol->name);
		smeg(sbuf, type->link);
		break;

	case typeARRAY:

		smeg(sbuf, type->link);
		sbuf->ptr += _stprintf(sbuf->ptr, TEXT("[%d]"), Evaluate(type->elements));

		break;
		
	default:
		break;
		}
		

	return 0;
}

//
//
//
void render(TCHAR *buf, Type *type)
{
	string sbuf;
	sbuf.ptr = buf;
//	sbuf.buf = buf;
	//type = MakeDisplayType(type);//
	type = InvertType(type);
	smeg(&sbuf, type);
	type = InvertType(type);
}

void FormatDataItem(HWND hwndGV, HGRIDITEM hItem, Type *type, DWORD dwOffset)
{
	GVITEM gvitem = { 0 };
	TCHAR buf[100] = TEXT("");

	DWORD i, count = 20;

	//sprintf(buf, "   0x%08x", dwOffset);

	if(type->link->ty == typeARRAY)
	{
		type = type->link->link;
	}
	else
		return;

	switch(type->ty)
	{
	case typeCHAR:
		_stprintf(buf, TEXT("\"%hs\""),  (char *)(&HexData[dwOffset]));
		gvitem.state	= 0;
		break;

	case typeDWORD:
		{
			DWORD *dwptr = (DWORD *)&HexData[dwOffset];
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
			WORD *dwptr = (WORD *)&HexData[dwOffset];
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

	gvitem.pszText	= buf;
	gvitem.iSubItem = 1;
	gvitem.param	= (ULONG_PTR)type;
	GridView_SetItem(hwndGV, hItem, &gvitem);

}

void FormatWhitespace(FILE_REF *fileRef, TCHAR *buf)
{
	if(fileRef->wspEnd > fileRef->wspStart)
	{
		FILE_DESC *fileDesc = fileRef->fileDesc;//fileHistory[fileRef->stackIdx];
		
		char *s1 = fileDesc->buf + fileRef->wspStart;
		char *s2 = fileDesc->buf + fileRef->wspEnd;

		// skip over space/tabs
		while(s1 < s2 && isspace(*s1))
			s1++;

		// skip over start of comment (// chars)
		while(s1 < s2 && *s1 == '/')
			s1++;

		// skip spaces again
		while(s1 < s2 && isspace(*s1))
			s1++;

		while(s2 > s1 && isspace(*(s2-1)))
			s2--;

		_stprintf(buf, TEXT("%.*hs"), s2-s1, s1);

		//printf("%.*s", s2-s1, s1);
		
		//printf("%.*s",  fileRef->wspEnd - fileRef->wspStart - 1, 
		//				fileDesc->buf + fileRef->wspStart

		//);
	}
}


//
//	hwndGV		- handle to GridView control
//	hRoot		- root item-handle 
//	type		- node in the type-chain
//	dwOffset	- current offset
//
//	returns:	size of type
//
DWORD smeg2(HWND hwndGV, HGRIDITEM hRoot, Type *type, DWORD dwOffset, TypeDecl *typeDecl)
{
	GVITEM gvitem = { 0 };
	Structure *sptr;
	Enum *eptr;
	int i;
	TCHAR buf[200];
	if(type == 0)
		return 0;

	DWORD typeLenLookup[] = { 0, 1, 2, 1, 2, 4, 8 };

	DWORD dwLength = 0;



	switch(type->ty)
	{
		
	case typeSTRUCT: case typeUNION:
		
		sptr = type->sptr;

		{
			//sprintf(buf, "%d", rand());
			/*gvitem.pszText = "{ ... }";
			gvitem.iSubItem = 1;
			gvitem.state = 0;
			GridView_SetItem(hwndGV, hRoot, &gvitem);
			gvitem.iSubItem = 0;*/
		}
		
		for(i = 0; i < sptr->typeDeclList.size(); i++)
		{
			TypeDecl *typeDecl = sptr->typeDeclList[i];
			
			dwLength += head(hwndGV, typeDecl, hRoot, dwOffset + dwLength);
		}
				
		break;
		
	case typeCHAR:		
		_stprintf(buf, TEXT("%d"), HexData[dwOffset]);// < ' ' ? '.' : HexData[dwOffset]);
		goto eek;

	case typeWCHAR: 
		_stprintf(buf, TEXT("%lc"), *(WCHAR *)(&HexData[dwOffset]));
		goto eek;

	case typeBYTE:		
		_stprintf(buf, TEXT("%02x"), HexData[dwOffset]);
		goto eek;
	
	case typeWORD:
		_stprintf(buf, TEXT("%04x"), *(WORD *)(&HexData[dwOffset]));
		goto eek;

	case typeDWORD:		
		_stprintf(buf, TEXT("%08x"), *(DWORD *)(&HexData[dwOffset]));
		goto eek;

	case typeQWORD:
		_stprintf(buf, TEXT("%d"), *(DWORD *)(&HexData[dwOffset]));
		goto eek;

		eek:
		{
			//sprintf(buf, "%d", rand());
			gvitem.pszText = buf;
			gvitem.iSubItem = 1;
			gvitem.state = 0;
			GridView_SetItem(hwndGV, hRoot, &gvitem);
			gvitem.iSubItem = 0;
		}

		dwLength = typeLenLookup[type->ty];
		break;

	case typeENUM:
		eptr = type->eptr;
		
		{
			WORD *wptr = (WORD *)(&HexData[dwOffset]);

			_stprintf(buf, TEXT("%04x"), *wptr);

			for(i = 0; i < eptr->fieldList.size(); i++)
			{
				if(Evaluate(eptr->fieldList[i]->expr) == *wptr)
				{
					_stprintf(buf, TEXT("%hs"), eptr->fieldList[i]->name);
					break;
				}
			}
		}

		gvitem.pszText = buf;
		gvitem.iSubItem = 1;
		gvitem.state = GVIS_DROPDOWN;
		GridView_SetItem(hwndGV, hRoot, &gvitem);
		gvitem.iSubItem = 0;					

		dwLength = 2;
		break;
		
	case typeIDENTIFIER:

		if(fShowFullType)
		{
			render(buf, type);
			gvitem.pszText = buf;
		}
		else
		{
			_stprintf(buf, TEXT("%hs"), type->sym->name);
			gvitem.pszText = buf;//type->sym->name;
		}

		gvitem.state   = GVIS_IMAGE;

		if(type->link && type->link->ty != typeARRAY)
			gvitem.state |= GVIS_EXPANDED;
		
		bool isstruct;
		{
			isstruct= false;
			for(Type *ptr = type; ptr; ptr = ptr->link)
				if(ptr->ty == typeSTRUCT || ptr->ty == typeUNION)
					isstruct = true;
		
			gvitem.iImage  = isstruct ? 1 : 0;//rand() % 2;
		}

		gvitem.param = (ULONG_PTR)typeDecl;
		hRoot = GridView_InsertChild(hwndGV, hRoot, &gvitem);

		if(type->link && type->link->ty == typeARRAY)
		{
			FormatDataItem(hwndGV, hRoot, type, dwOffset);
		}

		if(isstruct)
		{
			gvitem.iSubItem = 1;
			gvitem.state	= GVIS_READONLY;
			gvitem.pszText	= TEXT("{...}");
			gvitem.param	= 0;
			GridView_SetItem(hwndGV, hRoot, &gvitem);

		}
		//if(type->link && (type->link->ty == typeSTRUCT || type->link->ty == typeUNION))

		dwLength = smeg2(hwndGV, hRoot, type->link, dwOffset, typeDecl);
		break;

	case typeTYPEDEF:		
	case typePOINTER:
		dwLength = smeg2(hwndGV, hRoot, type->link, dwOffset, typeDecl);
		break;

	case typeARRAY:

		for(i = 0; i < Evaluate(type->elements); i++)
		{
			HGRIDITEM hItem;
			TCHAR buf[32];

			_stprintf(buf, TEXT("[%d]"), i);
			gvitem.pszText	= buf;
			gvitem.state	= GVIS_READONLY;//GVIS_EXPANDED;//|GVIS_IMAGE;
			gvitem.iImage	= 0;//rand() % 2;
			gvitem.param	= (ULONG_PTR)typeDecl;
			gvitem.iSubItem = 0;
			hItem = GridView_InsertChild(hwndGV, hRoot, &gvitem);

			dwLength += smeg2(hwndGV, hItem, type->link, dwOffset + dwLength, 0);
		}

		//sbuf->ptr += sprintf(sbuf->ptr, "[%d]", Evaluate(type->elements));
		break;
		
	default:

		break;
		}
		
	if(hRoot)
	{
		_stprintf(buf, TEXT("%08x"), dwOffset);
		gvitem.pszText = buf;
		gvitem.iSubItem = 2;
		gvitem.state = 0;
		GridView_SetItem(hwndGV, hRoot, &gvitem);
		gvitem.iSubItem = 0;

		//_stprintf(buf, TEXT("   %08x"), dwOffset);
		if(typeDecl)
		{
			FILE_REF *ref;
			
			if(typeDecl->fileRef.lineNo == typeDecl->postRef.lineNo - 1)
				ref = &typeDecl->postRef;
			else
				ref = &typeDecl->fileRef;

			FormatWhitespace(ref, buf);
			gvitem.pszText = buf;
			gvitem.iSubItem = 3;
			gvitem.state = 0;
			GridView_SetItem(hwndGV, hRoot, &gvitem);
			gvitem.iSubItem = 0;
		}

	}

	return dwLength;
}


DWORD head(HWND hwndGV, TypeDecl *typeDecl, HGRIDITEM hRoot, DWORD dwOffset)
{
	Type *type;
	DWORD dwLength = 0;

	// display the basetype
	//typeDecl->baseType = InvertType(typeDecl->baseType);
	//smeg(hwndGV, hRoot, typeDecl->baseType);
	//typeDecl->baseType = InvertType(typeDecl->baseType);

	// display each variable-decl
	for(int i = 0; i < typeDecl->declList.size(); i++)
	{
		//string sbuf;
		//sbuf.ptr = sbuf.buf;
		//sbuf.brackets = false;

		/*typeDecl->declList[i] = InvertType(typeDecl->declList[i]);
		type = typeDecl->declList[i];

		AppendType(type, typeDecl->baseType);

		*/
		type = typeDecl->declList[i];
		//type = MakeFullType(typeDecl->baseType, typeDecl->declList[i]);

		if(typeDecl->tagList)
		{
			Tag *tag = typeDecl->tagList;

			if(tag->tok == TOK_OFFSET)
			{
				dwOffset = Evaluate(tag->expr);
			}
		}

		dwLength += smeg2(hwndGV, hRoot, type, dwOffset + dwLength, typeDecl);

		/*Type *ptr = type;
		while(ptr->link != typeDecl->baseType)
		{
			ptr = ptr->link;
		}
		
		// restore
		ptr->link = 0;
		typeDecl->declList[i] = InvertType(typeDecl->declList[i]);*/
		
		
		//type = InvertType(type);
		//smeg(&sbuf, type);
		//type = InvertType(type);
		//InvertType(typeDecl->declList[i]);

		/*GVITEM gvitem = { 0 };
		HGRIDITEM hItem;
		gvitem.pszText = sbuf.buf;//type->var->varName;
		gvitem.state   = GVIS_IMAGE;//GVIS_EXPANDED;
		gvitem.iImage  = rand() % 2;
		hItem = GridView_InsertChild(hwndGV, hRoot, &gvitem);

		if(typeDecl->baseType->ty == typeSTRUCT)// || typeDecl->baseType->ty == typeUNION)
		{
			Structure *sptr = typeDecl->baseType->sptr;

			for(int i = 0; i < sptr->typeDeclList.size(); i++)
			{
				TypeDecl *typeDecl = sptr->typeDeclList[i];
				head(hwndGV, typeDecl, hItem);
			}
		}*/
	}

	return dwLength;
}

void GetModuleDirectory(HMODULE hModule, TCHAR *szPath, DWORD nSize)
{
	TCHAR *ptr;
	GetModuleFileName(hModule, szPath, nSize);

	if((ptr = _tcsrchr(szPath, '\\')) != 0)
		*ptr = '\0';
}

extern "C"
void FillTypeView(HWND hwndGV)
{
	//Parse("c:\\src\\hex\\typelib\\test.txt");

	TCHAR szPath[MAX_PATH];
	char path[MAX_PATH];
	GetModuleDirectory(0, szPath, MAX_PATH);
	lstrcat(szPath, TEXT("\\pe.txt"));

	sprintf(path, "%ls", szPath);
	Parser::Initialize();
	Parse(path);
//	Parse("zip.txt");

	int i;

	GridView_SetStyle(hwndGV, -1, GVS_FULLROWSELECT|GVS_GRIDLINES|GVS_TREELINES
		//|GVS_SHOWFOCUS
		);//,GVS_FULLROWSELECT|GVS_GRIDLINES);

	for(i = 0; i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];

		//DisplayTags(fp, typeDecl->tagList);

		head(hwndGV, typeDecl, 0, 0);

		//GVITEM gvitem = { 0 };
		//gvitem.pszText = "smeg";//typeDecl->baseType->
		//GridView_InsertTree(hwndGV, 0, &gvitem);
		//DisplayType(
	}

}

BOOL g_fShowGrid = FALSE;
BOOL g_fShowTreeLines = FALSE;
BOOL g_fShowFullSelection = FALSE;
BOOL g_fShowFocus = FALSE;

void FillTypeView(HWND hwndGV);

extern "C" HWND PrepGridView(HWND hwndParent)
{
	GVCOLUMN gvcol = { 0 };
	HFONT hFont, hBold;
	HIMAGELIST hImgList ;

	HWND hwndGridView;

	hwndGridView = CreateGridView(hwndParent);

	hBold = CreateFont(-14,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0, TEXT("Segoe UI"));
	//SendMessage(g_hWndGridView, WM_SETFONT, (WPARAM)hFont, 0);

	//hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0,"Segoe UI");
	hFont = CreateFont(-14,0,0,0,0,0,0,0,0,0,0,0,0, TEXT("Verdana"));
	//SendMessage(GetWindow(g_hWndGridView, GW_CHILD), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(hwndGridView, WM_SETFONT, (WPARAM)hFont, 0);

//	gvcol.hFont = hBold;
	gvcol.xWidth = 300;
	gvcol.pszText = TEXT("Name");
	gvcol.uState = GVCS_BLENDIMAGE;
	GridView_InsertColumn(hwndGridView, 0, &gvcol);


	gvcol.hFont = hFont;
	gvcol.xWidth  = 200;
	gvcol.pszText = TEXT("Value");
	gvcol.uState  = 0;//GVCS_ALIGN_RIGHT;
	GridView_InsertColumn(hwndGridView, 1, &gvcol);

	gvcol.xWidth = 100;
	gvcol.pszText = TEXT("Offset");
	gvcol.uState  = GVCS_READONLY;
	GridView_InsertColumn(hwndGridView, 2, &gvcol);

	//GridView_InsertColumn(hwndGridView, 1, &gvcol);
	gvcol.xWidth = 300;
	gvcol.pszText = TEXT("Comment");
	gvcol.uState  = 0;
	GridView_InsertColumn(hwndGridView, 3, &gvcol);


	hImgList = ImageList_LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP9), 16, 0, RGB(255,0,255));
	//hImgList = ImageList_LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP2), 16, 0, RGB(255,0,255), IMAGE_BITMAP, LR_CREATEDIBSECTION |LR_LOADTRANSPARENT);
	GridView_SetImageList(hwndGridView, hImgList);

	return hwndGridView;
}


HGRIDITEM SetItem(HWND hwndGV, HGRIDITEM hItem, UINT gviWhere, TCHAR *text)
{
	GVITEM gvitem = { 0 };

	gvitem.pszText = text;
	gvitem.iImage  = rand() % 3;
	gvitem.state   = GVIS_EXPANDED;//rand() % 2 ? GVIS_EXPANDED : 0;
	//gvitem.iIndent = gviWhere;

	//gvitem.iIndent = indent;
	//gvitem.iItem   = idx;
	switch(gviWhere)
	{
	case GVI_CHILD: return GridView_InsertChild(hwndGV, hItem, &gvitem);
	case GVI_BEFORE: return GridView_InsertBefore(hwndGV, hItem, &gvitem);
	case GVI_AFTER: return GridView_InsertAfter(hwndGV, hItem, &gvitem);
	case GVI_ROOT: return GridView_InsertChild(hwndGV, hItem, &gvitem);
	default:		return 0;
	}

}

extern "C" void FillGridView(HWND hwndGV)
{
	//GVITEM gvitem = { 0 };

	//HGRIDITEM hRoot, hItem;

	FillTypeView(hwndGV);
	//main(hwndGV);

	// insert root item
	/*hRoot = SetItem(hwndGV, NULL, GVI_ROOT, "aardvark");
	
	hItem = SetItem(hwndGV, hRoot, GVI_CHILD, "badger");
	hItem = SetItem(hwndGV, hItem, GVI_CHILD,  "camel");
	hItem = SetItem(hwndGV, hItem, GVI_AFTER,  "dog");
	hItem = SetItem(hwndGV, hItem, GVI_CHILD, "dinosaur");
	hItem = SetItem(hwndGV, hItem, GVI_CHILD, "elephant");
	hItem = SetItem(hwndGV, hRoot, GVI_CHILD, "fish");*/
}


void InsertType(HWND hwndGridView, HGRIDITEM hRoot, TypeDecl *typeDecl)
{
	GVITEM gvitem = { 0 };
	hRoot = GridView_InsertChild(hwndGridView, hRoot, &gvitem);
	head(hwndGridView, typeDecl, hRoot, 0);
}

extern "C"
void AddType(HWND hwndGridView, HGRIDITEM hItem, TypeDecl *typeDecl)
{
	//TypeDecl *typeDecl = globalTypeDeclList[7];


	head(hwndGridView, typeDecl, hItem, 0);
}

