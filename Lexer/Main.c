#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>

#include "Lexer.h"

//#define _NO_OUTPUT

int main(void)
{
	LPCSTR source = "D:\\Irssi.txt";
	LPCSTR destination = "D:\\IrssiProcessed.txt";
	LARGE_INTEGER start,end,frequency;
	LEXER lexer;
	FILE* file;

#ifndef _NO_OUTPUT
	file = fopen(destination,"w");
	if(!file)
		return 1;
#endif

	InitializeLexer(&lexer,CPPPUNCTUATIONS,CPPCOMMENT,CPPMULTILINECOMMENTBEGIN,CPPMULTILINECOMMENTEND);
	//InitializeLexer(&lexer,CPPPUNCTUATIONS,";","<*","*>");

	if(!LoadFile(&lexer,source))
	{
		UninitializeLexer(&lexer);
		return 1;
	}

	QueryPerformanceCounter(&start);

	while(1)
	{
		TOKEN token;

		InitializeToken(&token);

		if(ExpectTokenAny(&lexer,&token))
			break;

#ifndef _NO_OUTPUT
		fprintf(file,"TOKEN\n{\n");
		fprintf(file,"  Line: %d\n",token.LineNumber);

		switch(token.Type)
		{
		case TOKEN_NONE:
			fprintf(file,"  Type: TOKEN_NONE\n\n");
			break;

		case TOKEN_PUNCTUATION:
			fprintf(file,"  Type: TOKEN_PUNCTUATION\n");
			fprintf(file,"  TypeEx: %d\n",token.TypeEx);
			fprintf(file,"  Value: '%s'\n",token.Value.Buffer);		// fprintf(file,"  '%s'\n",GetPunctuationName(token.TypeEx));
			break;

		case TOKEN_IDENTIFIER:
			fprintf(file,"  Type: TOKEN_IDENTIFIER\n");
			fprintf(file,"  Value: '%s'\n",token.Value.Buffer);
			break;

		case TOKEN_NUMBER:
			fprintf(file,"  Type: TOKEN_NUMBER\n");

			switch(token.TypeEx)
			{
			case NUMBER_INTEGER:
				fprintf(file,"  TypeEx: NUMBER_INTEGER\n");
				break;

			case NUMBER_NEGATIVE_INTEGER:
				fprintf(file,"  TypeEx: NUMBER_NEGATIVE_INTEGER\n");
				break;

			case NUMBER_HEX:
				fprintf(file,"  TypeEx: NUMBER_HEX\n");
				break;

			case NUMBER_NEGATIVE_HEX:
				fprintf(file,"  TypeEx: NUMBER_NEGATIVE_HEX\n");
				break;

			case NUMBER_OCTAL:
				fprintf(file,"  TypeEx: NUMBER_OCTAL\n");
				break;

			case NUMBER_NEGATIVE_OCTAL:
				fprintf(file,"  TypeEx: NUMBER_NEGATIVE_OCTAL\n");
				break;

			case NUMBER_FLOAT:
				fprintf(file,"  TypeEx: NUMBER_FLOAT\n");
				break;

			case NUMBER_NEGATIVE_FLOAT:
				fprintf(file,"  TypeEx: NUMBER_NEGATIVE_FLOAT\n");
				break;
			}

			fprintf(file,"  Value: '%s'\n",token.Value.Buffer);
			break;

		case TOKEN_STRING:
			fprintf(file,"  Type: TOKEN_STRING\n");
			fprintf(file,"  Value: '%s'\n",token.Value.Buffer);
			break;

		case TOKEN_LITERAL:
			fprintf(file,"  Type: TOKEN_LITERAL\n");
			fprintf(file,"  Value: '%s'\n",token.Value.Buffer);
			break;
		}

		fprintf(file,"}\n");
#endif

		UninitializeToken(&token);
	}

	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&frequency);

	UninitializeLexer(&lexer);

#ifndef _NO_OUTPUT
	fprintf(file,"\n------------------------------------------------------------------------------------------\n");
	fprintf(file,"- Source file: %s\n",source);
	fprintf(file,"- Parse time: %g miliseconds",(end.QuadPart - start.QuadPart) / (double)(frequency.QuadPart / 1000LL));
	
	fclose(file);
#endif

	printf("Source: %s\n",source);
	printf("Destination: %s\n",destination);
	printf("Parsing time: %g miliseconds\n",(end.QuadPart - start.QuadPart) / (double)(frequency.QuadPart / 1000LL));

	system("pause");
	return 0;
}