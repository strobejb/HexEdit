//
//  types.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef TYPE_INCLUDED
#define TYPE_INCLUDED

enum TYPE
{
	typeNULL		= 0,

	// basic types
	typeCHAR,
	typeWCHAR,
	typeBYTE,
	typeWORD,
	typeDWORD,
	typeQWORD,
	typeFLOAT,
	typeDOUBLE,

	// others
	typeDOSTIME,
	typeDOSDATE,
	typeTIMET,
	typeFILETIME,

	typeENUMVALUE,

	typeMODIFIERSTART = 100,
	
	// type modifiers
	typeTYPEDEF,
	typePOINTER,
	typeARRAY,
	typeSTRUCT,
	typeUNION,
	typeENUM,
	typeCONST,
	typeSIGNED,
	typeUNSIGNED,
	typeFUNCTION,
	typeIDENTIFIER,
};

#ifdef __cplusplus

#include <tchar.h>
#include <vector>
using std::vector;

struct ExprNode;
struct Symbol;
struct Enum;
struct EnumField;
struct Structure;
struct Function;
struct Type;
struct TypeDecl;
struct FILEREF;

typedef vector<Symbol *> SymbolTable;
typedef vector<Type *> TypeList;
typedef vector<TypeDecl *> TypeDeclList;

Symbol *InstallSymbol(SymbolTable &table, char *name);
Symbol *LookupSymbol(SymbolTable &table, char *name);

TypeDecl *LookupTypeDecl(char *name);
Type *BreakLink(Type *type, Type *term);

template <typename t>
void EmptyVector(t v)
{
	for(size_t i = 0; i < v.size(); i++)
		delete v[i];

	v.clear();
}

/*template <class type>
class Container
{
public:
	~Container()
	{
		printf("auto!\n");
		for(size_t i = 0; i < v.size(); i++)
			delete v[i];

		v.clear();
	}

	size_t size()
	{
		return v.size();
	}

	type * operator [] ( size_t idx)
	{
		return v[idx];
	}

	void push_back(type * const x)
	{
		v.push_back(x);
	}

	operator vector<type *> & ()
	{
		return v;
	}

private:
	vector<type*> v;
};
*/


struct Type;

// symbol
struct Symbol
{
	char		name[MAX_STRING_LEN];
	bool		anonymous;
	FILEREF	fileRef;

	Type		* type;
	SymbolTable	* parent;
};

struct Tag
{
	Tag(TOKEN t, Tag *l = 0, ExprNode *e = 0) : tok(t), link(l), expr(e)
	{
	}

	~Tag()
	{
		delete expr;
		delete link;
	}

	TOKEN			tok;
	Tag			*	link;
	ExprNode	*	expr;
};

struct Type
{
	Type(TYPE t, Type *l=0) : ty(t), link(l), brackets(false), elements(0), parent(0), offset(0)
	{
		static int i;
		id=i++;
		//extern Type *smegHead[];
		//smegHead[id]=this;
	}

	~Type()
	{
		if(ty == typeARRAY)
			delete elements;

		//extern Type *smegHead[];
		//smegHead[id]=0;

		//if(ty == typeIDENTIFIER || ty == typeTYPEDEF)
		//	printf("**del** %d %s\n", id, sym->name);
		//else
		//	printf("**del** %d\n", id);
		
		delete link;
	}

	int			id;
	TYPE		ty;
	Type *		link;
	bool		brackets;
	FILEREF		fileRef;

	union
	{
		ExprNode  * elements;		// typeARRAY
		Structure * sptr;			// typeSTRUCT
		Enum	  * eptr;			// typeENUM
		EnumField * evptr;			// typeENUMVALUE
		Function  * fptr;			// typeFUNCTION
		Symbol	  * sym;			// typeIDENTIFIER/typeTYPEDEF
	};

	TypeDecl *			parent;
	unsigned __int64	offset;		// only
};

void PrintType(Type *type);

struct TypeDecl
{
	TypeDecl() : baseType(0), tagList(0), comment(0), parent(0), typeAlias(false), nested(false), compoundType(false), exported(true)
	{
	}

	~TypeDecl()
	{
		for(size_t i = 0; i < declList.size(); i++)
		{
			Type *type = declList[i];
			
			BreakLink(type, baseType);
//
		//	printf("  deleting [%d] %s : ", t->id, t->sym->name);
		//	PrintType(t);
		//	printf("\n");
			
			delete type;
		}

	//	printf("deleting base: ");
//		PrintType(baseType);
		//printf("\n\n");
		delete baseType;
		delete tagList;
	}

	Type		*	baseType;
	TypeList		declList;
	Tag			*	tagList;

	FILEREF			fileRef;
	FILEREF			tagRef;
	FILEREF			postRef;
	char		*	comment;

	Type		*	parent;
	bool			typeAlias;
	bool			nested;
	bool			compoundType;
	bool			exported;
};


struct EnumField
{
	EnumField(Symbol *s, ExprNode *e) : name(s), expr(e), val(0)
	{
	}

	~EnumField()
	{
		// just delete the enum value-expression. It's destructor will
		// recursively delete all child nodes.
		//
		// Note that the 'name' symbol-table entry belongs to the global 
		// 'identifier list' so we must not delete that one here
		//
		delete expr;
	}

	Symbol	 *	name;
	ExprNode *	expr;
	INUMTYPE	val;

	FILEREF	fileRef;
	FILEREF	after;
	FILEREF		postRef;

};

struct Enum
{
	Enum(Symbol *sym) : symbol(sym)
	{
	}

	~Enum()
	{
		EmptyVector(fieldList);
	}

	// symbol for the enum-name and type
	Symbol		*	symbol;
	vector <EnumField*> fieldList;

	FILEREF	postNameRef;
	FILEREF	lastBraceRef;
	//Container<EnumField> fieldList;
};

struct Field
{
	Field(Type *type, Tag *tag) : typeList(type), tagList(tag), offset(0) 
	{
	}

	Type *typeList;
	Tag  *tagList;
	unsigned offset;
};

struct Structure
{
	Structure(Symbol *sym) : symbol(sym), tagList(0), exported(true)
	{
		

		//for(size_t i = 0; i < typeDeclList.size(); i++)
		//	delete typeDeclList[i];
	}

	~Structure()
	{
		//printf("deleting STRUCT %s\n", symbol->name);

		delete tagList;

		EmptyVector(symbolTable);
		EmptyVector(typeDeclList);
	}
	
	Symbol				*symbol;		// name of the structure
	Tag					*tagList;		// tags that apply to the struct type

	//Container<EnumField> fieldList;
	SymbolTable			 symbolTable;//fieldList;
	TypeDeclList		 typeDeclList;
	
	//bool Evaluate(ExprNode *expr, unsigned *result);

	void Build();

	vector<Field>		fieldList;

	FILEREF				 postNameRef;
	FILEREF				 lastBraceRef;
	FILEREF				 postBraceRef;

	bool				 exported;
};

struct Function
{
	Function(Symbol *sym) : symbol(sym), tagList(0) 
	{
	}

	~Function()
	{
		delete tagList;

		EmptyVector(paramSymTable);
		EmptyVector(paramDeclList);
	}

	Symbol			*	symbol;			// name of function
	Tag				*	tagList;		// tags that apply to this function
	
	SymbolTable			paramSymTable;		// argument name list
	TypeDeclList		paramDeclList;	// argument declarations
};


Type *FindType(Type *type, TYPE ty);
Type *CopyType(Type *type);
Type *MakeTypeDef(TYPE base, char *name, TYPE base2 = typeNULL);
Type *BaseNode(Type *type);
TYPE BaseType(Type *type);

TOKEN TypeToToken(TYPE ty);
TYPE  TokenToType(TOKEN tok);

bool IsInt(TYPE ty);
bool IsFloat(TYPE ty);
bool IsStruct(TYPE ty);
bool IsStruct(Type *type);
bool IsExportedStruct(Type *type);
Enum * FindEnum(char *enumName);

void RenderType(TCHAR *buf, int len, Type *type);

bool AppendTypeDecl(Type *parent, TypeDecl *child, int pos);
Type *	InvertType(Type *type);
Type *	AppendType(Type *type, Type *append);

//void FormatData(char *data, int len, T

#endif

#endif
