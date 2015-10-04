#include <windows.h>
#include <stdio.h>

#include "Assembler.h"

int main(void)
{
	LPCSTR input = "C:\\Test.asm";
	LPCSTR output = "C:\\Test.nb0";
	ASSEMBLER assembler;

	InitializeAssembler(&assembler);

	if(!AssembleFile(&assembler,input))
	{
		UninitializeAssembler(&assembler);
		system("pause");
		return 1;
	}

	if(!AssembleBinary(&assembler,output))
	{
		UninitializeAssembler(&assembler);
		system("pause");
		return 2;
	}

	UninitializeAssembler(&assembler);

	system("pause");
	return 0;
}

/*
int main(void)
{
	LPCSTR input = "C:\\Documents And Settings\\Administrator\\My Documents\\atlas\\vjezba7\\prog.p";
	LPCSTR output = "C:\\Test.nb0";
	CHAR line[1024];
	FILE* inputf,*outputf;

	inputf = fopen(input,"r");
	if(!inputf)
		return 1;
	
	outputf = fopen(output,"wb");
	if(!outputf)
		return 1;

	while(fgets(line,sizeof(line),inputf))
	{
		BYTE byte[4];
		LPSTR str = line;

		if((str[0] < '0' || str[0] > '9') && (str[0] < 'A' || str[0] > 'F'))
			continue;

		str += 8 + 2;

		byte[0] = strtol(str,NULL,16);

		str += 3;

		byte[1] = strtol(str,NULL,16);

		str += 3;

		byte[2] = strtol(str,NULL,16);

		str += 3;

		byte[3] = strtol(str,NULL,16);

		fwrite(byte,1,4,outputf);
	}

	fclose(inputf);
	fclose(outputf);

	return 0;
}

/*
000000E8  7F 00 00 E2  LCD	AND R0,R0,#7F 
000000EC  04 00 CC E5  	STRB R0,[R12,#4] 
000000F0  80 00 80 E3  	ORR R0,R0,#80 
000000F4  04 00 CC E5  	STRB R0,[R12,#4] 
000000F8  7F 00 00 E2  	AND R0,R0,#7F 
000000FC  04 00 CC E5  	STRB R0,[R12,#4] 
00000100  0E F0 A0 E1  	MOV R15,R14 
                        
00000104  00 FE FF FF  RTC	DW	0FFFFFE00 
00000108  00 FF FF FF  GPIO	DW	0FFFFFF00 
*/