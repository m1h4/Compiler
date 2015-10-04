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

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>

#include "Lexer.h"

BOOL InitializeLexer(LPLEXER lexer,LPPUNCTUATION punctuations,LPSTR comment,LPCSTR multilineCommentBegin,LPCSTR multilineCommentEnd)
{
	memset(lexer,0,sizeof(LEXER));

	lexer->Punctuations = punctuations;
	lexer->Comment[0] = comment[0];
	lexer->Comment[1] = comment[1];
	lexer->MultilineCommentBegin[0] = multilineCommentBegin[0];
	lexer->MultilineCommentBegin[1] = multilineCommentBegin[1];
	lexer->MultilineCommentEnd[0] = multilineCommentEnd[0];
	lexer->MultilineCommentEnd[1] = multilineCommentEnd[1];

	return TRUE;
}

VOID UninitializeLexer(LPLEXER lexer)
{
	UnloadFile(lexer);
}

BOOL StoreLexerState(LPLEXER lexer,LPLEXERSTATE state)
{
	state->FileBuffer = malloc(FILE_BLOCK);
	if(!state->FileBuffer)
		return FALSE;

	state->FilePointer = _ftelli64(lexer->File);
	state->FileBufferLength = lexer->FileBufferLength;
	state->FilePosition = lexer->FilePosition;
	CopyMemory(state->FileBuffer,lexer->FileBuffer,FILE_BLOCK);
	state->LineNumber = lexer->LineNumber;

	return TRUE;
}

BOOL RestoreLexerState(LPLEXER lexer,LPLEXERSTATE state)
{
	//if(!state->FileBuffer)	// Should do more than just check for buffer existance if we are error checking
	//	return FALSE;

	CopyMemory(lexer->FileBuffer,state->FileBuffer,FILE_BLOCK);

	lexer->FilePosition = state->FilePosition;
	lexer->LineNumber = state->LineNumber;
	lexer->FileBufferLength = state->FileBufferLength;

	_fseeki64(lexer->File,state->FilePointer,SEEK_SET);

	return TRUE;
}

VOID FreeLexerState(LPLEXERSTATE state)
{
	free(state->FileBuffer);
	state->FileBuffer = NULL;
}

BOOL InitializeToken(LPTOKEN token)
{
	memset(token,0,sizeof(TOKEN));

	InitializeString(&token->Value);

	return TRUE;
}

VOID UninitializeToken(LPTOKEN token)
{
	UninitializeString(&token->Value);
}

BOOL ResetToken(LPTOKEN token)
{
	UninitializeToken(token);
	return InitializeToken(token);
}

BOOL InitializeString(LPSTRING string)
{
	memset(string,0,sizeof(STRING));

	return TRUE;
}

VOID UninitializeString(LPSTRING string)
{
	free(string->Buffer);
	string->Buffer = NULL;
}

BOOL LoadFile(LPLEXER lexer,LPCSTR path)
{
	lexer->File = fopen(path,"r");
	if(!lexer->File)
		return FALSE;

	lexer->FileBuffer = malloc(FILE_BLOCK);
	if(!lexer->FileBuffer)
	{
		fclose(lexer->File);
		return FALSE;
	}

	// Do the inital buffer fill
	//lexer->FileBufferLength = (ULONG)fread(lexer->FileBuffer,1,FILE_BLOCK - 1,lexer->File);
	lexer->FileBufferLength = 0;	// Let the first Peek/Get operation cause a buffer refill

	lexer->FileBuffer[lexer->FileBufferLength] = 0;

	lexer->FileName = _strdup(path);
	lexer->FilePosition = 0;
	lexer->LineNumber = 1;

	return TRUE;
}

VOID UnloadFile(LPLEXER lexer)
{
	fclose(lexer->File);

	free(lexer->FileName);
	lexer->FileName = NULL;

	free(lexer->FileBuffer);
	lexer->FileBuffer = NULL;

	lexer->FilePosition = 0;
	lexer->FileBufferLength = 0;
}

ULONG GetCharEx(LPLEXER lexer,ULONG offset)
{
	if(lexer->FilePosition + offset >= lexer->FileBufferLength)
	{
		if(feof(lexer->File))
		{
			lexer->FilePosition = lexer->FileBufferLength;
			return EOF;
		}

		offset -= lexer->FileBufferLength - lexer->FilePosition;

		lexer->FileBufferLength = (ULONG)fread(lexer->FileBuffer,1,FILE_BLOCK - 1,lexer->File);
		lexer->FileBuffer[lexer->FileBufferLength] = 0;
		lexer->FilePosition = 0;

#ifdef _DEBUG
		OutputDebugStringA(__FUNCTION__": Refiling buffer.\n");
#endif

		if(lexer->FilePosition + offset >= lexer->FileBufferLength)
		{
			lexer->FilePosition = lexer->FileBufferLength;
			return EOF;
		}
	}

	lexer->FilePosition += offset;

	return lexer->FileBuffer[lexer->FilePosition++];
}

ULONG GetChar(LPLEXER lexer)
{
	return GetCharEx(lexer,0);
}

ULONG GetCharFar(LPLEXER lexer)
{
	return GetCharEx(lexer,1);
}

ULONG PeekCharEx(LPLEXER lexer,ULONG offset)
{
	if(lexer->FilePosition + offset >= lexer->FileBufferLength)
	{
		ULONG salvaged;

		if(feof(lexer->File))
			return EOF;

		salvaged = lexer->FileBufferLength - lexer->FilePosition;

		MoveMemory(lexer->FileBuffer,lexer->FileBuffer + lexer->FilePosition,salvaged);

		lexer->FileBufferLength = salvaged + (ULONG)fread(lexer->FileBuffer + salvaged,1,FILE_BLOCK - 1 - salvaged,lexer->File);
		lexer->FileBuffer[lexer->FileBufferLength] = 0;
		lexer->FilePosition = 0;

#ifdef _DEBUG
		OutputDebugStringA(__FUNCTION__": Refiling buffer.\n");
#endif
	}

	// TODO Check if removing this line would cause problems
	if(!lexer->FileBuffer[lexer->FilePosition + offset])
		return EOF;

	return lexer->FileBuffer[lexer->FilePosition + offset];
}

ULONG PeekChar(LPLEXER lexer)
{
	return PeekCharEx(lexer,0);
}

ULONG PeekCharFar(LPLEXER lexer)
{
	return PeekCharEx(lexer,1);
}

ULONG ReadWhitespace(LPLEXER lexer)
{
	while(1)
	{
		// Whitespace chars
		while(PeekChar(lexer) <= ' ')
		{
			if(PeekChar(lexer) == '\n')
				++lexer->LineNumber;

			GetChar(lexer);
		}

		if(PeekChar(lexer) == EOF)
			return ERROR_EOF;

		// Single-line Comments
		if(lexer->Comment[0] && PeekChar(lexer) == lexer->Comment[0] && (!lexer->Comment[1] || PeekCharFar(lexer) == lexer->Comment[1]))
		{
			if(!lexer->Comment[1])
				GetChar(lexer);
			else
				GetCharFar(lexer);

			while(PeekChar(lexer) != '\n')
			{
				if(PeekChar(lexer) == EOF)
					return ERROR_EOF;

				GetChar(lexer);
			}

			continue;
		}
		// Multi-line Comments
		else if(lexer->MultilineCommentBegin[0] && PeekChar(lexer) == lexer->MultilineCommentBegin[0] && (!lexer->MultilineCommentBegin[1] || PeekCharFar(lexer) == lexer->MultilineCommentBegin[1]))
		{
			if(!lexer->MultilineCommentBegin[1])
				GetChar(lexer);
			else
				GetCharFar(lexer);

			while(1)
			{
				if(PeekChar(lexer) == EOF)
					return ERROR_EOF;
				else if(PeekChar(lexer) == '\n')
					lexer->LineNumber++;
				else if(PeekChar(lexer) == lexer->MultilineCommentBegin[0] && (!lexer->MultilineCommentBegin[1] || PeekCharFar(lexer) == lexer->MultilineCommentBegin[1]))
					LexerWarning(lexer,"nested comment");
				else if(PeekChar(lexer) == lexer->MultilineCommentEnd[0] && (!lexer->MultilineCommentEnd[1] || PeekCharFar(lexer) == lexer->MultilineCommentEnd[1]))
				{
					if(!lexer->MultilineCommentEnd[1])
						GetChar(lexer);
					else
						GetCharFar(lexer);

					break;
				}

				GetChar(lexer);
			}

			continue;
		}

		break;
	}

	return ERROR_NONE;
}

ULONG ReadEscapeSequence(LPLEXER lexer,PCHAR escape)
{
	ULONG i;
	ULONG total;
	ULONG current;

	// Skip the '\\'
	GetChar(lexer);

	if(PeekChar(lexer) == EOF)
	{
		LexerError(lexer,"escape sequence not complete");
		return ERROR_EOF;
	}

	switch(PeekChar(lexer))
	{
		case '\\':
			GetChar(lexer);
			escape[0] = '\\';
			break;

		case 'n':
			GetChar(lexer);
			escape[0] = '\n';
			break;

		case 'r':
			GetChar(lexer);
			escape[0] = '\r';
			break;

		case 't':
			GetChar(lexer);
			escape[0] = '\t';
			break;

		case 'v':
			GetChar(lexer);
			escape[0] = '\v';
			break;

		case 'b':
			GetChar(lexer);
			escape[0] = '\b';
			break;

		case 'f':
			GetChar(lexer);
			escape[0] = '\f';
			break;

		case 'a':
			GetChar(lexer);
			escape[0] = '\a';
			break;

		case '\'':
			GetChar(lexer);
			escape[0] = '\'';
			break;

		case '\"':
			GetChar(lexer);
			escape[0] = '\"';
			break;

		case '\?':
			GetChar(lexer);
			escape[0] = '\?';
			break;

		case 'x':	// Hex
		{
			// Skip the inital
			GetChar(lexer);

			for(i = 0, total = 0; ; ++i, GetChar(lexer))
			{
				if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9')
					current = PeekChar(lexer) - '0';
				else if(PeekChar(lexer) >= 'A' && PeekChar(lexer) <= 'F')
					current = PeekChar(lexer) - 'A' + 10;
				else if(PeekChar(lexer) >= 'a' && PeekChar(lexer) <= 'f')
					current = PeekChar(lexer) - 'a' + 10;
				else
					break;

				total = (total << 4) + current;
			}

			if(total > 0xFF)
			{
				LexerWarning(lexer,"too large value in escape character");
				total = 0xFF;
			}

			escape[0] = (CHAR)total;
			break;
		}
		case '0':	// Octal or null char
		{
			// Skip the inital
			GetChar(lexer);

			// Check for null char
			if(PeekChar(lexer) < '0' && PeekChar(lexer) > '9')
			{
				GetChar(lexer);
				escape[0] = '\0';
				break;
			}

			for(i = 0, total = 0; ; ++i, GetChar(lexer))
			{
				if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '7')
					current = PeekChar(lexer) - '0';
				else
				{
					if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9' && i < 3)
						LexerWarning(lexer,"decimal digit terminates octal escape sequence");	

					break;
				}

				total = (total << 3) + current;
			}

			if(total > 0xFF)
			{
				LexerWarning(lexer,"too large value in escape character");
				total = 0xFF;
			}

			escape[0] = (CHAR)total;
			break;
		}
		default:
		{
			if(PeekChar(lexer) < '0' && PeekChar(lexer) > '9')
			{
				LexerError(lexer,"unknown escape sequence");
				break;
			}
			
			// Decimal
			
			// Skip the inital
			GetChar(lexer);

			for(i = 0, total = 0; ; ++i, GetChar(lexer))
			{
				if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9')
					current = PeekChar(lexer) - '0';
				else
					break;

				total = (total << 3) + (total << 1) + current;
			}

			if(total > 0xFF)
			{
				LexerWarning(lexer,"too large value in escape character");
				total = 0xFF;
			}

			escape[0] = (CHAR)total;
			break;
		}
	}

	return 0;
}

ULONG ReadString(LPLEXER lexer,LPTOKEN token)
{
	// Remember the type of quote
	ULONG quote = PeekChar(lexer);

	// See what type of string is it
	if(quote == '\"')
		token->Type = TOKEN_STRING;
	else if(quote == '\'')
		token->Type = TOKEN_LITERAL;
	//else
		// Shoul not come here

	token->LineNumber = lexer->LineNumber;

	// Skip quote
	GetChar(lexer);

	// Inital alloc of string
	AppendChar(&token->Value,0);	// TODO This was added 25.10.2008 as a fix for (null) empty strings

	while(1)
	{
		if(PeekChar(lexer) == EOF)
		{
			LexerError(lexer,"missing trailing quote");
			return ERROR_EOF;
		}

		// Escape sequence
		if(PeekChar(lexer) == '\\')
		{
			CHAR escape;
			ULONG error;

			if(error = ReadEscapeSequence(lexer,&escape))
				return error;

			AppendChar(&token->Value,escape);

			continue;
		}
		// Terminating quote
		else if(PeekChar(lexer) == quote)
		{
			LEXERSTATE state;

			// Skip terminating quote
			GetChar(lexer);

			// Concatenate sequental strings seperated by whitespace
			StoreLexerState(lexer,&state);

			if(!ReadWhitespace(lexer) && PeekChar(lexer) == quote)
			{
				FreeLexerState(&state);
				GetChar(lexer);
				continue;
			}

			RestoreLexerState(lexer,&state);
			FreeLexerState(&state);

			break;
		}
		else if(PeekChar(lexer) == '\n')
		{
			LexerWarning(lexer,"line break in string");

			++lexer->LineNumber;
			//++token->Span;
		}
		else
			AppendChar(&token->Value,(CHAR)PeekChar(lexer));
	
		GetChar(lexer);
	}

	if(token->Type == TOKEN_LITERAL && strlen(token->Value.Buffer) > 1)
		LexerWarning(lexer,"literal is longer that one character");

	return 0;
}

ULONG ReadIdentifier(LPLEXER lexer,LPTOKEN token)
{
	token->Type = TOKEN_IDENTIFIER;
	token->LineNumber = lexer->LineNumber;

	while((PeekChar(lexer) >= 'a' && PeekChar(lexer) <= 'z') || (PeekChar(lexer) >= 'A' && PeekChar(lexer) <= 'Z') || (PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9') || PeekChar(lexer) == '_')
		AppendChar(&token->Value,(CHAR)GetChar(lexer));

	// TODO Unnecessary?
	if(PeekChar(lexer) == EOF)
		return ERROR_INVALID;

	return ERROR_NONE;
}

ULONG ReadNumber(LPLEXER lexer,LPTOKEN token)
{
	ULONG i;

	token->Type = TOKEN_NUMBER;
	token->LineNumber = lexer->LineNumber;

	// Set sign
	if(PeekChar(lexer) == '-')
	{
		// Negative
		AppendChar(&token->Value,(CHAR)GetChar(lexer));
		token->TypeEx = NUMBER_SIGN_MASK;
	}
	else if(PeekChar(lexer) == '+')
	{
		// Positive
		AppendChar(&token->Value,(CHAR)GetChar(lexer));
		token->TypeEx = 0;
	}

	// Hex
	if(PeekChar(lexer) == '0' && (PeekCharFar(lexer) == 'x' || PeekCharFar(lexer) == 'X'))
	{
		GetCharFar(lexer);

		token->TypeEx |= NUMBER_HEX;

		for(i = 0; ; ++i, GetChar(lexer))
		{
			if((PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9') || (PeekChar(lexer) >= 'A' && PeekChar(lexer) <= 'F') || (PeekChar(lexer) >= 'a' && PeekChar(lexer) <= 'f'))
				AppendChar(&token->Value,(CHAR)PeekChar(lexer));
			else
			{
				if(!strlen(token->Value.Buffer))
					LexerError(lexer,"hex number must have at least one digit");

				break;
			}
		}
	}
	// Octal
	else if(PeekChar(lexer) == '0' && PeekCharFar(lexer) != '.')	// TODO Floating point numbers may have more than one leading zero
	{
		//GetChar(lexer);	// WTF

		token->TypeEx |= NUMBER_OCTAL;

		for(i = 0; ; ++i, GetChar(lexer))
		{
			if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '7')
				AppendChar(&token->Value,(CHAR)PeekChar(lexer));
			else
			{
				if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9')
					LexerError(lexer,"decimal digit in octal number");	
				else if((PeekChar(lexer) >= 'a' && PeekChar(lexer) <= 'f') || (PeekChar(lexer) >= 'A' && PeekChar(lexer) <= 'F'))
					LexerError(lexer,"hex digit in octal number");	

				break;
			}
		}
	}
	// Integer or Floating point
	else
	{
		ULONG dot = 0;

		while(1)
		{
			if(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9')
			{
			}
			else if(PeekChar(lexer) == '.')
				dot++;
			else
				break;

			AppendChar(&token->Value,(CHAR)GetChar(lexer));
		}

		// Scientific notation
		if(PeekChar(lexer) == 'e' && !dot)
			dot++;

		// Floating point
		if(dot == 1)
		{
			token->TypeEx |= NUMBER_FLOAT;

			if(PeekChar(lexer) == 'e')
			{
				AppendChar(&token->Value,(CHAR)GetChar(lexer));

				if(PeekChar(lexer) == '-' || PeekChar(lexer) == '+')
					AppendChar(&token->Value,(CHAR)GetChar(lexer));

				while(PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9')
					AppendChar(&token->Value,(CHAR)GetChar(lexer));
			}
		}
		else if(dot)
			LexerError(lexer,"floating point number has more than one dot");
		// Integer
		else
			token->TypeEx |= NUMBER_INTEGER;
	}

	return ERROR_NONE;
}

ULONG ReadPunctuation(LPLEXER lexer,LPTOKEN token)
{
	ULONG i;

	for(i = 0; lexer->Punctuations[i].name; ++i)
	{
		ULONG j;

		for(j = 0; PeekCharEx(lexer,j) && lexer->Punctuations[i].name[j]; ++j)
			if(PeekCharEx(lexer,j) != lexer->Punctuations[i].name[j])
				break;

		// Have we gotten through the whole punctuation
		if(!lexer->Punctuations[i].name[j])
		{
			GetCharEx(lexer,j - 1);

			token->LineNumber = lexer->LineNumber;
			token->Type = TOKEN_PUNCTUATION;
			token->TypeEx = lexer->Punctuations[i].id;
			token->Value.Buffer = _strdup(lexer->Punctuations[i].name);
			token->Value.Block = (ULONG)strlen(lexer->Punctuations[i].name) + 1;

			return ERROR_NONE;
		}
	}

	return ERROR_INVALID;
}

BOOL IsNumber(LPLEXER lexer)
{
	// Digits and decimal dot
	if((PeekChar(lexer) >= '0' && PeekChar(lexer) <= '9') || (PeekChar(lexer) == '.' && PeekCharFar(lexer) >= '0' && PeekCharFar(lexer) <= '9'))
		return TRUE;

	// Negative numbers
	if(PeekChar(lexer) == '-' && ((PeekCharFar(lexer) >= '0' && PeekCharFar(lexer) <= '9') || (PeekCharFar(lexer) == '.' && (PeekCharEx(lexer,2) >= '0' && PeekCharEx(lexer,2) <= '9'))))
		return TRUE;

	// Positive numbers
	if(PeekChar(lexer) == '+' && ((PeekCharFar(lexer) >= '0' && PeekCharFar(lexer) <= '9') || (PeekCharFar(lexer) == '.' && (PeekCharEx(lexer,2) >= '0' && PeekCharEx(lexer,2) <= '9'))))
		return TRUE;

	return FALSE;
}

BOOL IsString(LPLEXER lexer)
{
	// Single and double quotes
	if(PeekChar(lexer) == '\"' || PeekChar(lexer) == '\'')
		return TRUE;

	return FALSE;
}

BOOL IsIdentifier(LPLEXER lexer)
{
	// Only letters and underscore
	if((PeekChar(lexer) >= 'a' && PeekChar(lexer) <= 'z') || (PeekChar(lexer) >= 'A' && PeekChar(lexer) <= 'Z') || PeekChar(lexer) == '_')
		return TRUE;

	return FALSE;
}

ULONG ReadToken(LPLEXER lexer,LPTOKEN token)
{
	ULONG error;

	// Read inital whitespace up to token
	if(error = ReadWhitespace(lexer))
		return error;

	// Number
	if(IsNumber(lexer))
	{
		if(error = ReadNumber(lexer,token))
			return error;
	}

	// String
	else if(IsString(lexer))
	{
		if(error = ReadString(lexer,token))
			return error;
	}

	// Identifier
	else if(IsIdentifier(lexer))
	{
		if(error = ReadIdentifier(lexer,token))
			return error;
	}

	// Punctuation
	else
	{
		if(error = ReadPunctuation(lexer,token))
			return error;
	}

	return ERROR_NONE;
}

ULONG ExpectTokenString(LPLEXER lexer,LPCSTR string)
{
	ULONG error;
	TOKEN token;

	InitializeToken(&token);

	if(error = ReadToken(lexer,&token))
	{
		if(error != ERROR_EOF)
			LexerError(lexer,"cound not find expected '%s'",string);

		UninitializeToken(&token);
		return error;
	}

	if(strcmp(token.Value.Buffer,string))
	{
		LexerError(lexer,"expected '%s' but found '%s'",string,token.Value);
		UninitializeToken(&token);
		return ERROR_INVALID;
	}

	UninitializeToken(&token);

	return ERROR_NONE;
}

ULONG ExpectTokenType(LPLEXER lexer,ULONG type,ULONG typeex,LPTOKEN token)
{
	ULONG error;

	if(error = ReadToken(lexer,token))
	{
		if(error != ERROR_EOF)
			LexerError(lexer,"could not read expected token");

		return error;
	}

	if(token->Type != type)
	{
		LPCSTR typestring;

		switch(type)
		{
		case TOKEN_PUNCTUATION:
			//typestring = "punctuation";
			typestring = GetPunctuationName(lexer,typeex);
			break;

		case TOKEN_IDENTIFIER:
			typestring = "identifier";
			break;

		case TOKEN_NUMBER:
			typestring = "number";
			break;

		case TOKEN_STRING:
			typestring = "string";
			break;

		case TOKEN_LITERAL:
			typestring = "literal";
			break;

		default:
			typestring = "unknown";
			break;
		}

		LexerError(lexer,"expected '%s' but found '%s'",typestring,token->Value);
		return ERROR_INVALID;
	}

	if(typeex && token->TypeEx != typeex)
	{
		if(token->Type == TOKEN_PUNCTUATION)
		{
			LexerError(lexer,"expected '%s' but found '%s'",GetPunctuationName(lexer,typeex),token->Value);
			return ERROR_INVALID;
		}
		else
		{
			LPCSTR typeexstring;

			switch(typeex)
			{
			case NUMBER_INTEGER:
				typeexstring = "integer";
				break;

			case NUMBER_HEX:
				typeexstring = "hex";
				break;

			case NUMBER_OCTAL:
				typeexstring = "octal";
				break;

			case NUMBER_FLOAT:
				typeexstring = "float";
				break;

			default:
				typeexstring = "unknown";
				break;
			}

			LexerError(lexer,"expected '%s' but found '%s'",typeexstring,token->Value);
			return ERROR_INVALID;
		}
	}

	return ERROR_NONE;
}

ULONG ExpectTokenAny(LPLEXER lexer,LPTOKEN token)
{
	ULONG error;

	if(error = ReadToken(lexer,token))
	{
		if(error != ERROR_EOF)
			LexerError(lexer,"could not read expected token");

		return error;
	}

	return ERROR_NONE;
}

ULONG PeekTokenString(LPLEXER lexer,LPCSTR string)
{
	LEXERSTATE state;
	ULONG error;
	TOKEN token;

	StoreLexerState(lexer,&state);

	InitializeToken(&token);

	if(error = ReadToken(lexer,&token))
	{
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return error;
	}

	if(strcmp(token.Value.Buffer,string))
	{
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return ERROR_INVALID;
	}

	UninitializeToken(&token);

	RestoreLexerState(lexer,&state);
	FreeLexerState(&state);

	return ERROR_NONE;
}

ULONG PeekTokenType(LPLEXER lexer,ULONG type,ULONG typeex,LPTOKEN token)
{
	LEXERSTATE state;
	ULONG error;

	StoreLexerState(lexer,&state);

	if(error = ReadToken(lexer,token))
	{
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return error;
	}

	if(token->Type != type)
	{
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return ERROR_INVALID;
	}

	if(typeex && token->TypeEx != typeex)
	{
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return ERROR_INVALID;
	}

	RestoreLexerState(lexer,&state);
	FreeLexerState(&state);

	return ERROR_NONE;
}

ULONG PeekTokenAny(LPLEXER lexer,LPTOKEN token)
{
	LEXERSTATE state;
	ULONG error;

	StoreLexerState(lexer,&state);

	if(error = ReadToken(lexer,token))
	{
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return error;
	}

	RestoreLexerState(lexer,&state);
	FreeLexerState(&state);

	return ERROR_NONE;
}

ULONG SkipTokenAny(LPLEXER lexer)
{
	ULONG error;
	TOKEN token;

	InitializeToken(&token);

	error = ReadToken(lexer,&token);

	UninitializeToken(&token);
	return error;
}

ULONG SkipTokenType(LPLEXER lexer,ULONG type,ULONG typeex)
{
	LEXERSTATE state;
	ULONG error;
	TOKEN token;

	StoreLexerState(lexer,&state);

	InitializeToken(&token);

	if(error = ReadToken(lexer,&token))
	{
		UninitializeToken(&token);
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return error;
	}

	if(token.Type != type)
	{
		UninitializeToken(&token);
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return ERROR_INVALID;
	}

	if(typeex && token.TypeEx != typeex)
	{
		UninitializeToken(&token);
		RestoreLexerState(lexer,&state);
		FreeLexerState(&state);
		return ERROR_INVALID;
	}

	UninitializeToken(&token);
	FreeLexerState(&state);
	return ERROR_NONE;
}

VOID LexerWarning(LPLEXER lexer,LPCSTR format,...)
{
	CHAR buffer[2048];
    
    va_list args;
    va_start(args,format);
	_vsnprintf(buffer,sizeof(buffer),format,args);
    va_end(args);

	printf("%s(%d): warning: %s.\n",lexer->FileName,lexer->LineNumber,buffer);
}

VOID LexerError(LPLEXER lexer,LPCSTR format,...)
{
	CHAR buffer[2048];
    
    va_list args;
    va_start(args,format);
	_vsnprintf(buffer,sizeof(buffer),format,args);
    va_end(args);

	printf("%s(%d): error: %s.\n",lexer->FileName,lexer->LineNumber,buffer);
}

LPCSTR GetPunctuationName(LPLEXER lexer,ULONG id)
{
	ULONG i;

	for(i = 0; lexer->Punctuations[i].name; ++i)
		if(lexer->Punctuations[i].id == id)
			return lexer->Punctuations[i].name;

	return NULL;
}

ULONG GetPunctuationId(LPLEXER lexer,LPCSTR name)
{
	ULONG i;

	for(i = 0; lexer->Punctuations[i].name; ++i)
		if(!strcmp(lexer->Punctuations[i].name,name))
			return lexer->Punctuations[i].id;

	return PUNCTUATION_NONE;
}

BOOL AppendChar(LPSTRING string,CHAR chr)
{
	ULONG length;

	if(!string->Buffer)
	{
		string->Buffer = (LPSTR)malloc(STRING_BLOCK);
		if(!string->Buffer)
			return FALSE;

		string->Buffer[0] = 0;
		string->Block = STRING_BLOCK;
	}

	length = (ULONG)strlen(string->Buffer);
	if(string->Block == length + 1)
	{
		// Expand
		LPSTR buffer = (LPSTR)malloc(string->Block + STRING_BLOCK);
		if(!buffer)
			return FALSE;

		memcpy(buffer,string->Buffer,string->Block);

		free(string->Buffer);

		string->Buffer = buffer;
		string->Block += STRING_BLOCK;
	}

	string->Buffer[length] = chr;
	string->Buffer[length + 1] = 0;

	return TRUE;
}