//#include <filesystem>
//namespace fs = std::filesystem;
#include <stdlib.h>
#include <stdio.h>
#include "Files.h"
#include "Tools.h"

u8* readEntireFile(Arena* storage, const char* path, u32* outBufferSize, FileType fileType) {
	const char* mode = 0;
	switch (fileType)
	{
	case FileType::binary:	mode = "rb"; break;
	case FileType::text:	mode = "rt"; break;
	default:
		FatalError("Not implemented"); break;
	}

	FILE* file = fopen(path, mode);
	if (!file) {
		FatalError("Error on reading file\n");
		*outBufferSize = 0;
		return 0;
	}

	fseek(file, 0, SEEK_END);
	u32 fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	u8* fileBuffer = (u8*)ArenaPushZero(storage, fileSize + 1);
	if (!fileBuffer) {
		FatalError("Alloc failed\n");
		*outBufferSize = 0;
		return 0;
	}
	u32 bytesRead = fread(fileBuffer, 1, fileSize, file);
	fclose(file);

	if (fileType == FileType::text)
		fileBuffer[bytesRead] = '\0';

	*outBufferSize = fileSize;
	return fileBuffer;
}

// TODO: убрать эту перегрузку?
u8* readEntireFile(const char* path, u32* outBufferSize, FileType fileType) {
	const char* mode = 0;
	switch (fileType)
	{
	case FileType::binary:	mode = "rb"; break;
	case FileType::text:	mode = "rt"; break;
	default:
		FatalError("Not implemented"); break;
	}

	FILE* file = fopen(path, mode);
	if (!file) {
		FatalError("Error on reading file\n");
		*outBufferSize = 0;
		return 0;
	}

	fseek(file, 0, SEEK_END);
	u32 fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	u8* fileBuffer = (u8*)malloc(fileSize + 1);
	if (!fileBuffer) {
		FatalError("Alloc failed\n");
		*outBufferSize = 0;
		return 0;
	}
	u32 bytesRead = fread(fileBuffer, 1, fileSize, file);
	fclose(file);

	if (fileType == FileType::text)
		fileBuffer[bytesRead] = '\0';

	*outBufferSize = fileSize;
	return fileBuffer;
}
