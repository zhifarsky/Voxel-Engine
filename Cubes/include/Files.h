#pragma once
#include "Typedefs.h"
#include "DataStructures.h"

#ifndef TEX_FOLDER
#define TEX_FOLDER "res/textures/"
#endif

#ifndef SHADER_FOLDER
#define SHADER_FOLDER "res/shaders/"
#endif

#ifndef FONT_FOLDER
#define FONT_FOLDER "res/fonts/"
#endif

#ifndef MESH_FOLDER
#define MESH_FOLDER "res/meshes/"
#endif

#ifndef WORLDS_FOLDER
#define WORLDS_FOLDER "worlds/"
#endif

enum class FileType {
	binary,
	text
};

// читает весь файл, выдел€€ под него пам€ть
u8* readEntireFile(const char* path, u32* outBufferSize, FileType fileType);

#define SerializeVariable(fileStream, var) fwrite(&var, sizeof(var), 1, fileStream);
#define DeserializeVariable(fileStream, var) fread(&var, sizeof(var), 1, fileStream);
