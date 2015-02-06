//
//  configlib.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "tinyxml.h"

typedef TiXmlNode * HCONFIG;
#define CONFIG_LIB
#include "ConfigLib.h"


HCONFIG CreateConfig(HCONFIG config)
{
	char def[] = "<?xml version=\"1.0\" standalone=\"yes\" encoding=\"UTF-8\">"
				 //"<HexEdit/>"
				 ;

	TiXmlDocument *xmldoc = config->ToDocument();
	
	if(xmldoc)
	{
		xmldoc->Parse(def);
		return xmldoc;
	}
	else
	{
		return 0;
	}
}

HCONFIG CreateConfig()
{
	return CreateConfig(new TiXmlDocument);
}

HCONFIG OpenConfig(LPCTSTR pszConfigFile)
{
	TiXmlDocument *xmldoc = new TiXmlDocument();

	FILE * fp = _tfopen(pszConfigFile, TEXT("rb"));

	if(!xmldoc->LoadFile(fp))
	{
		CreateConfig(xmldoc);
	}

	if(fp)
		fclose(fp);

	return xmldoc;
}

BOOL SaveConfig(LPCTSTR pszConfigFile, HCONFIG config)
{
	FILE * fp = _tfopen(pszConfigFile, TEXT("wt"));

	if(fp == NULL)
		return FALSE;

	TiXmlDocument *xmldoc = config->GetDocument();

	if(xmldoc)
	{
		xmldoc->SaveFile(fp);
	}
	
	fclose(fp);
	return TRUE;
}

void CloseConfig(HCONFIG config)
{
	TiXmlDocument *xmldoc = config->ToDocument();

	if(xmldoc)
	{
		delete xmldoc;
	}
}

void DeleteConfigSection(HCONFIG config)
{
	TiXmlNode *parent = config->Parent();
	if (parent)
	{
		parent->RemoveChild(config);
	}
}

HCONFIG EnumConfigSection(HCONFIG config, LPCTSTR szSectionName, int idx)
{
	char path8[100], *tok, *next;

	if(config == 0)
		return 0;

	// make a UTF8 version of the name
	WideCharToMultiByte(CP_UTF8, 0, szSectionName, -1, path8, 100, 0, 0);

	TiXmlNode *node = config;

	const char *a = node->Value();

	tok = strtok(path8, ".\\/");

	TiXmlNode *child = 0;

	while(tok)
	{
		child = node->FirstChild(tok);
		
		next = strtok(NULL, ".\\/");

		// if we found no node, or we are the last component of the 'path'
		if(child == 0 || next == 0)
		{
			if(idx == -1)
			{
				child = node->InsertEndChild(TiXmlElement(tok));
			}
			else if(child == 0)
			{
				return 0;
			}
		}

		tok  = next;
		node = child;
		
		// was this the last one?
		if(tok == 0)
		{
			// enumerate until we find the right one
			for(int i = 0; idx >= 0 && i < idx; i++)
			{
				node = node->NextSiblingElement();
			}
		}
	}

	return node;
}

HCONFIG GetConfigSection(HCONFIG config, LPCTSTR szSectionName)
{
	return EnumConfigSection(config, szSectionName, 0);
}

HCONFIG OpenConfigSection(HCONFIG config, LPCTSTR szSectionName)
{
	return EnumConfigSection(config, szSectionName, 0);
}

HCONFIG CreateConfigSection(HCONFIG config,  LPCTSTR szSectionName)
{
	return EnumConfigSection(config, szSectionName, -1);
}

extern "C" void debug(HCONFIG config)
{
	FILE *fp = fopen("debug_config.txt", "wt");
	config->Print(fp, 10);
	fclose(fp);
}

static const char * GetNodeText(TiXmlNode *node)
{
	TiXmlElement *elem = node->ToElement();
	const char *ptr = 0;

	if(node)
		ptr = elem->GetText();

	if(ptr && *ptr == '\0')
		ptr = 0;
	
	return ptr;
}

static BOOL UnformatElement(HCONFIG hConfig, LPCSTR szFmt, void *p)
{
	const char *str = GetNodeText(hConfig);

	if(str)
	{
		if(sscanf(str, szFmt, p) == 1)
			return TRUE;
	}

	return FALSE;
}

BOOL GetConfigI32(HCONFIG config, LPCTSTR szKeyName, UINT32 *pnValue, UINT32 nDefault)
{
	if((config = GetConfigSection(config, szKeyName)) == 0)
		return FALSE;

	if(UnformatElement(config, "%d", pnValue))
	{
		return TRUE;
	}
	else
	{
		*pnValue = nDefault;
		return FALSE;
	}
}

BOOL GetConfigH32(HCONFIG config, LPCTSTR szKeyName, UINT32 *pnValue, UINT32 nDefault)
{
	if((config = GetConfigSection(config, szKeyName)) == 0)
		return FALSE;

	if(UnformatElement(config, "%x", pnValue))
	{
		return TRUE;
	}
	else
	{
		*pnValue = nDefault;
		return FALSE;
	}
}

BOOL GetConfigI64(HCONFIG config, LPCTSTR szKeyName, UINT64 *pnValue, UINT64 nDefault)
{
	if((config = GetConfigSection(config, szKeyName)) == 0)
		return FALSE;

	if(UnformatElement(config, "%I64d", pnValue))
	{
		return TRUE;
	}
	else
	{
		*pnValue = nDefault;
		return FALSE;
	}
}

BOOL GetConfigH64(HCONFIG config, LPCTSTR szKeyName, UINT64 *pnValue, UINT64 nDefault)
{
	if((config = GetConfigSection(config, szKeyName)) == 0)
		return FALSE;

	if(UnformatElement(config, "%I64x", pnValue))
	{
		return TRUE;
	}
	else
	{
		*pnValue = nDefault;
		return FALSE;
	}
}

BOOL GetConfigStr(HCONFIG config, LPCTSTR szKeyName, LPTSTR pszValue, DWORD nLength, LPCTSTR szDefault)
{
	if((config = GetConfigSection(config, szKeyName)) == 0)
		return FALSE;

	const char *str = GetNodeText(config);

	if(str)
	{
#ifdef UNICODE
		if(!MultiByteToWideChar(CP_UTF8, 0, str, -1, pszValue, nLength))
		{
			lstrcpyn(pszValue, szDefault, nLength);
			return FALSE;
		}
#else
		lstrcpyn(pszValue, str, nLength);
#endif
		return TRUE;
	}
	else if(szDefault)
	{
		lstrcpyn(pszValue, szDefault, nLength);
	}

	return FALSE;
}

static void StringElement(HCONFIG hConfig, LPCSTR szString)
{
	TiXmlElement *elem = hConfig->ToElement();
	elem->InsertEndChild(TiXmlText(szString));
}

static void FormatElement(HCONFIG hConfig, LPCSTR szFmt, ...)
{
	va_list varg;
	char buf[200];

	TiXmlElement *elem = hConfig->ToElement();

	va_start(varg, szFmt);

	vsprintf(buf, szFmt, varg);

	elem->InsertEndChild(TiXmlText(buf));

	va_end(varg);
}

BOOL SetConfigI32(HCONFIG config, LPCTSTR szKeyName, UINT32  nValue)
{
	if((config = CreateConfigSection(config, szKeyName)) == 0)
		return FALSE;

	FormatElement(config, "%d", nValue);
	return TRUE;
}

BOOL SetConfigH32(HCONFIG config, LPCTSTR szKeyName, UINT32  nValue)
{
	if((config = CreateConfigSection(config, szKeyName)) == 0)
		return FALSE;

	FormatElement(config, "%x", nValue);
	return TRUE;
}


BOOL SetConfigI64(HCONFIG config, LPCTSTR szKeyName, UINT64  nValue)
{
	if((config = CreateConfigSection(config, szKeyName)) == 0)
		return FALSE;

	FormatElement(config, "%I64d", nValue);
	return TRUE;
}

BOOL SetConfigH64(HCONFIG config, LPCTSTR szKeyName, UINT64  nValue)
{
	if((config = CreateConfigSection(config, szKeyName)) == 0)
		return FALSE;

	FormatElement(config, "%I64x", nValue);
	return TRUE;
}

BOOL SetConfigStr(HCONFIG config, LPCTSTR szKeyName, LPCTSTR szValue)
{
	char str[100];
	size_t len = lstrlen(szValue);

	if((config = CreateConfigSection(config, szKeyName)) == 0)
		return FALSE;

	if(len < 50)
	{
		WideCharToMultiByte(CP_UTF8, 0, szValue, -1, str, 100, 0, 0);
		StringElement(config, str);
	}
	else
	{
		len *= 2;
		char *ptr = new char[len];

		// make a UTF8 version of the name
		while(WideCharToMultiByte(CP_UTF8, 0, szValue, -1, ptr, (int)len, 0, 0) == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			delete[] ptr;
			len *= 2;
			ptr = new char[len];
		}

		StringElement(config, ptr);
		delete[] ptr;
	}
	
	return TRUE;
}


BOOL SetConfigBin(HCONFIG config, LPCTSTR szKeyName, PVOID data, DWORD len)
{
	char str[100];

	if((config = CreateConfigSection(config, szKeyName)) == 0)
		return FALSE;

	BYTE *b = (BYTE *)data;

	for(DWORD i = 0; i < len; i++)
	{
		sprintf(&str[i*2], "%02x", b[i]);
	}

	TiXmlElement *elem = config->ToElement();
	elem->InsertEndChild(TiXmlText(str));
	return TRUE;
}
