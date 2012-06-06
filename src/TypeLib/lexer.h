//
//  lexer.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef LEXER_INCLUDED
#define LEXER_INCLUDED

#include <stdlib.h>

#ifdef __cplusplus
#include <vector>
using std::vector;

struct Statement;
typedef vector<Statement *> StatementList;

#endif

typedef enum TOKEN
{
	TOK_NULL	= 0,
	TOK_ILLEGAL = -1,

	// basic tokens
	TOK_IDENTIFIER = 1000,
	TOK_STRINGBUF,
	TOK_INUMBER,
	TOK_FNUMBER,


	// operators
	TOK_SHL			= 2000,
	TOK_SHR,
	TOK_ANDAND,
	TOK_OROR,
	TOK_EQU,
	TOK_NEQ,
	TOK_LE,
	TOK_GE,
	TOK_INC,
	TOK_DEC,
	TOK_DEREF,
	
	// keywords
#undef  DEFINE_KEYWORD
#define DEFINE_KEYWORD(tok, str) tok,
#include "keywords.h"

	TOK_STATIC,
	TOK_EXTERN,

/*	tokAlign= 3000,
	tokBitflag,
	tokCase,
	tokConst,
	tokDisplay,
	tokEndian,
	tokEnum,
	tokIgnore,
	tokInclude,
	tokLengthIs,
	tokOffset,
	tokSigned,
	tokSizeIs,
	tokString,
	tokStruct,
	tokStyle,
	tokSwitchIs,
	tokTypedef,
	tokUnion,
	tokUnsigned,

	alignTok		= 3000,
	bitflagTok,
	caseTok,
	constTok,
	displayTok,
	endianTok,
	enumTok,
	ignoreTok,
	includeTok,
	lengthisTok,
	offsetTok,
	signedTok,
	sizeisTok,
	stringTok,
	structTok,
	styleTok,
	switchisTok,
	typedefTok,
	unionTok,
	unsignedTok,
*/

	// basetypes
	TOK_CHAR		= 4000,
	TOK_BYTE,
	TOK_WORD,
	TOK_DWORD,
	TOK_QWORD,
	TOK_FLOAT,
	TOK_DOUBLE,
	TOK_WCHAR,
} TOKEN;

#ifdef __cplusplus

// token-name lookup
struct TOKEN_LOOKUP
{
	TOKEN tok;
	char *str;
};

// file-descriptor
struct FILE_DESC
{
	FILE_DESC(const char *path, const char *name) : buf(0), len(0), pos(0), curLine(1), wspStart(0), wspEnd(0)
	{
		origPath[0] = '\0';
		filePath[0] = '\0';
		fileName[0] = '\0';

		strncpy_s(filePath, _MAX_PATH, path, _MAX_PATH);
		strncpy_s(fileName, _MAX_PATH, name, _MAX_PATH);
		strcpy_s(origPath, _MAX_PATH, path);
	}


	char		filePath[_MAX_PATH];	// actual path
	char		fileName[_MAX_PATH];	// display name (can change due to preprocessor)
	char		origPath[_MAX_PATH];	// actual path

	size_t		curLine;

	char	*	buf;
	size_t		len;
	size_t		pos;

	size_t		wspStart;
	size_t		wspEnd;
	
	StatementList stmtList;

};

enum NUMTYPE
{
	//UN
};

typedef enum NUMBASE
{
	HEX,// = 16,
	DEC,// = 10,
	OCT,// = 8,
	//FLOAT,// = 1,
} NUMBASE;

#define MAX_STRING_LEN 200

#ifdef __cplusplus

extern struct TOKEN_LOOKUP	toklook[];
extern vector <FILE_DESC*>	globalFileHistory;

// file-reference
struct FILEREF
{
	FILEREF(FILE_DESC *fd = 0) : lineNo(0), fileDesc(0), wspStart(0), wspEnd(0)
	{
		MakeRef(fd);

		//lex_fileref(
		/*if(curFile)
		{
			//stackIdx = fileStack.size() - 1;
			fileDesc = curFile;
			lineNo   = curFile->curLine;
		}*/
	}

	void MakeRef(FILE_DESC *fd)
	{
		if(fd)
		{
			lineNo		= fd->curLine;
			fileDesc	= fd;
			wspStart	= fd->wspStart;
			wspEnd		= fd->wspEnd;
		}
	}


	//int		stackIdx;
	size_t			lineNo;
	FILE_DESC *		fileDesc;

	// whitespace markers
	size_t			wspStart;
	size_t			wspEnd;
};

#endif
#endif

#endif