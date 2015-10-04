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

#include "Assembler.h"

#include "..\Lexer\Lexer.h"
#include "..\Lexer\Lexer.c"	// Nasty

BOOL Equal(LPCSTR str1,LPCSTR str2)
{
	while(str1[0] && str2[0])
	{
		if(str1[0] != str2[0])
			return FALSE;

		++str1;
		++str2;
	}

	return str1[0] == str2[0];	// If both '\0'
}

BOOL EqualNoCase(LPCSTR str1,LPCSTR str2)
{
	while(str1[0] && str2[0])
	{
		if(tolower(str1[0]) != tolower(str2[0]))
			return FALSE;

		++str1;
		++str2;
	}

	return str1[0] == str2[0];	// If both '\0'
}

// Second string is the mask
BOOL MatchNoCase(LPCSTR str1,LPCSTR str2)
{
	while(str1[0] && str2[0])
	{
		if(tolower(str1[0]) != tolower(str2[0]))
			return FALSE;

		++str1;
		++str2;
	}

	return str2[0] == 0;
}

// Advances the input string pointer if match
BOOL MatchNoCaseAdvance(LPCSTR* str1,LPCSTR str2)
{
	LPCSTR str = *str1;

	while(str[0] && str2[0])
	{
		if(tolower(str[0]) != tolower(str2[0]))
			return FALSE;

		++str;
		++str2;
	}

	if(str2[0] == 0)
	{
		// Match
		*str1 = str;

		return TRUE;
	}

	return FALSE;
}

BOOL InitializeAssembler(LPASSEMBLER assembler)
{
	memset(assembler,0,sizeof(ASSEMBLER));

	return TRUE;
}

VOID UninitializeAssembler(LPASSEMBLER assembler)
{
	FreeInstructions(&assembler->Instructions);
	FreeLabels(&assembler->Labels);
}

ULONG ReadLabelDefinition(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	TOKEN parameter;
	ULONG address;

	InitializeToken(&parameter);

	// Label definition
	if(PeekTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter) || !EqualNoCase(parameter.Value.Buffer,"equ"))
	{
		UninitializeToken(&parameter);
		return 0;
	}

	// Skip equ
	SkipTokenAny(lexer);

	// Need to reset token before reuse
	ResetToken(&parameter);

	// Get the label value
	if(ExpectTokenType(lexer,TOKEN_NUMBER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	// Covert address
	if(!TokenToUnsignedLong(&parameter,&address))
	{
		AssemblerError(lexer,&parameter,"invalid number format");
		UninitializeToken(&parameter);
		return -1;
	}

	UninitializeToken(&parameter);

	// Check if alias already exists
	if(GetLabel(assembler->Labels,token->Value.Buffer))
	{
		AssemblerError(lexer,token,"label with the name '%s' already defined",token->Value.Buffer);
		return -1;
	}

	// Add the label
	AddLabel(&assembler->Labels,token->Value.Buffer,address);

	return 2;	// Don't advance the current location
}

ULONG ReadLabel(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	// Label
	if(SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COLON))
		return 0;

	// Check if already defined
	if(GetLabel(assembler->Labels,token->Value.Buffer))
	{
		AssemblerError(lexer,token,"label with the same name already exists");
		return -1;
	}

	// Add the label
	AddLabel(&assembler->Labels,token->Value.Buffer,assembler->Location);

	return 2;	// Don't advance the current location
}

ULONG ReadDefine(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	TOKEN parameter;

	// Define word/halfword/byte
	if(!EqualNoCase(token->Value.Buffer,"dw") && !EqualNoCase(token->Value.Buffer,"dh") && !EqualNoCase(token->Value.Buffer,"db"))
		return 0;

	while(1)
	{
		ULONG data;

		InitializeToken(&parameter);

		if(ExpectTokenAny(lexer,&parameter))
		{
			UninitializeToken(&parameter);
			return -1;
		}

		if(parameter.Type == TOKEN_NUMBER)
		{
			// Convert to number
			if(!TokenToUnsignedLong(&parameter,&data))
			{
				AssemblerError(lexer,&parameter,"unsupported number format");
				UninitializeToken(&parameter);
				return -1;
			}

			if(EqualNoCase(token->Value.Buffer,"dw"))
			{
				// Generate instruction
				AddInstruction(&assembler->Instructions,INSTRUCTION_DATA,INSTRUCTION_DATA_32,assembler->Location,NULL,NULL,NULL,data,0,0,NULL,NULL,NULL);

				assembler->Location += 4;
			}
			else if(EqualNoCase(token->Value.Buffer,"dh"))
			{
				if(data != (data & 0xFFFF))
					AssemblerWarning(lexer,&parameter,"number too large");

				data &= 0xFFFF;

				// Generate instruction
				AddInstruction(&assembler->Instructions,INSTRUCTION_DATA,INSTRUCTION_DATA_16,assembler->Location,NULL,NULL,NULL,data,0,0,NULL,NULL,NULL);

				assembler->Location += 2;
			}
			else if(EqualNoCase(token->Value.Buffer,"db"))
			{
				if(data != (data & 0xFF))
					AssemblerWarning(lexer,&parameter,"number too large");

				data &= 0xFF;

				// Generate instruction
				AddInstruction(&assembler->Instructions,INSTRUCTION_DATA,INSTRUCTION_DATA_8,assembler->Location,NULL,NULL,NULL,data,0,0,NULL,NULL,NULL);

				assembler->Location += 1;
			}
			//else ASSERT(FALSE);
		}
		else if(parameter.Type == TOKEN_STRING)
		{
			LPCSTR buffer;
			// Convert to data
			for(buffer = parameter.Value.Buffer; buffer[0]; ++buffer)
			{
				AddInstruction(&assembler->Instructions,INSTRUCTION_DATA,INSTRUCTION_DATA_8,assembler->Location,NULL,NULL,NULL,buffer[0],0,0,NULL,NULL,NULL);

				assembler->Location += 1;
			}
		}
		else if(parameter.Type == TOKEN_LITERAL)
		{
			AddInstruction(&assembler->Instructions,INSTRUCTION_DATA,INSTRUCTION_DATA_8,assembler->Location,NULL,NULL,NULL,parameter.Value.Buffer[0],0,0,NULL,NULL,NULL);

			assembler->Location += 1;
		}
		else
		{
			AssemblerError(lexer,&parameter,"invalid type");
			UninitializeToken(&parameter);
			return -1;
		}

		UninitializeToken(&parameter);
		
		if(SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA))
			break;
	}

	if(assembler->Location % 4)
		assembler->Location += 4 - assembler->Location % 4;

	return 2;	// We manualy advance the current position
}

LPCONDITION GetCondition(LPCSTR name)
{
	ULONG i;

	for(i = 0; ARMCONDITIONS[i].Name; ++i)
		if(EqualNoCase(name,ARMCONDITIONS[i].Name))
			return &ARMCONDITIONS[i];

	return NULL;
}

LPREGISTER GetRegister(LPCSTR name)
{
	ULONG i;

	for(i = 0; ARMREGISTERS[i].Name; ++i)
		if(EqualNoCase(name,ARMREGISTERS[i].Name))
			return &ARMREGISTERS[i];

	return NULL;
}

BYTE GetShift(LPCSTR name)
{
	ULONG i;

	for(i = 0; ARMSHIFTS[i].Name; ++i)
		if(EqualNoCase(name,ARMSHIFTS[i].Name))
			return ARMSHIFTS[i].Type;

	return 0;
}

ULONG ReadOptionalShift(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token,LPSHIFTER shift)
{
	ULONG value;
	TOKEN parameter;

	if(token->Type != TOKEN_PUNCTUATION && token->TypeEx != PUNCTUATION_COMMA)
		return 0;

	InitializeToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	shift->Type = GetShift(parameter.Value.Buffer);
	if(!shift->Type)
	{
		AssemblerError(lexer,&parameter,"invalid shift type");
		UninitializeToken(&parameter);
		return -1;
	}

	if(ExpectTokenType(lexer,TOKEN_NUMBER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	if(!TokenToUnsignedLong(&parameter,&value))
	{
		AssemblerError(lexer,&parameter,"invalid number format");
		UninitializeToken(&parameter);
		return -1;
	}

	shift->Value = value;

	UninitializeToken(&parameter);

	return 1;
}

ULONG ReadOperand(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token,LPOPERAND operand)
{
	TOKEN parameter;

	if(token->Type != TOKEN_PUNCTUATION && token->TypeEx != PUNCTUATION_COMMA)
		return 0;

	InitializeToken(&parameter);

	if(ExpectTokenAny(lexer,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	if(parameter.Type == TOKEN_NUMBER)
	{
		ULONG immediate;

		if(!TokenToUnsignedLong(&parameter,&immediate))
		{
			AssemblerError(lexer,&parameter,"invalid number format");
			UninitializeToken(&parameter);
			return -1;
		}

		operand->Shift = immediate;
		operand->Type = SHIFT_IMM;
	}
	else if(parameter.Type == TOKEN_IDENTIFIER)
	{
		LPREGISTER registr;

		registr = GetRegister(parameter.Value.Buffer);
		if(!registr)
		{
			AssemblerError(lexer,&parameter,"invalid register");
			UninitializeToken(&parameter);
			return -1;
		}

		operand->Register = registr->Code;
		operand->Type = SHIFT_REG;

		if(!SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA))
		{
			ResetToken(&parameter);

			if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
			{
				UninitializeToken(&parameter);
				return -1;
			}

			operand->Type = GetShift(parameter.Value.Buffer);
			if(!operand->Type)
			{
				AssemblerError(lexer,&parameter,"invalid shift");
				UninitializeToken(&parameter);
				return -1;
			}

			ResetToken(&parameter);

			if(ExpectTokenAny(lexer,&parameter))
			{
				UninitializeToken(&parameter);
				return -1;
			}

			if(parameter.Type == TOKEN_NUMBER)
			{
				ULONG immediate;

				if(!TokenToUnsignedLong(&parameter,&immediate))
				{
					AssemblerError(lexer,&parameter,"invalid number format");
					UninitializeToken(&parameter);
					return -1;
				}

				operand->Shift = immediate;
				operand->Type |= SHIFT_IMM;
			}
			else if(parameter.Type == TOKEN_IDENTIFIER)
			{
				LPREGISTER registr;

				registr = GetRegister(parameter.Value.Buffer);
				if(!registr)
				{
					AssemblerError(lexer,&parameter,"invalid register");
					UninitializeToken(&parameter);
					return -1;
				}

				operand->Shift = registr->Code;
			}
			else
			{
				AssemblerError(lexer,&parameter,"invalid shift operand");
				UninitializeToken(&parameter);
				return -1;
			}
		}
	}
	else
	{
		AssemblerError(lexer,token,"invalid instruction");
		UninitializeToken(&parameter);
		return -1;
	}

	UninitializeToken(&parameter);

	return 1;
}

ULONG ReadInstructionBranch(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	LPCSTR instruction = token->Value.Buffer;
	ULONG typeex = 0;
	TOKEN address;
	LPCONDITION condition;

	// Branch and link
	if(!MatchNoCaseAdvance(&instruction,"bl"))
	{
		// Branch
		if(!MatchNoCaseAdvance(&instruction,"b"))
			return 0;
	}
	else
	{
		typeex |= INSTRUCTION_BRANCH_LINK;
	}

	// Get condition code
	condition = GetCondition(instruction);

	// Check if invalid condition code specified
	if(!condition && instruction[0])
	{
		AssemblerError(lexer,token,"invalid instruction");
		return -1;
	}

	InitializeToken(&address);

	// Get address
	if(ExpectTokenAny(lexer,&address) || (address.Type != TOKEN_IDENTIFIER && address.Type != TOKEN_NUMBER))
	{
		AssemblerError(lexer,&address,"expected address or label");
		UninitializeToken(&address);
		return -1;
	}

	// Check if its a label
	if(address.Type == TOKEN_IDENTIFIER)
	{
		// Generate instruction
		AddInstruction(&assembler->Instructions,INSTRUCTION_BRANCH,typeex,assembler->Location,condition,NULL,NULL,0,0,0,address.Value.Buffer,NULL,NULL);
	}
	else // Number
	{
		ULONG value;

		// Convert address
		if(!TokenToUnsignedLong(&address,&value))
		{
			AssemblerError(lexer,&address,"invalid number format");
			UninitializeToken(&address);
			return -1;
		}

		// Generate instruction
		AddInstruction(&assembler->Instructions,INSTRUCTION_BRANCH,typeex,assembler->Location,condition,NULL,NULL,value,0,0,NULL,NULL,NULL);
	}

	UninitializeToken(&address);

	return 1;
}

ULONG ReadInstructionLoadStore(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	LPCSTR instruction = token->Value.Buffer;
	ULONG type;
	ULONG typeex = 0;
	LPCONDITION condition;
	LPREGISTER source;
	TOKEN parameter;

	// Load
	if(!MatchNoCaseAdvance(&instruction,"ldr"))
	{
		// Store
		if(!MatchNoCaseAdvance(&instruction,"str"))
			return 0;

		type = INSTRUCTION_STORE;
	}
	else
	{
		type = INSTRUCTION_LOAD;
	}

	// Check if operand size specified
	if(MatchNoCaseAdvance(&instruction,"d"))
		typeex |= INSTRUCTION_LOAD_DOUBLEWORD;
	else if(MatchNoCaseAdvance(&instruction,"sh"))
		typeex |= INSTRUCTION_LOAD_SIGNED_HALFWORD;
	else if(MatchNoCaseAdvance(&instruction,"sb"))
		typeex |= INSTRUCTION_LOAD_SIGNED_BYTE;
	else if(MatchNoCaseAdvance(&instruction,"h"))
		typeex |= INSTRUCTION_LOAD_HALFWORD;
	else if(MatchNoCaseAdvance(&instruction,"b"))
		typeex |= INSTRUCTION_LOAD_BYTE;

	// TODO This should only be allowed in certan combinations of the previous flag
	if(MatchNoCaseAdvance(&instruction,"t"))
		typeex |= INSTRUCTION_LOAD_TRANSLATE;

	// Get condition code
	condition = GetCondition(instruction);

	// Check if invalid condition code specified
	if(!condition && instruction[0])
	{
		AssemblerError(lexer,token,"invalid instruction");
		return -1;
	}

	InitializeToken(&parameter);

	// Get first parameter
	if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	// Check if parameter is register
	source = GetRegister(parameter.Value.Buffer);
	if(!source)
	{
		AssemblerError(lexer,&parameter,"invalid source register");
		UninitializeToken(&parameter);
		return -1;
	}

	// Need to reset token before reuse
	ResetToken(&parameter);

	// Get parameter seperator
	// TODO Could use SkipTokenType but would not get error message
	if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	// Need to reset token before reuse
	ResetToken(&parameter);

	if(ExpectTokenAny(lexer,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	if(parameter.Type == TOKEN_PUNCTUATION && parameter.TypeEx == PUNCTUATION_SQBRACKETOPEN)
	{
		LPREGISTER destination;

		ResetToken(&parameter);

		// Read the second register
		if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
		{
			UninitializeToken(&parameter);
			return -1;
		}

		destination = GetRegister(parameter.Value.Buffer);
		if(!destination)
		{
			AssemblerError(lexer,&parameter,"invalid destination register");
			UninitializeToken(&parameter);
			return -1;
		}

		ResetToken(&parameter);

		if(ExpectTokenAny(lexer,&parameter))
		{
			UninitializeToken(&parameter);
			return -1;
		}

		if(parameter.Type == TOKEN_PUNCTUATION && parameter.TypeEx == PUNCTUATION_SQBRACKETCLOSE)
		{
			if(!PeekTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA,&parameter))
			{
				SkipTokenAny(lexer);

				ResetToken(&parameter);

				if(ExpectTokenAny(lexer,&parameter))
				{
					UninitializeToken(&parameter);
					return -1;
				}

				if(parameter.Type == TOKEN_NUMBER)
				{
					ULONG address;

					if(!TokenToUnsignedLong(&parameter,&address))
					{
						AssemblerError(lexer,&parameter,"invalid number format");
						UninitializeToken(&parameter);
						return -1;
					}
					
					AddInstruction(&assembler->Instructions,type,typeex|INSTRUCTION_LOAD_IMMEDIATE|INSTRUCTION_LOAD_POSTINDEX,assembler->Location,condition,NULL,NULL,source->Code,destination->Code,address,NULL,NULL,NULL);
				}
				else if(parameter.Type == TOKEN_IDENTIFIER || (parameter.Type == TOKEN_PUNCTUATION && (parameter.TypeEx == PUNCTUATION_ADD || parameter.TypeEx == PUNCTUATION_SUB)))
				{
					SHIFTER shift = {0,0};
					LPREGISTER offset;

					if(parameter.Type == TOKEN_PUNCTUATION)
					{
						ResetToken(&parameter);

						if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
						{
							UninitializeToken(&parameter);
							return -1;
						}
					}

					offset = GetRegister(parameter.Value.Buffer);
					if(!offset)
					{
						AssemblerError(lexer,&parameter,"invalid register");
						UninitializeToken(&parameter);
						return -1;
					}

					if(parameter.TypeEx == PUNCTUATION_SUB)
						typeex |= INSTRUCTION_LOAD_REVERSE;

					if(!SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA))
					{
						ResetToken(&parameter);

						if(ReadOptionalShift(assembler,lexer,&parameter,&shift) == -1)
						{
							UninitializeToken(&parameter);
							return -1;
						}
					}

					AddInstruction(&assembler->Instructions,type,typeex|INSTRUCTION_LOAD_POSTINDEX,assembler->Location,condition,shift.Type ? &shift : NULL,NULL,source->Code,destination->Code,offset->Code,NULL,NULL,NULL);
				}
				else
				{
					AssemblerError(lexer,&parameter,"invalid instruction");
					UninitializeToken(&parameter);
					return -1;
				}
			}
			else
			{
				if(!SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_LOGIC_NOT))
					typeex |= INSTRUCTION_LOAD_MODIFY;

				AddInstruction(&assembler->Instructions,type,typeex|INSTRUCTION_LOAD_POSTINDEX,assembler->Location,condition,NULL,NULL,source->Code,destination->Code,0,NULL,NULL,NULL);
			}
		}
		else if(parameter.Type == TOKEN_PUNCTUATION && parameter.TypeEx == PUNCTUATION_COMMA)
		{
			ResetToken(&parameter);

			if(ExpectTokenAny(lexer,&parameter))
			{
				UninitializeToken(&parameter);
				return -1;
			}

			if(parameter.Type == TOKEN_NUMBER)
			{
				ULONG address;

				if(!TokenToUnsignedLong(&parameter,&address))
				{
					AssemblerError(lexer,&parameter,"invalid number format");
					UninitializeToken(&parameter);
					return -1;
				}

				ResetToken(&parameter);

				if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_SQBRACKETCLOSE,&parameter))
				{
					UninitializeToken(&parameter);
					return -1;
				}

				if(!SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_LOGIC_NOT))
					typeex |= INSTRUCTION_LOAD_MODIFY;
				
				AddInstruction(&assembler->Instructions,type,typeex|INSTRUCTION_LOAD_IMMEDIATE,assembler->Location,condition,NULL,NULL,source->Code,destination->Code,address,NULL,NULL,NULL);
			}
			else if(parameter.Type == TOKEN_IDENTIFIER || (parameter.Type == TOKEN_PUNCTUATION && (parameter.TypeEx == PUNCTUATION_ADD || parameter.TypeEx == PUNCTUATION_SUB)))
			{
				SHIFTER shift = {0,0};
				LPREGISTER offset;

				if(parameter.Type == TOKEN_PUNCTUATION)
				{
					ResetToken(&parameter);

					if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
					{
						UninitializeToken(&parameter);
						return -1;
					}
				}

				offset = GetRegister(parameter.Value.Buffer);
				if(!offset)
				{
					AssemblerError(lexer,&parameter,"invalid register");
					UninitializeToken(&parameter);
					return -1;
				}

				if(parameter.TypeEx == PUNCTUATION_SUB)
					typeex |= INSTRUCTION_LOAD_REVERSE;

				if(!SkipTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA))
				{
					ResetToken(&parameter);

					if(ReadOptionalShift(assembler,lexer,&parameter,&shift) == -1)
					{
						UninitializeToken(&parameter);
						return -1;
					}
				}

				ResetToken(&parameter);

				if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_SQBRACKETCLOSE,&parameter))
				{
					UninitializeToken(&parameter);
					return -1;
				}

				AddInstruction(&assembler->Instructions,type,typeex,assembler->Location,condition,shift.Type ? &shift : NULL,NULL,source->Code,destination->Code,offset->Code,NULL,NULL,NULL);
			}
			else
			{
				AssemblerError(lexer,&parameter,"invalid instruction");
				UninitializeToken(&parameter);
				return -1;
			}
		}
		else
		{
			AssemblerError(lexer,&parameter,"invalid instruction");
			UninitializeToken(&parameter);
			return -1;
		}
	}
	// Label
	else if(parameter.Type == TOKEN_IDENTIFIER)
	{
		// Label
		AddInstruction(&assembler->Instructions,type,0,assembler->Location,condition,NULL,NULL,source->Code,0,0,NULL,parameter.Value.Buffer,NULL);
	}
	else
	{
		AssemblerError(lexer,&parameter,"invalid instruction");
		UninitializeToken(&parameter);
		return -1;
	}

	UninitializeToken(&parameter);

	return 1;
}

ULONG ReadInstructionMove(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	LPCSTR instruction = token->Value.Buffer;
	ULONG typeex = 0;
	LPCONDITION condition;
	LPREGISTER destination;
	OPERAND operand;
	TOKEN parameter;

	// Load
	if(!MatchNoCaseAdvance(&instruction,"mov"))
	{
		// Store
		if(!MatchNoCaseAdvance(&instruction,"mvn"))
			return 0;
	}
	else
	{
		typeex |= INSTRUCTION_MOVE_INVERSE;
	}

	// Check if operand size specified
	if(MatchNoCaseAdvance(&instruction,"s"))
		typeex |= INSTRUCTION_MOVE_STATUS;

	// Get condition code
	condition = GetCondition(instruction);

	// Check if invalid condition code specified
	if(!condition && instruction[0])
	{
		AssemblerError(lexer,token,"invalid instruction");
		return -1;
	}

	InitializeToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	destination = GetRegister(parameter.Value.Buffer);
	if(!destination)
	{
		AssemblerError(lexer,&parameter,"invalid destination register");
		return -1;
	}

	ResetToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	if(ReadOperand(assembler,lexer,&parameter,&operand) != 1)
	{
		UninitializeToken(&parameter);
		return -1;
	}

	AddInstruction(&assembler->Instructions,INSTRUCTION_MOVE,typeex,assembler->Location,condition,NULL,&operand,destination->Code,0,0,NULL,NULL,NULL);

	UninitializeToken(&parameter);

	return 1;
}

ULONG ReadInstructionAddSub(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	LPCSTR instruction = token->Value.Buffer;
	ULONG type;
	ULONG typeex = 0;
	LPCONDITION condition;
	LPREGISTER destination;
	LPREGISTER source;
	OPERAND operand;
	TOKEN parameter;

	// Load
	if(!MatchNoCaseAdvance(&instruction,"add"))
	{
		// Store
		if(!MatchNoCaseAdvance(&instruction,"adc"))
		{
			if(!MatchNoCaseAdvance(&instruction,"sub"))
			{
				if(!MatchNoCaseAdvance(&instruction,"sub"))
					return 0;

				type = INSTRUCTION_SUB;
				typeex |= INSTRUCTION_ADD_CARRY;
			}
			else
			{
				type = INSTRUCTION_SUB;
			}
		}
		else
		{
			type = INSTRUCTION_ADD;
			typeex |= INSTRUCTION_ADD_CARRY;
		}
	}
	else
	{
		type = INSTRUCTION_ADD;
	}

	// Check if operand size specified
	if(MatchNoCaseAdvance(&instruction,"s"))
		typeex |= INSTRUCTION_ADD_STATUS;

	// Get condition code
	condition = GetCondition(instruction);

	// Check if invalid condition code specified
	if(!condition && instruction[0])
	{
		AssemblerError(lexer,token,"invalid instruction");
		return -1;
	}

	InitializeToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	destination = GetRegister(parameter.Value.Buffer);
	if(!destination)
	{
		AssemblerError(lexer,&parameter,"invalid register");
		return -1;
	}

	ResetToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	ResetToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	source = GetRegister(parameter.Value.Buffer);
	if(!source)
	{
		AssemblerError(lexer,&parameter,"invalid register");
		return -1;
	}

	ResetToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	if(ReadOperand(assembler,lexer,&parameter,&operand) != 1)
	{
		UninitializeToken(&parameter);
		return -1;
	}

	AddInstruction(&assembler->Instructions,type,typeex,assembler->Location,condition,NULL,&operand,destination->Code,source->Code,0,NULL,NULL,NULL);

	UninitializeToken(&parameter);

	return 1;
}

ULONG ReadInstructionTest(LPASSEMBLER assembler,LPLEXER lexer,LPTOKEN token)
{
	LPCSTR instruction = token->Value.Buffer;
	ULONG typeex = 0;
	LPCONDITION condition;
	LPREGISTER destination;
	OPERAND operand;
	TOKEN parameter;

	// Load
	if(!MatchNoCaseAdvance(&instruction,"tst"))
	{
		// Store
		if(!MatchNoCaseAdvance(&instruction,"teq"))
			return 0;

		typeex |= INSTRUCTION_TEST_EQ;
	}

	// Get condition code
	condition = GetCondition(instruction);

	// Check if invalid condition code specified
	if(!condition && instruction[0])
	{
		AssemblerError(lexer,token,"invalid instruction");
		return -1;
	}

	InitializeToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	destination = GetRegister(parameter.Value.Buffer);
	if(!destination)
	{
		AssemblerError(lexer,&parameter,"invalid destination register");
		UninitializeToken(&parameter);
		return -1;
	}

	ResetToken(&parameter);

	if(ExpectTokenType(lexer,TOKEN_PUNCTUATION,PUNCTUATION_COMMA,&parameter))
	{
		UninitializeToken(&parameter);
		return -1;
	}

	if(ReadOperand(assembler,lexer,&parameter,&operand) != 1)
	{
		UninitializeToken(&parameter);
		return -1;
	}

	AddInstruction(&assembler->Instructions,INSTRUCTION_TEST,typeex,assembler->Location,condition,NULL,&operand,destination->Code,0,0,NULL,NULL,NULL);

	UninitializeToken(&parameter);

	return 1;
}

typedef ULONG(*LPREADFUNCTION)(LPASSEMBLER,LPLEXER,LPTOKEN);

static LPREADFUNCTION READFUNCTIONS[] = 
{
	ReadLabelDefinition,
	ReadLabel,
	ReadDefine,
	ReadInstructionBranch,
	ReadInstructionLoadStore,
	ReadInstructionMove,
	ReadInstructionAddSub,
	ReadInstructionTest,
};

BOOL AssembleFile(LPASSEMBLER assembler,LPCSTR path)
{
	ULONG error;
	LEXER lexer;

	InitializeLexer(&lexer,CPPPUNCTUATIONS,ASMCOMMENT,ASMMULTILINECOMMENTBEGIN,ASMMULTILINECOMMENTEND);

	if(!LoadFile(&lexer,path))
	{
		UninitializeLexer(&lexer);
		return FALSE;
	}

	while(1)
	{
		ULONG i;
		TOKEN token;

		InitializeToken(&token);

		// The current state expects only a identifier
		if(error = ExpectTokenType(&lexer,TOKEN_IDENTIFIER,TOKEN_NONE,&token))
		{
			UninitializeToken(&token);
			UninitializeLexer(&lexer);
			return error == ERROR_EOF;
		}

		for(i = 0; READFUNCTIONS[i]; ++i)
		{
			error = READFUNCTIONS[i](assembler,&lexer,&token);
			
			// Check if error
			if(error == -1)
			{
				UninitializeToken(&token);
				UninitializeLexer(&lexer);
				return FALSE;
			}

			// Advance location counter
			if(error == 1)
				assembler->Location += 4;

			// Check if processed
			if(error)
				break;
		}

		if(!READFUNCTIONS[i])
		{
			// Unknown
			AssemblerError(&lexer,&token,"unknown instruction '%s'",token.Value.Buffer);
			UninitializeToken(&token);
			UninitializeLexer(&lexer);
			return FALSE;
		}

		UninitializeToken(&token);
	}

	UninitializeLexer(&lexer);

	return TRUE;
}

VOID AssemblerWarning(LPLEXER lexer,LPTOKEN token,LPCSTR format,...)
{
	CHAR buffer[2048];
    
    va_list args;
    va_start(args,format);
	_vsnprintf(buffer,sizeof(buffer),format,args);
    va_end(args);

	if(lexer && token)
		printf("%s(%d): warning: %s.\n",lexer->FileName,token->LineNumber,buffer);
	else if(token)
		printf("%s: warning: %s.\n",lexer->FileName,buffer);
	else
		printf("warning: %s.\n",buffer);
}

VOID AssemblerError(LPLEXER lexer,LPTOKEN token,LPCSTR format,...)
{
	CHAR buffer[2048];
    
    va_list args;
    va_start(args,format);
	_vsnprintf(buffer,sizeof(buffer),format,args);
    va_end(args);

	if(lexer && token)
		printf("%s(%d): error: %s.\n",lexer->FileName,token->LineNumber,buffer);
	else if(lexer)
		printf("%s: error: %s.\n",lexer->FileName,buffer);
	else
		printf("error: %s.\n",lexer->FileName,buffer);
}

BOOL AddLabel(LPLABEL* head,LPCSTR name,ULONG address)
{
	LPLABEL label = (LPLABEL)malloc(sizeof(LABEL));
	if(!label)
		return FALSE;	// Should assert

	label->Name = _strdup(name);
	label->Address = address;
	label->Next = *head;

	*head = label;

	return TRUE;
}

LPLABEL GetLabel(LPLABEL head,LPCSTR name)
{
	while(head)
	{
		if(EqualNoCase(head->Name,name))
			return head;

		head = head->Next;
	}

	return NULL;	// Not found
}

VOID FreeLabels(LPLABEL* head)
{
	while(*head)
	{
		LPLABEL next = (*head)->Next;

		free((*head)->Name);
		free(*head);

		*head = next;
	}
}

BOOL AddInstruction(LPINSTRUCTION* head,ULONG type,ULONG typeex,ULONG location,LPCONDITION condition,LPSHIFTER shift,LPOPERAND operand,ULONG parameter0,ULONG parameter1,ULONG parameter2,LPCSTR label0,LPCSTR label1,LPCSTR label2)
{
	LPINSTRUCTION instruction = (LPINSTRUCTION)malloc(sizeof(INSTRUCTION));
	if(!instruction)
		return FALSE;	// Should assert

	memset(instruction,0,sizeof(INSTRUCTION));

	instruction->Type = type;
	instruction->TypeEx = typeex;
	instruction->Location = location;
	instruction->Condition = condition;
	instruction->Parameters[0] = parameter0;
	instruction->Parameters[1] = parameter1;
	instruction->Parameters[2] = parameter2;
	instruction->Labels[0] = _strdup(label0);
	instruction->Labels[1] = _strdup(label1);
	instruction->Labels[2] = _strdup(label2);

	if(shift)
		memcpy(&instruction->Shift,shift,sizeof(SHIFTER));

	if(operand)
		memcpy(&instruction->Operand,operand,sizeof(OPERAND));

	instruction->Next = *head;

	*head = instruction;

	return TRUE;
}

VOID FreeInstructions(LPINSTRUCTION* head)
{
	while(*head)
	{
		LPINSTRUCTION next = (*head)->Next;

		free((*head)->Labels[0]);
		free((*head)->Labels[1]);
		free((*head)->Labels[2]);
		free(*head);

		*head = next;
	}
}

BOOL TokenToUnsignedLong(LPTOKEN token,PULONG value)
{
	if(token->Type != TOKEN_NUMBER)
		return FALSE;

	switch(token->TypeEx & NUMBER_TYPE_MASK)	// Ignore sign
	{
	case NUMBER_INTEGER: *value = strtoul(token->Value.Buffer,NULL,10); return TRUE;
	case NUMBER_HEX: *value = strtoul(token->Value.Buffer,NULL,16); return TRUE;
	case NUMBER_OCTAL: *value = strtoul(token->Value.Buffer,NULL,8); return TRUE;
	}

	return FALSE;
}

BOOL TokenToLong(LPTOKEN token,PULONG value)
{
	if(token->Type != TOKEN_NUMBER)
		return FALSE;

	switch(token->TypeEx & NUMBER_TYPE_MASK)	// Ignore sign
	{
	case NUMBER_INTEGER: *value = strtol(token->Value.Buffer,NULL,10); return TRUE;
	case NUMBER_HEX: *value = strtol(token->Value.Buffer,NULL,16); return TRUE;
	case NUMBER_OCTAL: *value = strtol(token->Value.Buffer,NULL,8); return TRUE;
	}

	return FALSE;
}

BOOL AssembleBinary(LPASSEMBLER assembler,LPCSTR path)
{
	LPINSTRUCTION instruction;
	FILE* file;

	file = fopen(path,"wb");
	if(!file)
		return FALSE;

	for(instruction = assembler->Instructions; instruction; instruction = instruction->Next)
	{
		LPLABEL label = 0;
		ULONG encoded = 0;

		if(fseek(file,instruction->Location,SEEK_SET))
			DebugBreak();

		// Handle condition, will be skipped if not used
		if(instruction->Condition)
			encoded |= instruction->Condition->Code << 28;
		else
			encoded |= 0xE << 28;	// Always

		// Shifter
		if(instruction->Operand.Type == SHIFT_REG)
			encoded |= instruction->Operand.Register & 0x7;
		else if(instruction->Operand.Type == SHIFT_IMM)
		{
			encoded |= 1 << 25;

			// TODO Doens't support rotation
			encoded |= instruction->Operand.Shift;
		}

		switch(instruction->Type)
		{
		case INSTRUCTION_DATA:
			switch(instruction->TypeEx)
			{
			case INSTRUCTION_DATA_8:
				fwrite(&instruction->Parameters[0],1,1,file);
				break;
			case INSTRUCTION_DATA_16:
				fwrite(&instruction->Parameters[0],1,2,file);
				break;
			case INSTRUCTION_DATA_32:
				fwrite(&instruction->Parameters[0],1,4,file);
				break;
			default:
				//ASSERT(FALSE);
				DebugBreak();
				break;
			}
			break;
		
		case INSTRUCTION_BRANCH:
			encoded |= 1 << 27;
			encoded |= 1 << 25;

			if(instruction->TypeEx & INSTRUCTION_BRANCH_LINK)
				encoded |= 1 << 24;

			encoded |= (instruction->Parameters[0] - instruction->Location) & 0xFFFFFF;

			fwrite(&encoded,1,4,file);
			break;
		
		case INSTRUCTION_STORE:
		case INSTRUCTION_LOAD:
			encoded |= 1 << 26;

			if(instruction->Type == INSTRUCTION_LOAD)
				encoded |= 1 << 20;

			if(!(instruction->TypeEx & INSTRUCTION_LOAD_REVERSE))
				encoded |= 1 << 23;

			if(!(instruction->TypeEx & INSTRUCTION_LOAD_POSTINDEX))
				encoded |= 1 << 24;

			encoded |= (instruction->Parameters[0] - instruction->Location) & 0xFFFFFF;

			fwrite(&encoded,1,4,file);
			break;
		
		case INSTRUCTION_MOVE:
			encoded |= 1 << 24;
			encoded |= 1 << 23;

			if(instruction->TypeEx & INSTRUCTION_MOVE_STATUS)
				encoded |= 1 << 20;

			if(instruction->TypeEx & INSTRUCTION_MOVE_INVERSE)
				encoded |= 1 << 22;

			// Destination register
			encoded |= (instruction->Parameters[0] & 0xF) << 12;

			fwrite(&encoded,1,4,file);
			break;
		
		case INSTRUCTION_SUB:
		case INSTRUCTION_ADD:
			if(instruction->Type == INSTRUCTION_ADD)
			{
				if(instruction->Type == INSTRUCTION_ADD_CARRY)
					encoded |= 1 << 21;

				encoded |= 1 << 23;
			}
			else	// INSTRUCTION_SUB
			{
				if(instruction->Type == INSTRUCTION_ADD_CARRY)
					encoded |= 1 << 23;

				encoded |= 1 << 22;
			}

			if(instruction->TypeEx & INSTRUCTION_ADD_STATUS)
				encoded |= 1 << 20;

			// Destination register
			encoded |= (instruction->Parameters[0] & 0xF) << 16;

			// Source register
			encoded |= (instruction->Parameters[1] & 0xF) << 12;

			fwrite(&encoded,1,4,file);
			break;

		case INSTRUCTION_TEST:
			encoded |= 1 << 24;

			encoded |= 1 << 20;

			// Source register
			encoded |= (instruction->Parameters[0] & 0xF) << 16;

			fwrite(&encoded,1,4,file);
			break;
	
		default:
			//ASSERT(FALSE);
			DebugBreak();
			break;
		}
	}

	fclose(file);

	return TRUE;
}

BOOL AssembleLabels(LPASSEMBLER assembler)
{
	LPINSTRUCTION instruction;

	for(instruction = assembler->Instructions; instruction; instruction = instruction->Next)
	{
		if(instruction->Labels[0])
		{
			LPLABEL label = GetLabel(assembler->Labels,instruction->Labels[0]);
			if(!label)
			{
				AssemblerError(NULL,NULL,"failed to translate label %s",instruction->Labels[0]);
				return FALSE;
			}

			instruction->Parameters[0] = label->Address;
		}

		if(instruction->Labels[1])
		{
			LPLABEL label = GetLabel(assembler->Labels,instruction->Labels[1]);
			if(!label)
			{
				AssemblerError(NULL,NULL,"failed to translate label %s",instruction->Labels[1]);
				return FALSE;
			}

			instruction->Parameters[1] = label->Address;
		}

		if(instruction->Labels[2])
		{
			LPLABEL label = GetLabel(assembler->Labels,instruction->Labels[2]);
			if(!label)
			{
				AssemblerError(NULL,NULL,"failed to translate label %s",instruction->Labels[2]);
				return FALSE;
			}

			instruction->Parameters[2] = label->Address;
		}
	}
}