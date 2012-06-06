//
//  error.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef ERROR_INCLUDED
#define ERROR_INCLUDED

enum ERROR 
{
	ERROR_NONE								= 0,
	ERROR_FIRST								= 2000,

#undef  DEFINE_ERR
#define DEFINE_ERR(err,msg) err,
#include "errordef.h"

};

struct ERROR_LOOKUP 
{
	ERROR	err;
	char	*fmt;
};

typedef void (* ERROR_CALLBACK)(ERROR err, char *errstr, void *param);

/*bool Expected(int tok);
void Unexpected(int tok);
void Error(ERROR err, const char *fmt, ...);
*/

//
//	Error reporting
//

/*
void			Error(ERROR err, ...);
//void			ErrorRef(ERROR err, FILEREF ref, ...);
//	void			ErrorLineNo(ERROR err, int lineno, ...);
bool			Warning(ERROR err, ...);
char *inenglish(TOKEN t, bool use_t_state = false);


//
//	Input-stream validation
//
bool			Test(TOKEN tok);
bool			Expected(TOKEN tok, char *terminal=0);
bool			Expected(int t, char *terminal=0);
void			Unexpected(TOKEN tok);
*/
//extern int errcount;

#endif
