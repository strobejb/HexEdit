//
//  parse_expr.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//
//  ---------------------------------------------------------
//	'Top Down' Recursive Descent Parser for C-style expressions
//
//	Derived from the Fraser&Hanson LCC compiler described in 
//	the book "A Retargetable C Compiler"
//

#define _CRT_SECURE_NO_DEPRECATE
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "stringprint.h"

#if (_MSC_VER == 1300)
#define _strdup strdup
#endif

#define ASSIGNMENT_EXPRESSIONS
//#define FUNCTION_EXPRESSIONS
//#define INCDEC_EXPRESSIONS

//
//	Binary operator precedence lookup. Defines the grouping of binary sub-expressions
//
static int Precedence(int t)
{
	switch(t)
	{
	// unary operators at 14+
	// '++', '--', '*', '&', '+', '-', '~', '!'

	// binary operators at 4 to 13 inclusive
	case '*': case '/': case '%':	return 13;
	case '+': case '-':				return 12;
	case TOK_SHL: case TOK_SHR:		return 11;
	case '<': case '>':				
	case TOK_LE : case TOK_GE:		return 10;
	case TOK_EQU: case TOK_NEQ:		return 9;
	case '&':						return 8;
	case '^':						return 7;
	case '|':						return 6;
	case TOK_ANDAND:				return 5;
	case TOK_OROR:					return 4;

	// not referenced
	case '?':						return 3;	
	case '=':						return 2;	
	case ',':						return 1;	
	default:						return 0;
	}
}


//
//	primary:	<number>
//				<identifier>
//				<string-literal>
//
ExprNode * Parser::PrimaryExpression()
{
	ExprNode *p = 0;

	switch(t)
	{
	// Integer number
	case TOK_INUMBER:
		
		p = new ExprNode(EXPR_NUMBER, t);
		p->val		= tnum;
		p->base		= tnum_base;
		t = gettok();
		break;

	// Floating-point number
	case TOK_FNUMBER:

		p = new ExprNode(EXPR_NUMBER, t);
		p->fval		= tfnum;
		p->base		= DEC;
		t = gettok();
		break;

	// Identifier name (variable)
	case TOK_IDENTIFIER: 

		p = new ExprNode(EXPR_IDENTIFIER, t);
		p->str = _strdup(tstr);
		t = gettok();
		break;

	// Quoted string-literal
	case TOK_STRINGBUF: 

		p = new ExprNode(EXPR_STRINGBUF, t);
		p->str = _strdup(tstr);
		t = gettok();
		break;

	default:
		Error(ERROR_SYNTAX_ERROR, inenglish(t));
		p = new ExprNode(EXPR_NULL, TOK_NULL);
		break;
	}

	return p;
}

//
//	postfix: primary [postfix-op]
//
ExprNode * Parser::PostfixExpression(ExprNode *p)
{
	ExprNode *q;

	for(;;)
	{
		TOKEN op = t;

		switch(op)
		{
		// array bounds
		case '[':

			t = gettok();
			q = Expression( TOKEN(']') );
			p = new ExprNode(EXPR_ARRAY, op, p, q);
			break;
			
		// field access
		case '.':
			
			t = gettok();
			q = Expression(TOK_NULL);
			p = new ExprNode(EXPR_FIELD, op, p, q);
			break;

		// pointer dereference '->'
		case TOK_DEREF:

			t = gettok();
			q = Expression(TOK_NULL);
			p = new ExprNode(EXPR_DEREF, op, p, q);
			break;

		// post-increment and post-decrement operators
		case TOK_INC: case TOK_DEC:
#ifdef INCDEC_EXPRESSIONS
			return p;
#else
			Error(ERROR_OPERATOR_NOTSUPPORTED, inenglish(t));
			return p;
#endif
			
		// function calls (not supported)
		case '(':
#ifdef FUNCTION_EXPRESSIONS
			return p;			
#else
			Error(ERROR_FUNC_NOTSUPPORTED);
			return p;
#endif
			
		// pass as-is
		default:
			return p;
		}
	}
}

//
//	unary: [unary-op] postfix
//
//	unary-op: one of '-', '+', '*' etc
//
ExprNode * Parser::UnaryExpression(void)
{
	ExprNode *p = 0;
	TOKEN op = t;

	switch(op)
	{
	// pointer dereference / address-of
	case '*':	
	case '&':

		t = gettok(); 
		p = UnaryExpression();
		p = new ExprNode(op == '*' ? EXPR_POINTER : EXPR_ADDRESS, op, p);
		break;

	// regular unary operators
	case '+':	
	case '-':	
	case '~':	
	case '!':	
		
		t = gettok(); 
		p = UnaryExpression();
		p = new ExprNode(op == '*' ? EXPR_POINTER : EXPR_UNARY, op, p);
		break;
				
	// expressions within parenthesis
	case '(':	
		
		t = gettok(); 
		p = FullExpression( TOKEN(')') );
		if(p) p->brackets = true;
		p = PostfixExpression(p);	
		break;

	// pre-increment & pre-decrement operators
	case TOK_INC: case TOK_DEC:		
#ifdef INCDEC_SUPPORTED
		Error(ERROR_OPERATOR_NOTSUPPORTED, inenglish(t));
		break;
#else
		Error(ERROR_OPERATOR_NOTSUPPORTED, inenglish(t));
		break;
#endif

	default:	
		
		p = PrimaryExpression();
		p = PostfixExpression(p);
		break;
	}

	return p;
}

//
//	binary:	unary [binary-op binary]
//
//	binary-op:  one of '+', '-', '*' etc
//
ExprNode * Parser::BinaryExpression(int k)
{
	ExprNode *p = UnaryExpression();
	ExprNode *r;

	// Modified Fraser-Hanson expression parser
	for(int i = Precedence(t); i >= k && i > 0; i--)
		while(Precedence(t) == i && t != '=' && ch != '=')
		{
			TOKEN op = t;
			t = gettok();

			if(op == TOK_ANDAND || op == TOK_OROR)		// Right-associative
			{
				r = BinaryExpression(i);
			}
			else										// Left-associative
			{
				r = BinaryExpression(i+1);
			}
			
			p = new ExprNode(EXPR_BINARY, op, p, r);
		}

	return p;
}

//
//	cond: binary ['?' expr ':' cond]
//
ExprNode * Parser::ConditionalExpression(void)
{
	ExprNode *p = BinaryExpression(4);
	
	if(t == '?')
	{
		ExprNode *l, *r;
		
		t = gettok();
		l = Expression(TOKEN(':'));
		r = ConditionalExpression();

		p = new ExprNode(EXPR_TERTIARY, TOKEN('?'), l, r, p);
	}

	return p;
}

//
//	assign: cond
//			unary assign-op expr
//
ExprNode * Parser::AssignmentExpression(TOKEN term)
{
//	static int stop[] = { IF, IDENTIFIER, 0 };
	
	ExprNode *p = ConditionalExpression();		

#ifdef ASSIGNMENT_EXPRESSIONS
	if(t == '='
		|| (Precedence(t) >= 6  && Precedence(t) <= 8)		// & ^ |
		|| (Precedence(t) >= 11 && Precedence(t) <= 13))	// << >> + - * / %
	{
		ExprNode *q;
		TOKEN op = t;
		
		t = gettok();

		// normal assignment ( '=' )
		if(op == '=')	
		{
			q = AssignmentExpression(TOK_NULL);
			p = new ExprNode(EXPR_ASSIGN, op, p, q);
		}
		// augmented assignment ( &=, *=, +=, etc )
		else			
		{
			Expected('=');
			q = AssignmentExpression(TOK_NULL);
			p = new ExprNode(EXPR_ASSIGN, op, p, q);
		}
	}
#else
	if(t == '=')
	{
		Error(ERROR_ASSIGN_NOTSUPPORTED);
	}
#endif

	//if(tok) 
	//	Test(tok, stop);

	return p;
}

//
//	comma:	[','] expr
//
//
//	Comma-separated expressions are binary trees.
//	On return, the 'left' nodes hold the values, the 'right' nodes
//  are used to recursively link to the next expression
//
ExprNode *Parser::CommaExpression(TOKEN tok)
{
	ExprNode *left = AssignmentExpression(tok);
	ExprNode *right = 0;
	if(t == ',')
	{
		t     = gettok();
		right = CommaExpression(tok);
	}

	return new ExprNode(EXPR_COMMA, TOKEN(','), left, right);
}

ExprNode * Parser::Expression(TOKEN term)
{
	ExprNode *p;
	
	//p = AssignmentExpression(TOK_NULL);
	p = ConditionalExpression();

	Test(term);
	return p;
}

//
//	expr: cond
//
ExprNode * Parser::FullExpression(TOKEN term)
{
	ExprNode *p = AssignmentExpression(TOK_NULL);
	
	while(p && t == ',')
	{
		ExprNode *q;

		t = gettok();
		
		if((q = AssignmentExpression(TOK_NULL)) != 0)
		{
			p = new ExprNode(EXPR_COMMA, TOKEN(','), p, q);
		}
		else
		{
			delete p;
			return 0;
		}
	}

	if(term && !Test(term))
		return 0;
	else
		return p;
}


int PrintCppString(stringprint &sbuf, char *buf)
{
	int ch = *buf;

	sbuf._stprintf(TEXT("\""));

	while(ch)
	{
		if(ch >= 32 && ch < 127)
		{
			sbuf._stprintf(TEXT("%c"), ch);
		}
		else
		{
			TCHAR hex[10], *str = 0;

			switch(ch)
			{
			case '\a' : str = TEXT("\\a");	break; // bell (alert)
			case '\b' : str = TEXT("\\b");	break; // backspace
			case '\f' : str = TEXT("\\f");	break; // formfeed
			case '\n' : str = TEXT("\\n");	break; // newline
			case '\r' : str = TEXT("\\r");	break; // carriage return
			case '\t' : str = TEXT("\\t");	break; // horizontal tab
			case '\v' : str = TEXT("\\v");	break; // vertial tab
			case '\'' : str = TEXT("\\'");	break; // single quotation
			case '\"' : str = TEXT("\\\"");	break; // double quotation
			case '\\' : str = TEXT("\\\\");	break; // backslash
			case '\?' : str = TEXT("\\?");	break; 

			default:
				_stprintf_s(hex, 10, TEXT("\\x%02x"), ch);
				str = hex;
				break;
			}

			sbuf._stprintf(str);
		}

		ch = *(++buf);
	}

	sbuf._stprintf(TEXT("\""));
	return 0;
}



//
//	Display (flatten) the expression to the specified stream
//
int RecurseFlatten(stringprint &sbuf, ExprNode *expr)
{
	int len = 0;
	static const TCHAR *numfmt[] = { TEXT("0x%x"), TEXT("%d"), TEXT("%o"), TEXT("%g") };

	if(expr == 0)
		return 0;

	if(expr->brackets)
		sbuf._stprintf(TEXT("("));

	switch(expr->type)
	{
	case EXPR_UNARY:
		sbuf._stprintf(TEXT("%hs"), Parser::inenglish(expr->tok));
		RecurseFlatten(sbuf, expr->left);
		break;

	case EXPR_POINTER: case EXPR_ADDRESS:
		sbuf._stprintf(TEXT("%hs"), Parser::inenglish(expr->tok));
		RecurseFlatten(sbuf, expr->left);
		break;

	case EXPR_NUMBER:
		if(expr->tok == TOK_INUMBER)
			sbuf._stprintf(numfmt[expr->base], expr->val);
		else
			sbuf._stprintf(TEXT("%g"), expr->fval);

		break;

	case EXPR_IDENTIFIER:
		sbuf._stprintf(TEXT("%hs"), expr->str);
		break;

	case EXPR_STRINGBUF:
		//len += printf("\"%s\"", expr->str);
		PrintCppString(sbuf, expr->str);
		break;

	case EXPR_BINARY:
		RecurseFlatten(sbuf, expr->left);
		sbuf._stprintf(TEXT(" %hs "), Parser::inenglish(expr->tok));
		RecurseFlatten(sbuf, expr->right);
		break;

	case EXPR_ARRAY:
		RecurseFlatten(sbuf, expr->left);
		sbuf._stprintf(TEXT("["));
		RecurseFlatten(sbuf, expr->right);
		sbuf._stprintf(TEXT("]"));
		break;
	
	case EXPR_DEREF:
		RecurseFlatten(sbuf, expr->left);
		sbuf._stprintf(TEXT("->"));
		RecurseFlatten(sbuf, expr->right);
		break;

	case EXPR_FIELD:
		RecurseFlatten(sbuf, expr->left);
		sbuf._stprintf(TEXT("."));
		RecurseFlatten(sbuf, expr->right);
		break;

	case EXPR_TERTIARY:
		RecurseFlatten(sbuf, expr->cond);
		sbuf._stprintf(TEXT(" ? "));
		RecurseFlatten(sbuf, expr->left);
		sbuf._stprintf(TEXT(" : "));
		RecurseFlatten(sbuf, expr->right);
		break;

	case EXPR_COMMA:
		RecurseFlatten(sbuf, expr->left);
		sbuf._stprintf(TEXT(" %hs "), Parser::inenglish(expr->tok));
		RecurseFlatten(sbuf, expr->right);
		sbuf._stprintf(TEXT(" : "));
		break;
	}

	if(expr->brackets)
		sbuf._stprintf(TEXT(")"));

	return len;
}

size_t Flatten(FILE *fp, ExprNode *expr)
{
	stringprint sbuf(fp);
	RecurseFlatten(sbuf, expr);
	return sbuf.length();
}

size_t Flatten(TCHAR *buf, int len, ExprNode *expr)
{
	stringprint sbuf(buf, len);
	RecurseFlatten(sbuf, expr);
	return sbuf.length();
}

//
//	Evaluate (flatten) the specified expression and
//	return it's numeric value
//
INUMTYPE Evaluate(ExprNode *expr)
{
	INUMTYPE left, right;

	if(expr == 0)
		return false;

	switch(expr->type)
	{
	case EXPR_IDENTIFIER:
		return 0;

	case EXPR_NUMBER:
		return expr->tok == TOK_INUMBER ? expr->val : (int)expr->fval;

	case EXPR_UNARY:

		left = Evaluate(expr->left);

		switch(expr->tok)
		{
		case '+':			return left;//+left;
		case '-':			return left;//-left;
		case '!':			return !left;
		case '~':			return ~left;
		default:			return 0;
		}
		
	case EXPR_BINARY:

		if(expr->tok == TOK_ANDAND)
		{
			return Evaluate(expr->left) && Evaluate(expr->right);
		}
		else if(expr->tok == TOK_OROR)
		{
			return Evaluate(expr->left) || Evaluate(expr->right);
		}
		else
		{
			
			left  = Evaluate(expr->left);
			right = Evaluate(expr->right);
			
			switch(expr->tok)
			{
			case '+':			return left +  right;
			case '-':			return left -  right;
			case '*':			return left *  right;
			case '%':			return left %  right;
			case '/':			return left /  right;
			case '|':			return left |  right;
			case '&':			return left &  right;
			case '^':			return left ^  right;
			case TOK_ANDAND:	return left && right;
			case TOK_OROR:		return left || right;
			case TOK_SHR:		return left << right;
			case TOK_SHL:		return left >> right;
			case TOK_GE:		return left >= right;
			case TOK_LE:		return left <= right;
			default:			return 0;
			}
		}

	case EXPR_TERTIARY:

		if(Evaluate(expr->cond))
			return Evaluate(expr->left);
		else
			return Evaluate(expr->right);

	default:
		// don't understand anything else
		return 0;
	}
}


//
//	Recursively duplicate the specified expression tree
//
ExprNode * CopyExpr(ExprNode *expr)
{
	ExprNode *expr2;

	// create the new node
	if(expr == 0 || (expr2 = new ExprNode(expr->type, expr->tok)) == 0)
		return 0;

	// copy the contents
	expr2->brackets = expr->brackets;
	expr2->val		= expr->val;

	// copy the children
	expr2->left		= CopyExpr(expr->left);
	expr2->right	= CopyExpr(expr->right);
	expr2->cond		= CopyExpr(expr->cond);

	// make sure it all copied ok
	if(!expr2->left && expr->left || !expr2->right && expr->right || !expr2->cond && expr->cond)
	{
		delete expr2;
		return 0;
	}

	return expr2;
}

/*//
//	Recursively free the specified expression tree
//
void FreeExpr(ExprNode *expr)
{
	if(expr != 0)
	{
		FreeExpr(expr->left);
		FreeExpr(expr->right);
		FreeExpr(expr->cond);

		delete expr;
	}
}
*/


ExprNode * Parser::ParseExpression()
{
	return Expression(TOK_NULL);
}