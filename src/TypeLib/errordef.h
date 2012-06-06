//
//  errordef.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//


DEFINE_ERR(	ERROR_EXPECTED_TYPENAME,	"Expected a typename"	)
DEFINE_ERR(	ERROR_NOT_TYPENAME,			"'%s' is not a type name"	)
DEFINE_ERR(	ERROR_UNKNOWN_STRUCT,		"Unknown struct type '%s'"	)
DEFINE_ERR(	ERROR_UNDEFINED_STRUCT,		"Undefined struct '%s'"	)
DEFINE_ERR(	ERROR_UNKNOWN_ENUM,			"Unknown enum type '%s'"	)
DEFINE_ERR(	ERROR_UNDEFINED_ENUM,		"Undefined enum type '%s'"	)
DEFINE_ERR(	ERROR_TYPE_REDEFINITION,	"Redefinition of type '%s'"	)
DEFINE_ERR(	ERROR_EXPECTED_TOKEN,		"Expected '%s', found '%s'" )
DEFINE_ERR(	ERROR_SYNTAX_ERROR,			"Syntax error : '%s'" )
DEFINE_ERR(	ERROR_ILLEGAL_TAG,			"Tag '%s' cannot be used in this context" )
DEFINE_ERR(	ERROR_NOTA_TAG,			"Identifier '%s' is invalid in context" )
DEFINE_ERR(	ERROR_UNEXPECTED,			"Unexpected: '%s'" )
DEFINE_ERR(	ERROR_NOFUNCPTR,			"Function pointers not supported")
DEFINE_ERR( ERROR_OVERFLOW,				"Overflow in constant value")
DEFINE_ERR( ERROR_OPERATOR_NOTSUPPORTED,"'%s' operator not supported")
DEFINE_ERR( ERROR_ASSIGN_NOTSUPPORTED,	"Assignment operator not supported")
DEFINE_ERR( ERROR_FUNC_NOTSUPPORTED,	"Function call operator not supported")
DEFINE_ERR( ERROR_ILLEGAL_SUFFIX,		"Illegal suffix '%c' on number")
DEFINE_ERR( ERROR_ILLEGAL_DIGIT,		"Illegal digit '%c' in base-%d number")
DEFINE_ERR( ERROR_ILLEGAL_HEXNUM,		"Syntax error in hex constant")
DEFINE_ERR( ERROR_PREPROC,				"Error in preprocessor")
DEFINE_ERR( ERROR_BITFIELDUNION,		"Bitfield not allowed in Union")
DEFINE_ERR( ERROR_FILENOTFOUND,			"Failed to open '%s'")
DEFINE_ERR( ERROR_NOSUCHFILE,			"Filename '%s' does not exist")
DEFINE_ERR(	ERROR_UNKNOWN,				"Unknown error" )


