//
//  display.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _CRT_SECURE_NO_DEPRECATE
#include <ctype.h>

#include "Parser.h"

TYPE TokenToType(TOKEN t);
TOKEN TypeToToken(TYPE t);

size_t DisplayTypeDecl(FILE *fp, TypeDecl *typeDecl, int indent);
Type *InvertType(Type *);
Type *MakeFullType(Type *base, Type *decl);
extern TypeDeclList globalTypeDeclList;

const char * inenglish(TYPE ty);

bool LocateComment(FILEREF *fileRef, char **s, char **cs, char **ce, char **e)
{
	if(fileRef->wspEnd <= fileRef->wspStart)
		return false;

	FILE_DESC *fileDesc = fileRef->fileDesc;
	
	char *s0 = fileDesc->buf + fileRef->wspStart;
	char *s1 = fileDesc->buf + fileRef->wspStart;
	char *s2 = fileDesc->buf + fileRef->wspEnd;

	*s = s0;
	
	// skip over space/tabs up to the first '/'
	while(s1 < s2 && isspace(*s1))
		s1++;

	// has to be either '//' or '/*'
	s1 += 2;

	// locate start of first word
	while(s1 < s2 && isspace(*s1))
		s1++;

	*cs = s0 = s1;

	// locate up to end of line
	while(s1 < s2 && *s1 != '\n')
		s1++;

	while(s1 > s0 && isspace(*(s1-1)))
		s1--;

	// strip 'whitespace' from end
	if(s1 > s0 && *(s1-1) == '/')
	{
		s1--;
		if(s1 > s0 && *(s1-1) == '*')
			s1--;
	}

	while(s1 > s0 && isspace(*(s1-1)))
		s1--;

	*ce = s1;
	*e = s2;
	return true;
}

int DisplayWhitespace(FILE *fp, FILEREF *fileRef, char *newcomment)
{
	char *s, *cs, *ce, *e;
	if(LocateComment(fileRef, &s, &cs, &ce, &e))
	{
		fprintf(fp, "%.*s", (int)(cs-s), s);
		fprintf(fp, newcomment);
		fprintf(fp, "%.*s", (int)(e-ce), ce);
	}

/*	if(fileRef->wspEnd > fileRef->wspStart)
	{
		FILE_DESC *fileDesc = fileRef->fileDesc;//fileHistory[fileRef->stackIdx];
		
		char *s0 = fileDesc->buf + fileRef->wspStart;
		char *s1 = fileDesc->buf + fileRef->wspStart;
		char *s2 = fileDesc->buf + fileRef->wspEnd;

		// skip over space/tabs up to the first '/'
		while(s1 < s2 && isspace(*s1))
			s1++;

		s1 += 2;
		while(s1 < s2 && isspace(*s1))
			s1++;

		fprintf(fp, "%.*s", s1-s0, s0);

		fprintf(fp, newcomment);

		// skip over first line of comment
		while(s1 < s2 && *s1 != '\n')
			s1++;

		fprintf(fp, "%.*s", s2-s1, s1);
	}*/

	return 0;
}

int DisplayWhitespace(FILE *fp, FILEREF *fileRef)
{
	if(fileRef->wspEnd > fileRef->wspStart)
	{
		FILE_DESC *fileDesc = fileRef->fileDesc;
		
		char *s1 = fileDesc->buf + fileRef->wspStart;
		char *s2 = fileDesc->buf + fileRef->wspEnd;

		//while(s1 < s2 && isspace(*s1))
		//	s1++;

		//while(s2 > s1 && isspace(*(s2-1)))
		//	s2--;

		return fprintf(fp, "%.*s", (int)(s2-s1), s1);
	}

	return 0;
}

size_t DisplayTags(FILE *fp, Tag *tagList)
{
	size_t len = 0;

	if(tagList)
	{
		Tag *tag;
		len += fprintf(fp, "[");
		
		for(tag = tagList; tag; tag = tag->link)
		{
			len += fprintf(fp, "%s", Parser::inenglish(tag->tok));

			if(tag->expr)
			{
				len += fprintf(fp, "(");
				len += Flatten(fp, tag->expr);
				len += fprintf(fp, ")");
			}

			if(tag->link)
				len += fprintf(fp, ", ");
		}

		len += fprintf(fp, "]");
	}

	return len;
}

void PrintType(Type *type)
{
	if(type->ty == typeIDENTIFIER || type->ty == typeTYPEDEF)
		printf("'%s' :-  ", type->sym->name);

	while(type)
	{
		switch(type->ty)
		{
		case typePOINTER:	printf(" POINTER(*) -> "); break;
		case typeARRAY:		printf(" ARRAY[%d] -> ", (int)Evaluate(type->elements)); break;
		case typeTYPEDEF:	printf(" TYPEDEF(%s) -> ", type->sym->name); break;
		case typeSTRUCT:	printf(" STRUCT(%s) ", type->sptr->symbol->name);	break;
		case typeENUM:		printf(" ENUM(%s) ", type->eptr->symbol->name);		break;
		case typeFUNCTION:	printf(" FUNC-returning(%s)", type->fptr->symbol->name);		break;	

		case typeCHAR:		case typeWCHAR: 
		case typeBYTE:		case typeWORD:
		case typeDWORD:		case typeQWORD:
			printf("%s ", Parser::inenglish(TypeToToken(type->ty))); 
			break;

		case typeUNSIGNED:	printf(" UNSIGNED -> "); break;
		case typeSIGNED:	printf(" SIGNED -> "); break;
		case typeCONST:		printf(" CONST -> "); break;

		case typeIDENTIFIER:
			//printf("%s ", type->var->varName);
			//printf("ID(%s) > ", type->sym->name);
			break;
		}

		type = type->link;
	}

	//printf("\n");
}

int print_indent(FILE *fp, int level)
{
	int len = 0;

	while(level--)
		len += fprintf(fp, "    ");

	return len;
}

bool fPreserveWhitespace = true;

size_t RecurseDisplayType(FILE *fp, TypeDecl *typeDecl, Type *type, int indent, int padtype)
{
	Enum		*eptr;
	Structure	*sptr;
	Function	*fptr;

	size_t i;
	size_t len = 0;

	if(type)
	{
		if(type->brackets)
			len += fprintf(fp, "(");

		switch(type->ty)
		{
		case typeIDENTIFIER:

			if(fPreserveWhitespace)
				DisplayWhitespace(fp, &type->fileRef);
			len += fprintf(fp, "%s", type->sym->name);
			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;

		case typePOINTER:	
			
			len += fprintf(fp, "*"); 
			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;

		case typeARRAY:		

			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			len += fprintf(fp, "[");
			len += Flatten(fp, type->elements);
			len += fprintf(fp, "]");
			break;

		case typeENUM:

			eptr = type->eptr;

			len += fprintf(fp, "enum %s ", eptr->symbol->anonymous ? "" : eptr->symbol->name);

			if(typeDecl->compoundType)
			{
				if(fPreserveWhitespace)
				{
					len += DisplayWhitespace(fp, &eptr->postNameRef);
					len += fprintf(fp, "{");
					//len += DisplayWhitespace(fp, &eptr->postBraceRef);
					//len += DisplayWhitespace(fp, 
				}
				else
				{
					len += fprintf(fp, "\n");
					len += print_indent(fp, indent);
					len += fprintf(fp, "{\n");
				}
				
				for(i = 0; i < eptr->fieldList.size(); i++)
				{
					if(fPreserveWhitespace)
						len += DisplayWhitespace(fp, &eptr->fieldList[i]->fileRef);
					else
						len += print_indent(fp, indent+1);

					len += fprintf(fp, "%s", (char *)eptr->fieldList[i]->name);
					
					if(eptr->fieldList[i]->expr)
					{
						DisplayWhitespace(fp, &eptr->fieldList[i]->after);
						len += fprintf(fp, " = ");
						len += Flatten(fp, eptr->fieldList[i]->expr);
					}
					
					if(i < eptr->fieldList.size() - 1)
						len += fprintf(fp, ","); // ",\n")
					else
						len += fprintf(fp, "");	 // "\n"

					//DisplayWhitespace(fp, &eptr->fieldList[i]->postRef);
				}
				
				if(fPreserveWhitespace)
					len += DisplayWhitespace(fp, &eptr->lastBraceRef);
				else
					len += print_indent(fp, indent);
				len += fprintf(fp, "}");
			}

			if(type->link)
				len += fprintf(fp, " ");

			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;

		case typeTYPEDEF:	
			len += fprintf(fp, "%s", type->sym->name); 
			break;

		case typeFUNCTION:

			// display the return-type
			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			
			fptr = type->fptr;

			len += fprintf(fp, " (\n");//, fptr->symbol->name);

			for(i = 0; i < fptr->paramDeclList.size(); i++)
			{
				TypeDecl *typeDecl = fptr->paramDeclList[i];
				len += DisplayTypeDecl(fp, typeDecl, indent+1);

				if(i < fptr->paramDeclList.size() - 1)
				{
					len += fprintf(fp, ",\n");
				}
			}

			len += fprintf(fp, "\n)");
			break;

		case typeSTRUCT: case typeUNION:
			
			sptr = type->sptr;

			FILE_DESC *fileDesc;
			fileDesc = sptr->symbol->fileRef.fileDesc;

			if(type->ty == typeUNION)
				len += fprintf(fp, "union");
			else
				len += fprintf(fp, "struct");

			len += fprintf(fp, " %s", sptr->symbol->anonymous ? "" : sptr->symbol->name); 
			
			if(typeDecl->compoundType)
			{
				if(fPreserveWhitespace)
				{
					len += DisplayWhitespace(fp, &sptr->postNameRef);
					len += fprintf(fp, "{");
					len += DisplayWhitespace(fp, &sptr->postBraceRef);
				}
				else
				{
					len += fprintf(fp, "\n");
					len += print_indent(fp, indent);
					len += fprintf(fp, "{\n");
				}
				
				for(i = 0; i < sptr->typeDeclList.size(); i++)
				{
					TypeDecl *typeDecl = sptr->typeDeclList[i];

					if(typeDecl->nested && typeDecl->compoundType && i > 0 && !fPreserveWhitespace)
					{
						len += fprintf(fp, "\n");
					}

					//len += DisplayWhitespace(fp, &typeDecl->fileRef);
					len += DisplayTypeDecl(fp, typeDecl, indent+1);
					len += fprintf(fp, ";");//\n");
					//len += DisplayWhitespace(fp, &typeDecl->postRef);

					if(typeDecl->comment)
					{
						if(typeDecl->postRef.wspStart < typeDecl->postRef.wspEnd)
						{
							DisplayWhitespace(fp, &typeDecl->postRef, typeDecl->comment);
						}
						else
						{
							len += fprintf(fp, " // %s\n", typeDecl->comment);
							len += print_indent(fp, indent);
						}
					}
					else
					{
						if(DisplayWhitespace(fp, &typeDecl->postRef) == 0)
						{
							fprintf(fp, "\n");
							print_indent(fp, indent);
						}
					}

					if(typeDecl->nested && typeDecl->compoundType && i < sptr->typeDeclList.size()-1 && !fPreserveWhitespace)
					{
						len += fprintf(fp, "\n");
					}
				}
				
				if(fPreserveWhitespace)
					;//len += DisplayWhitespace(fp, &sptr->lastBraceRef);
				else
					len += print_indent(fp, indent);

				len += fprintf(fp, "}");
			}

			if(type->link)
				len += fprintf(fp, " ");

			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;
			
		case typeCHAR:		case typeWCHAR: 
		case typeBYTE:		case typeWORD:
		case typeDWORD:		case typeQWORD:
		case typeFLOAT:		case typeDOUBLE:
			
			//if(type->link)
			if(padtype && !fPreserveWhitespace)
				len += fprintf(fp, "%-7s ", Parser::inenglish(TypeToToken(type->ty))); 
			else
				len += fprintf(fp, "%s", Parser::inenglish(TypeToToken(type->ty))); 

			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;

		case typeCONST:

			len += fprintf(fp, "const ");
			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;
			
		case typeUNSIGNED:

			len += fprintf(fp, "unsigned ");
			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;

		case typeSIGNED:

			len += fprintf(fp, "signed ");
			len += RecurseDisplayType(fp, typeDecl, type->link, indent, padtype);
			break;
			
		default:
			break;
		}

		if(type->brackets) 
			len += fprintf(fp, ")");
		
		type = type->link;
	}

	return len;
}

bool NeedsBrackets(Type *type)
{
	type = type->link;

	bool foundArray = false;
	bool foundPtr   = false;
	bool complex	= false;

	while(type)
	{
		if(type->ty == typeARRAY)
			foundArray = true;

		if(type->ty == typePOINTER)
		{
			complex = foundArray ? true : complex;
			foundPtr = true;
		}

		type = type->link;
	}

	return complex;//foundArray && foundPtr;
}

size_t DisplayType(FILE *fp, TypeDecl *typeDecl, Type *type, int indent)
{
	size_t len = 0;

	type = InvertType(type);
	
	//len += print_indent(fp, indent);
	len += RecurseDisplayType(fp, typeDecl, type, indent, false);

	type = InvertType(type);
	
	return len;
}

Type *BreakLink(Type *type, Type *term);
void RestoreLink(Type *type, Type *term);

size_t DisplayTypeDecl(FILE *fp, TypeDecl *typeDecl, int indent)
{
	Type *type;
	size_t len = 0;
	size_t i;

	bool padtype  = typeDecl->declList.size() ? true : false;

	DisplayTags(fp, typeDecl->tagList);
	DisplayWhitespace(fp, &typeDecl->tagRef);


	if(typeDecl->typeAlias)
		fprintf(fp, "typedef ");
	
	len += RecurseDisplayType(fp, typeDecl, typeDecl->baseType, indent, padtype && typeDecl->nested);

	// display each variable-decl
	for(i = 0; i < typeDecl->declList.size(); i++)
	{
		len += fprintf(fp, " ");

		type = typeDecl->declList[i];
		
		Type *restore = BreakLink(type, typeDecl->baseType);
	
		type = InvertType(type);

		len += RecurseDisplayType(fp, typeDecl, type, indent, false);

		if(i < typeDecl->declList.size() - 1)
			len += fprintf(fp, ",");

		type = InvertType(type);
		RestoreLink(restore, typeDecl->baseType);
	}

	return len;
}

extern "C"
void Dump(FILE *fp)
{
	size_t i;


	FILE_DESC *fdesc = globalFileHistory[0];

	//r(i = 0; i < globalTypeDeclList.size(); i++)
	for(i = 0; i < fdesc->stmtList.size(); i++)
	{
		Statement *stmt = fdesc->stmtList[i];
		TypeDecl *typeDecl;

		switch(stmt->stmtType)
		{
		case stmtTYPEDECL:

			typeDecl = stmt->typeDecl;

			if(fPreserveWhitespace)
				DisplayWhitespace(fp, &typeDecl->fileRef);
			
			DisplayTypeDecl(fp, typeDecl, 0);
			fprintf(fp, ";");//\n\n");

			break;


		case stmtINCLUDE:
			fprintf(fp, "include \"%s\";\n", stmt->str);
			break;
		}
	
	}
}


void Dump2(FILE *fp)
{
	size_t i;

	for(i = 0; i < globalTypeDeclList.size(); i++)
	{
		TypeDecl *typeDecl = globalTypeDeclList[i];

		for(size_t x = 0; x < typeDecl->declList.size(); x++)
		{
			Type *t = typeDecl->declList[x];

			PrintType(t);
			printf(";\n");
		}
	}

	printf("\n\n");
}


