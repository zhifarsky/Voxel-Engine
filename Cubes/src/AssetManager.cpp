#include "Files.h"
#include "Renderer.h"

Arena* g_TempStorage;

void Renderer::Begin(Arena* tempStorage) {
	g_TempStorage = tempStorage;
}

Asset g_Assets[(int)AssetID::COUNT] = { 
	{.path = SHADER_FOLDER "block.glsl"},
	{.path = SHADER_FOLDER "blockShadow.glsl"},
	{.path = SHADER_FOLDER "polyMesh.glsl"},
	{.path = SHADER_FOLDER "polyMeshShadow.glsl"},
	{.path = SHADER_FOLDER "flat.glsl"},
	{.path = SHADER_FOLDER "sprite.glsl"},
	{.path = SHADER_FOLDER "uiElement.glsl"},
	{.path = SHADER_FOLDER "screen.glsl"},

	{.path = TEX_FOLDER "TextureAtlas.png"},
	{.path = TEX_FOLDER "UiAtlas.png"},
	{.path = TEX_FOLDER "zombie_temp.png"},
	{.path = TEX_FOLDER "uv.png"},
};

// TODO: отдавать вместо ссылки на Asset, сразу Texture или Geometry

Asset* GetAsset(AssetID id) {
	Asset* asset = &g_Assets[(int)id];
	
	if (!asset->IsInitialized) {
		dbgprint("[ASSETS] Loading %s\n", asset->path);
		
		asset->id = id;

		switch (id)
		{
		case AssetID::CubeShader:
		case AssetID::CubeShadowShader:
		case AssetID::PolymeshShader:
		case AssetID::PolymeshShadowShader:
		case AssetID::FlatShader:
		case AssetID::SpriteShader:
		case AssetID::UIShader:
		case AssetID::ScreenShader:
			asset->type = AssetType::Shader;
			asset->shader = Renderer::createShaderFromFile(g_TempStorage, asset->path, asset->shader);
			break;
		case AssetID::EnvTexture:
		case AssetID::UITexture:
		case AssetID::EntityTexture:
		case AssetID::TestTexture:
			asset->type = AssetType::Texture;
			asset->texture = Renderer::createTextureFromFile(asset->path, PixelFormat::RGBA);
			break;
		case AssetID::EntityMesh:
			asset->type = AssetType::Mesh;
			break;
		case AssetID::DefaultBox:
			asset->type = AssetType::Mesh;
			break;
		default:
			break;
		}

		asset->IsInitialized = true;
	}

	return asset;
}

void InvalidateShaders() {
	for (size_t i = 0; i < ArrayCount(g_Assets); i++)
	{
		if (g_Assets[i].type == AssetType::Shader) {
			g_Assets[i].IsInitialized = false;
		}
	}
}