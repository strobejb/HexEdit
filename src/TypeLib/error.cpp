//
//  error.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "parser.h"


ERROR_LOOKUP err_lookup[] = 
{
#undef DEFINE_ERR
#define DEFINE_ERR(err,msg) {err,msg},
#include "errordef.h"

	//{	ERROR_UNKNOWN,				"Unknown error"	}
	{	ERROR_NONE,					"No Error"	}

};

#define MAX_STRING_LEN 200

const char * Parser::inenglish(TOKEN t, bool use_t_state)
{
	static char bufarr[4][MAX_STRING_LEN+20];
	static int  ibuf = 0;
	char *buf = bufarr[ibuf];

	if(use_t_state == false)
		return inenglish(t);

	if(t == TOK_IDENTIFIER)
	{
		sprintf(buf, "Identifier \'%s\'", tstr);
		return buf;
	}
	else if(t == TOK_STRINGBUF)
	{
		sprintf(buf, "String '%s'", tstr);
		return buf;
	}
	else if(t == TOK_INUMBER)
	{
		sprintf(buf, "'%d'", (int)tnum);
		return buf;
	}
	else if(t == TOK_FNUMBER)
	{
		sprintf(buf, "'%f'", tfnum);
		return buf;
	}
	else
	{
		return inenglish(t);
	}
}

const char * Parser::inenglish(TOKEN t)
{
	static char bufarr[4][MAX_STRING_LEN+20];
	static int  ibuf = 0;

	char *buf = bufarr[ibuf];

	ibuf = (ibuf + 1) % 4;

	// match against keywords
	for(int i = 0; toklook[i].tok != TOK_NULL; i++)
	{
		if(t == toklook[i].tok)
			return toklook[i].str;
	}

	switch(t)
	{
	case TOK_IDENTIFIER:	return "Identifier";
	case TOK_STRINGBUF:		return "String";
	case TOK_INUMBER:		return "Number";
	case TOK_FNUMBER:		return "Floating-point Number";

	case TOK_CHAR:			return "char";
	case TOK_WCHAR:			return "wchar_t";
	case TOK_BYTE:			return "byte";
	case TOK_WORD:			return "word";
	case TOK_DWORD:			return "dword";
	case TOK_QWORD:			return "qword";
	case TOK_FLOAT:			return "float";
	case TOK_DOUBLE:		return "double";
		
	case '(':				return "(";
	case ')':				return ")";
	case '[':				return "[";
	case ']':				return "]";
	case '{':				return "{";
	case '}':				return "}";
	case ';':				return ";";
	case ':':				return ":";
	case '.':				return ".";
	case ',':				return ",";
	
	case TOK_ANDAND:		return "&&";
	case TOK_OROR:			return "||";
	case TOK_LE:			return "<=";
	case TOK_GE:			return ">=";
	case TOK_NEQ:			return "!=";
	case TOK_EQU:			return "==";
	case TOK_INC:			return "++";
	case TOK_DEC:			return "--";
		//case '=':

	case TOK_NULL:			return "";
	case TOK_ILLEGAL:		return "Illegal character";

	default:		
		if(t >= 32 && t < 127)
		{
			sprintf(buf, "%c", t);
			return buf;
		}
		else
		{
			sprintf(buf, "<unknown: %d>", t);
			return buf;
		}
	}
}

const char * inenglish(TYPE ty)
{
	switch(ty)
	{
	case typeCHAR: return "typeCHAR";
	case typeWCHAR: return "typeWCHAR";
	case typeBYTE: return "typeBYTE";
	case typeWORD: return "typeWORD";
	case typeDWORD: return "typeDWORD";
	case typeQWORD: return "typeQWORD";
	
	// type modifiers
	case typeTYPEDEF: return "typeTYPEDEF";
	case typePOINTER: return "typePOINTER";
	case typeARRAY: return "typeARRAY";
	case typeSTRUCT: return "typeSTRUCT";
	case typeUNION: return "typeUNION";
	case typeENUM: return "typeENUM";
	case typeCONST: return "typeCONST";
	case typeSIGNED: return "typeSIGNED";
	case typeUNSIGNED: return "typeUNSIGNED";
	case typeFUNCTION: return "typeFUNCTION";
	case typeIDENTIFIER: return "typeIDENTIFIER";
	default: return "<unknown>";
	}
}

ERROR_LOOKUP *LookupError(ERROR err)
{
	int i;

	// lookup error number
	for(i = 0; err_lookup[i].err != ERROR_NONE; i++)
	{
		if(err_lookup[i].err == err)
			break;
	}

	return &err_lookup[i];
}

/*void ErrorRef(ERROR err, FILEREF ref, ...)
{
	va_list vargs;
	char *name = filenameHistory[ref.fileidx];

	printf("%s(%d) : error WIDL%d : ", name, ref.lineno, err);
	
	va_start(vargs, ref);
	vprintf(LookupError(err)->fmt, vargs);
	va_end(vargs);

	errcount++;
}
*/

const char * Parser::LastErrStr()
{
	return errstr;
}

ERROR Parser::LastErr()
{
	return lasterr;
}

void Parser::Error(ERROR err, ...)
{
	va_list vargs;
	char *e = errstr;

	if(curFile->filePath && curFile->filePath[0])
		e += sprintf(e, "%s(%d) : ", curFile->filePath, (int)curFile->curLine);

	e += sprintf(e, "error E%d : ", err);
	
	va_start(vargs, err);
	e += vsprintf(e, LookupError(err)->fmt, vargs);
	va_end(vargs);

	e += sprintf(e, "\n");
	errcount++;

	fprintf(fperr, errstr);
	lasterr = err;

	if(errcallback)
		errcallback(err, errstr, errparam);
} 
/*
bool Warning(ERROR err, ...)
{
	va_list vargs;

	printf("%s(%d) : warning WIDL%d : ", curFile->name, curFile->curline, err);
	
	va_start(vargs, err);
	vprintf(LookupError(err)->fmt, vargs);
	va_end(vargs);

	errcount++;
	return false;
}
*/

bool Parser::Test(TOKEN tok)
{
	if(tok)
	{
		if(t == tok) 
		{
			t = gettok();
		}
		else
		{
			if(t != TOK_NULL)
			{
				if(!Expected(tok))
					return false;
			}
			else
			{
				Unexpected(TOK_NULL);
				return false;
			}
				
			t = gettok();
		}
	}

	return true;
}

//
//	match with current token
//
bool Parser::Expected(TOKEN tok, const char *terminal_desc/*=0*/)
{
	if(t == tok)
	{
		t = gettok();
		return true;
	}
	else
	{
		ERROR err = ERROR_EXPECTED_TOKEN;
		
		//if(t == TOK_IDENTIFIER || t == TOK_NUMBER || t == TOK_STRINGBUF)
		//	err = ERROR_SYNTAX_ERROR2;

		if(terminal_desc == 0)
			terminal_desc = inenglish(tok);//, false);

		Error(err, terminal_desc, inenglish(t, true));
		return false;
	}
}

bool Parser::Expected(int tok, const char *terminal_desc/*=0*/)
{
	return Expected(TOKEN(tok), terminal_desc);
}


void Parser::Unexpected(TOKEN tok)
{
	if(tok == TOK_IDENTIFIER)
		Error(ERROR_UNEXPECTED, tstr);
	else
		Error(ERROR_UNEXPECTED, inenglish(tok, false));
}

void Parser::SetErrorStream(FILE *ferr)
{
	fperr = ferr;
}

void Parser::SetErrorCallback(ERROR_CALLBACK callback, void *param)
{
	errcallback = callback;
	errparam    = param;
}