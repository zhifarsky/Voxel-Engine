#include <glad/glad.h>
#include <unordered_map>
#include <string>

#include "Files.h"
#include "Renderer.h"

Arena* g_TempStorage;

void Renderer::Begin(Arena* tempStorage) {
	g_TempStorage = tempStorage;
	g_RendererStats = { 0 };
}

std::unordered_map<std::string, s32> g_UniformLocationCache[(int)ShaderAssetID::COUNT];

Asset g_Shaders[(int)ShaderAssetID::COUNT] = {
	{.path = SHADER_FOLDER "block.glsl"},
	{.path = SHADER_FOLDER "blockShadow.glsl"},
	{.path = SHADER_FOLDER "polyMesh.glsl"},
	{.path = SHADER_FOLDER "polyMeshShadow.glsl"},
	{.path = SHADER_FOLDER "flat.glsl"},
	{.path = SHADER_FOLDER "sprite.glsl"},
	{.path = SHADER_FOLDER "uiElement.glsl"},
	{.path = SHADER_FOLDER "screen.glsl"},
};

Asset g_Textures[(int)TextureAssetID::COUNT] = {
	{.path = TEX_FOLDER "TextureAtlas.png"},
	{.path = TEX_FOLDER "UiAtlas.png"},
	{.path = TEX_FOLDER "zombie_temp.png"},
	{.path = TEX_FOLDER "uv.png"}, 
};

Asset g_Meshes[(int)MeshAssetID::COUNT] = {
	{.path = MESH_FOLDER "zombie.mesh"},
};

void InvalidateShaders() {
	for (size_t i = 0; i < ArrayCount(g_Shaders); i++)
		g_Shaders[i].IsInitialized = false;
}

//
// Shader Uniforms Caching
//

// TODO: тестова€ верси€, сделать лучше
// можно при компил€ции шейдера запрашивать местоположение всех переменных и сохран€ть внутри шейдереа
// например 
// struct Uniform {
//  const char* name;
//  s32 location;
//  bool isInitialized;
// } uniforms[16];
//
s32 Renderer::GetUniformLocation(Shader shader, const char* name) {
	//return glGetUniformLocation(shader, name);
	
	// TODO: не должно быть поиска ID шейдера
	for (size_t i = 0; i < ArrayCount(g_Shaders); i++)
	{
		if (g_Shaders[i].shader == shader) {
			ShaderAssetID id = (ShaderAssetID)i;
			auto& map = g_UniformLocationCache[(u32)id];
			s32 loc = -1;

			if (map.find(name) != map.end()) {
				loc = map[name];
			}
			else {
				loc = glGetUniformLocation(shader, name);
				map[name] = loc;
			}

#if _DEBUG
			if (loc == -1)
				dbgprint("[SHADER] shader %d: no uniform named \"%s\"\n", shader, name);
#endif
			return loc;
		}
	}
	
	return -1;
}


Shader GetShader(ShaderAssetID id)
{
	Asset* asset = &g_Shaders[(int)id];

#if _DEBUG
	if (!asset->IsInitialized || asset->fileWriteTime != GetFileWriteTime(asset->path))
#else
	if (!asset->IsInitialized)
#endif
	{
		

		Shader newShader = Renderer::CreateShaderFromFile(g_TempStorage, asset->path, asset->shader);

		if (asset->shader != 0 && asset->shader != newShader) {
			dbgprint("cache clear\n");
			g_UniformLocationCache[(u32)id].clear(); // очищаем uniform кэш
		}

		asset->shader = newShader;

		asset->fileWriteTime = GetFileWriteTime(asset->path);
		asset->IsInitialized = true;
	}

	return asset->shader;
}

Geometry* GetMesh(MeshAssetID id)
{
	Asset* asset = &g_Meshes[(int)id];

#if _DEBUG
	if (!asset->IsInitialized || asset->fileWriteTime != GetFileWriteTime(asset->path))
#else
	if (!asset->IsInitialized)
#endif
	{
		asset->mesh = Renderer::CreateGeometryFromFile(g_TempStorage, asset->path);

		asset->fileWriteTime = GetFileWriteTime(asset->path);
		asset->IsInitialized = true;
	}

	return &asset->mesh;
}

Texture* GetTexture(TextureAssetID id)
{
	Asset* asset = &g_Textures[(int)id];

#if _DEBUG
	if (!asset->IsInitialized || asset->fileWriteTime != GetFileWriteTime(asset->path))
#else
	if (!asset->IsInitialized)
#endif
	{
		asset->texture = Renderer::CreateTextureFromFile(asset->path, PixelFormat::RGBA);

		asset->fileWriteTime = GetFileWriteTime(asset->path);
		asset->IsInitialized = true;
	}

	return &asset->texture;
}