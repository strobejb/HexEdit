//
//  expr.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef EXPR_INCLUDED
#define EXPR_INCLUDED

#include <tchar.h>
#include "lexer.h"

enum EXPR
{ 
	EXPR_NUMBER,	EXPR_STRINGBUF, EXPR_IDENTIFIER, 
	EXPR_POINTER,	EXPR_DEREF,		EXPR_ADDRESS,
	EXPR_FIELD,		EXPR_ARRAY,		EXPR_FUNCTION,
	EXPR_UNARY,		EXPR_BINARY,	EXPR_TERTIARY, 
	EXPR_ASSIGN,	EXPR_COMMA,		EXPR_NULL
};

struct ExprNode
{
	ExprNode() : type(EXPR_NULL), left(0), right(0), cond(0)
	{
	}

	// constructor: initialize with default values
	ExprNode(EXPR ty, TOKEN t, ExprNode *l=0, ExprNode *r=0, ExprNode *c=0) :
		left(l), right(r), cond(c), tok(t), type(ty), brackets(false), base(DEC), val(0)
	{
	}

	// destructor: recursively delete all child expressions
	~ExprNode()
	{
		delete left;
		delete right;
		delete cond;
	}

	ExprNode		*	left;		// used for all expressions
	ExprNode		*	right;		// only used for binary
	ExprNode		*	cond;		// only used for tertiary (conditionals)

	TOKEN				tok;		// the operator token (e.g. '+', '&' etc)
	EXPR				type;		// type of expression
	bool				brackets;	// was this node enclosed in brackets?
	NUMBASE				base;		// dec/hex/oct etc (for EXPR_NUMBER only)

	union 
	{
		INUMTYPE			val;
		double				fval;
		char		*		str;
	};
};

// Parse a 'simple' expression, not including any 'comma' operators
ExprNode *Expression(TOKEN term);

// Parse a 'full' expression, includes the 'comma' operator
ExprNode *FullExpression(TOKEN term);

// Parse a 'primary' expression only (i.e. just a single number/identifier)
ExprNode *PrimaryExpression();

// Duplicate the specified expression-tree
ExprNode * CopyExpr(ExprNode *expr);

// Display (flatten) expression to specified stream
size_t Flatten(FILE *fp, ExprNode *expr);
size_t Flatten(TCHAR *buf, size_t len, ExprNode *expr);

// Evaluate (flatten) to a constant integer. Returns 0 if not contant.
INUMTYPE Evaluate(ExprNode *expr);

#endif