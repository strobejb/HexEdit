//
//  dlgfont.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//
//  ----------------------------------------------------------------
//	DlgFont library - provides routines to create regular win32 dialogs
//	with a different font/size than what is specified in the dialog's resources
//
//		* DialogBoxWithFont
//		* CreateDialogWithFont
//		* DialogExFromTemplate
//
//
#include <windows.h>
#include "dlgfont.h"

#pragma pack(push, 1)

typedef struct 
{  
  WORD      dlgVer; 
  WORD      signature; 
  DWORD     helpID; 
  DWORD     exStyle; 
  DWORD     style; 
  WORD      cDlgItems; 
  short     x; 
  short     y; 
  short     cx; 
  short     cy; 

} DLGTEMPLATEEX;

typedef struct 
{
  WORD     pointsize; 
  WORD     weight; 
  BYTE     italic;
  BYTE     charset; 
  WCHAR    typeface[1];  

} DLGTEMPLATEEX2; 

typedef struct 
{ 
  DWORD  helpID; 
  DWORD  exStyle; 
  DWORD  style; 
  short  x; 
  short  y; 
  short  cx; 
  short  cy; 
  WORD   id; 
  WORD	 reserved;		// Q141201 - there is an extra WORD here

} DLGITEMTEMPLATEEX;

#pragma pack(pop)

static PVOID RoundUp(PVOID ptr, DWORD align)
{
	DWORD_PTR p = (DWORD_PTR)ptr;
	
	if(p % align != 0)
	{
		p += align;
		p -= p % align;
		return (PVOID)(p);
	}
	else
		return ptr;
}

//
//	Return pointer to the specified resource 
//
static HGLOBAL GetResourcePtr(HINSTANCE hInstance, LPCTSTR lpTemplateName, LPCTSTR lpType)
{
	// Load the specified resource
	HRSRC   hResInfo   = FindResource(hInstance, lpTemplateName, lpType);
	HGLOBAL hResource  = LoadResource(hInstance, hResInfo);
	
	return LockResource(hResource);
}

//
//	Copy the specified *dialog-item* (in whatever format it may be), 
//	convert to an 'extended' dialog-item if necessary.
//
//	Returns pointer to the next item
//
static PVOID CopyDialogItem(DLGITEMTEMPLATEEX *destDlg, PVOID source, BOOL fExtended)
{
	DWORD dwSize = 0;
	WCHAR *sptr, *sptr2;

	// if source is an extended dialog-item template, just to a direct copy
	if(fExtended)
	{
		DLGITEMTEMPLATEEX *srcDlg = source;

		if(destDlg)
		{
			*destDlg = *srcDlg;
		}

		sptr  = (WCHAR *)(srcDlg + 1);
		sptr2 = sptr;
	}
	// otherwise need to convert from a DIALOG to a DIALOG-EX
	else
	{
		DLGITEMTEMPLATE *srcDlg = source;

		if(destDlg)
		{
			destDlg->helpID		= 0;
			destDlg->exStyle	= srcDlg->dwExtendedStyle;
			destDlg->style		= srcDlg->style;
			destDlg->x			= srcDlg->x;
			destDlg->y			= srcDlg->y;
			destDlg->cx			= srcDlg->cx;
			destDlg->cy			= srcDlg->cy;
			destDlg->id			= srcDlg->id;
		}

		sptr  = (WCHAR *)(srcDlg + 1);
		sptr2 = sptr;
	}

	// copy the class, title and creation data
	if(*sptr == 0xffff) sptr += 2;			// class-ordinal
	else				while(*sptr++);		// class-string
	if(*sptr == 0xffff) sptr += 2;			// title-ordinal
	else				while(*sptr++);		// title-string
	if(*sptr == 0x0000) sptr += 1;			// creation data
	else				//sptr += RoundUp(sptr, 
						sptr = (WCHAR *)((DWORD_PTR)sptr + (DWORD_PTR)(*sptr));// creation data

	sptr = RoundUp(sptr, sizeof(DWORD));

	if(destDlg)
	{
		memcpy(destDlg+1, sptr2, (DWORD_PTR)sptr - (DWORD_PTR)sptr2);
	}

	//while((DWORD_PTR)sptr & 3)
	//	sptr++;

	return sptr;
}

//
//	Copy the specified dialog template (in whatever format it may be),
//	convert to an extended-dialog (DLGTEMPLATEX format), with a new font/size
//
//	Returns pointer to the new resource in GLOBAL memory. Pointer must
//	be freed using GlobalFree when no longer required
//
static PVOID CopyDialog(DLGTEMPLATEEX *destDlg, PVOID source, BOOL fExtended, WCHAR *szNewFontName, WORD nPointSize)
{
	DWORD			dwSize = 0;
	WCHAR			*sptr, *sptr2;
	BOOL			fIncludeFont;

	// if source is an extended dialog template, do a direct copy
	if(fExtended)
	{
		DLGTEMPLATEEX *srcDlg = source;
		
		if(destDlg)
			*destDlg = *srcDlg;

		sptr  = (WCHAR *)(srcDlg + 1);
		sptr2 = sptr;

		// does the source specify a font?
		fIncludeFont = (srcDlg->style & (DS_SETFONT | DS_SHELLFONT));
	}
	// otherwise we need to manually create the extended template
	else
	{
		DLGTEMPLATE *srcDlg = source;

		if(destDlg)
		{
			destDlg->dlgVer		= 1;
			destDlg->signature	= 0xFFFF;
			destDlg->helpID		= 0;
			destDlg->exStyle	= srcDlg->dwExtendedStyle;
			destDlg->style		= srcDlg->style;
			destDlg->cDlgItems	= srcDlg->cdit;
			destDlg->x			= srcDlg->x;
			destDlg->y			= srcDlg->y;
			destDlg->cx			= srcDlg->cx;
			destDlg->cy			= srcDlg->cy;
		}
			
		sptr  = (WCHAR *)(srcDlg + 1);
		sptr2 = sptr;

		// does the source specify a font?
		fIncludeFont = (srcDlg->style & (DS_SETFONT | DS_SHELLFONT));
	}

	// always specify the font for the new template
	if(destDlg)
		destDlg->style |= DS_SETFONT | DS_SHELLFONT;

	// menu, class and title
	if(*sptr == 0xffff) sptr += 2;			// menu-ordinal
	else				while(*sptr++);		// menu-string
	if(*sptr == 0xffff) sptr += 2;			// class-ordinal
	else				while(*sptr++);		// class-string
	while(*sptr++);							// null-terminated title

	// copy everything we just parsed above
	if(destDlg)
	{
		destDlg++;
		memcpy(destDlg, sptr2, (DWORD_PTR)sptr - (DWORD_PTR)sptr2);
		destDlg = (DLGTEMPLATEEX *)((DWORD_PTR)destDlg + (DWORD_PTR)sptr - (DWORD_PTR)sptr2); 
	}

	if(fIncludeFont)
	{
		DLGTEMPLATEEX2 *destDlg2 = (DLGTEMPLATEEX2 *)destDlg;

		if(fExtended)
		{
			if(destDlg)
			{
				sptr++;
				destDlg2->pointsize = nPointSize;
				destDlg2->weight	= *sptr++;
				destDlg2->italic	= *sptr & 0xff;
				destDlg2->charset	= *sptr >> 8;
				sptr++;
			}
			else
			{
				sptr += 3;
			}
		}
		else 
		{
			if(destDlg)
			{
				destDlg2->pointsize = nPointSize;
				destDlg2->weight	= FW_NORMAL;
				destDlg2->italic	= FALSE;
				destDlg2->charset	= DEFAULT_CHARSET;
				sptr ++;
			}
			else
			{
				sptr ++;
			}
		}

		// ignore source fontname
		while(*sptr++);

		// copy new fontname
		if(destDlg)
			lstrcpyW(destDlg2->typeface, szNewFontName);		
	}
	// if there's no font in the source template then formulate our own
	else if(destDlg)
	{
		DLGTEMPLATEEX2 *destDlg2 = (DLGTEMPLATEEX2 *)destDlg;
			
		destDlg2->pointsize = nPointSize;
		destDlg2->weight	= FW_NORMAL;
		destDlg2->italic	= FALSE;
		destDlg2->charset	= DEFAULT_CHARSET;

		// copy new fontname
		lstrcpyW(destDlg2->typeface, szNewFontName);
	}

	//while((DWORD_PTR)sptr & 3)
	//	sptr = (WCHAR *)(DWORD)sptr;
	sptr = RoundUp(sptr, sizeof(DWORD));
	
	return sptr;
}

//
//	DialogExFromTemplate
//
//	Convert the specified dialog-template (DIALOG or DIALOGEX format) 
//	to an extended dialog-template (DIALOGEX), changing the dialog-font in the process
//
HGLOBAL DialogExFromTemplate(HINSTANCE hInst, HANDLE hTemplate, LPCTSTR szFontName, WORD nPointSize)
{
	DLGTEMPLATEEX *		srcDlg;
	DLGTEMPLATEEX *		destDlg;
	PVOID				srcItem;
	PVOID				destItem;
	BOOL				fExtended;
	DWORD				i;
	DWORD				count;
	WCHAR				wszFontName[200];

#ifdef UNICODE
	lstrcpy(wszFontName, szFontName);
#else
	MultiByteToWideChar(CP_ACP, 0, szFontName, -1, wszFontName, sizeof(wszFontName) / sizeof(wszFontName[0]));
#endif

	if((srcDlg = GlobalLock(hTemplate)) == 0)
		return NULL;

	// is this an extended dialog-template?
	if(srcDlg->signature == 0xFFFF)
	{
		fExtended = TRUE;
		count	  = ((DLGTEMPLATEEX *)srcDlg)->cDlgItems;
	}
	else
	{
		fExtended = FALSE;
		count	  = ((DLGTEMPLATE *)srcDlg)->cdit;
	}

	// work out how much memory is needed to hold the new template
	srcItem = CopyDialog(NULL, srcDlg, fExtended, wszFontName, nPointSize);

	for(i = 0; i < count; i++)
		srcItem = CopyDialogItem(NULL, srcItem, fExtended);

	//
	// allocate room for the resulting dialogex template
	//
	if((destDlg = GlobalAlloc(GPTR, (DWORD_PTR)srcItem - (DWORD_PTR)srcDlg + (lstrlenW(wszFontName) + 1) * sizeof(WCHAR))) == 0)
	{
		GlobalUnlock(hTemplate);
		return NULL;
	}

	// 
	srcItem  = CopyDialog(destDlg, srcDlg,  fExtended, wszFontName, nPointSize);
	destItem = CopyDialog(NULL,    destDlg, TRUE, wszFontName, nPointSize);

	for(i = 0; i < count; i++)
	{
		srcItem  = CopyDialogItem(destItem, srcItem,  fExtended);
		destItem = CopyDialogItem(NULL,     destItem, TRUE);
	}

	GlobalUnlock(hTemplate);
	return destDlg;
}

//
//	LoadDialogTemplate
//
//	Loads a dialog-resource from the specified module,
//	modifies the dialog-template to use a new font
//
HGLOBAL LoadDialogTemplate(HINSTANCE hInst, LPCTSTR lpTemplateName, LPCTSTR szFontName, WORD nPointSize)
{
	HGLOBAL hResource;

	if((hResource = GetResourcePtr(hInst, lpTemplateName, RT_DIALOG)) == 0)
		return NULL;

	return DialogExFromTemplate(hInst, hResource, szFontName, nPointSize);
}

//
//	DialogBoxWithFont
//
//	Replacement of the DialogBox API, allows the dialog-font to be replaced at runtime
//	with a new font/size
//
BOOL DialogBoxWithFont(HINSTANCE hInst, LPCTSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam, LPCTSTR szFontName, WORD nPointSize)
{
	HGLOBAL hTemplate;
	BOOL	retCode;

	if((hTemplate = LoadDialogTemplate(hInst, lpTemplateName, szFontName, nPointSize)) == 0)
		return FALSE;

	retCode = (BOOL)DialogBoxIndirectParam(hInst, hTemplate, hwndParent, lpDialogFunc, dwInitParam);

	GlobalFree(hTemplate);
	return retCode;
}

//
//	CreateDialogWithFont
//
//	Replacement of the CreateDialog API, allows the dialog-font to be replaced at runtime
//	with a new font/size
//
HWND CreateDialogWithFont(HINSTANCE hInst, LPCTSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam, LPCTSTR szFontName, WORD nPointSize)
{
	HGLOBAL hTemplate;
	HWND	hwndDialog;

	if((hTemplate = LoadDialogTemplate(hInst, lpTemplateName, szFontName, nPointSize)) == 0)
		return FALSE;

	hwndDialog = CreateDialogIndirectParam(hInst, hTemplate, hwndParent, lpDialogFunc, dwInitParam);

	GlobalFree(hTemplate);
	return hwndDialog;
}
