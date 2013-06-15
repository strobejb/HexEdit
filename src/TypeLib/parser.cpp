//
//  parser.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_DEPRECATE

#include "parser.h"

TypeDecl * ParseTypeDecl(Tag *tagList, SymbolTable &table, bool nested = false, bool allowMultiDecl = true);

// Symbol-tables for C-language constructs. Note that we have separate tables 
// for structs/unions/enums - this is in accordance with C-language rules for identifier namespace rules
SymbolTable globalIdentifierList;
SymbolTable globalTagSymbolList;

// all global-scope type declarations are stored here
TypeDeclList globalTypeDeclList;

void InstallTypeAliases();
const char *inenglish(TYPE ty);

Tag * FindTag(Tag *tag, TOKEN tok, ExprNode **expr /*= 0*/)
{
	for( ; tag; tag = tag->link)
	{
		if(tag->tok == tok)
		{
			if(expr)
				*expr = tag->expr;

			return tag;
		}
	}

	if(expr)
		*expr = 0;
	return 0;
}


//
//	Parse any IDL-style tags, return into the Tag* linked-list
//
bool Parser::ParseTags(Tag **tagList, TOKEN allowed[])
{
	Tag *	tag = 0;
	bool	foundtag = false;
	int		i;
	TOKEN	tmp;
	ExprNode *expr;

	if(t != '[')
	{
		*tagList = 0;
		return true;
	}

	t = gettok();

	while(t != ']')
	{
		foundtag = true;

		//
		//	Make sure that the tag is allowed
		//
		for(i = 0; allowed[i]; i++)
		{
			if(allowed[i] == t)
				break;
		}

		if(allowed[i] == TOK_NULL)
		{
			if(t == TOK_IDENTIFIER)
				Error(ERROR_NOTA_TAG, tstr);
			else
				Error(ERROR_ILLEGAL_TAG, tstr);

			return false;
		}

		//
		//	Parse the tag and any parameters it might have
		//
		switch(t)
		{
		// TAGS which don't take any parameters
		case TOK_IGNORE:	case TOK_STRING:	case TOK_EXPORT:
			tag = new Tag(t, tag);
			t = gettok();
			break;

		// TAGS which take expression-parameters
		case TOK_OFFSET:	case TOK_ALIGN:	 
		case TOK_BITFLAG:	case TOK_ENDIAN:
		case TOK_SIZEIS:	case TOK_LENGTHIS: 	
		case TOK_STYLE:		case TOK_SWITCHIS:
		case TOK_CASE:		case TOK_DISPLAY:
		case TOK_NAME:		case TOK_ENUM:
		case TOK_ASSOC:

			tmp = t;
			t = gettok();

			// size_is(expression)
			if(!Expected('('))
				return false;

			if(tmp == TOK_SIZEIS || tmp == TOK_LENGTHIS || tmp == TOK_ASSOC)
			{
				// full comma-separated expression 
				if((expr = CommaExpression(TOK_NULL)) == 0)
				{
					Error(ERROR_SYNTAX_ERROR, inenglish(t));
					return false;
				}
			}
			else
			{
				// simple expression (number/string)
				if((expr = Expression(TOK_NULL)) == 0)
					return false;
			}

			tag = new Tag(tmp, tag, expr);

			if(!Expected(')'))
				return false;

			break;

		default:
			return false;
		}

		// comma's mean another parameter so loop again
		if(t == ',')
		{
			t = gettok();
			continue;
		}
		// anything else is an error
		else if(t != ']')
		{
			Expected(',');
			return false;
		}
	}

	if(!Expected(']'))
		return false;

	*tagList = tag;

	return true;
}

//
//	Parse any 'include "filename"; ' statements
//	
Statement * Parser::ParseInclude()
{
	char fileName[MAX_STRING_LEN];

	// 'include'
	if(!Expected(TOK_INCLUDE))
		return 0;

	// "filename"
	strcpy(fileName, tstr);
	if(!Expected(TOK_STRINGBUF))
		return 0;

	// terminating semi-colon
	if(t == ';')
	{
		// initialize the lexical-analyser with this new file
		if(file_included(fileName) == false)
		{
			Parser p(this);
			p.SetErrorStream(fperr);
		
			if(!p.Ooof(fileName))
			{
				errcount += p.errcount;
				lasterr   =  p.lasterr;
				strcpy(errstr, p.errstr);
				return 0;
			}
		}

		Expected(';');
	}
	else
	{
		Expected(';');
		return 0;
	}

	// start parsing!
	//t = gettok();
	return new Statement(_strdup(fileName));
}

void Parser::Initialize()
{
	// install DWORD, WORD, BYTE etc
	InstallTypeAliases();
}

bool Parser::Init(const char *file)
{
//	fileStack.clear();

	if(!lex_init(file))
	{
		return false;
	}

	t = gettok();

	return true;
}

bool Parser::Init(const char *buf, size_t len)
{	
//	fileStack.clear();
	if(!lex_initbuf(buf, len))
		return false;

	t = gettok();

	return true;
}

bool Parser::Init(const wchar_t *buf, size_t len)
{
	char *b = new char[len+1];
	wcstombs(b, buf, len);
	b[len] = '\0';
	return Init(b, len);
}

bool Parser::Ooof(const char *file)
{
	if(!Init(file))
		return false;

	return Parse() ? true : false;
}

//
//	associate("*.txt", "*.exe");
//
/*
bool Parser::ParseAssociate()
{
	char str[MAX_STRING_LEN];

	if(!Expected(TOK_ASSOC))
		return false;

	if(!Expected('('))
		return false;

	for(;;)
	{
		strcpy(str, tstr);
		if(!Expected(TOK_STRINGBUF))
			return 0;

		if(t == ',')
		{
			t = gettok();
		}
		else if(t == ')')
		{
			break;
		}
		else
		{
			Unexpected(t);
			return false;
		}
	} 

	if(!Expected(')'))
		return false;

	if(!Expected(';'))
		return false;

	return true;
}
*/

int Parser::Parse()
{
	Tag *tagList;
	TypeDecl *typeDecl;
	Statement *stmt;


	// keep going until there are no more tokens!
	while(t)
	{
		TOKEN allowed[] = 
		{ 
			TOK_LENGTHIS, TOK_SIZEIS, TOK_IGNORE, TOK_STRING,
			TOK_OFFSET, TOK_ALIGN, TOK_BITFLAG, TOK_STYLE, TOK_DISPLAY,
			TOK_ENDIAN,	TOK_SWITCHIS, TOK_CASE, TOK_NAME, 
			TOK_ENUM, TOK_EXPORT, TOK_ASSOC,
			TOK_NULL 

		};

		// save any whitespace before the tags
		FILEREF fileRef;
		lex_fileref(&fileRef);

		if(!ParseTags(&tagList, allowed))
			return 0;
		
		//
		//	Decide what kind of statement/construct we need to parse 
		//
		switch(t)
		{
		case TOK_INCLUDE:
			
			// include-statement (not the same as #include which
			// is a C-preprocessor thing)
			if((stmt = ParseInclude()) == 0)
			{
				return 0;
			}

			curFile->stmtList.push_back(stmt);

			break;

		default:

			size_t s1 = globalFileHistory.size();

			// anything else must be a type-declaration
			if((typeDecl = ParseTypeDecl(tagList, globalIdentifierList, false, true)) == 0)
				return 0;

			size_t s2 = globalFileHistory.size();


			// store in the global list
			typeDecl->fileRef = fileRef;
			globalTypeDeclList.push_back(typeDecl);

			//curFile->typeDeclList.push_back(typeDecl);

			curFile->stmtList.push_back(new Statement(typeDecl));

			// every type-declaration must end with a ';'
			if(!Expected(';'))
				return 0;

			// record any whitespace that appears after the type-decl
			lex_fileref(&typeDecl->postRef);
			 
			break;
		}
	}

	ExportStructs();

	return (errcount == 0) ? 1 : 0;
}

void Parser::ExportStructs()
{
	bool foundExport = false;

	// go through every struct that we parsed in THIS file
	for(size_t i = 0; i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];

		if(typeDecl->baseType->ty == typeSTRUCT && typeDecl->fileRef.fileDesc == curFile)
		{
			if(FindTag(typeDecl->tagList, TOK_EXPORT, 0))
				foundExport = true;
		}
	}

	if(foundExport)
	{
		// clear the export flag on all structs
		for(size_t i = 0; i < globalTypeDeclList.size(); i++)
		{
			TypeDecl *typeDecl = globalTypeDeclList[i];

			if(typeDecl->baseType->ty == typeSTRUCT && typeDecl->fileRef.fileDesc == curFile)
			{
				typeDecl->exported = false;
				typeDecl->baseType->sptr->exported = false;
			}
		}

		// set the export flag on explicity defined structs (with the "export" attribute)
		for(size_t i = 0; i < globalTypeDeclList.size(); i++)
		{
			TypeDecl *typeDecl = globalTypeDeclList[i];

			if(typeDecl->baseType->ty == typeSTRUCT && typeDecl->fileRef.fileDesc == curFile)
			{
				if(FindTag(typeDecl->tagList, TOK_EXPORT, 0))
				{
					typeDecl->exported = true;
					typeDecl->baseType->sptr->exported = true;
				}
			}
		}
	}
}

void Parser::Cleanup()
{
	size_t i;


	// delete all symbols first
	for(i = 0; i < globalIdentifierList.size(); i++)
	{
		Symbol *s = globalIdentifierList[i];
//		delete s;
	}

	for(i = globalTagSymbolList.size(); i > 0; i--)
	{
		Symbol *s = globalTagSymbolList[i - 1];
		
		if(s->type)
		{
			if(s->type->ty == typeENUM)
				delete s->type->eptr;
			else						
				delete s->type->sptr;
		}
		
//		delete s;
	}

	for(i = 0; i < globalTypeDeclList.size(); i++)
	{
		delete globalTypeDeclList[i];
	}


	lex_cleanup();

	/*for(i = 0; i < 100; i++)
	{
		Type *type = smegHead[i];
		if(type)
		{
			printf("[%d] %s", i, ::inenglish(type->ty));

			if(type->ty == typeIDENTIFIER || type->ty == typeTYPEDEF)
			{
				printf("   %s\n", type->sym->name);
			}
			else
			{
				printf("\n");
			}
		}
	}*/

}

Parser::Parser()
{
	curFile		= 0;
	fperr		= stderr;
	parent		= 0;
	errcallback = 0;
}

Parser::Parser(Parser *p)
{
	curFile		= 0;
	fperr		= stderr;
	parent		= p;
	errcallback = 0;
}



extern "C" void * AllocParser(const char *buf, size_t len)
{
	Parser *p = new Parser();

	if(p && p->Init(buf, len))
	{
		return (void *)p;
	}
	else
	{
		delete p;
		return 0;
	}
}

extern "C" TOKEN nexttok(void *p)
{
	Parser *parser = (Parser *)p;

	return parser->nexttok();
}

extern "C" INUMTYPE INUM(void *p)
{
	Parser *parser = (Parser *)p;
	return parser->INUM();
}

extern "C" FNUMTYPE FNUM(void *p)
{
	Parser *parser = (Parser *)p;
	return parser->FNUM();
}

