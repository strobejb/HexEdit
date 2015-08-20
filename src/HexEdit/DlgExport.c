//
//  DlgExport.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define STRICT
#define _CRT_SECURE_NO_WARNINGS

#define _WIN32_WINNT 0x501

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <stdio.h>
#include <commctrl.h>
#include <commdlg.h>
#include "HexEdit.h"
#include "HexUtils.h"
#include "resource.h"
#include "base64.h"
#include "endian.h"

#include "..\HexView\HexView.h"

extern TCHAR g_szFileName[];
extern TCHAR g_szFileTitle[];
static HHOOK g_hHook;

size_t motorola_frame(char *srec, int type, size_t count, unsigned long addr, BYTE *data);
size_t intel_frame(char *hrec, int type, size_t count, unsigned long addr, BYTE *data);

#define MAX_SRECORD_LEN 16

// default to 'plain text'
IMPEXP_OPTIONS g_ExportOptions = { FORMAT_HEXDUMP, SEARCHTYPE_BYTE };

INT_PTR CALLBACK ExportHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndRelative;
	OFNOTIFY *ofn;

	static IMPEXP_OPTIONS *eopt;

	switch(uiMsg)
	{
	case WM_INITDIALOG:
		eopt = (IMPEXP_OPTIONS *)((OPENFILENAME *)lParam)->lCustData;
		return TRUE;

	case WM_SHOWWINDOW:

		hwndRelative = GetDlgItem(GetParent(hdlg), 0x470);

		AlignWindow(GetDlgItem(hdlg, IDC_DATATYPE),			hwndRelative, ALIGN_LEFT);
		AlignWindow(GetDlgItem(hdlg, IDC_ZEROBASEDADDRESS), hwndRelative, ALIGN_LEFT);
		AlignWindow(GetDlgItem(hdlg, IDC_ENDIAN),			hwndRelative, ALIGN_LEFT);
		AlignWindow(GetDlgItem(hdlg, IDC_APPENDTOFILE),		hwndRelative, ALIGN_LEFT);

		hwndRelative = GetDlgItem(GetParent(hdlg), 0x441);
		AlignWindow(GetDlgItem(hdlg, IDC_DATALABEL),		hwndRelative, ALIGN_LEFT);

		AddSearchTypes(GetDlgItem(hdlg, IDC_DATATYPE), SEARCHTYPE_BYTE, SEARCHTYPE_DOUBLE, eopt->basetype - SEARCHTYPE_BYTE);

		CheckDlgButton(hdlg, IDC_ENDIAN, eopt->fBigEndian);
		CheckDlgButton(hdlg, IDC_APPENDTOFILE, eopt->fAppend);

		EnableDlgItem(hdlg, IDC_DATATYPE, eopt->format == FORMAT_CPP || eopt->format == FORMAT_ASM);
		EnableDlgItem(hdlg, IDC_ENDIAN, eopt->format == FORMAT_CPP || eopt->format == FORMAT_ASM);
		
		return -1;

	/*case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDC_ENDIAN: case IDC_ZEROBASEDADDRESS: case IDC_APPENDTOFILE:

			fLastEndian   = IsDlgButtonChecked(hdlg, IDC_ENDIAN);
			fLastZeroAddr = IsDlgButtonChecked(hdlg, IDC_ZEROBASEDADDRESS);
			fLastAppend   = IsDlgButtonChecked(hdlg, IDC_APPENDTOFILE);

			break;
		}
	
		break;*/

	case WM_NOTIFY:
		ofn = (OFNOTIFY *)lParam;

		if(ofn->hdr.code == CDN_TYPECHANGE)
		{
			BOOL fEnable = FALSE;
			int  idx = ofn->lpOFN->nFilterIndex - 1;

			if(idx == FORMAT_CPP || idx == FORMAT_ASM)
				fEnable = TRUE;

			EnableDlgItem(hdlg, IDC_DATATYPE, fEnable);
			EnableDlgItem(hdlg, IDC_ENDIAN, fEnable);

			break;
		}
		else if(ofn->hdr.code == CDN_FILEOK)
		{
			g_ExportOptions.format		= ofn->lpOFN->nFilterIndex - 1;
			g_ExportOptions.fBigEndian	= IsDlgButtonChecked(hdlg, IDC_ENDIAN);
			g_ExportOptions.fAppend		= IsDlgButtonChecked(hdlg, IDC_APPENDTOFILE);
			g_ExportOptions.basetype	= ComboBox_GetSelData(GetDlgItem(hdlg, IDC_DATATYPE));
		}

		return 0;
	}

	return 0;
}

BOOL ExportRaw(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	while(length && !ferror(fp))
	{
		BYTE buf[256];
		size_t len = (size_t)min(256, length);

		HexView_GetDataAdv(hwndHexView, buf, len);
		fwrite(buf, len, 1, fp);

		offset += len;
		length -= len;
	}

	return !ferror(fp);
}

void FreeHexFmtParams(HEXFMT_PARAMS *hfmt)
{
	free(hfmt->szText);
	free(hfmt->attrList);
}

BOOL AllocHexFmtParams(HEXFMT_PARAMS *hfmt, HWND hwndHexView)
{
	size_t len = HexView_GetLineChars(hwndHexView);

	hfmt->bufferSize	= len;
	hfmt->szText		= (TCHAR *)malloc(len * sizeof(TCHAR));
	hfmt->attrList		= (ATTR *)malloc(len * sizeof(ATTR));

	if(hfmt->szText == 0 || hfmt->attrList == 0)
	{
		FreeHexFmtParams(hfmt);
		return FALSE;
	}

	return TRUE;
}

BOOL ExportText(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	HEXFMT_PARAMS	hfmt;
	
	if(!AllocHexFmtParams(&hfmt, hwndHexView))
		return FALSE;

	while(length > 0 && !ferror(fp))
	{
		hfmt.offset		= offset;
		hfmt.length		= (size_t)min(length, eopt->linelen);

		if(HexView_FormatData(hwndHexView, &hfmt))
		{
			// skip any leading whitespace
			TCHAR *ptr = hfmt.szText;

			while(*ptr == ' ')
				ptr++;

			_ftprintf(fp, TEXT("%s\n"), ptr);
		}
		else
		{
			break;
		}

		length -= hfmt.length;
		offset += hfmt.length;
	}

	FreeHexFmtParams(&hfmt);
	
	return !ferror(fp);
}

//
//	Export the data as raw hex digits, i.e:
//
//		0323fea53c34bd
//
BOOL ExportRawHex(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	while(length && !ferror(fp))
	{
		BYTE buf[256];
		size_t i;
		size_w len = min(eopt->linelen, length);

		HexView_GetDataAdv(hwndHexView, buf, len);
		
		for(i = 0; i < len; i++)
			fprintf(fp, "%02X", buf[i]);

		//fprintf(fp, "\n");

		offset += len;
		length -= len;
	}

	return !ferror(fp);
}

BOOL ExportHtml(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	HEXFMT_PARAMS	hfmt;
	
	if(!AllocHexFmtParams(&hfmt, hwndHexView))
		return FALSE;

	fprintf(fp, "<html>\n<head>\n");
	fprintf(fp, "<title>%ls</title>\n", g_szFileTitle);
	fprintf(fp, "<style>\n");
	fprintf(fp, "  body {\n    font-family: Calibri,Verdana,sans-serif;\n  }\n");
	fprintf(fp, "  pre  {\n    font-family: Monospaced;\n  }\n");
	fprintf(fp, "</style>\n");
	fprintf(fp, "</head>\n");
	fprintf(fp, "<body>\n<h1>%ls</h1>\n<hr>\n", g_szFileTitle);
	fprintf(fp, "<pre>\n");
	
	while(length > 0 && !ferror(fp))
	{
		ATTR *attrList	= hfmt.attrList;

		size_t j, lastj, len;

		hfmt.offset		= offset;
		hfmt.length		= (size_t)min(length, eopt->linelen);
		
		// ask the HexView for a formatted line of text with colour information
		if(!HexView_FormatData(hwndHexView, &hfmt))
			break;

		// loop over the attribute-buffer/text buffer
		for(j = 0, lastj = 0, len = lstrlen(hfmt.szText); j <= len; j++)
		{
			// detect changes in colour
			if( attrList[j].colFG != attrList[lastj].colFG ||
				attrList[j].colBG != attrList[lastj].colBG ||
				j == eopt->linelen)
			{
				BOOL addCodetag = FALSE;
				
				// add a background colour CSS style
				if(attrList[lastj].colBG != 0xFFFFFF)
				{
					if(addCodetag == FALSE)
					{
						fprintf(fp, "<code style=\"");
						addCodetag = TRUE;
					}
					
					fprintf(fp, "background-color:#%02X%02X%02X;",
						GetRValue(attrList[lastj].colBG), GetGValue(attrList[lastj].colBG), GetBValue(attrList[lastj].colBG));
				}
				
				// add a foreground colour CSS style
				if(attrList[lastj].colFG != 0x000000)
				{
					if(addCodetag == FALSE)
					{
						fprintf(fp, "<code style=\"");
						addCodetag = TRUE;
					}
					
					fprintf(fp, "color:#%02X%02X%02X;", 
						GetRValue(attrList[lastj].colFG), GetGValue(attrList[lastj].colFG), GetBValue(attrList[lastj].colFG));
				}
				
				if(addCodetag == TRUE)
					fprintf(fp, "\">");
				
				// display the text fragment
				fprintf(fp, "%.*ls", (int)(j - lastj), hfmt.szText + lastj);
				
				if(addCodetag == TRUE)
					fprintf(fp, "</code>");
				
				lastj = j;
			}
		}
		
		fprintf(fp, "\n");
		
		length -= hfmt.length;
		offset += hfmt.length;
	}
	
	fprintf(fp, "\n</pre>\n");
	fprintf(fp, "<hr>\n<font size=\"-3\">");
	fprintf(fp, "Generated by <a href=\"http://www.catch22.net/\">Catch22 HexEdit</a>");
	fprintf(fp, "</font>\n");
	fprintf(fp, "</body>\n</html>\n");

	FreeHexFmtParams(&hfmt);

	return !ferror(fp);
}


BOOL ExportASM(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	fprintf(fp, "; Generated by HexEdit\n");
	fprintf(fp, "; %ls\n", g_szFileName);
	
	while(length && !ferror(fp))
	{
		BYTE buf[256];
		WORD *wptr = (WORD *)buf;
		DWORD *dptr = (DWORD *)buf;
		QWORD *qptr = (QWORD *)buf;
		size_t i;
		size_w len = min(eopt->linelen, length);

		HexView_GetDataAdv(hwndHexView, buf, len);

		switch(eopt->basetype)
		{
		case SEARCHTYPE_BYTE:

			fprintf(fp, "db ");
		
			for(i = 0; i < len; i++)
				fprintf(fp, "0%02Xh ", buf[i]);

			break;

		case SEARCHTYPE_WORD:

			fprintf(fp, "dw ");
		
			for(i = 0; i < len / sizeof(WORD); i++)
				fprintf(fp, "0%08Xh ", ENDIAN_TO_NATIVE16(eopt->fBigEndian, wptr[i]));

			break;

		case SEARCHTYPE_DWORD:
			fprintf(fp, "dd ");
		
			for(i = 0; i < len / sizeof(DWORD); i++)
				fprintf(fp, "0%08Xh ", ENDIAN_TO_NATIVE32(eopt->fBigEndian, dptr[i]));

			break;

		case SEARCHTYPE_QWORD:
			fprintf(fp, "dq ");
		
			for(i = 0; i < len / sizeof(QWORD); i++)
				fprintf(fp, "0%015I64Xh ", ENDIAN_TO_NATIVE64(eopt->fBigEndian, qptr[i]));

			break;
		}

		fprintf(fp, "\n");
		length -= len;
		offset += len;
	}

	return !ferror(fp);
}


BOOL ExportCPP(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	static const char * tlook[] = 
	{
		0,0,0,0,0,"BYTE", "WORD", "DWORD", "UINT64", "float", "double"
	};

	fprintf(fp, "/* Generated by HexEdit */\n");
	fprintf(fp, "/* %ls */\n", g_szFileName);
	fprintf(fp, "%s hexData[0x%x] = \n{\n", tlook[eopt->basetype], (int)length);

	while(length && !ferror(fp))
	{
		BYTE buf[256];
		UINT16 *ptr16 = (UINT16 *)buf;
		UINT32 *ptr32 = (UINT32 *)buf;
		UINT64 *ptr64 = (UINT64 *)buf;
		float  *fptr  = (float *)buf;
		double *dptr  = (double *)buf;
		
		size_t i;
		size_w len = min(eopt->linelen, length);
		
		HexView_GetDataAdv(hwndHexView, buf, len);

		fprintf(fp, "  ");

		switch(eopt->basetype)
		{
		case SEARCHTYPE_BYTE:

			for(i = 0; i < len; i++)
				fprintf(fp, "0x%02X, ", buf[i]);

			 break;

		case SEARCHTYPE_WORD:

			for(i = 0; i < len / sizeof(UINT16); i++)
				fprintf(fp, "0x%04X, ", ENDIAN_TO_NATIVE16(eopt->fBigEndian, ptr16[i]));

			 break;

		case SEARCHTYPE_DWORD:

			for(i = 0; i < len / sizeof(UINT32); i++)
				fprintf(fp, "0x%08X, ", ENDIAN_TO_NATIVE32(eopt->fBigEndian, ptr32[i]));

			 break;

		case SEARCHTYPE_QWORD:

			for(i = 0; i < len / sizeof(UINT64); i++)
				fprintf(fp, "0x%016I64X, ", ENDIAN_TO_NATIVE64(eopt->fBigEndian, ptr64[i]));

			 break;

		case SEARCHTYPE_FLOAT:

			for(i = 0; i < len / sizeof(float); i++)
			{
				double d = fptr[i];
				fprintf(fp, "%g, ", d);
			}

			 break;
		
		case SEARCHTYPE_DOUBLE:

			for(i = 0; i < len / sizeof(double); i++)
				fprintf(fp, "%g, ", dptr[i]);

			 break;
			 
		}

		fprintf(fp, "\n");
		offset += len;
		length -= len;
	}

	fprintf(fp, "};\n");
	return !ferror(fp);
}

BOOL ExportIntelHex(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	const int frame_size = 20;

	char	ach[300];
	size_t  alen;
	size_t  count = 0;

	if(eopt->fUseAddress == FALSE)
		offset = 0;

	if(offset > 0xffffffff)
		return FALSE;

	if(1)
	{
		// extended linear address record (hex386, type=4)
		// contains the upper 16bits of the address
		WORD highaddr = reverse16((WORD)(offset >> 16) & 0xFFFF);
		alen = intel_frame(ach, 4, sizeof(WORD), 0, (BYTE *)&highaddr);
		fprintf(fp, "%.*s\n", (int)alen, ach);
	}
	else if(0)
	{
		// extended segment address record (hex86, type=2)
		// contains the upper 4bits of the address (=20bit address)
		WORD highaddr = reverse16((WORD)(offset >> 16) & 0x0FFF);
		alen = intel_frame(ach, 2, sizeof(WORD), 0, (BYTE *)&highaddr);
		fprintf(fp, "%.*s\n", (int)alen, ach);
	}
	else
	{
		// 8bit has no header
	}

	while(length > 0 && !ferror(fp))
	{
		BYTE   buf[MAX_SRECORD_LEN];
		size_t len = (size_t)min(length, MAX_SRECORD_LEN);

		if(count > 0xFFFF)
		{
			// write a new address record every 65536 bytes (hex386, type=4)
			// contains the upper 16bits of the address
			WORD highaddr = reverse16((WORD)(offset >> 16) & 0xFFFF);
			alen = intel_frame(ach, 4, sizeof(WORD), 0, (BYTE *)&highaddr);
			
			fprintf(fp, "%.*s\n", (int)alen, ach);
			count = 0;
		}

		HexView_GetDataAdv(hwndHexView, buf, len);

		// intel 'data' record (type=0)
		alen = intel_frame(ach, 0, len, ((size_t)offset) & 0xFFFF, buf);
		fprintf(fp, "%.*s\n", (int)alen, ach);

		length -= len;
		offset += len;
		count  += len;
	}

	// end-of-file record (type=1)
	alen = intel_frame(ach, 1, 0, 0, 0);
	fprintf(fp, "%.*s\n", (int)alen, ach);

	return !ferror(fp);
}

BOOL ExportMotorola(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	const int frame_size = 20;

	char	ach[300];
	size_t  alen;

	if(offset > 0xffffffff)
		return FALSE;

	// frame header (type=0)
	alen = motorola_frame(ach, 0, 3, 0, "HDR");
	fprintf(fp, "%.*s\n", (int)alen, ach);

	while(length > 0 && !ferror(fp))
	{
		BYTE   buf[256];
		size_t len = (size_t)min(length, MAX_SRECORD_LEN);

		HexView_GetDataAdv(hwndHexView, buf, len);

		// 'data' record (type=3)
		alen = motorola_frame(ach, 3, (unsigned)len, (unsigned long)offset, buf);
		fprintf(fp, "%.*s\n", (int)alen, ach);

		length -= len;
		offset += len;
	}

	// termination record
	// 7=4byte, 8=3byte, 9=2byte
	alen = motorola_frame(ach, 7, 0, 0, 0);
	fprintf(fp, "%.*s\n", (int)alen, ach);

	return !ferror(fp);
}

BOOL ExportBase64(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	while(length > 0 && !ferror(fp))
	{
		BYTE   buf[256];
		char   ach[256];
		size_t len = (size_t)min(length, 54);
		size_t alen;

		HexView_GetDataAdv(hwndHexView, buf, len);

		alen = base64_encode(buf, len, ach);
		fprintf(fp, "%.*s\n", (int)alen, ach);

		length -= len;
	}

	return !ferror(fp);
}

BOOL ExportUUEncode(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *eopt)
{
	char name[100];

	WideCharToMultiByte(CP_ACP, 0, g_szFileTitle, -1, name, 100, 0, 0);
	fprintf(fp, "begin 666 %s\n", name);

	while(length > 0 && !ferror(fp))
	{
		BYTE   buf[256];
		char   ach[256];
		size_t len = (size_t)min(length, 45);
		size_t alen;

		HexView_GetDataAdv(hwndHexView, buf, len);

		alen = uu_encode(buf, len, ach);
		fprintf(fp, "%.*s\n", (int)alen, ach);

		length -= len;
	}

	fprintf(fp, "end\n");
	return !ferror(fp);
}


BOOL Export(TCHAR *szFileName, HWND hwndHexView, IMPEXP_OPTIONS *eopt)
{
	FILE *fp;
	BOOL  success;
	const TCHAR *mode;
	
	size_w offset, length;
	
	HexView_GetSelStart(hwndHexView, &offset);
	HexView_GetSelSize(hwndHexView, &length);

	if(length == 0)
	{
		offset = 0;
		HexView_GetFileSize(hwndHexView, &length);
	}
	
	eopt->linelen  = HexView_GetLineLen(hwndHexView);

	if(eopt->format == FORMAT_RAWDATA)
	{
		mode = eopt->fAppend ? TEXT("a+b") : TEXT("wb");
	}
	else
	{
		mode = eopt->fAppend ? TEXT("a+t") : TEXT("wt");
	}
	
	if((fp = _tfopen(szFileName, mode)) == 0)
	{
		return FALSE;
	}

	HexView_SetCurPos(hwndHexView, offset);
	SendMessage(hwndHexView, WM_SETREDRAW, FALSE, 0);
	
	switch(eopt->format)
	{
	case FORMAT_RAWDATA:	success = ExportRaw			(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_HEXDUMP:	success = ExportText		(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_RAWHEX:		success = ExportRawHex		(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_HTML:		success = ExportHtml		(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_ASM:		success = ExportASM			(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_CPP:		success = ExportCPP			(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_INTELHEX:	success = ExportIntelHex	(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_SRECORD:	success = ExportMotorola	(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_BASE64:		success = ExportBase64		(fp, hwndHexView, offset, length, eopt);	break;
	case FORMAT_UUENCODE:	success = ExportUUEncode	(fp, hwndHexView, offset, length, eopt);	break;
	default: success = FALSE; break;
	}
	
	HexView_SetSelStart(hwndHexView, offset);
	HexView_SetSelEnd(hwndHexView, offset + length);
	SendMessage(hwndHexView, WM_SETREDRAW, TRUE, 0);

	fclose(fp);
	return success;
}

BOOL ShowExportDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName)
{
	TCHAR *szFilter		= 	TEXT( "Raw binary - no transformation (*.*)\0*.*\0" )							\
							TEXT( "Plain text (*.txt)\0*.txt\0" )											\
							TEXT( "Hex string (*.txt)\0*.txt\0" )											\
							TEXT( "HTML files (*.htm;*.html)\0*.htm;*.html\0" )							\
							TEXT( "C++ source (*.c;*.cpp;*.cc;*.h;*.hpp)\0*.c;*.cpp;*.cc;*.h;*.hpp\0" )	\
							TEXT( "Assembler source (*.asm;*.inc;*.s)\0*.asm;*.inc;*.s\0" )				\
							TEXT( "Intel Hex Records (*.hex)\0*.hex\0" )									\
							TEXT( "Motorola S-Records (*.S;*.S19;*.S28;*.S37)\0*.s;*.s19;*.s28;*.s37\0" )			\
							TEXT( "Base64 (*.b64;*.txt)\0*.b64;*.txt\0" )								\
							TEXT( "UUEncode (*.uue;*.txt)\0*.uue;*.txt\0" )								\
							TEXT( "\0" ) ;

	OPENFILENAME ofn = { sizeof(ofn) };

	TCHAR szFileName[MAX_PATH];
	TCHAR szTitleName[MAX_PATH]; 
	pszFileName=szFileName;
	pszTitleName=szTitleName;

	ofn.hwndOwner		= hwnd;
	ofn.hInstance		= GetModuleHandle(0);
	ofn.lpstrFilter		= szFilter;
	ofn.lpstrFile		= pszFileName;
	ofn.lpstrFileTitle	= pszTitleName;
	ofn.lpfnHook		= ExportHookProc;
	ofn.lpstrTitle		= TEXT("Export");
	
	ofn.nFilterIndex	= g_ExportOptions.format + 1;
	ofn.nMaxFile		= MAX_PATH;
	ofn.nMaxFileTitle	= MAX_PATH;

	// flags to control appearance of open-file dialog
	ofn.Flags			=	OFN_EXPLORER			|
							OFN_ENABLESIZING		|
							OFN_ENABLEHOOK			|
							OFN_HIDEREADONLY		|
							OFN_ENABLETEMPLATE		|
							OFN_OVERWRITEPROMPT		;

	ofn.lpTemplateName  = MAKEINTRESOURCE(IDD_EXPORTEXTEND);
	
	// nul-terminate filename
	pszFileName[0] = '\0';

	// setup our custom parameters
	ofn.lCustData = (LPARAM)&g_ExportOptions;

	if(GetSaveFileName(&ofn)) 
	{
		return Export(szFileName, g_hwndHexView, &g_ExportOptions);
	}
	else
	{
		return FALSE;
	}
}


