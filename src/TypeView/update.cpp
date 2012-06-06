//
//  update.cpp
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

size_w FmtData(HWND hwndGV, HGRIDITEM hRoot, Type *type, size_w dwOffset, TypeDecl *typeDecl);

extern "C"
void UpdateTypeViewWorker(HWND hwndGV, HGRIDITEM hParent)
{
	HGRIDITEM hItem;

	if((hItem = GridView_GetFirstChild(hwndGV, hParent)) != 0)
	{
		do
		{
			GVITEM gvi = { GVIF_TEXT|GVIF_PARAM };
			TCHAR buf[100];
			gvi.pszText = buf;
			gvi.cchTextMax = 100;
			
			if(GridView_GetItem(hwndGV, hItem, &gvi))
			{
				Type *type = (Type *)gvi.param;
				TRACE(TEXT("%s\n"), buf);

				gvi.iSubItem = COLIDX_OFFSET;
				GridView_GetItem(hwndGV, hItem, &gvi);

				type = type->link;

				if(type && (type->ty < typeMODIFIERSTART || type->ty == typeTYPEDEF))
				{
					FmtData(hwndGV, hItem, type, (size_w)gvi.param, 0);
				}
			}
			
			UpdateTypeViewWorker(hwndGV, hItem);
		} 
		while(hItem = GridView_GetNextSibling(hwndGV, hItem));
	}

}

