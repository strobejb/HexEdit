//
//  parser.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef PARSER_INCLUDED
#define PARSER_INCLUDED

#define _CRT_SECURE_NO_DEPRECATE

#include "lexer.h"

typedef unsigned __int64  INUMTYPE;
typedef double			  FNUMTYPE;

#ifdef __cplusplus

#include "error.h"
#include "expr.h"
#include "types.h"
#include "stmt.h"

//#include "lexer.h"
//#include "error.h"
//#include "expr.h"
//#include "types.h"

class Parser
{
public:
	Parser();
	bool Ooof(const char *file);
	void SetErrorStream(FILE *fperr);
	void SetErrorCallback(ERROR_CALLBACK callback, void *param);

	static void Initialize();
	bool Init(const char *file);
	bool Init(const char *buf, size_t len);
	bool Init(const wchar_t *buf, size_t len);
	//int  Parse(const char *file);
	//int  Parse(const char *buf, size_t len);
	int Parse();

	TypeDecl * ParseTypeDecl(Tag *tagList, SymbolTable &symTable, bool nested /*=false*/, bool allowMultiDecl /*=true*/);
	ExprNode * ParseExpression();

	
	static const char *inenglish(TOKEN t);

	ERROR LastErr();
	const char *LastErrStr();

	TOKEN nexttok()
	{
		TOKEN tmp = t;
		t = gettok();
		return tmp;
	}

	INUMTYPE INUM() { return tnum; }
	FNUMTYPE FNUM() { return tfnum; }
	
private:

	Parser(Parser *p);

	// parser
	bool		ParseTags(Tag **tagList, TOKEN allowed[]);
	Statement * ParseInclude();
	void		ExportStructs();
	void		Cleanup();

	// typedecls
	Type	 * ParseBaseType(TypeDecl *typeDecl, bool nested);
	Type	 * ParseStructBody(Symbol *sym, TYPE ty);
	Type	 * ParseEnumBody(Symbol *sym);
	EnumField* AddEnumField(Enum *enumPtr, char *name, ExprNode *expr, unsigned val);
	Type	 * Decl(TOKEN term, SymbolTable &symTable);
	Type	 * PrefixDecl(SymbolTable &symTable);
	Type	 * PostfixDecl(Type *tptr);
	Function * ParseFuncDecl(Symbol *sym);

	// expressions
	ExprNode * PrimaryExpression();
	ExprNode * PostfixExpression(ExprNode *p);
	ExprNode * UnaryExpression(void);
	ExprNode * BinaryExpression(int k);
	ExprNode * ConditionalExpression(void);
	ExprNode * AssignmentExpression(TOKEN term);
	ExprNode * Expression(TOKEN term);
	ExprNode * FullExpression(TOKEN term);
	ExprNode * CommaExpression(TOKEN tok);


	// lexer
	void newline();
	bool lex_initbuf(const char *buffer, size_t len);
	bool lex_init(const char *filename);
	bool file_included(const char *filename);
	void lex_cleanup();
	void lex_fileref(FILEREF *fileRef);
	TOKEN gettok();
	int parse_identifier();
	int parse_string(int term);
	int backslash();
	TOKEN parse_number();
	int skipwhitespace(size_t *startpos, size_t *endpos);
	int preprocess();
	int skipspaces();
	int skipeol();
	int nextch();
	int peekch(int advance = 0);

	// error handling
	const char *inenglish(TOKEN t, bool use_t_state);
	void Error(ERROR err, ...);
	bool Test(TOKEN tok);
	bool Expected(TOKEN tok, const char *terminal_desc = 0);
	bool Expected(int tok, const char *terminal_desc = 0);
	void Unexpected(TOKEN tok);

	// lexer
	int				ch;
	TOKEN			t;
	char			tstr[MAX_STRING_LEN+1];
	INUMTYPE		tnum;
	FNUMTYPE		tfnum;
	NUMBASE			tnum_base;
	int				wspStart;
	int				wspEnd;

	FILE_DESC	*	curFile;


	// parser
	Parser		*	parent;

	// errors
	int				errcount;
	char			errstr[200];
	ERROR			lasterr;
	FILE		  * fperr;
	ERROR_CALLBACK  errcallback;
	void          * errparam;

};

Tag * FindTag(Tag *tag, TOKEN tok, ExprNode **expr /*= 0*/);

extern SymbolTable globalIdentifierList;
extern SymbolTable globalTagSymbolList;

#else

void * AllocParser(const char * buf, size_t len);
TOKEN    nexttok(void * p);
INUMTYPE INUM(void *p);
FNUMTYPE FNUM(void *p);

#endif


#endif