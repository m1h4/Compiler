/*
 *	Lexer - Scanner and Lexical Analyzer
 *	Copyright (C) 2007 Marko Mihovilic
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// TODO Floating point numbers can start with more than one zero before the dot. (Can't just assume octal)

// Basic token types
#define TOKEN_NONE			0
#define TOKEN_PUNCTUATION	1
#define TOKEN_IDENTIFIER	2
#define TOKEN_NUMBER		3
#define TOKEN_STRING		4
#define TOKEN_LITERAL		5

// Number extended types
#define NUMBER_INTEGER		1
#define NUMBER_HEX			2
#define NUMBER_OCTAL		3
#define NUMBER_FLOAT		4

#define NUMBER_NEGATIVE_INTEGER	(NUMBER_SIGN_MASK | 1)
#define NUMBER_NEGATIVE_HEX		(NUMBER_SIGN_MASK | 2)
#define NUMBER_NEGATIVE_OCTAL	(NUMBER_SIGN_MASK | 3)
#define NUMBER_NEGATIVE_FLOAT	(NUMBER_SIGN_MASK | 4)

#define NUMBER_SIGN_MASK	0x80000000
#define NUMBER_TYPE_MASK	0x7FFFFFFF

// Punctuation extended types/punctuation identifiers
#define PUNCTUATION_NONE				0
#define PUNCTUATION_RSHIFT_ASSIGN		1
#define PUNCTUATION_LSHIFT_ASSIGN		2
#define PUNCTUATION_PARMETERS			3
#define PUNCTUATION_PREPROCESSORMERGE	4
#define PUNCTUATION_LOGIC_AND			5
#define PUNCTUATION_LOGIC_OR			6
#define PUNCTUATION_LOGIC_GEQ			7
#define PUNCTUATION_LOGIC_LEQ			8
#define PUNCTUATION_LOGIC_EQ			9
#define PUNCTUATION_LOGIC_UNEQ			10
#define PUNCTUATION_MUL_ASSIGN			11
#define PUNCTUATION_DIV_ASSIGN			12
#define PUNCTUATION_MOD_ASSIGN			13
#define PUNCTUATION_ADD_ASSIGN			14
#define PUNCTUATION_SUB_ASSIGN			15
#define PUNCTUATION_INC					16
#define PUNCTUATION_DEC					17
#define PUNCTUATION_BIN_AND_ASSIGN		18
#define PUNCTUATION_BIN_OR_ASSIGN		19
#define PUNCTUATION_BIN_XOR_ASSIGN		20
#define PUNCTUATION_RSHIFT				21
#define PUNCTUATION_LSHIFT				22
#define PUNCTUATION_POINTERREF			23
#define PUNCTUATION_STATICMEMBER		24
#define PUNCTUATION_MEMBER				25
#define PUNCTUATION_MUL					26
#define PUNCTUATION_DIV					27
#define PUNCTUATION_MOD					28
#define PUNCTUATION_ADD					29
#define PUNCTUATION_SUB					30
#define PUNCTUATION_ASSIGN				31
#define PUNCTUATION_BIN_AND				32
#define PUNCTUATION_BIN_OR				33
#define PUNCTUATION_BIN_XOR				34
#define PUNCTUATION_BIN_NOT				35
#define PUNCTUATION_LOGIC_NOT			36
#define PUNCTUATION_LOGIC_GREATER		37
#define PUNCTUATION_LOGIC_LESS			38
#define PUNCTUATION_COMMA				39
#define PUNCTUATION_SEMICOLON			40
#define PUNCTUATION_COLON				41
#define PUNCTUATION_QUESTIONMARK		42
#define PUNCTUATION_PARENTHESESOPEN		43
#define PUNCTUATION_PARENTHESESCLOSE	44
#define PUNCTUATION_BRACEOPEN			45
#define PUNCTUATION_BRACECLOSE			46
#define PUNCTUATION_SQBRACKETOPEN		47
#define PUNCTUATION_SQBRACKETCLOSE		48
#define PUNCTUATION_BACKSLASH			49
#define PUNCTUATION_PREPROCESSOR		50
#define PUNCTUATION_DOLLAR				51

// Function result codes
#define ERROR_NONE		0	// No error occured
#define ERROR_INVALID	1	// Something is invalid in the input data
#define ERROR_EOF		2	// End of the file was reached before the input could be processed

// This structure represents a punctuation in the punctuation list
typedef struct
{
	LPSTR name;
	ULONG id;
} PUNCTUATION, *LPPUNCTUATION;

#define CPPCOMMENT "//"
#define CPPMULTILINECOMMENTBEGIN "/*"
#define CPPMULTILINECOMMENTEND "*/"

// C++ punctuation list
static PUNCTUATION CPPPUNCTUATIONS[] = 
{
	{">>=",PUNCTUATION_RSHIFT_ASSIGN},
	{"<<=",PUNCTUATION_LSHIFT_ASSIGN},
	{"...",PUNCTUATION_PARMETERS},
	{"##",PUNCTUATION_PREPROCESSORMERGE},
	{"&&",PUNCTUATION_LOGIC_AND},
	{"||",PUNCTUATION_LOGIC_OR},
	{">=",PUNCTUATION_LOGIC_GEQ},
	{"<=",PUNCTUATION_LOGIC_LEQ},
	{"==",PUNCTUATION_LOGIC_EQ},
	{"!=",PUNCTUATION_LOGIC_UNEQ},
	{"*=",PUNCTUATION_MUL_ASSIGN},
	{"/=",PUNCTUATION_DIV_ASSIGN},
	{"%=",PUNCTUATION_MOD_ASSIGN},
	{"+=",PUNCTUATION_ADD_ASSIGN},
	{"-=",PUNCTUATION_SUB_ASSIGN},
	{"++",PUNCTUATION_INC},
	{"--",PUNCTUATION_DEC},
	{"&=",PUNCTUATION_BIN_AND_ASSIGN},
	{"|=",PUNCTUATION_BIN_OR_ASSIGN},
	{"^=",PUNCTUATION_BIN_XOR_ASSIGN},
	{">>",PUNCTUATION_RSHIFT},
	{"<<",PUNCTUATION_LSHIFT},
	{"->",PUNCTUATION_POINTERREF},
	{"::",PUNCTUATION_STATICMEMBER},
	{".",PUNCTUATION_MEMBER},
	{"*",PUNCTUATION_MUL},
	{"/",PUNCTUATION_DIV},
	{"%",PUNCTUATION_MOD},
	{"+",PUNCTUATION_ADD},
	{"-",PUNCTUATION_SUB},
	{"=",PUNCTUATION_ASSIGN},
	{"&",PUNCTUATION_BIN_AND},
	{"|",PUNCTUATION_BIN_OR},
	{"^",PUNCTUATION_BIN_XOR},
	{"~",PUNCTUATION_BIN_NOT},
	{"!",PUNCTUATION_LOGIC_NOT},
	{">",PUNCTUATION_LOGIC_GREATER},
	{"<",PUNCTUATION_LOGIC_LESS},
	{",",PUNCTUATION_COMMA},
	{";",PUNCTUATION_SEMICOLON},
	{":",PUNCTUATION_COLON},
	{"?",PUNCTUATION_QUESTIONMARK},
	{"(",PUNCTUATION_PARENTHESESOPEN},
	{")",PUNCTUATION_PARENTHESESCLOSE},
	{"{",PUNCTUATION_BRACEOPEN},
	{"}",PUNCTUATION_BRACECLOSE},
	{"[",PUNCTUATION_SQBRACKETOPEN},
	{"]",PUNCTUATION_SQBRACKETCLOSE},
	{"\\",PUNCTUATION_BACKSLASH},
	{"#",PUNCTUATION_PREPROCESSOR},
	{"$",PUNCTUATION_DOLLAR},
	{NULL,PUNCTUATION_NONE}
};

#define STRING_BLOCK 32		// Size of string allocation blocks

// This Structure represents a string object
typedef struct
{
	LPSTR Buffer;
	ULONG Block;
} STRING, *LPSTRING;

// This structure represents a token
typedef struct
{
	STRING Value;
	ULONG LineNumber;
	//ULONG LineSpan;	// Used to count number of lines a multiline string span over
	ULONG Type;
	ULONG TypeEx;
} TOKEN, *LPTOKEN;

#define FILE_BLOCK 4096		// Size of file allocation blocks

// This structure represents a lexer object, members should not be accessed directly
typedef struct
{
	FILE* File;
	LPSTR FileName;
	LPSTR FileBuffer;
	ULONG FileBufferLength;
	ULONG FilePosition;

	ULONG LineNumber;

	LPPUNCTUATION Punctuations;
	CHAR Comment[2];
	CHAR MultilineCommentBegin[2];
	CHAR MultilineCommentEnd[2];
} LEXER, *LPLEXER;

// This structure represents a lexer stream, members should not be accessed directly
typedef struct
{
	LONGLONG FilePointer;
	LPSTR FileBuffer;
	ULONG FileBufferLength;
	ULONG FilePosition;

	ULONG LineNumber;
} LEXERSTATE,*LPLEXERSTATE;

// Initialization functions
BOOL InitializeLexer(LPLEXER lexer,LPPUNCTUATION punctuations,LPSTR comment,LPCSTR multilineCommentBegin,LPCSTR multilineCommentEnd);
VOID UninitializeLexer(LPLEXER lexer);

BOOL InitializeToken(LPTOKEN token);
VOID UninitializeToken(LPTOKEN token);
BOOL ResetToken(LPTOKEN token);

// Input stream funtions
BOOL LoadFile(LPLEXER lexer,LPCSTR path);
VOID UnloadFile(LPLEXER lexer);

// Lexer state functions
BOOL StoreLexerState(LPLEXER lexer,LPLEXERSTATE state);
BOOL RestoreLexerState(LPLEXER lexer,LPLEXERSTATE state);
VOID FreeLexerState(LPLEXERSTATE state);

// Token stream functions
ULONG ExpectTokenString(LPLEXER lexer,LPCSTR string);
ULONG ExpectTokenType(LPLEXER lexer,ULONG type,ULONG typeex,LPTOKEN token);
ULONG ExpectTokenAny(LPLEXER lexer,LPTOKEN token);
ULONG PeekTokenString(LPLEXER lexer,LPCSTR string);
ULONG PeekTokenType(LPLEXER lexer,ULONG type,ULONG typeex,LPTOKEN token);
ULONG PeekTokenAny(LPLEXER lexer,LPTOKEN token);
ULONG SkipTokenType(LPLEXER lexer,ULONG type,ULONG typeex);
ULONG SkipTokenAny(LPLEXER lexer);

// Punctuation list helper functions
LPCSTR GetPunctuationName(LPLEXER lexer,ULONG id);
ULONG GetPunctuationId(LPLEXER lexer,LPCSTR name);

// Internal functions
ULONG ReadWhitespace(LPLEXER lexer);
ULONG ReadEscapeSequence(LPLEXER lexer,PCHAR sequence);
ULONG ReadString(LPLEXER lexer,LPTOKEN token);
ULONG ReadIdentifier(LPLEXER lexer,LPTOKEN token);
ULONG ReadNumber(LPLEXER lexer,LPTOKEN token);
ULONG ReadPunctuation(LPLEXER lexer,LPTOKEN token);
ULONG ReadToken(LPLEXER lexer,LPTOKEN token);

// Internal parsing functions
BOOL IsNumber(LPLEXER lexer);
BOOL IsString(LPLEXER lexer);
BOOL IsIdentifier(LPLEXER lexer);

// Internal error reporting functions
VOID LexerWarning(LPLEXER lexer,LPCSTR format,...);
VOID LexerError(LPLEXER lexer,LPCSTR format,...);

// Internal string manipulation functions
BOOL InitializeString(LPSTRING string);
VOID UninitializeString(LPSTRING string);
BOOL AppendChar(LPSTRING string,CHAR chr);