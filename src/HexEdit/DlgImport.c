//
//  DlgImport.c
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
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include "HexEdit.h"
#include "HexUtils.h"
#include "resource.h"
#include "trace.h"
#include "base64.h"
#include "endian.h"


//bool lex_initbuf(char *buffer, int len);
#include "..\TypeLib\parser.h"//lexer.h"
#include "..\HexView\HexView.h"

extern BOOL g_fFileChanged;
extern TCHAR g_szFileName[];
extern TCHAR g_szFileTitle[];
static HHOOK g_hHook;

BOOL InitTransformList(HWND hwndDlg, UINT uCtrlId);

size_t motorola_to_bin(char *srec, int *type, int *count, unsigned long *addr, BYTE *data);
size_t intel_to_bin(char *hrec, int *type, int *count, unsigned long *addr, BYTE *data);

int hex2dec(int ch);

IMPEXP_OPTIONS g_ImportOptions = { FORMAT_HEXDUMP, SEARCHTYPE_BYTE };

INT_PTR CALLBACK ImportHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndRelative;
	static IMPEXP_OPTIONS *ieopt;//g_ImportOptions
	OFNOTIFY *ofn;

	switch(uiMsg)
	{
	case WM_INITDIALOG:
		ieopt = (IMPEXP_OPTIONS *)((OPENFILENAME *)lParam)->lCustData;
		return TRUE;

	case WM_SHOWWINDOW:

		hwndRelative = GetDlgItem(GetParent(hdlg), 0x470);
		AlignWindow(GetDlgItem(hdlg, IDC_ENDIAN),		hwndRelative, ALIGN_LEFT);
		AlignWindow(GetDlgItem(hdlg, IDC_USEADDRESS),	hwndRelative, ALIGN_LEFT);
		AlignWindow(GetDlgItem(hdlg, IDC_QUICKLOAD),	hwndRelative, ALIGN_LEFT);

		CenterWindow(GetParent(hdlg));

		return -1;

	case WM_NOTIFY:
		ofn = (OFNOTIFY *)lParam;

		if(ofn->hdr.code == CDN_TYPECHANGE)
		{
			BOOL fEnable = FALSE;
			int  idx = ofn->lpOFN->nFilterIndex - 1;

			if(idx == FORMAT_CPP || idx == FORMAT_ASM)
				fEnable = TRUE;

			//EnableDlgItem(hdlg, IDC_DATATYPE, fEnable);
			EnableDlgItem(hdlg, IDC_ENDIAN, fEnable);

			break;
		}
		else if(ofn->hdr.code == CDN_FILEOK)
		{
			ieopt->format		= ofn->lpOFN->nFilterIndex - 1;
			ieopt->fBigEndian	= IsDlgButtonChecked(hdlg, IDC_ENDIAN);
			ieopt->fUseAddress  = IsDlgButtonChecked(hdlg, IDC_USEADDRESS);
			//g_ExportOptions.fAppend		= IsDlgButtonChecked(hdlg, IDC_APPENDTOFILE);
			//g_ExportOptions.basetype	= ComboBox_GetSelData(GetDlgItem(hdlg, IDC_DATATYPE));
		}

		return 0;
	}

	return 0;
}

char *skipspace(char *ach)
{
	while(isspace(*ach))
		ach++;

	return ach;
}

char *hexdata(char *ach, UINT64 *data, int *count)
{
	*data  = 0;
	*count = 0;

	if(!isxdigit(*ach))
		return 0;

	//ach = skipspace(ach);

	while(isxdigit(*ach))
	{
		*data = (*data << 4) | hex2dec(*ach);
		(*count)++;
		ach++;
	}

	// allow one space or a '-' following this
	if(isspace(*ach) || *ach == '-')
		ach++;

	//return (*ach == 0 || isspace(*ach) || *ach == '-') ? ach : NULL;
	return ach;//(*ach == 0 || isxdigit(*ach)) ? ach : NULL;
}

BOOL JumpAndPad(HWND hwndHexView, size_w addr)
{
	size_w filesize; 
	BYTE   zero = 0;

	HexView_GetFileSize(hwndHexView, &filesize);
				
	if(addr > filesize)
	{
		// goto end of file
		HexView_SetCurPos(hwndHexView, filesize);

		// fill with zeros up to address
		HexView_FillData(hwndHexView, &zero, 1, addr - filesize);
	}
			
	if(HexView_SetCurPos(hwndHexView, addr))
	{
		return TRUE;
	}
	else
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}
}

//
//	Import a plain-text hex dump, complete with address column 
//  and hex/ascii data columns
//
size_w ImportText(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	char ach[512];
	UINT64 addr, data;
	int    count;
	size_w total = 0;

	// read in a line of text
	while(fgets(ach, sizeof(ach), fp))
	{
		char *ptr = ach;

		// get the address/offset
		if((ptr = hexdata(ptr, &addr, &count)) == 0)
		{
			if(total > 0)
			{
				SetLastError(ERROR_INVALID_DATA);
			}

			break;
		}

		// was it an address? 
		if(count > 2)
		{
			if(ieopt->fUseAddress)
			{
				JumpAndPad(hwndHexView, addr);
			}

			// skip whitespace
			ptr = skipspace(ptr);
		}
		// otherwise the address column is missing 
		else
		{
			ptr = ach;
		}
	
		// get any hex data
		while(ptr && *ptr)
		{
			// try to get hex-data. 
			if((ptr = hexdata(ptr, &data, &count)) != 0)
			{
				int len = (count+1)/2;
				reverse((BYTE *)&data, len);
				HexView_SetDataAdv(hwndHexView, &data, len);
				total += len;
			}
			else
			{
				// if it fails then maybe it's the ascii column....
				break;
			}
		}	
	}

	return total;//!ferror(fp);
}

size_w ImportRawHex(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	size_w  total = 0;
	char    tmp[100];
	size_t	len;

	BYTE val = 0;
	int  count = 0;
	size_t  i;

	while((len = fread(tmp, 1, 100, fp)) > 0)
	{
		for(i = 0; i < len; i++)
		{
			if(isxdigit(tmp[i]))
			{
				if(count) val <<= 4;
				else      val = 0;
				
				val |= hex2dec(tmp[i]);
					
				if((count = !count) == 0)
				{
					HexView_SetDataAdv(hwndHexView, &val, 1);
					total++;
				}
			}
			else if(isspace(tmp[i]))
			{
				if(count)
				{
					HexView_SetDataAdv(hwndHexView, &val, 1);
					total++;
					count = 0;
				}
			}
			else
			{
				break;
			}
		}
	}
	
	if(count)
	{
		HexView_SetDataAdv(hwndHexView, &val, 1);
		total++;
	}

	return total;
}

size_w ImportHtml(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	size_w total = 0;
	return total;
}

size_w ImportCPP(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	TOKEN   type   = TOK_NULL;
	TOKEN   t      = 0;
	int     width  = 1;
	size_w  total  = 0;
	void *  parser;

	char    tmp[100];
	size_t	len;
	char *  buf    = 0;
	size_t  buflen = 0;
	BOOL    fBegin = FALSE;

	while((len = fread(tmp, 1, 100, fp)) > 0)
	{
		buf = realloc(buf, buflen + len);
		memcpy(buf + buflen, tmp, len);
		buflen += len;
	}
		
	if((parser = AllocParser(buf, buflen)) == 0)
	{
		free(buf);
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	SetLastError(ERROR_NO_MORE_ITEMS);

	do
	{
		INUMTYPE tnum = INUM(parser);
		FNUMTYPE dnum = FNUM(parser);

		ENDIAN_TO_NATIVE(ieopt->fBigEndian, (BYTE *)&tnum, width);

		switch(t)
		{
		case TOK_CHAR: case TOK_BYTE:
			width = 1; 
			type = t;
			break;
		
		case TOK_WORD: case TOK_WCHAR:
			type = t;
			width = sizeof(WORD); break;

		case TOK_DWORD: case TOK_FLOAT: 
			type = t;
			width = sizeof(DWORD); break;

		case TOK_QWORD: case TOK_DOUBLE: 
			type = t;
			width = sizeof(QWORD); break;

		case '{':
			fBegin = TRUE; break;

		case '}':
			fBegin = FALSE; break;
		
		case TOK_INUMBER:
			if(fBegin)
			{
				//if(type == TOK_DOUBLE)
				HexView_SetDataAdv(hwndHexView, &tnum, width);
				total += width;
			}

			break;
		
		case TOK_FNUMBER:
			if(fBegin)
			{
				if(type == TOK_FLOAT)
				{
					float fnum = (float)dnum;
					HexView_SetDataAdv(hwndHexView, &fnum, width);
				}
				else
				{
					HexView_SetDataAdv(hwndHexView, &dnum, width);
				}
				total += width;
			}
			break;
		}
	}
	while((t = nexttok(parser)) != 0);

	free(buf);

	return total;
}

size_w ImportASM(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	char ach[256];

	size_w baseaddr = 0;
	size_w total    = 0;
	
	// read in a line of text
	while(fgets(ach, 256, fp))
	{
		char *ptr = ach;
		int  width = 0;
		BYTE buf[256];
		int  buflen = 0;

		// skip whitespace
		while(*ptr && isspace(*ptr))
			ptr++;

		// comments
		if(*ptr == ';')
			continue;

		if(memcmp(ptr, "db ", 3) == 0)
			width = 1;
		else if(memcmp(ptr, "dw ", 3) == 0)
			width = 2;
		else if(memcmp(ptr, "dd ", 3) == 0)
			width = 4;
		else
			break;

		ptr += 3;
		
		while(*ptr)
		{
			char	numstr[40], *numptr = numstr;
			BOOL	hexnum = FALSE;
			UINT64  num;
			int		len = 0;

			// skip whitespace
			while(*ptr && isspace(*ptr))
				ptr++;

			while(*ptr && (isxdigit(*ptr) || *ptr == 'h' || *ptr == 'H'))
			{
				if(*ptr == 'h')
				{
					hexnum = TRUE;
					ptr++;
					break;
				}

				*numptr++ = *ptr++;
			}

			*numptr = '\0';

			if(hexnum)
			{
				len = sscanf(numstr, "%I64x", &num);
			}
			else
			{
				len = sscanf(numstr, "%I64u", &num);
			}

			num = ENDIAN_TO_NATIVE64(ieopt->fBigEndian, num);

			if(len > 0)
			{
				switch(width)
				{
				case 1: *(BYTE  *)(&buf[buflen]) = (BYTE)num; break;
				case 2: *(WORD  *)(&buf[buflen]) = (WORD)num; break;
				case 4: *(DWORD *)(&buf[buflen]) = (DWORD)num; break;
				}

				buflen += width;
			}
		}

		HexView_SetDataAdv(hwndHexView, buf, buflen);
		total += buflen;
	}

	return total;//!ferror(fp);
}

size_w ImportIntelHex(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	char   ach[256];
	size_w total    = 0;
	size_w baseaddr = 0;
	
	// read in a line of text
	while(fgets(ach, 256, fp))
	{
		BYTE data[256];
		int type;
		int count;
		unsigned long addr;

		// parse an S-record
		if(intel_to_bin(ach, &type, &count, &addr, data) == 0)
			return FALSE;

		// what kind of record is it?
		switch(type)
		{
		// data!
		case 0: 
			
			if(ieopt->fUseAddress)
			{
				JumpAndPad(hwndHexView, addr | baseaddr);
			}

			HexView_SetDataAdv(hwndHexView, data, count);
			total += count;
			break;

		// segment address
		case 2:	
			baseaddr = addr << 16;
			break;
		
		// linear address high16
		case 4:
			baseaddr = addr << 16;
			break;

		// ignore anything else
		default:
			break;
		}
	}
	
	return total;//!ferror(fp);
}


size_w ImportMotorola(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	char ach[256];
	size_w total = 0;
	
	// read in a line of text
	while(fgets(ach, 256, fp))
	{
		BYTE data[256];
		int type;
		int count;
		unsigned long addr;

		// parse an S-record
		if(motorola_to_bin(ach, &type, &count, &addr, data) == 0)
			return FALSE;

		// what kind of record is it?
		switch(type)
		{
		// 2/3/4 byte address with data?
		case 1: case 2: case 3:
			
			if(ieopt->fUseAddress)
			{
				JumpAndPad(hwndHexView, addr);
			}
				
			HexView_SetDataAdv(hwndHexView, data, count);
			total += count;
			break;

		// ignore anything else
		default:
			break;
		}
	}
	
	return total;//!ferror(fp);
}

typedef struct
{
	FILE *	fp;
	char *  buf;
	size_t  pos;
	size_t  len;
	//char *  tmp;
	//size_t	tmplen;
	size_t  size;
	//BOOL	startln;
} FILE_READER;

void init_reader(FILE_READER *rfp, FILE *fp, size_t size)
{
	rfp->fp			= fp;
	rfp->size		= size;
	rfp->buf		= malloc(size);
	//rfp->tmp		= malloc(size);
	//rfp->tmplen		= 0;
	//rfp->startln	= 0;
	rfp->pos		= 0;
	rfp->len		= 0;
}

static int frgetch(FILE_READER *rfp, BOOL consume)
{
	if(rfp->pos == rfp->len)
	{
		rfp->len	= fread(rfp->buf, 1, rfp->size, rfp->fp);
		rfp->pos    = 0;
	}

	if(rfp->len == 0)
	{
		return 0;
	}
	else
	{
		int ch = rfp->buf[rfp->pos];
		rfp->pos += consume;
		return ch;
	}
}

char * fgets_r(FILE_READER *rfp, char *buf, size_t len)
{
	size_t i = 0;

	if(len < 1)
		return NULL;

	while(i < len - 1)
	{
		int ch = frgetch(rfp, TRUE);

		// deal with CR/LF combos
		if(ch == '\r')
		{
			ch = '\n';
			if(frgetch(rfp, FALSE) == '\n')
				frgetch(rfp, TRUE);
		}
		else if(ch == 0)
			break;
		
		buf[i++] = ch;
	}

	buf[i] = '\0';
	return i == 0 ? NULL : buf;
}

void fgets_skip(FILE_READER *rfp)
{
	int ch = frgetch(rfp, TRUE);
	
	while(ch && ch != '\n')
		ch = frgetch(rfp, TRUE);
}

/*
char * fgets_r(FILE_READER *rfp)
{
	size_t rlen = rfp->tmplen;
	size_t i	= 0;

	// restore from last read
	memmove(rfp->buf, rfp->tmp, rlen);

	// fill up the rest of the user buffer (if there's any space left)
	rlen += fread(rfp->buf + rlen, 1, rfp->size - rlen, rfp->fp);

	// scan for the end of line
	while(i < rlen && rfp->buf[i] != '\n' && rfp->buf[i] != '\r')
		i++;

	// turn CR -> LF
	if(rlen > 1 && i < rlen && rfp->buf[i] == '\r') 
	{
		rfp->buf[i] = '\n';

		if(i < rlen - 1 && rfp->buf[i+1] == '\r')
			rlen--;
	}

	// make sure there is space for a nul-terminator
	i = min(i, rfp->size - 1);

	// move anything after the newline, back into the tmp buffer
	memmove(rfp->tmp, rfp->buf + i, rlen - i);
	rfp->tmplen = rlen - i;

	// nul-terminate
	rfp->buf[i] = '\0';

	return i > 0 ? rfp->buf : NULL;
}

void fgets_s(FILE_READER *rfp)
{
	size_t i;

	// the user got a whole line last time, so don't worry
	if(rfp->tmplen == 0)
		return;

	for(;;)
	{
		for(i = 0; i < rfp->tmplen; i++)
		{
			if(rfp->tmp[i] == '\r')
			{
				memmove(rfp->tmp, rfp->tmp+i, rfp->tmplen - i);
			}
			else if(rfp->tmp[i] == '\n')
			{
				break;
			}
		}

		if(i == rfp->tmplen)
		{
		}


	}
}
*/
size_w ImportBase64(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	char   ach[65];
	BYTE   buf[64];
	size_t len;
	size_t alen;
	size_t rem = 0;
	size_w total = 0;

	size_t rlen = 0;
	size_t i = 0;

	FILE_READER rfp;
	init_reader(&rfp, fp, 64);

	//while(fread(ach, 200, fp))//) > 0)
	//while((rlen = fread(ach, 1, 200, fp) > 0)
	
	while(fgets_r(&rfp, ach, sizeof(ach)))
	{
		alen = strlen(ach);

		// skip the --BEGIN at the top
		// just check for a '-' because that's not a base64 character anyway
		if(ach[0] == '-' || isspace(ach[0])) 
		{
			fgets_skip(&rfp);
			continue;
		}
		
		if((len = base64_decode(ach, alen - rem, buf)) != 0)
		{
			total += len;

			if(HexView_SetDataAdv(hwndHexView, buf, len) != len)
			{
				break;
			}
		}
		else if(len == 0 && buf[0] && !isspace(buf[0]))
		{
			break;
		}

		//memmove(ach, ach+alen-rem, rem);
	}

	return total;//!ferror(fp);
}

size_w ImportUUEncode(FILE *fp, HWND hwndHexView, size_w offset, size_w length, IMPEXP_OPTIONS *ieopt)
{
	char   ach[130];
	BYTE   buf[200];
	size_t len;
	size_t alen;
	size_w total = 0;

	// skip lines until we get a 'begin'
	while(fgets(ach, sizeof(ach), fp))
	{
		if(memcmp(ach, "begin ", 6) == 0)
		{
			int  mode;
			sscanf(ach, "begin %x ", &mode);
			break;
		}
	}
	
	// get a line of text
	while(fgets(ach, sizeof(ach), fp))
	{
		if(memcmp(ach, "end", 3) == 0)
			break;

		alen = strlen(ach);

		// check if the line was truncated
		if(alen && ach[alen-1] != '\n')
			break;

		len = ach[0];
		
		if((len = uu_decode(ach, alen, buf)) == 0)
			break;

		total += len;

		if(HexView_SetDataAdv(hwndHexView, buf, len) != len)
		{
			break;
		}
	}

	return total;//!ferror(fp);
}


size_w Import(TCHAR *szFileName, HWND hwndHexView, IMPEXP_OPTIONS *ieopt)
{
	FILE *fp = 0;
	size_w count;
	const TCHAR *mode;
	DWORD dwError;

	size_w offset, length;
	
	HexView_GetSelStart(hwndHexView, &offset);
	HexView_GetSelSize(hwndHexView, &length);
	ieopt->linelen = HexView_GetLineLen(hwndHexView);
	
	if(ieopt->format == FORMAT_RAWDATA)
	{
		HexView_ImportFile(hwndHexView, szFileName, 0);
		return 1;
	}

	if(ieopt->format == FORMAT_RAWDATA)
		mode = TEXT("rb");
	else
		mode = TEXT("rt");
		
	if((fp = _tfopen(szFileName, mode)) == 0)
	{
		HexWinErrorBox(GetLastError(), 0);
		return FALSE;
	}

	HexView_SetCurPos(hwndHexView, offset);
	SendMessage(hwndHexView, WM_SETREDRAW, FALSE, 0);

	SetLastError(ERROR_NO_MORE_ITEMS);
	
	switch(ieopt->format)
	{
	//case FORMAT_RAWDATA:	count = ImportRaw		(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_HEXDUMP:	count = ImportText		(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_RAWHEX:		count = ImportRawHex	(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_HTML:		count = ImportHtml		(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_ASM:		count = ImportASM		(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_CPP:		count = ImportCPP		(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_INTELHEX:	count = ImportIntelHex	(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_SRECORD:	count = ImportMotorola	(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_BASE64:		count = ImportBase64	(fp, hwndHexView, offset, length, ieopt);	break;
	case FORMAT_UUENCODE:	count = ImportUUEncode	(fp, hwndHexView, offset, length, ieopt);	break;
	default: 
		count = 0; 
		SetLastError(ERROR_TRANSFORM_NOT_SUPPORTED);
		break;
	}

	dwError = GetLastError();
	
	if(fp)
		fclose(fp);

	SendMessage(hwndHexView, WM_SETREDRAW, TRUE, 0);

	SetLastError(dwError);

	return count;
}

BOOL ShowImportDlg(HWND hwnd, LPTSTR pszFileName, LPTSTR pszTitleName)
{
	static int g_nLastImportType = FORMAT_RAWDATA;

	TCHAR *szFilter		= 	TEXT( "Raw binary - no transformation (*.*)\0*.*\0" )							\
							TEXT( "Plain text (*.txt)\0*.txt\0" )											\
							TEXT( "Hex string (*.txt)\0*.txt\0" )											\
							TEXT( "HTML files (*.htm;*.html)\0*.htm;*.html\0" )							\
							TEXT( "C++ source (*.c;*.cpp;*.cc;*.h;*.hpp)\0*.c;*.cpp;*.cc;*.h;*.hpp\0" )	\
							TEXT( "Assembler source (*.asm;*.inc;*.s)\0*.asm;*.inc;*.s\0" )				\
							TEXT( "Intel Hex Records (*.hex)\0*.hex\0" )									\
							TEXT( "Motorola S-Records (*.S;*.S19;*.S28;*.S37)\0*.s,*.s19;*.s28;*.s37\0" )			\
							TEXT( "Base64 (*.b64;*.txt)\0*.b64;*.txt\0" )								\
							TEXT( "UUEncode (*.uue;*.txt)\0*.uue;*.txt\0" )								\
							TEXT( "\0" ) ;

	OPENFILENAME ofn = { sizeof(ofn) };

	TCHAR szFileName[MAX_PATH];
	TCHAR szTitleName[MAX_PATH]; 
	pszFileName = szFileName;
	pszTitleName = szTitleName;

	ofn.hwndOwner		= hwnd;
	ofn.hInstance		= GetModuleHandle(0);
	ofn.lpstrFilter		= szFilter;
	ofn.lpstrFile		= pszFileName;
	ofn.lpstrFileTitle	= pszTitleName;
	ofn.lpfnHook		= ImportHookProc;
	ofn.lpstrTitle		= TEXT("Import");
	
	ofn.nFilterIndex	= g_nLastImportType + 1;
	ofn.nMaxFile		= MAX_PATH;
	ofn.nMaxFileTitle	= MAX_PATH;

	// flags to control appearance of open-file dialog
	ofn.Flags			=	OFN_EXPLORER			|
							OFN_ENABLESIZING		|
							OFN_FILEMUSTEXIST		|
							OFN_ENABLEHOOK			|
							OFN_HIDEREADONLY		|
							OFN_ENABLETEMPLATE		;
							//OFN_CREATEPROMPT;

	ofn.lpTemplateName  = MAKEINTRESOURCE(IDD_IMPORTEXTEND);

	ofn.lCustData = (LPARAM)&g_ImportOptions;
	
	// nul-terminate filename
	pszFileName[0] = '\0';

	if(GetOpenFileName(&ofn)) 
	{
		g_nLastImportType = ofn.nFilterIndex - 1;
		
		//SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_FILE_NEW, 0), 0);
		//CreateNewFile(hwnd);
		if(Import(szFileName, g_hwndHexView, &g_ImportOptions) == 0)
		{
			if(GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				HexWinErrorBox(GetLastError(), 0);
			}
			else
			{
				HexErrorBox(TEXT("%s"), TEXT("File contents could not be transformed\n"));
			}
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


