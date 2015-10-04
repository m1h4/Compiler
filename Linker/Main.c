#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
	ULONG signature = IMAGE_NT_SIGNATURE;
	IMAGE_FILE_HEADER header;
	IMAGE_OPTIONAL_HEADER optional;
	BYTE sections[2],initialized,uninitialized;
	BYTE code[] = {0,0,0,0};
	ULONG baseofcode = 0x1000;
	ULONG baseofdata = 0x2000;
	ULONG entrypoint = 0x00401000;
	FILE* file;

	memset(&header,0,sizeof(header));
	memset(&optional,0,sizeof(optional));

	file = fopen("C:\\Test.exe","wb");
	if(!file)
		return 1;

	fwrite(&signature,1,sizeof(signature),file);

    header.Machine = IMAGE_FILE_MACHINE_I386;
	header.NumberOfSections = sizeof(sections)/sizeof(sections[0]);
	time((time_t*)&header.TimeDateStamp); 
	header.SizeOfOptionalHeader = sizeof(optional);
	header.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE|IMAGE_FILE_32BIT_MACHINE|IMAGE_FILE_RELOCS_STRIPPED;

	fwrite(&header,1,sizeof(header),file);

    optional.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    optional.MajorLinkerVersion = 0;
    optional.MinorLinkerVersion = 1;
    optional.SizeOfCode = sizeof(code);
    optional.SizeOfInitializedData = sizeof(initialized);
    optional.SizeOfUninitializedData = sizeof(uninitialized);
    optional.AddressOfEntryPoint = entrypoint;
    optional.BaseOfCode = baseofcode;
    optional.BaseOfData = baseofdata;
    optional.ImageBase = 0x00400000;
    optional.SectionAlignment = 0x1000;
    optional.FileAlignment = 0x200;
    optional.MajorOperatingSystemVersion = 4;
    optional.MinorOperatingSystemVersion = 0;
    optional.MajorImageVersion = 0;
    optional.MinorImageVersion = 0;
    optional.MajorSubsystemVersion = 4;
    optional.MinorSubsystemVersion = 0;
	optional.SizeOfImage = optional.SectionAlignment * 4;
	optional.SizeOfHeaders = ((sizeof(optional) + sizeof(header)) / optional.SectionAlignment + 1) * optional.SectionAlignment;
    optional.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    optional.SizeOfStackReserve = 0x00100000;
    optional.SizeOfStackCommit = 0x00001000;
    optional.SizeOfHeapReserve = 0x00100000;
    optional.SizeOfHeapCommit = 0x00001000;
    optional.NumberOfRvaAndSizes = 0;

	fwrite(&optional,1,sizeof(optional),file);

	fclose(file);

	return 0;
}