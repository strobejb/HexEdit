//
//  lexer.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//
//  -----
//	Lexical analysis
//
//	convert the a text input-stream into a series
//  of TOKENS which are used by the parser to build the
//  syntax-tree
//

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <errno.h>
#include <direct.h>
#include <io.h>

#include "parser.h"
using std::vector;


TOKEN_LOOKUP toklook[] = 
{
	{	TOK_CHAR,		"char"			},
	{	TOK_WCHAR,		"wchar_t"		},
//	{	TOK_BYTE,		"int8"			},
//	{	TOK_WORD,		"int16"			},
//	{	TOK_DWORD,		"int32"			},
//	{	TOK_QWORD,		"int64"			},
	{	TOK_BYTE,		"byte"			},
	{	TOK_WORD,		"word"			},
	{	TOK_DWORD,		"dword"			},
	{	TOK_QWORD,		"qword"			},
	{	TOK_FLOAT,		"float"			},
	{	TOK_DOUBLE,		"double"		},

#undef DEFINE_KEYWORD
#define DEFINE_KEYWORD(tok,str) {tok, str},
#include "keywords.h"

	{	TOK_NULL,		0				}
};

vector <FILE_DESC*>	globalFileHistory;

//
//	preprocessor stuff
//
vector <char *> cpp_options;
bool	 cpp_save = false;
bool	 cpp_none;

#ifdef _WIN32
char	cpp_cmdstr[_MAX_PATH] = "CL.EXE";
char	PATHSEP[] = ";";
#else
char	cpp_cmdstr[_MAX_PATH] = "gcc";
char	PATHSEP[] = ":";
#endif

void Parser::newline()
{
	curFile->curLine++;
}

void Parser::lex_fileref(FILEREF *fileRef)
{
	fileRef->lineNo		= curFile->curLine;
	fileRef->fileDesc	= curFile;
	fileRef->wspStart	= curFile->wspStart;
	fileRef->wspEnd		= curFile->wspEnd;
}

static void dirname(char *dirname, const char *filepath)
{
	const char *ptr;

	for(ptr = filepath + strlen(filepath); ptr > filepath; ptr--)
	{
		if(*ptr == '\\' || *ptr == '/')
			break;
	}

	strncpy(dirname, filepath, ptr-filepath+1);
	dirname[ptr-filepath+1]='\0';
}

//
//	"is absolute path"
//
static bool isabspath(const char *name)
{
	return (name[0] == '/' || name[0] == '\\' || name[0] == '~' || name[1] == ':');
}

static bool exists(const char *name)
{
	return _access(name, 0) == 0 ? true : false;
}

static void joinpath(char *dest, const char *dir, const char *name)
{
	size_t len;
	strcpy(dest, dir);
		
	len = strlen(dest);
	
	if(len > 0 && dest[len-1] != '/' && dest[len-1] != '\\')
		strcat(dest, "/");

	strcat(dest, name);
}

//
//	Search the specified environment-string
//
static bool searchenv(const char *envname, const char *name, char *fullname, size_t fullnamelen)
{
	char *includePaths;

	if((includePaths = getenv(envname)) != 0)
	{
		char *pathDup = _strdup(includePaths);
		char *tok;

		tok = strtok(pathDup, PATHSEP);
		
		while(tok)
		{
			joinpath(fullname, tok, name);

			if((fullname))
				return true;

			tok = strtok(NULL, PATHSEP);
		}
	}

	return false;
}

//
//	Search order:
//
//		1. current directory
//		2. directories specified by /I switch
//		3. directories specified by INCLUDE environment variable
//
static bool getfullname(const char *curpath, const char *name, char *fullname, size_t fullnamelen)
{
	size_t i;
	//FILE_DESC *curFile;

	// if this is the first time we've been called then
	// work off the full path of the file
	if(curpath == 0 || curpath[0] == '\0' || isabspath(name))
	{
		_fullpath(fullname, name, _MAX_PATH);
	}
	// otherwise we need to do a search relative to the directory
	// of the current file we are parsing
	else
	{
		char cur[_MAX_PATH], dir[_MAX_PATH];

		// get the directory of the current file being parsed
		dirname(dir, curpath);//curFile->filePath);

		// make the full path of the new file we want to parse
		_getcwd(cur, _MAX_PATH);
		_chdir(dir);		
		_fullpath(fullname, name, _MAX_PATH);
		_chdir(cur);
	}

	// 1. search local directory
	if(exists(fullname))
	{
		//strcpy(fullname, name);
		return true;
	}

	// 2. search directories specified by -I commandline option
	for(i = 0; i < cpp_options.size(); i++)
	{
		char *opt = cpp_options[i];
		
		if(opt[1] == 'I' && (opt[0] == '/' || opt[0] == '-' ))
		{
			opt += 2;	// skip the '-I'

			if(*opt)
			{
				for( ; *opt && *opt != ' '; opt++)
					;
			}
			else
			{
				opt = cpp_options[++i];
			}

			//printf("searching in %s\n", opt);
			joinpath(fullname, opt, name);

			if(exists(fullname))
				return true;
		}
	}

	// 3. search directories specified by INCLUDE environment
	if(searchenv("INCLUDE", name, fullname, fullnamelen))
		return true;

	// couldn't find it...
	strcpy(fullname, name);
	return false;
}

bool Parser::lex_initbuf(const char *buffer, size_t len)
{
	if(buffer == 0 || len == 0)
		return false;

	// create a new file-descriptor for the in-memory buffer,
	// but don't add it to the 'filehistory' list
	if((curFile = new FILE_DESC("", "")) == 0)
		return false;

	curFile->buf	= (char *)malloc(len + 1);
	curFile->len	= len;

	memcpy(curFile->buf, buffer, len);
	curFile->buf[len] = '\0';
		
	t		 = TOK_NULL;
	ch		 = ' ';
	errcount = 0;
	tnum	 = 0;
	tfnum	 = 0.0;

	return true;
}

bool Parser::file_included(const char *filename)
{
	char fullpath[_MAX_PATH];

	// get the full path to the specified file.
	if(getfullname(curFile->filePath, filename, fullpath, _MAX_PATH) == false)
	{
		if(!parent) parent = this;
		parent->Error(ERROR_NOSUCHFILE, filename);
		return false;
	}

	// has it been included already?
	for(size_t i = 0; i < globalFileHistory.size(); i++)
	{
		FILE_DESC *fd = globalFileHistory[i];

		if(_strcmpi(fd->filePath, fullpath) == 0)
		{
			// do nothing!
			return true;
		}
	}

	return false;
}

bool Parser::lex_init(const char *filename)
{
	bool   preprocess = true;//false;
	char   buf[100];
	size_t len;

	char fullpath[_MAX_PATH];
	FILE *fp;

	// get the full path to the specified file.
	if(getfullname(parent ? parent->curFile->filePath : NULL, filename, fullpath, _MAX_PATH) == false)
	{
		if(!parent) parent = this;
		parent->Error(ERROR_NOSUCHFILE, filename);
		return false;
	}

	if((fp = fopen(fullpath, "rb")) == NULL)
	{
		if(!parent) parent = this;
		parent->Error(ERROR_FILENOTFOUND, filename);
		return false;
	}

	if((curFile = new FILE_DESC(fullpath, filename)) == 0)
		return false;
	
	globalFileHistory.push_back(curFile);


	// read the file into FILE_DESC::buf
	while(!feof(fp))
	{
		char *tmp;

		len = fread(buf, 1, sizeof(buf), fp);

		// reallocate the buffer as necessary (+1 for null-terminator)
		tmp = (char *)realloc(curFile->buf, curFile->len + len + 1);

		if(tmp == 0)
		{
			// error!
			break;
		}
		else
		{
			curFile->buf = tmp;
			memcpy(curFile->buf + curFile->len, buf, len);
			curFile->len += len;
		}
	}

	// null-terminate the buffer
	curFile->buf[curFile->len] = '\0';
	fclose(fp);
		
	t		 = TOK_NULL;
	ch		 = ' ';
	errcount = 0;
	tnum	 = 0;
	tfnum	 = 0.0;

	return true;
}

int Parser::peekch(int advance /*= 0*/)
{
	if(curFile->pos + advance < curFile->len)
		return curFile->buf[curFile->pos + advance];
	else
		return 0;
}

//
//	Return next character in the input-stream, keep track of
//  line-number information at the same time
//
int Parser::nextch()
{
	if(curFile->pos < curFile->len)
	{
		int c = curFile->buf[curFile->pos++];
		
		if(c == '\n')
		{
			newline();
		}
		else if(c == '\\')	// line-continuation
		{
			int nc = peekch(0);
			
			if(nc == '\n')
			{
				curFile->pos++;
				c = curFile->buf[curFile->pos++];
				newline();
			}
			else if(nc == '\r' && peekch(1) == '\n')
			{
				curFile->pos+=2;
				c = curFile->buf[curFile->pos++];
				newline();
			}			
		}

		return c;
	}
	else
	{
		return 0;	
	}
}

int Parser::skipeol()
{
	while(ch != '\n')
		ch = nextch();

	if(ch == '\n')
		ch = nextch();

	return ch;
}

int Parser::skipspaces()
{
	while(ch == ' ')
		ch = nextch();

	return ch;
}

//
//	Really basic preprocessor to handle #line directives
//
//	#line <number> "filename"
//	# <number> "filename"
//
int Parser::preprocess()
{
	if(ch == '#')
	{
		ch = nextch();
		ch = skipspaces();

		// optional #identifier 
		if(isalpha(ch))
		{
			ch = parse_identifier();

			// ignore pragmas entirely
			if(strcmp(tstr, "pragma") == 0)
			{
				return skipeol();
			}
			// we don't handle anything other that #line
			else if(strcmp(tstr, "line") != 0)
			{
				Error(ERROR_PREPROC);
				//exit(-1);
				return 0;
			}
		}

		ch = skipspaces();

		if(isdigit(ch))
		{
			// grab the line-number
			parse_number();

			ch = skipspaces();

			if(ch == '\"' || ch == '<')
			{
				// grab the filename
				ch = parse_string(ch);
				return skipeol();
			}

		}
		else
		{
			Error(ERROR_PREPROC);
			return 0;
		}
	}

	return ch;
}


//
//	Process any combination of whitespace, return the character-offsets
//  of where the whitespace starts/ends
//
int Parser::skipwhitespace(size_t *startpos, size_t *endpos)
{
	*startpos = curFile->pos;

	while(ch && ch <= ' ' || ch == '/' || ch == '#')
	{
		if(ch == '#')				// #line directive?
		{
			ch = preprocess();
			continue;
		}
		else if(ch == '/')
		{
			ch = nextch();

			if(ch == '/')			// single-line comment
			{
				ch = nextch();

				while(ch && ch != '\n')
					ch = nextch();
			}
			else if(ch == '*')		// block comment
			{
				do
				{
					ch = nextch();

					while(ch && ch != '*')
						ch = nextch();

					ch = nextch();
				}
				while(ch && ch != '/');
			}
			else					// really was a '/' by itself
			{
				*endpos = curFile->pos - 1;
				return '/';
				//break;
			}
		}

		ch = nextch();
	}

	*endpos = curFile->pos - 1;
	return ch;
}

static
TOKEN match_keyword(const char *buf)
{
	// match the specified string against a predefined keyword
	for(int i = 0; toklook[i].tok != TOK_NULL; i++)
	{
		if(strcmp(buf, toklook[i].str) == 0)
			return toklook[i].tok;
	}

	// no match, treat as a normal identifier
	return TOK_IDENTIFIER;
}

static inline
unsigned long hexval(int ch)
{
	if(isdigit(ch))	return ch - '0';
	else			return (ch & ~0x20) - 'A' + 10;
}

//
//	Numbers are the most complicated to parse as we need
//	to handle hex, octal, decimal and also floating-point
//
TOKEN Parser::parse_number()
{
	bool floatnum = false;
	char numstr[MAX_STRING_LEN+4], *ep;
	int  i = 0;

	// Hex or Octal
	if(ch == '0')	
	{
		ch = nextch();

		// Hex numbers
		if(ch == 'x' || ch == 'X')
		{
			tnum_base = HEX;
			ch = nextch();

			// collect all hex-digits together
			while(i < MAX_STRING_LEN && isxdigit(ch))
			{
				numstr[i++] = ch;
				ch = nextch();
			}

			if(i == 0)
				Error(ERROR_ILLEGAL_HEXNUM);

			numstr[i] = '\0';
		}
		// Octal numbers
		else
		{
			tnum_base = OCT;

			// collect all digits together. Invalid octal sequences
			// will be caught when we call strtoul
			while(i < MAX_STRING_LEN && isdigit(ch))
			{
				numstr[i++] = ch;
				ch = nextch();
			}

			numstr[i] = '\0';
		}

	}
	// Decimal 
	else			
	{
		tnum_base = DEC;

		// Base digits
		while(i < MAX_STRING_LEN && isdigit(ch))
		{
			numstr[i++] = ch;
			ch = nextch();
		}
	}

	// Floating point!
	if(ch == '.' && tnum_base != HEX)
	{
		floatnum = true;
		numstr[i++] = ch;
		ch = nextch();

		// collect any exponent digits
		while(i < MAX_STRING_LEN && isdigit(ch))
		{
			numstr[i++] = ch;
			ch = nextch();
		}

		// optional exponent
		if(ch == 'e' || ch == 'E')
		{
			numstr[i++] = ch;
			ch = nextch();

			if(ch == '-' || ch == '+')
			{
				numstr[i++] = ch;
				ch = nextch();
			}

			if(!isdigit(ch))
			{
			}

			// exponent digits
			while(i < MAX_STRING_LEN && isdigit(ch))
			{
				numstr[i++] = ch;
				ch = nextch();
			}
		}
	}
	else if(ch == '.' && tnum_base == HEX)
	{
		Error(ERROR_ILLEGAL_HEXNUM);
	}

	// nul-terminate
	numstr[i] = '\0';
	errno = 0;

	if(floatnum)
	{
		if(ch == 'f' || ch == 'F') ch = nextch();
		if(ch == 'l' || ch == 'L') ch = nextch();

		//tnum_type = FLOAT;
		tfnum = strtod(numstr, &ep);
		int e = errno;
		return TOK_FNUMBER;
	}
	else
	{
		if(ch == 'u' || ch == 'U') ch = nextch();
		if(ch == 'l' || ch == 'L') ch = nextch();

		if(isalpha(ch))
			Error(ERROR_ILLEGAL_SUFFIX, ch);

		int base[] = { 16, 10, 8 };
		tnum = strtoul(numstr, &ep, base[tnum_base]);

		if(*ep)
		{
			Error(ERROR_ILLEGAL_DIGIT, *ep, base[tnum_base]);
		}
		if(errno)
		{
			int e = errno;
			Error(ERROR_OVERFLOW);
			//if(ep)
		//		printf("%s\n", ep);
			//return TOK_NULL;
		}

		return TOK_INUMBER;
	}


}

int Parser::backslash()
{
	int n = 0;

	ch = nextch();

	switch(ch)
	{
	case 'a'  : return '\a';	// bell (alert)
	case 'b'  : return '\b';	// backspace
	case 'f'  : return '\f';	// formfeed
	case 'n'  : return '\n';	// newline
	case 'r'  : return '\r';	// carriage return
	case 't'  : return '\t';	// horizontal tab
	case 'v'  : return '\v';	// vertial tab
	case '\'' : return '\'';	// single quotation
	case '\"' : return '\"';	// double quotation
	case '\\' : return '\\';	// backslash
	case '?'  : return '\?';	// literal question mark
	
	case 'x'  :					// hexadecimal
		
		ch = nextch();
		while(isxdigit(ch))
		{
			n = n * 0x10 + hexval(ch);
			ch = nextch();
		}
		
		return n;

	// octal
	case '0': case '1': case '2': case '3': 
	case '4': case '5': case '6': case '7': 

		while(ch >= '0' && ch <= '7')
		{
			n = n * 8 + (ch - '0');
			ch = nextch();
		}

		return n;
		
	// don't handle any other escape
	default:
		return 0;
	}
}

//
//	process string-literals
//
int Parser::parse_string(int term)
{
	int i = 0;

	// multiple quote strings are coallesced together
	while(ch == '\"')
	{
		ch = nextch();
		
		while(ch && ch != term)
		{
			// escape sequence
			if(ch == '\\')
			{
				ch = backslash();
			}
			
			if(i < MAX_STRING_LEN)
				tstr[i++] = (char)ch;
			
			ch = nextch();
		}
		
		// skip the terminating quote
		ch = nextch();
		ch = skipwhitespace(&curFile->wspStart, &curFile->wspEnd);
	}
	
	tstr[i] = '\0';
	
	return ch;
}

int Parser::parse_identifier()
{
	int i = 0;
	
	while(isalnum(ch) || ch == '_')
	{
		if(i < MAX_STRING_LEN)
			tstr[i++] = (char)ch;
		
		ch = nextch();
	}
	
	tstr[i] = '\0';
	
	return ch;
}

//
//	gettok() is the main entry-point into the lexer, it is called by
//  the parser to retrieve the sequence of tokens in each translation unit
//
TOKEN Parser::gettok()
{
	TOKEN tmp;

	if(ch == '/' && (peekch() != '/' && peekch() != '*'))
	{
		ch = nextch();
		return TOKEN('/');
	}

	// skip any whitespace, but remember where it was found
	ch = skipwhitespace(&curFile->wspStart, &curFile->wspEnd);

	// integer/floating-point numbers
	if(isdigit(ch) || (ch == '.' && isdigit(peekch())))
	{
		// will return TOK_INUMBER or TOK_FNUMBER, with the token-value
		// stored in tnum/tfnum
		return parse_number();
	}

	// wide character/string literals
	if(ch == 'L' && (peekch() == '\"' || peekch() == '\''))
	{

	}	

	// regular string literal
	if(ch == '\"')
	{
		ch = parse_string(ch);
		return TOK_STRINGBUF;
	}
	
	// match operators
	switch(ch)
	{
	case '!':		
		ch  = nextch();
		if(ch == '=')			// !=
		{
			ch = nextch();
			return TOK_NEQ;
		}
		return TOKEN('!');		// !

	case '=':		
		ch  = nextch();
		if(ch == '=')			// ==
		{
			ch = nextch();
			return TOK_EQU;
		}
		return TOKEN('=');		// =

	case '<':		
		ch  = nextch();
		if(ch == '=')			// <=
		{
			ch = nextch();
			return TOK_LE;
		}
		else if(ch == '<')		// <<
		{
			ch = nextch();
			return TOK_SHR;
		}
		return TOKEN('<');		// <

	case '>':		
		ch  = nextch();
		if(ch == '=')			// >=
		{
			ch = nextch();
			return TOK_GE;
		}
		else if(ch == '>')		// >>
		{
			ch = nextch();
			return TOK_SHL;
		}
		return TOKEN('>');		// >

	case '&':		
		ch  = nextch();
		if(ch == '&')			// &&
		{
			ch = nextch();
			return TOK_ANDAND;
		}
		return TOKEN('&');		// &

	case '|':		
		ch  = nextch();
		if(ch == '|')			// ||
		{
			ch = nextch();
			return TOK_OROR;
		}
		return TOKEN('|');		// |

	case '+':		
		ch  = nextch();
		if(ch == '+')			// ++
		{
			ch = nextch();
			return TOK_INC;
		}
		return TOKEN('+');		// +

	case '-':		
		ch  = nextch();
		if(ch == '-')			// --
		{
			ch = nextch();
			return TOK_DEC;
		}
		else if(ch == '>')		// ->
		{
			ch = nextch();
			return TOK_DEREF;
		}
		return TOKEN('-');		// -

	case '(': case ')': 
	case '{': case '}':
	case '[': case ']':
	case '.': case ',': 
	case ';': case ':': 
	case '~': case '^':
	case '*': case '%': 
	case '/': case '?':
		tmp = TOKEN(ch);
		ch  = nextch();
		return tmp;

	// end of input
	case '\0':
		return TOK_NULL;

	default:

		if(isalpha(ch) || ch == '_')
		{
			// match idenfifer to a keyword
			ch = parse_identifier();
			return match_keyword(tstr);
		}
		else
		{
			// error, invalid character encountered
			tmp = TOKEN(ch);
			ch = nextch();
			return TOK_ILLEGAL;
		}
	}
}

void Parser::lex_cleanup()
{
	for(size_t i = 0; i < globalFileHistory.size(); i++)
	{
		free(globalFileHistory[i]->buf);
		delete globalFileHistory[i];
	}
}