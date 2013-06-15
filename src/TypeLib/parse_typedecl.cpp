//
//  parse_typedecl.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//


#define _CRT_SECURE_NO_DEPRECATE

#include "parser.h"

// #define to enable function-declarations
#define FUNCTION_TYPES

// symbol tables for structs/enums/anything else
extern SymbolTable	globalIdentifierList;
extern SymbolTable  globalTagSymbolList;

// list of all type-declarations as they were encountered
extern TypeDeclList	globalTypeDeclList;

static void Unnamed(char *typeName)
{
	static int unnamed = 0;
	sprintf(typeName, "$noname$%04d", unnamed++);
}

Symbol *InstallSymbol(SymbolTable &table, char *name)
{
	Symbol *sym = new Symbol;
	
	strcpy(sym->name, name);
	table.push_back(sym);

	sym->anonymous	= name[0] == '$' ? true : false;
	sym->type		= 0;
	sym->parent		= &table;
	return sym;
}

Symbol *LookupSymbol(SymbolTable &table, char *name)
{
	Symbol *sym = 0;

	for(size_t i = 0; i < table.size(); i++)
	{
		if(strcmp(table[i]->name, name) == 0)
		{
			sym = table[i];
			break;
		}
	}

	return sym;
}

TypeDecl *LookupTypeDecl(char *name)
{
	TypeDeclList &table = globalTypeDeclList;
	TypeDecl *decl = 0;

	for(size_t i = 0; i < table.size(); i++)
	{
		TypeDecl *t = table[i];
		if(strcmp(t->baseType->sym->name, name) == 0)
		{
			decl = table[i];
			break;
		}
	}

	return decl;
}

void InstallTypeAliases()
{
	MakeTypeDef(typeTIMET,		"time_t", typeDWORD);
	MakeTypeDef(typeDOSTIME,	"DOSTIME", typeWORD);
	MakeTypeDef(typeDOSDATE,	"DOSDATE", typeWORD);
	MakeTypeDef(typeFILETIME,	"FILETIME", typeQWORD);

	//MakeTypeDef(typeQWORD,	"dword");
	//MakeTypeDef(typeDWORD,	"dword");
	//MakeTypeDef(typeWORD,	"word");
	//MakeTypeDef(typeBYTE,	"bte");

	//MakeTypeDef(typeQWORD,	"int64");
	//MakeTypeDef(typeDWORD,	"long");
	//MakeTypeDef(typeDWORD,	"int");
	//MakeTypeDef(typeWORD,	"short");
	//MakeTypeDef(typeBYTE,	"int8");

/*	MakeTypeDef(typeQWORD,	"INT64");
	MakeTypeDef(typeDWORD,	"INT32");
	MakeTypeDef(typeWORD,	"INT16");
	MakeTypeDef(typeBYTE,	"INT8");*/
}


Function * Parser::ParseFuncDecl(Symbol *sym)
{
	if(!Expected('('))
		return 0;

	Function * func = new Function(sym);

	//
	//	scan the function arguments until we hit the end of function
	//
	while(t && t != ')')
	{
		Tag *argTagList = 0;

	/*TOKEN allowed[] = 
		{ 
			TOK_IN,			TOK_OUT, 
			TOK_LENGTHIS, 	TOK_SIZEIS, 	
			TOK_STRING, 	
			TOK_REF,		TOK_UNIQUE,
			TOK_NOXML,
			TOK_NULL 
		};

		// get tags for the function-argument
		if(!ParseTags(&varTagList, allowed2))
			return false;*/

		// Get a single variable declaration and it's tags
		TypeDecl *paramDecl = ParseTypeDecl(argTagList, func->paramSymTable, true, false);

		if(paramDecl == 0)
			return 0;

		func->paramDeclList.push_back(paramDecl);

		if(t == ',')
			t = gettok();
	}

	if(!Expected(')'))
		return 0;

	return func;
}

//
//	Parse postfix decl operators -   
//	   operator []  - array bounds
//	   operator ()  - function prototype
//
Type * Parser::PostfixDecl(Type *tptr)
{
	for(;;)
	{
		ExprNode *elements;
		FILEREF fileref(curFile);

		switch(t)
		{
		// array bounds
		case '[':
			t = gettok();

			if(t != ']')
			{
				if((elements = Expression(TOKEN(']'))) == 0)
					return 0;
			}

			tptr = new Type(typeARRAY, tptr);
			tptr->elements = elements;
			tptr->fileRef  = fileref;

			// break for more dimensions
			break;	

		// function calls not supported!!
		case '(':
#ifdef FUNCTION_TYPES
			tptr			= new Type(typeFUNCTION, tptr);
			tptr->fptr		= ParseFuncDecl(0);
			tptr->fileRef	= fileref;

			return tptr;
#else
			Error(ERROR_NOFUNCPTR);
			return tptr;
#endif

		default:
			tptr->fileRef  = fileref;
			return tptr;
		}
	}
}

//
//	Parse prefix decl operators - '*', 'const' etc
//
//	  operator *  - pointer
//
Type * Parser::PrefixDecl(SymbolTable &symTable)
{
	Type *tptr;
	Symbol *sym;

	FILEREF fileref(curFile);

	switch(t)
	{
	case '*': 

		// pointer declarations
		t = gettok();
		tptr = PrefixDecl(symTable);
		tptr = new Type(typePOINTER, tptr);
		break;

	case TOK_CONST:

		// const-pointer declarations
		t = gettok();
		tptr = PrefixDecl(symTable);
		tptr = new Type(typeCONST, tptr);
		break;

	case TOK_IDENTIFIER:		
		
		// check if symbol already defined
		if((sym = LookupSymbol(symTable, tstr)) == 0)
		{
			sym = InstallSymbol(symTable, tstr);
			sym->fileRef = FILEREF(curFile);
		}
		else
		{
			Error(ERROR_TYPE_REDEFINITION, tstr);
		}

		//if(cc == 184)
		//	cc = cc;

		t = gettok();
		tptr = new Type(typeIDENTIFIER);
		
		tptr->sym = sym;
		sym->type = tptr;

		tptr = PostfixDecl(tptr);
		break;
	
	case '(': 

		// bracketed declaration
		t = gettok();
		tptr = Decl(TOKEN(')'), symTable);
		tptr->brackets = true;

		tptr = PostfixDecl(tptr);
		break;

	default:
		Error(ERROR_SYNTAX_ERROR, inenglish(t));
		//return 0;
		tptr = new Type(typeNULL);
		break;
	}

	tptr->fileRef = fileref;

	return tptr;
}

//
//	Parse a declaration
//
Type * Parser::Decl(TOKEN term, SymbolTable &symTable)
{
	Type *type = PrefixDecl(symTable);

	if(term)
	{
		if(t == term)
			t = gettok();
		else
			Expected(term);
	}

	return type;
}

EnumField * Parser::AddEnumField(Enum *enumPtr, char *name, ExprNode *expr, unsigned val)
{
	Symbol *sym;
	EnumField *field = 0;
		
	// make sure this name isn't already defined. Enum values share
	// the 'global' scope.
	if((sym = LookupSymbol(globalIdentifierList, name)) != 0)
	{
		Error(ERROR_TYPE_REDEFINITION, name);
		return 0;
	}
	else
	{
		// install the field-name
		sym = InstallSymbol(globalIdentifierList, name);

		// add to the enumeration
		field = new EnumField(sym, expr);//, enumPtr);
		enumPtr->fieldList.push_back(field);

		field->val = val;
		//field->neverPrefix

		sym->type = new Type(typeENUMVALUE, 0);
		sym->type->evptr = field;

		// add to the enumeration
		//enumPtr->fieldList.push_back(new EnumField(sym, expr));
	}

	return field;
}

//
//	Parse a *new* enum
//
Type * Parser::ParseEnumBody(Symbol *sym)
{
	Type *type = 0;
	Enum *enumPtr;
	char fieldName[MAX_STRING_LEN];
	
	unsigned int i = 0;

	if(t == '{')
	{
		enumPtr = new Enum(sym);
		enumPtr->postNameRef.MakeRef(curFile);
	}

	if(!Expected('{'))
		return 0;

	for( ; t != '}'; i++)
	{
		ExprNode *expr = 0;
		EnumField *field;

		strcpy(fieldName, tstr);

		FILEREF fileRef1(curFile);

		if(!Expected(TOK_IDENTIFIER))
			return 0;

		FILEREF fileRef2(curFile);

		// handle explicit values
		if(t == '=')
		{
			t = gettok();

			if((expr = Expression(TOK_NULL)) == 0)
				return 0;
			
			i = (unsigned int)Evaluate(expr);
			//i = expr->val;
		}

		// install the symbol and expression in the enumeration
		if((field = AddEnumField(enumPtr, fieldName, expr, i)) == 0)
			return 0;

		field->fileRef	= fileRef1;
		field->after	= fileRef2;

		if(t == ',')
		{
			t = gettok();
			lex_fileref(&field->postRef);
			continue;
		}

		break;
	}

	enumPtr->lastBraceRef.MakeRef(curFile);
	if(t != '}')
	{
		Unexpected(t);
		return 0;
	}

	t = gettok();
//	lex_fileref(&field->postRef);

	sym->type = new Type(typeENUM);
	sym->type->eptr = enumPtr;	
	//globalEnumList.push_back(sym);
	return sym->type;
}

//static int cc;

Type * Parser::ParseStructBody(Symbol *sym, TYPE ty)
{
	Structure	*sptr;
	Tag			*tagList;
	TypeDecl	*typeDecl;

	if(t == '{')
	{
		// create the type
		sptr			= new Structure(sym);
		sym->type		= new Type(ty);
		sym->type->sptr	= sptr;
		sptr->postNameRef.MakeRef(curFile);
	}

	// structure body must start with a '{'
	if(!Expected('{'))
		return 0;

	sptr->postBraceRef.MakeRef(curFile);

	while(t != '}')
	{
		TOKEN allowed[] = 
		{ 
			TOK_LENGTHIS, TOK_SIZEIS, TOK_IGNORE, TOK_STRING,
			TOK_OFFSET, TOK_ALIGN, TOK_BITFLAG, TOK_STYLE, TOK_DISPLAY,
			TOK_ENDIAN,	TOK_SWITCHIS, TOK_CASE, TOK_NAME, 
			TOK_ENUM, TOK_BITFLAG,
			TOK_NULL 

		};

		FILEREF tagRef(curFile);

		if(!ParseTags(&tagList, allowed))
			return 0;

		size_t s;
		s = globalFileHistory.size();

		FILEREF fileRef(curFile);

		//
		//	get the member-variable declaration
		//	nested structures + multi-decls are allowed
		//
		if((typeDecl = ParseTypeDecl(tagList, sptr->symbolTable, true, true)) == 0)
			return 0;

		//  get the optional bitfield
		if(t == ':')
		{
			if(ty == typeUNION)
			{
				Error(ERROR_BITFIELDUNION);
				return 0;
			}

			t = gettok();

			if(!Expected(TOK_INUMBER))
				return 0;
		}

		if(!Expected(';'))
			return 0;

		typeDecl->parent = sym->type;

		// record any whitespace that appears before, and after the type-decl
		typeDecl->fileRef = fileRef;
		typeDecl->tagRef  = tagRef;
		typeDecl->postRef.MakeRef(curFile);
		
		if(!AppendTypeDecl(sym->type, typeDecl, -1))
			return 0;

		//sptr->typeDeclList.push_back(typeDecl);
	}

	sptr->lastBraceRef.MakeRef(curFile);
	if(!Expected('}'))
		return 0;



	//globalStructList.push_back(sym);

	return sym->type;
}

//
//
//
Type * Parser::ParseBaseType(TypeDecl *typeDecl, bool nested)
{
	char    typeName[MAX_STRING_LEN];
	TOKEN   tmp;
	Symbol *sym;
	Type   *type = 0;

	for(;;)
	{
		FILEREF fileref(curFile);

		switch(t)
		{
		// standard basetypes
		case TOK_CHAR:  case TOK_WCHAR: 
		case TOK_FLOAT: case TOK_DOUBLE:
		case TOK_BYTE:  case TOK_WORD: 
		case TOK_DWORD: case TOK_QWORD:

			tmp  = t;
			t    = gettok();
			type = new Type(TokenToType(tmp), type);
			type->fileRef = fileref;
			return InvertType(type);

		// type aliases
		case TOK_IDENTIFIER:

			strcpy(typeName, tstr);
			if(!Expected(TOK_IDENTIFIER))
				return 0;

			// find the type and then duplicate it
			if((sym = LookupSymbol(globalIdentifierList, typeName)) != 0)
			{
				if(sym->type->ty == typeTYPEDEF)
				{
					type = InvertType(type);
					type = AppendType(type, CopyType(sym->type));
					type->fileRef = fileref;
					return type;
				}
			}

			Error(ERROR_NOT_TYPENAME, typeName);
			return 0;

		// enums
		case TOK_ENUM:
	
			t = gettok();

			if(t == TOK_IDENTIFIER)
			{
				strcpy(typeName, tstr);
				t = gettok();
			}
			else
			{
				Unnamed(typeName);
			}

			// new enum declaration
			if(t == '{')
			{
				if(LookupSymbol(globalTagSymbolList, typeName))
				{
					Error(ERROR_TYPE_REDEFINITION, typeName);
					return 0;
				}

				sym = InstallSymbol(globalTagSymbolList, typeName);
			
				if((type = ParseEnumBody(sym)) == 0)
					return 0;

				typeDecl->compoundType = true;
				return type;
			}
			else
			{
				if((sym = LookupSymbol(globalTagSymbolList, typeName)) != 0)
				{
					if(sym->type == 0)
					{
						//Error(ERROR_UNDEFINED);
						return 0;
					}
					else
					{
						return CopyType(sym->type);			
					}
				}
			}
		
			// not found
			Error(ERROR_UNKNOWN_ENUM, typeName);
			return 0;

		// structures and unions
		case TOK_STRUCT: case TOK_UNION:

			tmp = t;
			t = gettok();

			if(t == TOK_IDENTIFIER)
			{
				strcpy(typeName, tstr);
				t = gettok();
			}
			else
			{
				Unnamed(typeName);
			}

			// new structure declaration
			if(t == '{')
			{
				if(LookupSymbol(globalTagSymbolList, typeName))
				{
					Error(ERROR_TYPE_REDEFINITION, typeName);
					return 0;
				}

				sym = InstallSymbol(globalTagSymbolList, typeName);

				size_t s;
				s = globalFileHistory.size();

			
				if((type = ParseStructBody(sym, TokenToType(tmp))) == 0)
					return 0;

				//if(exportType) type->sptr->exported  = true;
				typeDecl->compoundType = true;
				return type;
			}
			else
			{
				if((sym = LookupSymbol(globalTagSymbolList, typeName)) != 0)
				{
					if(sym->type == 0)
					{
						Error(ERROR_UNDEFINED_STRUCT, sym->name);
						return 0;
					}
					else
					{
						return CopyType(sym->type);			
					}
				}
			}
		
			// not found
			Error(ERROR_UNKNOWN_STRUCT, typeName);
			return 0;
	
		// type modifiers
		case TOK_CONST:
			t = gettok();
			type = new Type(typeCONST, type);
			break;

		case TOK_SIGNED:
			t = gettok();
			type = new Type(typeSIGNED, type);
			break;

		case TOK_UNSIGNED:
			t = gettok();
			type = new Type(typeUNSIGNED, type);
			break;

		default:
			Error(ERROR_EXPECTED_TYPENAME);
			return 0;
		}
	}
}

//
//	Parse a type-declaration. 
//
//	table - symbol table in which to store any identifiers
//
TypeDecl * Parser::ParseTypeDecl(Tag *tagList, SymbolTable &symTable, bool nested /*=false*/, bool allowMultiDecl /*=true*/)
{
	TypeDecl *	typeDecl		= new TypeDecl();
	//bool		allowMultiDecl	= true;

	typeDecl->fileRef = FILEREF(curFile);

	switch(t)
	{
	// all built-in types / aliased types
	case TOK_BYTE:   case TOK_WORD: 
	case TOK_DWORD:  case TOK_QWORD:
	case TOK_CHAR:   case TOK_WCHAR:
	case TOK_FLOAT:  case TOK_DOUBLE:
	case TOK_STRUCT: case TOK_UNION:	
	case TOK_SIGNED: case TOK_UNSIGNED:
	case TOK_ENUM:	 case TOK_CONST: 
	case TOK_IDENTIFIER:
	case TOK_EXPORT:
		
		// parse the base-type portion of the type declaration
		if((typeDecl->baseType = ParseBaseType(typeDecl, nested)) == 0)
			return 0;

		typeDecl->typeAlias = false;
		break;
	
	// typedefs *statements* have slightly different rules
	case TOK_TYPEDEF:
		
		t = gettok();

		// parse the base-type as normal
		if((typeDecl->baseType = ParseBaseType(typeDecl, nested)) == 0)
			return 0;

		typeDecl->typeAlias = true;
		break;

	// anything else is a syntax error
	default:
		Error(ERROR_EXPECTED_TYPENAME);
		return 0;
	}

	typeDecl->tagList			= tagList;
	typeDecl->nested			= nested;

	// now parse any identifiers
	while(t && t != ';')
	{
		Type *type;
		
		//
		//	Parse the declaration. If this is a 'typedef' (a type-alias) then 
		//	the symbols must be installed in the 'global' identifier table. 
		//
		//	Otherwise use the symbol-table that the caller supplied.
		//
		if((type = Decl(TOK_NULL, (typeDecl->typeAlias ? globalIdentifierList : symTable))) == 0)
			return 0;

		if(errcount > 0)
		{
			delete type;
			return 0;
		}

		// glue the basetype and variable-declaration together
		type = AppendType(InvertType(type), typeDecl->baseType);

		// Change typeIDENTIFIER to typeTYPEDEF when appropriate
		if(typeDecl->typeAlias)
			type->ty = typeTYPEDEF;

		// structure bit-fields
		if(t == ':')
		{
			t = gettok();

			if(!Expected(TOK_INUMBER))
				return 0;
		}

		// store the type declaration
		type->parent = typeDecl;
		typeDecl->declList.push_back(type);
		
		if(t == ',' && allowMultiDecl)
		{
			t = gettok();
			continue;
		}
		else
		{
			break;
		}
	}

	return typeDecl;
}
