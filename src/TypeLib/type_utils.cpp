//
//  type_utils.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include "parser.h"
#include "stringprint.h"


#define max(a,b) (a>b?a:b)

extern SymbolTable	globalIdentifierList;
extern SymbolTable  globalTagSymbolList;

int RecurseFlatten(stringprint &sbuf, ExprNode *expr);

//
//	Invert the specified type by reversing the nodes in the chain
//
Type *FindType(Type *type, TYPE ty)
{
	for(; type; type = type->link)
	{
		if(type->ty == ty)
			return type;
	}

	return type;
}

//
//	Invert the specified type by reversing the nodes in the chain
//
Type *InvertType(Type *type)
{
	Type *t = 0;

	while(type)
	{
		Type *next	= type->link;

		type->link	= t;
		t			= type;
		type		= next;
	}

	return t;
}

//
//	Glue two type-chains together
//
Type * AppendType(Type *type, Type *append)
{
	Type *head = type;

	if(head == 0)
		return append;

	for(; type; type = type->link)
	{
		if(type->link == 0)
		{
			type->link = append;
			break;
		}
	}

	return head;
}

//
//	Duplicate the specified type-list
//
Type * CopyType(Type *type)
{
	Type *newType = 0;
	
	// duplicate + invert 
	while(type)
	{
		newType				= new Type(type->ty, newType);
		newType->sym		= type->sym;
		newType->brackets	= type->brackets;

		if(type->ty == typeARRAY)
			newType->elements = CopyExpr(type->elements);

		type = type->link;
	}

	return InvertType(newType);
}


//
//	Return the TYPE of the last node in the type-chain
//
TYPE BaseType(Type *type)
{
	while(type && type->link)
		type = type->link;

	return type ? type->ty : typeNULL;
}

//
//	Return the last node in the type-chain
//
Type *BaseNode(Type *type)
{
	while(type && type->link)
		type = type->link;

	return type;
}


//
//	Free the specified type-chain.
//
void FreeType(Type *type, Type *term = 0)
{
	while(type && type != term)
	{
		Type *link = type->link;
		
		//if(type->ty == 
		delete type;
		type = link;
	}
}

Type *MakeTypeDef(TYPE base, char *name, TYPE base2/* = typeNULL*/)
{
	Symbol *sym;
	Type *type;

	// create the symbol
	sym			= InstallSymbol(globalIdentifierList, name);
	type		= new Type(typeTYPEDEF);
	type->link	= new Type(base);

	if(base2)
		type->link->link = new Type(base2);

	type->sym	= sym;
	sym->type	= type;

	return type;
}


Type *BreakLink(Type *type, Type *term)
{
	while(type->link && type->link != term)
		type = type->link;
	
	type->link = 0;
	return type;
}

void RestoreLink(Type *type, Type *link)
{
	type->link = link;
}


TYPE TokenToType(TOKEN t)
{
	switch(t)
	{
	case TOK_CHAR:		return typeCHAR;
	case TOK_WCHAR:		return typeWCHAR;
	case TOK_BYTE:		return typeBYTE;
	case TOK_WORD:		return typeWORD;
	case TOK_DWORD:		return typeDWORD;
	case TOK_FLOAT:		return typeFLOAT;
	case TOK_DOUBLE:	return typeDOUBLE;
	case TOK_QWORD:		return typeQWORD;
	case TOK_STRUCT:	return typeSTRUCT;
	case TOK_UNION:		return typeUNION;
	case TOK_ENUM:		return typeENUM;
	default:			return typeNULL;
	}
}

TOKEN TypeToToken(TYPE ty)
{
	switch(ty)
	{
	case typeCHAR:		return TOK_CHAR;
	case typeWCHAR:		return TOK_WCHAR;
	case typeBYTE:		return TOK_BYTE;
	case typeWORD:		return TOK_WORD;
	case typeDWORD:		return TOK_DWORD;
	case typeQWORD:		return TOK_QWORD;
	case typeFLOAT:		return TOK_FLOAT;
	case typeDOUBLE:	return TOK_DOUBLE;
	case typeSTRUCT:	return TOK_STRUCT;
	case typeENUM:		return TOK_ENUM;
	case typeUNION:		return TOK_UNION;
	default:			return TOK_NULL;
	}
}

bool CheckRecursion(Structure *parent, TypeDecl *child)
{
	Type *cbt = BaseNode(child->baseType);

	if(cbt->ty == typeSTRUCT || cbt->ty == typeUNION)
	{
		if(cbt->sptr == parent)
			return false;

		for(size_t i = 0; i < cbt->sptr->typeDeclList.size(); i++)
		{
			// check the children as well
			if(!CheckRecursion(parent, cbt->sptr->typeDeclList[i]))
				return false;
		}
	}

	return true;
}

unsigned SizeOf(TypeDecl *td);


/*
unsigned SizeOf(TypeDecl *td)
{
	unsigned size = 0;
	
	if(td->declList.size())
	{
		for(int j = 0; j < td->declList.size(); j++)
		{
			td->declList[j]->offset = size;
			size += SizeOf(td->declList[j]);
		}
	}
	else if(td->nested)
	{
		size = SizeOf(td->baseType);
	}

	return size;
}*/

//
//	pos		- zero based index of where to insert, -1 for insert at end
//
bool AppendTypeDecl(Type *parent, TypeDecl *child, int pos)
{
	unsigned offset = 0;

	// can only add to struct/union types!
	if(parent->ty != typeSTRUCT && parent->ty != typeUNION)
		return false;

	// check that the child type doesn't introduce any kind of recursion
	if(!CheckRecursion(parent->sptr, child))
		return false;
	
	// append the typedecl
	if(pos == -1)
	{
		parent->sptr->typeDeclList.push_back(child);
	}
	// insert the typedecl
	else
	{
		TypeDeclList &tdl = parent->sptr->typeDeclList;
		TypeDeclList::iterator itor;// = parent->sptr->typeDeclList(;
		
		size_t i = 0;
		size_t s = tdl.size();
		for(itor = tdl.begin(); itor != tdl.end(); itor++)
		{
			// not taking the declList into account

			if(i++ == pos)
				break;
		}

		tdl.insert(itor, child);
	}

	// build the fieldlist
	parent->sptr->Build();

	//SizeOf(parent, 0);
	// work out the offsets of each member
	/*for(int i = 0; i < parent->typeDeclList.size(); i++)
	{
		TypeDecl *td = parent->typeDeclList[i];

		
	}*/

	return true;
}


/*
//
//	Invert the type-chain
//
Type * MakeDisplayType(Type *decl)
{
	Type *newType = 0;
	
	// duplicate + invert 
	while(decl)
	{
		newType = new Type(decl->ty, newType);
		newType->sym = decl->sym;
		newType->brackets = decl->brackets;

		decl = decl->link;

		if(newType->ty == typeTYPEDEF)
			break;
	}

	return newType;
}*/

/*
Type * MakeFullType(Type *base, Type *decl)
{
	Type *newType = 0, *ptr, *head = 0;
	
	// duplicate + invert 
	//while(decl)
	//{
	//	newType = new Type(decl->ty, newType);
	//	newType->name = decl->name;
	//
	//	decl = decl->link;
	//}

	while(decl)
	{
		newType = new Type(decl->ty, 0);
		newType->sym  = decl->sym;
		newType->brackets = decl->brackets;

		if(head == 0)
		{
			head = newType;
			ptr  = newType;
		}
		else
		{
			ptr->link = newType;
			ptr = newType;
		}

		decl = decl->link;
	}

	//typeDecl->declList[i] = InvertType(typeDecl->declList[i]);
	//	type = typeDecl->declList[i];

	AppendType(head, base);
	return head;

	//AppendType(newType, base);
	//return newType;
}
*/

void Structure::Build()
{
	fieldList.clear();

	for(size_t i = 0; i < typeDeclList.size(); i++)
	{
		TypeDecl *td = typeDeclList[i];

		if(td->declList.size())
		{
			for(size_t j = 0; j < typeDeclList[i]->declList.size(); j++)
			{
				fieldList.push_back(Field(td->declList[j], td->tagList));
			}
		}
		else
		{
			fieldList.push_back(Field(td->baseType, td->tagList));
		}
	}
}
	

static int RecurseRenderType(stringprint &sbuf, Type *type)
{
	Structure *sptr;

	if(type == 0)
		return 0;

	switch(type->ty)
	{
	case typeSTRUCT: case typeUNION:
		
		sptr = type->sptr;
		
		sbuf._stprintf(TEXT("%hs "), Parser::inenglish(TypeToToken(type->ty)));

		if(!sptr->symbol->anonymous)
			sbuf._stprintf(TEXT("%hs "), sptr->symbol->name); 

		RecurseRenderType(sbuf, type->link);
		
		break;
		
	case typeCHAR:		case typeWCHAR: 
	case typeBYTE:		case typeWORD:
	case typeDWORD:		case typeQWORD:
	case typeFLOAT:		case typeDOUBLE:
		sbuf._stprintf(TEXT("%hs "), Parser::inenglish(TypeToToken(type->ty))); 
		RecurseRenderType(sbuf, type->link);

		break;

	case typeTYPEDEF:		
		sbuf._stprintf(TEXT("%hs "), type->sym->name);
		RecurseRenderType(sbuf, type->link);
		break;
		
	case typeIDENTIFIER:
		sbuf._stprintf(TEXT("%hs"), type->sym->name);
		RecurseRenderType(sbuf, type->link);
		break;

	case typePOINTER:

		if(type->brackets)//sbuf.brackets)
			sbuf._stprintf(TEXT("("));

		sbuf._stprintf(TEXT("*"));
		RecurseRenderType(sbuf, type->link);

		if(type->brackets)//sbuf.brackets)
			sbuf._stprintf(TEXT(")"));

		break;

	case typeENUM:
		sbuf._stprintf(TEXT("enum %hs "), type->eptr->symbol->name);
		RecurseRenderType(sbuf, type->link);
		break;

	case typeARRAY:

		RecurseRenderType(sbuf, type->link);
		sbuf._stprintf(TEXT("["));
		RecurseFlatten(sbuf, type->elements);
		sbuf._stprintf(TEXT("]"));
		break;
		
	default:
		break;
		}
		

	return 0;
}

//
//
//
void RenderType(TCHAR *buf, int len, Type *type)
{
	stringprint sbuf(buf, len);

	Type *restore = 0;
	Type *tptr;

	for(tptr = type; tptr; tptr = tptr->link)
	{
		if(tptr->ty == typeTYPEDEF)
		{
			tptr = tptr->link;
			restore = BreakLink(type, tptr);
			break;
		}
	}

	type = InvertType(type);
	RecurseRenderType(sbuf, type);
	type = InvertType(type);

	if(restore)
		RestoreLink(restore, tptr);
}

bool IsInt(TYPE ty)
{
	switch(ty)
	{
	case typeCHAR: case typeWCHAR: case typeBYTE: 
	case typeWORD: case typeDWORD: case typeQWORD:
		return true;

	default:
		return false;
	}
}

bool IsFloat(TYPE ty)
{
	switch(ty)
	{
	case typeFLOAT: case typeDOUBLE:
		return true;

	default:
		return false;
	}
}

bool IsStruct(TYPE ty)
{
	switch(ty)
	{
	case typeSTRUCT: case typeUNION:
		return true;

	default:
		return false;
	}
}

bool IsStruct(Type *type)
{
	return IsStruct(BaseType(type));
}

bool IsExportedStruct(Type *type)
{
	return IsStruct(BaseType(type)) && type->sptr->exported;
}

Enum * FindEnum(char *enumName)
{
	Symbol *sym;

	if((sym = LookupSymbol(globalTagSymbolList, enumName)) != 0)
	{
		if(sym->type && sym->type->ty == typeENUM)
			return sym->type->eptr;
	}

	if((sym = LookupSymbol(globalIdentifierList, enumName)) != 0)
	{
		Type *baseType = BaseNode(sym->type);
		if(baseType && baseType->ty == typeENUM)
			return baseType ->eptr;
	}

	return 0;
}

void FormatData(TCHAR *buf, int len, char *data, int datalen, bool fHex, bool fSigned, bool fBigEndian, TYPE ty)
{
	stringprint sbuf(buf, len);


}