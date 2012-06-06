//
//  stringprint.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef STRINGPRINT_INCLUDED
#define STRINGPRINT_INCLUDED

#include <tchar.h>
#include <stdarg.h>

class stringprint
{
public:
	stringprint(char *s, int ml) : ptr(s), wptr(0), maxlen(ml), len(0), fp(0)
	{
	}

	stringprint(wchar_t *s, int ml) : ptr(0), wptr(s), maxlen(ml), len(0), fp(0)
	{
	}

	stringprint(FILE *f) : ptr(0), wptr(0), maxlen(0), len(0), fp(f)
	{
	}

	int _sprintf(const char *fmt, ...)
	{
		int n;
		size_t m = maxlen > len ? maxlen - len : 0;

		va_list varg;
		va_start(varg, fmt);

		if(fp)
		{
			// fprintf to the FILE* stream
			n = vfprintf(fp, fmt, varg);
		}
		else
		{
			// sprintf to the string buffer
			n = _vsnprintf(ptr + len, m, fmt, varg);
		}
		
		va_end(varg);
		len += n;
		return n;
	}

	int _swprintf(const wchar_t *fmt, ...)
	{
		int n;
		size_t m = maxlen > len ? maxlen - len : 0;

		va_list varg;
		va_start(varg, fmt);

		if(fp)
		{
			// fwprintf to the FILE* stream
			n = vfwprintf(fp, fmt, varg);
		}
		else
		{
			// swprintf to the string buffer
			n = _vsnwprintf(wptr + len, m, fmt, varg);
		}
		
		va_end(varg);
		len += n;
		return n;
	}

	size_t length()
	{
		return len;
	}

	size_t size()
	{
		return maxlen - len;
	}

	/*int _stprintf(const TCHAR *fmt, ...)
	{
#ifdef UNICODE
		return swprintf(fmt
	}*/

private:
	char	*	ptr;
	wchar_t *	wptr;
	int			len;
	int			maxlen;
	FILE  *		fp;
};

#ifndef TEXT
#define TEXT _T
#endif

#endif