#include <Windows.h>
#include "Files.h"

u64 GetFileWriteTime(const char* path) {
	HANDLE hFile = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;
	
	FILETIME writeTime;
	GetFileTime(hFile, NULL, NULL, &writeTime);
	
	u64 res = writeTime.dwLowDateTime | ((u64)writeTime.dwHighDateTime << 32);
	return res;
}