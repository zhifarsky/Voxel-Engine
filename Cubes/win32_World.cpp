#include <Windows.h>
#include "World.h"
#include "Files.h"
#include "Tools.h"

void EnumerateWorlds(DynamicArray<GameWorldInfo>* infos) {
	WIN32_FIND_DATAA data;
	const char* path = WORLDS_FOLDER"*.*";

	HANDLE hFind = FindFirstFileA(path, &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
				if (
					strcmp(data.cFileName, ".") == 0 ||
					strcmp(data.cFileName, "..") == 0)
				{
					continue;
				}

				// TODO: parse world seed
				GameWorldInfo info = {0};
				strncpy_s(info.name, data.cFileName, _TRUNCATE);
				info.seed = 0;
				infos->append(info);
			}
		} while (FindNextFileA(hFind, &data));

		FindClose(hFind);
	}
}