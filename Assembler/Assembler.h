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

#include "..\Lexer\Lexer.h"

// Instruction types
#define INSTRUCTION_DATA			1
#define INSTRUCTION_BRANCH			2
#define INSTRUCTION_LOAD			3
#define INSTRUCTION_STORE			4
#define INSTRUCTION_MOVE			5
#define INSTRUCTION_ADD				6
#define INSTRUCTION_SUB				7
#define INSTRUCTION_TEST			8

// Load/Store ex types
#define INSTRUCTION_LOAD_DOUBLEWORD			1
#define INSTRUCTION_LOAD_TRANSLATE			2
#define INSTRUCTION_LOAD_HALFWORD			4
#define INSTRUCTION_LOAD_BYTE				8
#define	INSTRUCTION_LOAD_SIGNED_HALFWORD	16
#define INSTRUCTION_LOAD_SIGNED_BYTE		32
#define INSTRUCTION_LOAD_IMMEDIATE			64
#define INSTRUCTION_LOAD_REVERSE			128	// When - found before preindex/postindex offset register
#define INSTRUCTION_LOAD_MODIFY				256	// When preindex used, if ! added after instruction
#define INSTRUCTION_LOAD_POSTINDEX			512	// When postindex used

// Data ex types
#define INSTRUCTION_DATA_32			1
#define INSTRUCTION_DATA_16			2
#define INSTRUCTION_DATA_8			3

// Branch ex types
#define INSTRUCTION_BRANCH_LINK		1

// Move ex types
#define INSTRUCTION_MOVE_INVERSE	1
#define INSTRUCTION_MOVE_STATUS		2

// Add/Sub ex types
#define INSTRUCTION_ADD_STATUS		1
#define INSTRUCTION_ADD_CARRY		2

// Test ex types
#define INSTRUCTION_TEST_EQ			1

#define ASMCOMMENT ";"
#define ASMMULTILINECOMMENTBEGIN "<;"
#define ASMMULTILINECOMMENTEND ";>"

// Condition codes
typedef struct
{
	LPCSTR Name;
	BYTE Code;
} CONDITION,*LPCONDITION;

// Standard condition codes
static CONDITION ARMCONDITIONS[] =
{
	{"EQ",0x0},
	{"NE",0x1},
	{"CS",0x2},
	{"HS",0x2},
	{"CC",0x3},
	{"LO",0x3},
	{"MI",0x4},
	{"PL",0x5},
	{"VS",0x6},
	{"VC",0x7},
	{"HI",0x8},
	{"LS",0x9},
	{"GE",0xA},
	{"LT",0xB},
	{"GT",0xC},
	{"LE",0xD},
	{"AL",0xE},
	{"NV",0xF},	// TODO Remove?
	{NULL,0},
};

// Register
typedef struct
{
	LPCSTR Name;
	BYTE Code;
} REGISTER,*LPREGISTER;

// Standard registers
static REGISTER ARMREGISTERS[] =
{
	{"R0",0},
	{"R1",1},
	{"R2",2},
	{"R3",3},
	{"R4",4},
	{"R5",5},
	{"R6",6},
	{"R7",7},
	{"R8",8},
	{"R9",9},
	{"R10",10},
	{"R11",11},
	{"R12",12},
	{"R13",13},
	{"SP",13},	// Stack pointer
	{"R14",14},
	{"LR",14},	// Link register
	{"R15",15},
	{"PC",15},	// Program counter
	{NULL,0},
};

typedef struct
{
	LPCSTR Name;
	BYTE Type;
} SHIFT,*LPSHIFT;

#define SHIFT_LSL 1
#define SHIFT_LSR 2
#define SHIFT_ASL 3
#define SHIFT_ASR 4
#define SHIFT_ROR 5
#define SHIFT_RRX 6
#define SHIFT_REG 7

#define SHIFT_IMM 128

// Types of shifts
static SHIFT ARMSHIFTS[] =
{
	{"LSL",SHIFT_LSL},
	{"LSR",SHIFT_LSR},
	{"ASL",SHIFT_ASL},
	{"ASR",SHIFT_ASR},
	{"ROR",SHIFT_ROR},
	//{"RRX",SHIFT_RRX},
	{NULL,0},
};

typedef struct
{
	BYTE Type;
	BYTE Value;
} SHIFTER,*LPSHIFTER;

typedef struct
{
	BYTE Type;
	BYTE Register;
	BYTE Shift;
} OPERAND,*LPOPERAND;

typedef struct _LABEL
{
	LPSTR Name;
	ULONG Address;

	struct _LABEL* Next;
} LABEL,*LPLABEL;

typedef struct _INSTRUCTION
{
	ULONG Type;
	ULONG TypeEx;
	ULONG Location;
	ULONG Parameters[3];
	LPSTR Labels[3];
	LPCONDITION Condition;
	OPERAND Operand;
	SHIFTER Shift;

	struct _INSTRUCTION* Next;
} INSTRUCTION,*LPINSTRUCTION;

typedef struct
{
	ULONG Location;
	LPLABEL Labels;
	LPINSTRUCTION Instructions;
} ASSEMBLER,*LPASSEMBLER;

BOOL InitializeAssembler(LPASSEMBLER assembler);
VOID UninitializeAssembler(LPASSEMBLER assembler);

BOOL AssembleFile(LPASSEMBLER assembler,LPCSTR path);
BOOL AssembleBinary(LPASSEMBLER assembler,LPCSTR path);
BOOL AssembleLabels(LPASSEMBLER assembler);

BOOL AddLabel(LPLABEL* head,LPCSTR name,ULONG address);
LPLABEL GetLabel(LPLABEL head,LPCSTR name);
VOID FreeLabels(LPLABEL* head);

BOOL AddInstruction(LPINSTRUCTION* head,ULONG type,ULONG typeex,ULONG location,LPCONDITION condition,LPSHIFTER shift,LPOPERAND operand,ULONG parameter0,ULONG parameter1,ULONG parameter2,LPCSTR label0,LPCSTR label1,LPCSTR label2);
VOID FreeInstructions(LPINSTRUCTION* head);

BOOL TokenToLong(LPTOKEN token,PULONG value);
BOOL TokenToUnsignedLong(LPTOKEN token,PULONG value);

VOID AssemblerWarning(LPLEXER lexer,LPTOKEN token,LPCSTR format,...);
VOID AssemblerError(LPLEXER lexer,LPTOKEN token,LPCSTR format,...);