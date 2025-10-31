#pragma region external dependencies
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/noise.hpp>
//#include <imgui.h>
//#include <imgui_impl_glfw.h>
//#include <imgui_impl_opengl3.h>
//#include <imgui_stdlib.h>
#pragma endregion
#pragma region internal dependencies
#include "Cubes.h"
#include "Typedefs.h"
#include "Tools.h"
#include "Mesh.h"
#include "ui.h"
#include "Files.h"
#include "Entity.h"
#include "DataStructures.h"
#include "World.h"
#include "Input.h"
#include "Renderer.h"
#include "ChunkManager.h"
#include "Settings.h"
#pragma endregion

#define rotateXYZ(matrix, x, y, z)\
matrix = glm::rotate(matrix, glm::radians(x), glm::vec3(1.0, 0.0, 0.0));\
matrix = glm::rotate(matrix, glm::radians(y), glm::vec3(0.0, 1.0, 0.0));\
matrix = glm::rotate(matrix, glm::radians(z), glm::vec3(0.0, 0.0, 1.0));

GameState gameState;

float cameraNearClip = 0.1f, cameraFarClip = 1000.0f;

float near_plane = 0.1f, far_plane = 1000.0f;
float projDim = 128;
float shadowLightDist = 100;

static bool g_vsyncOn = false;
static bool g_GenChunks = true;
static int g_maxInteractionDistance = 12;

static DynamicArray<Entity> entities;
bool g_inventoryOpened = false;

struct UVOffset {
	glm::vec2 offset;
	glm::vec2 scale;

	UVOffset() {};
	UVOffset(glm::vec2 offset, glm::vec2 scale) {
		this->offset = offset;
		this->scale = scale;
	}
};

// TDOO: перенести глобальные переменные

Renderer::MSAAFactor g_MSAAFactor = Renderer::MSAAFactor::X4;
int g_shadowQuality = 12;

float g_time = 0, g_deltaTime = 0, g_prevTime = 0;

FrameBufferInfo* fbInfo;

bool debugView_cb = false;
bool wireframe_cb = false;
bool g_ShowDebugInfo = true;

bool g_UpdateSun = true;
float sunSpeed = 0.05;
glm::vec3 sunDir(0, 0, 0);
glm::vec3 moonDir(0, 0, 0);
float isDay = true;
const glm::vec3 sunColor(1, 0.9, 0.8);
const glm::vec3 moonColor(0.6, 0.6, 0.9);
const glm::vec3 skyColor(0.4, 0.4, 0.8);

glm::vec3 directLightColor;
glm::vec3 ambientLightColor;
glm::vec3 directLightDir;

static UVOffset blocksUV[(int)BlockType::btCOUNT];
static UVOffset uiUV[uiCOUNT];
static float texUSize;
static float texVSize;

struct {
	Texture testTexture, textureAtlas, uiAtlas, entityTexture;
	Geometry screenQuad, defaultQuad, defaultBox, entityMesh;
	Sprite sunSprite, moonSprite;
	FrameBuffer depthMapFBO;
	// рендерим в screenFBO с MSAA, копируем результат в intermediateFBO и делаем постобработку
	FrameBuffer screenFBO, intermediateFBO; 
	//Texture depthMap;
	Font* bigFont, *regularFont;
} Assets;

void InitFramebuffers(int width, int height, Renderer::MSAAFactor MSAAFactor) {
	if ((width * height) == 0)
		return;

	Renderer::releaseFrameBuffer(&Assets.screenFBO);
	Renderer::releaseFrameBuffer(&Assets.intermediateFBO);

	Renderer::createMSAAFrameBuffer(&Assets.screenFBO, width, height, MSAAFactor);
	Renderer::createColorFrameBuffer(&Assets.intermediateFBO, width, height);
	Renderer::setViewportDimensions(width, height);
}

void InitShadowMapBuffer(u32 size) {
	Renderer::releaseFrameBuffer(&Assets.depthMapFBO);
	Renderer::createDepthMapFrameBuffer(&Assets.depthMapFBO, size);
}

void RenderMainMenu(Input* input);
void RenderPauseMenu(Input* input);
void RenderGame(Input* input);

void GameInit() {
	dbgprint("GL VERSION %s\n", glGetString(GL_VERSION));
	
	initShaders(); // компиляция шейдеров
	UI::Init();



	gameState = gsMainMenu;


	// загрузка текстур
	Assets.testTexture = Renderer::createTextureFromFile(TEX_FOLDER "uv.png", PixelFormat::RGBA);
	Assets.textureAtlas = Renderer::createTextureFromFile(TEX_FOLDER "TextureAtlas.png", PixelFormat::RGBA);
	{
		texUSize = 16.0 / (float)Assets.textureAtlas.width;
		texVSize = 16.0 / (float)Assets.textureAtlas.height;
		//blocksUV[(int)BlockType::btGround].offset = { 0, 0 };
		//blocksUV[(int)BlockType::btGround].scale = { texUSize, texVSize };
		//blocksUV[(int)BlockType::btStone].offset = { texUSize, 0 };
		//blocksUV[(int)BlockType::btStone].scale = { texUSize, texVSize };
		blocksUV[(int)BlockType::texSun].offset = { texUSize * 2, 0 };
		blocksUV[(int)BlockType::texSun].scale = { texUSize, texVSize };
		blocksUV[(int)BlockType::texMoon].offset = { texUSize * 3, 0 };
		blocksUV[(int)BlockType::texMoon].scale = { texUSize, texVSize };
	}
	// TODO: добавить координаты для ui элементов, отрендерить на экран
	Assets.uiAtlas = Renderer::createTextureFromFile(TEX_FOLDER "UiAtlas.png", PixelFormat::RGBA);
	{
		glm::vec2 tileSize(16.0f / (float)Assets.uiAtlas.width, 16.0f / (float)Assets.uiAtlas.height);
		glm::vec2 tileOrigin(0, 0);
		u32 tilesInRow = Assets.uiAtlas.width / 16;
		for (size_t i = 0; i < uiCOUNT; i++)
		{
			uiUV[i] = UVOffset(tileOrigin, tileSize);
			tileOrigin.x += tileSize.x;
			if (i % tilesInRow == tilesInRow - 1) {
				tileOrigin.x = 0;
				tileOrigin.y += tileSize.y;
			}
		}
	}

	Assets.entityTexture = Renderer::createTextureFromFile(TEX_FOLDER "zombie_temp.png", PixelFormat::RGBA);

	// загрузка шрифтов
	Assets.regularFont = loadFont(FONT_FOLDER "DigitalPixel.otf", 25);
	Assets.bigFont = loadFont(FONT_FOLDER "DigitalPixel.otf", 50);

	{
		Vertex boxVerts[] = {
			Vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
			Vertex(1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
			Vertex(1.0f, 1.0f, 0.0f, 1.0f, 1.0f),
			Vertex(0.0f, 1.0f, 0.0f, 0.0f, 1.0f),

			Vertex(0.0f, 0.0f, 1.0f, 0.0f, 1.0f),
			Vertex(1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(1.0f, 1.0f, 1.0f, 1.0f, 0.0f),
			Vertex(0.0f, 1.0f, 1.0f, 0.0f, 0.0f),
		};
		for (size_t i = 0; i < 8; i++)
		{
			boxVerts[i].normal = { 0,1,0 };
		}
		Triangle boxTris[] = {
			Triangle(0, 2, 1), Triangle(0, 3, 2),
			Triangle(4, 5, 7), Triangle(5, 6, 7),
			Triangle(0, 4, 3), Triangle(4, 7, 3),
			Triangle(1, 2, 6), Triangle(1, 6, 5),
			Triangle(2, 3, 6), Triangle(3, 7, 6),
			Triangle(0, 1, 4), Triangle(1, 5, 4)
		};
		Assets.defaultBox = Renderer::createGeometry(boxVerts, 8, boxTris, 12);

		Vertex quadVerts[] = {
			Vertex(0,0,0, 0,0),
			Vertex(1,0,0, 1,0),
			Vertex(1,1,0, 1,1),
			Vertex(0,1,0, 0,1),
		};
		Triangle quadTris[] = {
			Triangle(0,1,2),
			Triangle(0,2,3)
		};
		Assets.defaultQuad = Renderer::createGeometry(quadVerts, 4, quadTris, 2);

		Assets.entityMesh = Renderer::createGeometryFromFile(MESH_FOLDER "zombie.mesh");

		createSprite(Assets.sunSprite, 1, 1, blocksUV[(int)BlockType::texSun].offset, texUSize, texVSize);
		createSprite(Assets.moonSprite, 1, 1, blocksUV[(int)BlockType::texMoon].offset, texUSize, texVSize);
	}

	{
		Vertex vertices[] = {
			Vertex(-1,-1, 0, 0,0),
			Vertex(1,-1, 0, 1,0),
			Vertex(1, 1, 0, 1,1),
			Vertex(-1, 1, 0, 0,1),
		};
		Triangle triangles[] = {
			Triangle(0,1,2),
			Triangle(0,2,3)
		};

		Assets.screenQuad = Renderer::createGeometry(vertices, 4, triangles, 2);
	}

	{
		Entity entity;
		entity.type = entityZombie;
		entity.state = entityStateIdle;
		entity.pos = { 0, CHUNK_SY - 2, 0 };
		entities.append(entity);

		entity.type = entityZombie;
		entity.state = entityStateIdle;
		entity.pos = { 5, CHUNK_SY - 2, 5 };
		entities.append(entity);
	}

	{
		// shadow framebuffer
		InitShadowMapBuffer(pow(2, g_shadowQuality));
	}
}

void GameExit() {
	ChunksSaveToDisk(g_chunkManager.chunks, g_chunkManager.chunksCount, g_gameWorld.info.name);
	CloseWindow();
}

// time - время в секундах
void GameUpdateAndRender(float time, Input* input, FrameBufferInfo* frameBufferInfo) {
	g_time = time;
	g_deltaTime = g_time - g_prevTime;
	g_prevTime = g_time;

	fbInfo = frameBufferInfo;
	static FrameBufferInfo lastFBInfo = { 0 };
	if (lastFBInfo.sizeX != frameBufferInfo->sizeX || lastFBInfo.sizeY != frameBufferInfo->sizeY) {
		InitFramebuffers(frameBufferInfo->sizeX, frameBufferInfo->sizeY, g_MSAAFactor);
	}

	if (ButtonClicked(input->startGame)) {
		if (gameState == gsMainMenu)
			gameState = gsInGame;
	}
	if (ButtonClicked(input->switchExitMenu)) {
		if (gameState == gsInGame)
			gameState = gsExitMenu;
		else if (gameState == gsExitMenu) {
			// save settings to file
			{
				Settings settings;
				settings.FOV = g_gameWorld.player.camera.FOV;
				settings.shadowQuality = g_shadowQuality;
				settings.renderDistance = GetRenderDistance(g_chunkManager.chunksCount);
				settings.antiAliasingQuality = (int)g_MSAAFactor;
				settings.vsync = g_vsyncOn;
				SettingsSave(&settings);
			}

			gameState = gsInGame;
		}
	}
	if (ButtonClicked(input->switchFullscreenMode)) {
		switch (WindowGetCurrentMode())
		{
		case WindowMode::Windowed:
			WindowSwitchMode(WindowMode::WindowedFullScreen);
			break;
		case WindowMode::WindowedFullScreen:
			WindowSwitchMode(WindowMode::Windowed);
			break;
		}
	}
	if (ButtonClicked(input->rebuildShaders)) {
		dbgprint("Rebuilding shaders...\n");
		initShaders();
		dbgprint("Shaders rebuild done!\n");
	}
	if (ButtonClicked(input->showDebugInfo)) {
		g_ShowDebugInfo = !g_ShowDebugInfo;
	}

	switch (gameState)
	{
	case gsMainMenu:
		SetCursorMode(true);
		RenderMainMenu(input);
		break;
	case gsExitMenu:
		SetCursorMode(true);
		RenderPauseMenu(input);
		break;
	case gsInGame:
		RenderGame(input);
		break;
	}

	// в полноэкранном окне не отрисовывается системный курсор, поэтому отрисовываем его самостоятельно 
	if (gameState == gsMainMenu || 
		gameState == gsExitMenu)
	{
		UI::Start(input, Assets.regularFont, fbInfo);
		double xpos = 0, ypos = 0;
		GetCursorPos(&xpos, &ypos);
		UI::SetOrigin(xpos, fbInfo->sizeY - ypos);
		UI::DrawElement(&Assets.uiAtlas, glm::vec3(0, 0, 0), glm::vec3(64, 64, 1), uiUV[uiCross].scale, uiUV[uiCross].offset);
		UI::End();
	}
}

void RenderMainMenu(Input* input) {
	enum class MainMenuState {
		Main,
		SelectWorld
	};
	static MainMenuState menuState = MainMenuState::Main;

	Renderer::clear(0.6, 0.6, 0.6);

	UI::Start(input, Assets.regularFont, fbInfo);
	UI::SetAnchor(uiAnchor::Center, 0);
	UI::SetAdvanceMode(AdvanceMode::Down);
	
	switch (menuState)
	{
	case MainMenuState::Main:
	{
		if (UI::Button("Start game", {0,0}, true)) {
			menuState = MainMenuState::SelectWorld;
		}
	} break;
	case MainMenuState::SelectWorld:
	{
		static DynamicArray<GameWorldInfo> worldsList = {0};
		static bool updateWorldsList = true;
		if (updateWorldsList) {
			updateWorldsList = false;
			worldsList.clear();
			EnumerateWorlds(&worldsList);
		}
		if (UI::Button("Update", {0,0}, true)) {
			updateWorldsList = true;
		}

		auto StartGame = [](GameWorldInfo* worldInfo) {
			Settings settings;
			SettingsLoad(&settings);
			g_shadowQuality = settings.shadowQuality;
			g_MSAAFactor = (Renderer::MSAAFactor)settings.antiAliasingQuality;
			g_vsyncOn = settings.vsync;
			SetVsync(settings.vsync);

			Player& player = g_gameWorld.player;
			player.camera.pos = glm::vec3(8, 30, 8);
			player.camera.front = glm::vec3(0, 0, -1);
			player.camera.up = glm::vec3(0, 1, 0);
			player.camera.FOV = settings.FOV;
			player.maxSpeed = 15;
			player.inventory = InventoryCreate();
#if _DEBUG
			InventoryAddItem(&player.inventory, ItemType::GroundBlock, 64);
			InventoryAddItem(&player.inventory, ItemType::StoneBlock, 64);
			InventoryAddItem(&player.inventory, ItemType::Sword, 64);
			InventoryAddItem(&player.inventory, ItemType::Pickaxe, 64);
#endif

			g_gameWorld.init(worldInfo, settings.renderDistance);

			gameState = gsInGame;
		};
		
		// load existing world
		for (size_t i = 0; i < worldsList.count; i++)
		{
			if (UI::Button(worldsList[i].name, {0,0}, true)) {
				StartGame(&worldsList.items[i]);
			}
		}

		// create new world
		if (UI::Button("Start new world", {0,0}, true)) {
			GameWorldInfo worldInfo;
			worldInfo.seed = 0;
			
			// find name for new world
			{
				char worldPath[256];
				const char *newWorldNameBase = "New World";

				int i = 1;
				do {
					sprintf(worldInfo.name, "%s %d", newWorldNameBase, i);
					GetWorldPath(worldPath, worldInfo.name);
					i++;
				} while (IsFileExists(worldPath));
			}

			StartGame(&worldInfo);
		}
	} break;
	}

	UI::SetAnchor(uiAnchor::Top, 100);
	UI::UseFont(Assets.bigFont);
	UI::Text("Main menu", true);
	UI::End();
}

void RenderPauseMenu(Input* input) {
	enum class PauseMenuState {
		MainMenu,
		SettingsMenu
	};
	static PauseMenuState menuState = PauseMenuState::MainMenu;

	Renderer::clear(0.6, 0.6, 0.6);

	UI::Start(input, Assets.regularFont, fbInfo);
	UI::SetAdvanceMode(AdvanceMode::Down);
	float elemWidth = 400;

	switch (menuState)
	{
	case PauseMenuState::MainMenu: {
		UI::SetAnchor(uiAnchor::Center, 0);
		if (UI::Button("Return", glm::vec2(elemWidth, 0), true)) {
			gameState = gsInGame;
		}
		if (UI::Button("Settings", glm::vec2(elemWidth, 0), true)) {
			menuState = PauseMenuState::SettingsMenu;
		}
		if (UI::Button("Exit", glm::vec2(elemWidth, 0), true)) {
			GameExit();
		}
	} break;
	case PauseMenuState::SettingsMenu: {
		UI::SetAnchor(uiAnchor::Center, 0);
		UI::ShiftOrigin(-elemWidth / 2, 400);

		bool fullScreen = (WindowGetCurrentMode() == WindowMode::WindowedFullScreen);
		if (UI::CheckBox("Fullscreen mode", &fullScreen)) {
			if (fullScreen)
				WindowSwitchMode(WindowMode::WindowedFullScreen);
			else
				WindowSwitchMode(WindowMode::Windowed);
		}

		UI::SliderFloat("FOV", &g_gameWorld.player.camera.FOV, 40, 120, elemWidth);

		int renderDistanceSlider = GetRenderDistance(g_chunkManager.chunksCount);
		if (UI::SliderInt("Render distance", &renderDistanceSlider, MIN_RENDER_DISTANCE, MAX_RENDER_DISTANCE, elemWidth)) {
			if (GetRenderDistance(g_chunkManager.chunksCount) != renderDistanceSlider) {
				ChunkManagerReleaseChunks(&g_chunkManager);
				ChunkManagerAllocChunks(&g_chunkManager, renderDistanceSlider);
			}
		}

		if (UI::SliderInt("Shadow quality", &g_shadowQuality, 10, 14, elemWidth)) {
			InitShadowMapBuffer(pow(2, g_shadowQuality));
		}

		static int MSAAFactorSlider = sqrt((int)g_MSAAFactor);
		if (UI::SliderInt("Anti Aliasing quality", &MSAAFactorSlider, 0, 4, elemWidth)) {
			g_MSAAFactor = (Renderer::MSAAFactor)pow(2, MSAAFactorSlider);
			InitFramebuffers(fbInfo->sizeX, fbInfo->sizeY, g_MSAAFactor);
		}

		UI::CheckBox("Update sun", &g_UpdateSun);
		if (!g_UpdateSun) {
			static float sunSlider = 0;
			UI::SliderFloat("Sun direction", &sunSlider, -2, 2);
			sunDir.x = cos(sunSlider);
			sunDir.y = sin(sunSlider);
			sunDir.z = 0;
		}

		UI::CheckBox("Wireframe mode", &wireframe_cb);
		
		UI::CheckBox("Debug view", &debugView_cb);
		UI::CheckBox("Debug info", &g_ShowDebugInfo);
		UI::CheckBox("Generate chunks", &g_GenChunks);

		if (UI::CheckBox("Vsync", &g_vsyncOn)) {
			SetVsync(g_vsyncOn);
		}

		if (UI::Button("Back", glm::vec2(elemWidth, 0))) {
			menuState = PauseMenuState::MainMenu;
		}
	} break;
	default:
		break;
	}

	UI::End();
}

void DrawChunksShadow(Chunk* chunks, int chunksCount, FrameBuffer* depthMapFBO, Frustum* frustum, glm::mat4& lightSpaceMatrix) {
	//glCullFace(GL_FRONT);
	
	Renderer::bindShader(cubeInstancedShadowShader);
	Renderer::setUniformMatrix4(cubeInstancedShadowShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (size_t c = 0; c < chunksCount; c++)
	{
		Chunk* chunk = &chunks[c];
		if (chunk->status == ChunkStatus::ReadyToRender) {
			// frustum culling
			glm::vec3 chunkCenter(chunk->posx + CHUNK_SX / 2.0f, CHUNK_SY / 2.0f, chunk->posz + CHUNK_SZ / 2.0f);
			if (!FrustumSphereIntersection(frustum, chunkCenter, g_chunkRadius)) {
				continue;
			}
			
			Renderer::setUniformInt2(cubeInstancedShadowShader, "chunkPos", chunk->posx, chunk->posz);

			Renderer::drawInstancedGeo(chunk->mesh.VAO, 6, chunk->mesh.faceCount);
		}
	}
	
	//glCullFace(GL_BACK);
}

// draw to screen
int DrawChunks_old(
	Chunk* chunks, int chunksCount,
	FrameBuffer* depthMapFBO, FrameBuffer* screenFBO,
	Frustum *frustum,
	glm::mat4 &lightSpaceMatrix, glm::mat4 &view, glm::mat4 &projection, 
	bool overrideColor = false) 
{
	Renderer::bindShader(cubeInstancedShader);
	Renderer::bindTexture(&Assets.textureAtlas, 0);
	Renderer::bindTexture(&depthMapFBO->textures[0], 1);

	{
		if (!overrideColor) {
			Renderer::setUniformInt(cubeInstancedShader, "overrideColor", 0);
		}
		else {
			Renderer::setUniformInt(cubeInstancedShader, "overrideColor", 1);
			Renderer::setUniformFloat3(cubeInstancedShader, "color", 0, 0, 0);
		}
		
		Renderer::setUniformFloat3(cubeInstancedShader, "sunDir", directLightDir.x, directLightDir.y, directLightDir.z);
		Renderer::setUniformFloat3(cubeInstancedShader, "sunColor", directLightColor.x, directLightColor.y, directLightColor.z);
		Renderer::setUniformFloat3(cubeInstancedShader, "ambientColor", ambientLightColor.x, ambientLightColor.y, ambientLightColor.z);

		Renderer::setUniformMatrix4(cubeInstancedShader, "viewProjection", (float*)glm::value_ptr(projection * view));
		Renderer::setUniformMatrix4(cubeInstancedShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

		Renderer::setUniformInt(cubeInstancedShader, "texture1", 0);
		Renderer::setUniformInt(cubeInstancedShader, "shadowMap", 1);
	}

	int chunksRendered = 0;

	for (size_t c = 0; c < chunksCount; c++)
	{
		Chunk* chunk = &chunks[c];
		if (chunk->status == ChunkStatus::ReadyToRender) {
			// frustum culling
			glm::vec3 chunkCenter(chunk->posx + CHUNK_SX / 2.0f, CHUNK_SY / 2.0f, chunk->posz + CHUNK_SZ / 2.0f);
			if (!FrustumSphereIntersection(frustum, chunkCenter, g_chunkRadius)) {
				continue;
			}
			
			Renderer::setUniformInt2(cubeInstancedShader, "chunkPos", chunk->posx, chunk->posz);
			Renderer::setUniformInt2(cubeInstancedShader, "atlasSize", Assets.textureAtlas.width, Assets.textureAtlas.height);

			Renderer::drawInstancedGeo(chunk->mesh.VAO, 6, chunk->mesh.faceCount);

			chunksRendered++;
		}
	}

	Renderer::unbindTexture(0);
	Renderer::unbindTexture(1);
	Renderer::unbindShader();

	return chunksRendered;
}
// draw to screen
int DrawChunks(
	Chunk* chunks, int chunksCount,
	FrameBuffer* depthMapFBO, FrameBuffer* screenFBO,
	Frustum *frustum,
	glm::mat4 &lightSpaceMatrix, glm::mat4 &view, glm::mat4 &projection, 
	bool overrideColor = false) 
{
	Renderer::bindShader(cubeInstancedShader);
	Renderer::bindTexture(&Assets.textureAtlas, 0);
	Renderer::bindTexture(&depthMapFBO->textures[0], 1);

#define InitUniform(name) glGetUniformLocation(cubeInstancedShader, name)
	static u32 overrideColorUniform = InitUniform("overrideColor");
	static u32 colorUniform = InitUniform("color");
	static u32 sunDirUniform = InitUniform("sunDir");
	static u32 sunColorUniform = InitUniform("sunColor");
	static u32 ambientColorUniform = InitUniform("ambientColor");
	static u32 viewProjectionUnifrom = InitUniform("viewProjection");
	static u32 lightSpaceMatrixUniform = InitUniform("lightSpaceMatrix");
	static u32 texture1Uniform = InitUniform("texture1");
	static u32 shadowMapUniform = InitUniform("shadowMap");
	static u32 chunkPosUniform = InitUniform("chunkPos");
	static u32 atlasSizeUniform = InitUniform("atlasSize");
#undef InitUniform

	{

		if (!overrideColor) {
			glUniform1i(overrideColorUniform, 0);
		}
		else {
			glUniform1i(overrideColorUniform, 1);
			glUniform3f(colorUniform, 0, 0, 0);
		}

		glUniform3f(sunDirUniform, directLightDir.x, directLightDir.y, directLightDir.z);
		glUniform3f(sunColorUniform, directLightColor.x, directLightColor.y, directLightColor.z);
		glUniform3f(ambientColorUniform, ambientLightColor.x, ambientLightColor.y, ambientLightColor.z);

		
		glUniformMatrix4fv(viewProjectionUnifrom, 1, false, glm::value_ptr(projection * view));
		glUniformMatrix4fv(lightSpaceMatrixUniform, 1, false, glm::value_ptr(lightSpaceMatrix));

		glUniform1i(texture1Uniform, 0);
		glUniform1i(shadowMapUniform, 1);
	}

	int chunksRendered = 0;

	for (size_t c = 0; c < chunksCount; c++)
	{
		Chunk* chunk = &chunks[c];
		if (!chunk->generationInProgress && chunk->status == ChunkStatus::ReadyToRender) {
			// frustum culling
			glm::vec3 chunkCenter(chunk->posx + CHUNK_SX / 2.0f, CHUNK_SY / 2.0f, chunk->posz + CHUNK_SZ / 2.0f);
			if (!FrustumSphereIntersection(frustum, chunkCenter, g_chunkRadius)) {
				continue;
			}
			
			glUniform2i(chunkPosUniform, chunk->posx, chunk->posz);
			glUniform2i(atlasSizeUniform, Assets.textureAtlas.width, Assets.textureAtlas.height);

			Renderer::drawInstancedGeo(chunk->mesh.VAO, 6, chunk->mesh.faceCount);

			chunksRendered++;
		}
#if _DEBUG
		// draw debug info
		else {
			Renderer::bindShader(flatShader);
			flatApplyTransform({ chunk->posx, 0, chunk->posz }, { 0,0,0 }, { CHUNK_SX, CHUNK_SY, CHUNK_SZ });
			//if (chunk->generationInProgress) {
			if (chunk->status == ChunkStatus::ReadyToRender) {
				drawFlat(&Assets.defaultBox, { 1,0,0 });
			}
			else if (chunk->status == ChunkStatus::Generated)
				drawFlat(&Assets.defaultBox, { 0,1,0 });
			else if (chunk->status == ChunkStatus::Initialized)
				drawFlat(&Assets.defaultBox, { 0,0,0 });
			Renderer::bindShader(cubeInstancedShader);
		}
#endif
	}


	Renderer::unbindTexture(0);
	Renderer::unbindTexture(1);
	Renderer::unbindShader();

	return chunksRendered;
}

void DrawEntitiesShadow(Entity* entities, int entitiesCount, FrameBuffer* depthMapFBO, glm::mat4& lightSpaceMatrix) {
	//glCullFace(GL_FRONT);

	Renderer::bindShader(polyMeshShadowShader);
	Renderer::setUniformMatrix4(polyMeshShadowShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (size_t i = 0; i < entitiesCount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
		model = glm::translate(model, entities[i].pos);
		model = glm::rotate(model, glm::radians(entities[i].rot.x), glm::vec3(1.0, 0.0, 0.0));
		model = glm::rotate(model, glm::radians(entities[i].rot.y), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(entities[i].rot.z), glm::vec3(0.0, 0.0, 1.0));
		model = glm::scale(model, glm::vec3(1, 2, 1));
		Renderer::setUniformMatrix4(polyMeshShadowShader, "model", glm::value_ptr(model));

		// TODO: биндить нужный мэш в зависимости от типа entity
		Renderer::drawGeometry(&Assets.entityMesh);
	}

	//glCullFace(GL_BACK);
}

// draw to screen
void DrawEntities(Entity* entities, int entitiesCount, FrameBuffer* depthMapFBO, FrameBuffer* screenFBO, glm::mat4& lightSpaceMatrix, glm::mat4& view, glm::mat4& projection) {
	Renderer::bindShader(polyMeshShader);
	Renderer::bindTexture(&Assets.entityTexture, 0);
	Renderer::bindTexture(&Assets.depthMapFBO.textures[0], 1);

	{
		Renderer::setUniformFloat3(polyMeshShader, "sunDir", directLightDir.x, directLightDir.y, directLightDir.z);
		Renderer::setUniformFloat3(polyMeshShader, "sunColor", directLightColor.x, directLightColor.y, directLightColor.z);
		Renderer::setUniformFloat3(polyMeshShader, "ambientColor", ambientLightColor.x, ambientLightColor.y, ambientLightColor.z);

		Renderer::setUniformMatrix4(polyMeshShader, "projection", glm::value_ptr(projection));
		Renderer::setUniformMatrix4(polyMeshShader, "view", glm::value_ptr(view));
		Renderer::setUniformMatrix4(polyMeshShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

		Renderer::setUniformInt(polyMeshShader, "texture1", 0);
		Renderer::setUniformInt(polyMeshShader, "shadowMap", 1);

		Renderer::setUniformFloat(polyMeshShader, "lightingFactor", 1.0f);
	}

	for (size_t i = 0; i < entitiesCount; i++)
	{
		Entity* e = &entities[i];
		glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
		model = glm::translate(model, e->pos);
		rotateXYZ(model, e->rot.x, e->rot.y, e->rot.z);
		model = glm::scale(model, { 1,1,1 });
		Renderer::setUniformMatrix4(polyMeshShader, "model", glm::value_ptr(model));

		Renderer::drawGeometry(&Assets.entityMesh);
	}

	Renderer::unbindTexture(0);
	Renderer::unbindTexture(1);
	Renderer::unbindShader();
}

void RenderGame(Input* input) {
	Player& player = g_gameWorld.player;

	{
		if (g_inventoryOpened) {
			SetCursorMode(true);
		}
		else {
			SetCursorMode(false);
		}

		// update player rotation
		if (!g_inventoryOpened)
		{
			static float lastX = 0, lastY = 0; // позиция курсора
			static float yaw = 0, pitch = 0;
			static const float sensitivity = 0.1f; // TODO: перенести в настройки

			double xpos, ypos;
			GetCursorPos(&xpos, &ypos);
			//dbgprint("CURSOR %.1f %.1f\n", xpos, ypos);

			//if (cursorMode == true) {
			//	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
			//	lastX = xpos;
			//	lastY = ypos;
			//	return;
			//}

			float xoffset = xpos - lastX;
			float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
			lastX = xpos;
			lastY = ypos;

			xoffset *= sensitivity;
			yoffset *= sensitivity;

			yaw += xoffset;
			pitch += yoffset;

			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;

			// update player camera
			glm::vec3 direction;
			direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.y = sin(glm::radians(pitch));
			direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			player.camera.front = glm::normalize(direction);
		}

		// update player pos
		{
			float cameraSpeedAdj = player.maxSpeed * g_deltaTime; // делаем скорость камеры независимой от FPS
			float jumpSpeedAdj = 100 * g_deltaTime;
			static bool flyMode = true;
			if (flyMode) {
				if (ButtonHeldDown(input->forward))
					player.speedVector += cameraSpeedAdj * glm::normalize(player.camera.front * glm::vec3(1, 0, 1));
				if (ButtonHeldDown(input->backwards))
					player.speedVector -= cameraSpeedAdj * glm::normalize(player.camera.front * glm::vec3(1, 0, 1));
				if (ButtonHeldDown(input->left))
					player.speedVector -= glm::normalize(glm::cross(glm::normalize(player.camera.front * glm::vec3(1, 0, 1)), player.camera.up)) * cameraSpeedAdj;;
				if (ButtonHeldDown(input->right))
					player.speedVector += glm::normalize(glm::cross(glm::normalize(player.camera.front * glm::vec3(1, 0, 1)), player.camera.up)) * cameraSpeedAdj;

				if (ButtonHeldDown(input->up))
					player.speedVector += cameraSpeedAdj * player.camera.up;
				if (ButtonHeldDown(input->down))
					player.speedVector -= cameraSpeedAdj * player.camera.up;
			}
			else {
				if (ButtonHeldDown(input->forward))
					player.speedVector += cameraSpeedAdj * (player.camera.front * glm::vec3(1, 0, 1));
				if (ButtonHeldDown(input->backwards))
					player.speedVector -= cameraSpeedAdj * (player.camera.front * glm::vec3(1, 0, 1));
				if (ButtonHeldDown(input->left))
					player.speedVector -= glm::normalize(glm::cross((player.camera.front * glm::vec3(1,0,1)), player.camera.up)) * cameraSpeedAdj;;
				if (ButtonHeldDown(input->right))
					player.speedVector += glm::normalize(glm::cross((player.camera.front * glm::vec3(1, 0, 1)), player.camera.up)) * cameraSpeedAdj;
				
				if (ButtonClicked(input->up))
					player.speedVector += jumpSpeedAdj * player.camera.up;
			}

			glm::vec3 prevPos = player.camera.pos;
			player.camera.pos += player.speedVector;
			player.speedVector *= 0.8; // TODO: deltatime

#if 0
			// gravity, ground collision
			player.camera.pos.y -= 20 * g_deltaTime; // gravity
			Block *block1, *block2;
			//do {
			//	belowBlock = ChunkManagerPeekBlockFromPos(&g_chunkManager, player.camera.pos.x, player.camera.pos.y - 2, player.camera.pos.z);
			//	if (belowBlock && belowBlock->type != BlockType::btAir)
			//		player.camera.pos.y = (int)player.camera.pos.y + 1;
			//	else
			//		break;
			//} while (belowBlock);
			block1 = ChunkManagerPeekBlockFromPos(&g_chunkManager, player.camera.pos.x, player.camera.pos.y - 2, player.camera.pos.z);
			block2 = ChunkManagerPeekBlockFromPos(&g_chunkManager, player.camera.pos.x, player.camera.pos.y - 1, player.camera.pos.z);
			
			if ((block1 && block1->type != BlockType::btAir) ||
				(block2 && block2->type != BlockType::btAir)) 
			{
				player.camera.pos = { prevPos.x, floor(prevPos.y) + 0.01, prevPos.z };
			}
#endif
		}

		// player inventory
		{
			if (ButtonClicked(input->openInventory)) {
				g_inventoryOpened = !g_inventoryOpened;
			}

			for (size_t i = 0; i < INVENTORY_MAX_SIZE; i++)
			{
				if (ButtonClicked(input->inventorySlots[i])) {
					InventorySelectItem(&player.inventory, i);
				}
			}

			if (ButtonClicked(input->scrollUp)) {
				InventorySelectItem(&player.inventory, player.inventory.selectedIndex - 1);
			}
			if (ButtonClicked(input->scrollDown)) {
				InventorySelectItem(&player.inventory, player.inventory.selectedIndex + 1);
			}
		}

		// collect dropped items
		float collectRadius = 3;
		for (size_t i = 0; i < g_gameWorld.droppedItems.count; i++)
		{
			Item* item = &g_gameWorld.droppedItems.items[i];
			if (item->count < 1)
				continue;

			if (glm::distance(player.camera.pos, item->pos) < collectRadius) {
				InventoryAddItem(&player.inventory, item->type, item->count);
				item->count = 0; 
			}
		}

		// обновление направления солнца
#if 1
		if (g_UpdateSun) {
			sunDir.x = cos((g_time + 10) * sunSpeed);
			sunDir.y = sin((g_time + 10) * sunSpeed);
			sunDir.z = 0;
		}
#endif
		moonDir = -sunDir;
		isDay = (sunDir.y > 0);

		// цвет источников света
		if (isDay) {
			float sunIntencity = glm::max(sunDir.y, 0.0f);
			ambientLightColor = skyColor * glm::max(sunIntencity, 0.2f);
			directLightColor = sunColor * sunIntencity;

			directLightDir = sunDir;
		}
		else {
			float moonIntencity = glm::max(moonDir.y, 0.0f) * 0.3;
			directLightColor = moonColor * moonIntencity;
			ambientLightColor = skyColor * glm::max(moonIntencity, 0.2f);

			directLightDir = moonDir;
		}
	}

	Frustum frustum = FrustumCreate(
		player.camera.pos, player.camera.front, player.camera.up,
		(float)fbInfo->sizeX / fbInfo->sizeY,
		player.camera.FOV,
		cameraNearClip, cameraFarClip);

	int currentChunkPosX = (int)(player.camera.pos.x / CHUNK_SX) * CHUNK_SX;
	int currentChunkPosZ = (int)(player.camera.pos.z / CHUNK_SZ) * CHUNK_SZ;
	if (player.camera.pos.x < 0)
		currentChunkPosX -= CHUNK_SX;
	if (player.camera.pos.z < 0)
		currentChunkPosZ -= CHUNK_SZ;

	if (g_GenChunks) {
		if (ButtonClicked(input->regenerateChunks)) {
			g_chunkManager.queue->StopAndJoin();
			for (size_t i = 0; i < g_chunkManager.chunksCount; i++)
			{
				g_chunkManager.chunks[i].status = ChunkStatus::Uninitalized;
			}
			g_chunkManager.queue->Start(std::max(1, GetThreadsCount()));
		}

		ChunkManagerBuildChunks(&g_chunkManager, &frustum, player.camera.pos.x, player.camera.pos.z);
	}

	//
	// building / destruction
	//

	// place block
	if (ButtonClicked(input->placeBlock)) {
		InventoryCell *cell = &player.inventory.cells[player.inventory.selectedIndex];
		if (cell->itemsCount > 0) {
			BlockType blockType = GetItemInfo(cell->itemType)->blockType;
			auto res = ChunkManagerPlaceBlock(&g_chunkManager, blockType, player.camera.pos, player.camera.front, g_maxInteractionDistance);
			if (res.success) {
				InventoryDropItem(&player.inventory, player.inventory.selectedIndex, 1);
			}
		}
	}
	// destroy block
	else if (ButtonHeldDown(input->attack)) {
		{
			PeekBlockResult res = ChunkManagerPeekBlockFromRay(&g_chunkManager, player.camera.pos, player.camera.front, g_maxInteractionDistance);
			if (res.success) {
				static glm::ivec3 lastBlokPos(0);
				static float startTime = g_time;
				glm::ivec3 blockPos = res.blockPos;
				if (blockPos == lastBlokPos) {
					float blockHardness = GetBlockInfo(res.block->type)->hardness;
					float itemDamage = GetItemInfo(InventoryGetCurrentItem(&player.inventory).itemType)->blockDamage;
					float timeToDestroyBlock = blockHardness / itemDamage;

					if (g_time - startTime > timeToDestroyBlock) {
						PlaceBlockResult res = ChunkManagerDestroyBlock(&g_chunkManager, player.camera.pos, player.camera.front, g_maxInteractionDistance);
						if (res.success) {
							Item drop = { 
								.pos = glm::vec3(res.pos.x + 0.5, res.pos.y + 0.5, res.pos.z + 0.5), 
								.count = 1, 
								.type = GetBlockInfo(res.typePrev)->itemType 
							};
							g_gameWorld.droppedItems.append(drop);

						}
						startTime = g_time;
					}
				}
				else {
					startTime = g_time;
				}
				lastBlokPos = blockPos;
			}
		}
	}

	// update entities
	for (size_t i = 0; i < entities.count; i++)
	{
		Entity& entity = entities.items[i];

		if (entity.type == entityNull)
			continue;

		// gravity
		entity.pos.y -= 5 * g_deltaTime;
		entity.pos += entity.speed * g_deltaTime;
		entity.speed *= 0.1;
		Block* belowBlock;
#if 1
		do {
			belowBlock = ChunkManagerPeekBlockFromPos(&g_chunkManager, entity.pos.x, entity.pos.y, entity.pos.z);
			if (belowBlock && belowBlock->type != BlockType::btAir)
				entity.pos.y = (int)entity.pos.y + 1;
			else
				break;
		} while (belowBlock);
#endif

		if (glm::distance(entity.pos, player.camera.pos) < 16) {
			entity.state = entityStateChasing;
		}
		else {
			entity.state = entityStateIdle;
		}

		switch (entities.items[i].state) {
		case entityStateChasing:
			glm::vec3 toPlayer = (player.camera.pos - entity.pos) * glm::vec3(1, 0, 1);
			if (glm::length(toPlayer) > 0) {
				entities.items[i].pos += glm::normalize(toPlayer) * 3.0f * g_deltaTime;

				glm::vec3 dir = glm::normalize(toPlayer);
				float yaw = atan2f(dir.x, dir.z);
				float pitch = asinf(-dir.y);

				entities.items[i].rot = glm::degrees(glm::vec3(pitch, yaw, 0.0f));
			}
			break;
		case entityStateIdle:
			break;
		}
	}

#pragma region Отрисовка
	// clear framebuffers & bind Screen Framebuffer
	{
		Renderer::bindFrameBuffer(&Assets.depthMapFBO);
		Renderer::setViewportDimensions(Assets.depthMapFBO.textures[0].width, Assets.depthMapFBO.textures[0].height);
		Renderer::clear(0, 0, 0);
		
		Renderer::bindFrameBuffer(&Assets.screenFBO);
		Renderer::setViewportDimensions(fbInfo->sizeX, fbInfo->sizeY);
		Renderer::clear(ambientLightColor.r, ambientLightColor.g, ambientLightColor.b);
	}

	// трансформация вида (камера)
	glm::mat4 view, projection;
	{
		view = glm::lookAt(player.camera.pos, player.camera.pos + player.camera.front, player.camera.up);

		// матрица проекции (перспективная/ортогональная проекция)
		projection = glm::mat4(1.0);
		projection = glm::perspective(glm::radians(player.camera.FOV), (float)fbInfo->sizeX / (float)fbInfo->sizeY, 0.1f, 1000.0f);
	}

	// трансформация тени
	glm::mat4 lightSpaceMatrix;
	{
		glm::mat4 lightProjection = glm::ortho(-projDim, projDim, -projDim, projDim, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(
			shadowLightDist * directLightDir + player.camera.pos * glm::vec3(1, 0, 1),
			player.camera.pos * glm::vec3(1, 0, 1),
			glm::vec3(0.0f, 1.0f, 0.0f));

		lightSpaceMatrix = lightProjection * lightView;
	}

	// draw sun and moon
	{
		glDepthMask(GL_FALSE); // render on background
		useSpriteShader(projection, view);
		spriteApplyTransform(player.camera.pos + sunDir, 0.3, true);
		drawSprite(Assets.sunSprite, Assets.textureAtlas.ID);
		spriteApplyTransform(player.camera.pos + moonDir, 0.3, true);
		drawSprite(Assets.moonSprite, Assets.textureAtlas.ID);
		glDepthMask(GL_TRUE);
	}

	// TEST
	{
		useFlatShader(projection, view);

		float epsilon = 1e-6;

		auto RayIntersection = [](glm::vec3& rayOrigin, glm::vec3& rayDir, u32 plane, float planeOffset) -> glm::vec3 {
			float epsilon = 1e-6;
			if (plane > 2 ||
				abs(rayDir[plane]) < epsilon)
			{
				return { 0,0,0 };
			}

			float t = -(rayOrigin[plane] - planeOffset) / rayDir[plane];
			glm::vec3 res = rayOrigin + (t * rayDir);
			res[plane] = planeOffset;

			return res;
			};

		auto DrawDebugBox = [](glm::vec3& pos, glm::vec3 color) {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, pos);
			model = glm::scale(model, { 0.4, 0.4, 0.4 });
			model = glm::translate(model, { -.5,-.5,-.5 });
			glUniformMatrix4fv(glGetUniformLocation(flatShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
			drawFlat(&Assets.defaultBox, color, 1);
			};

		glm::vec3& rayDir = player.camera.front;
		glm::vec3& rayOrigin = player.camera.pos;

		for (size_t j = 0; j < 2; j++)
		{
			for (size_t i = 0; i < 3; i++)
			{
				glm::vec3 p = RayIntersection(rayOrigin, rayDir, i, j);
				if (p.length() > 0) {
					if (p.x >= 0 && p.x <= 1 &&
						p.y >= 0 && p.y <= 1 &&
						p.z >= 0 && p.z <= 1)
					{
						glm::vec3 color(0.0f);
						color[i] = 1;
						DrawDebugBox(p, color);
					}
				}
			}
		}

		// cube
		if (false)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, { 0,0,0 });
			model = glm::scale(model, { 1, 1, 1 });
			glUniformMatrix4fv(glGetUniformLocation(flatShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
			drawFlat(&Assets.defaultBox, { 0.5f, 0.5f, 0.5f });
		}

		//player.camera.pos.x = 0;
		//player.camera.pos.y = 0;

		Renderer::unbindShader();
	}
	// TEST

	// draw chunks & entities
	int chunksRendered = 0;
#if 1
	{
		// draw shadows
		Renderer::bindFrameBuffer(&Assets.depthMapFBO);
		Renderer::setViewportDimensions(Assets.depthMapFBO.textures[0].width, Assets.depthMapFBO.textures[0].height);
		DrawChunksShadow(g_chunkManager.chunks, g_chunkManager.chunksCount, &Assets.depthMapFBO, &frustum, lightSpaceMatrix);
		DrawEntitiesShadow(entities.items, entities.count, &Assets.depthMapFBO, lightSpaceMatrix);


		// draw to screen
		Renderer::bindFrameBuffer(&Assets.screenFBO);
		Renderer::setViewportDimensions(fbInfo->sizeX, fbInfo->sizeY);
		chunksRendered = DrawChunks(g_chunkManager.chunks, g_chunkManager.chunksCount, &Assets.depthMapFBO, &Assets.screenFBO, &frustum, lightSpaceMatrix, view, projection);
		DrawEntities(entities.items, entities.count, &Assets.depthMapFBO, &Assets.screenFBO, lightSpaceMatrix, view, projection);

		// draw wireframe
		if (wireframe_cb) {
			glDepthMask(GL_FALSE);
			glDepthFunc(GL_LEQUAL);
			glLineWidth(2);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			DrawChunks(g_chunkManager.chunks, g_chunkManager.chunksCount, &Assets.depthMapFBO, &Assets.screenFBO, &frustum, lightSpaceMatrix, view, projection, true);
			DrawEntities(entities.items, entities.count, &Assets.depthMapFBO, &Assets.screenFBO, lightSpaceMatrix, view, projection);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else 
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif

	// draw dropped items
	{
		Renderer::bindShader(polyMeshShader);
		Renderer::bindTexture(&Assets.testTexture, 0);
		Renderer::bindTexture(&Assets.depthMapFBO.textures[0], 1);

		for (size_t i = 0; i < g_gameWorld.droppedItems.count; i++)
		{
			Item* item = &g_gameWorld.droppedItems.items[i];
			if (item->count < 1)
				continue;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, {0.0f, sin(g_time) * 0.3, 0.0f});
			model = glm::translate(model, item->pos);
			rotateXYZ(model, 0.0f, g_time * 30, 0.0f);
			model = glm::scale(model, { 0.3, 0.3, 0.3 });
			model = glm::translate(model, {-0.5, -0.5, -0.5}); // center
			Renderer::setUniformMatrix4(polyMeshShader, "model", glm::value_ptr(model));

			Renderer::drawGeometry(&Assets.defaultBox);
		}

		Renderer::unbindShader();
		Renderer::unbindTexture(0);
		Renderer::unbindTexture(1);
	}

	// draw debug geometry
	useFlatShader(projection, view);
	// chunk borders
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (debugView_cb) {
		for (size_t c = 0; c < g_chunkManager.chunksCount; c++)
		{
			Chunk& chunk = g_chunkManager.chunks[c];
			flatApplyTransform(glm::vec3(chunk.posx, 0, chunk.posz), glm::vec3(0, 0, 0), glm::vec3(CHUNK_SX, CHUNK_SY, CHUNK_SZ));
			drawFlat(&Assets.defaultBox, glm::vec3(0, 0, 0));
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// axis
	useFlatShader(projection, view); // TODO: uniform переменные в шейдере можно установить один раз за кадр? Также можно испоьзовать uniform buffer
	flatApplyTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0.2, 0.2));
	drawFlat(&Assets.defaultBox, glm::vec3(1, 0, 0));
	flatApplyTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.2, 1, 0.2));
	drawFlat(&Assets.defaultBox, glm::vec3(0, 1, 0));
	flatApplyTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.2, 0.2, 1));
	drawFlat(&Assets.defaultBox, glm::vec3(0, 0, 1));

	// post processing & draw to default buffer
	{
		Renderer::switchDepthTest(false);
		Renderer::bindShader(screenShader);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, Assets.screenFBO.ID);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Assets.intermediateFBO.ID);
		glBlitFramebuffer(0, 0, fbInfo->sizeX, fbInfo->sizeY, 0, 0, fbInfo->sizeX, fbInfo->sizeY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		Renderer::bindTexture(&Assets.intermediateFBO.textures[0]);
		Renderer::drawGeometry(&Assets.screenQuad);
		Renderer::unbindTexture();

		Renderer::unbindShader();
		Renderer::switchDepthTest(true);
	}

	// draw ui
	UI::Start(input, Assets.regularFont, fbInfo);
	UI::SetAnchor(uiAnchor::Center, 0);
	UI::DrawElement(&Assets.uiAtlas, glm::vec3(0, 0, 0), glm::vec3(64, 64, 1), uiUV[uiCross].scale, uiUV[uiCross].offset); // cursor

	// debug info
	if (g_ShowDebugInfo)
	{
		UI::SetAnchor(uiAnchor::LeftTop, 25);
		UI::SetAdvanceMode(AdvanceMode::Down);
		char buf[128];
		sprintf(buf, "FPS: %d", (int)(1.0f / g_deltaTime));
		UI::Text(buf);
		sprintf(buf, "Frametime : %.3fms", g_deltaTime * 1000);
		UI::Text(buf);
		sprintf(buf, "%d chunks", g_chunkManager.chunksCount);
		UI::Text(buf);
		sprintf(buf, "%d chunks rendered", chunksRendered);
		UI::Text(buf);
		sprintf(buf, "%d blocks", g_chunkManager.chunksCount * CHUNK_SX * CHUNK_SY * CHUNK_SZ);
		UI::Text(buf);
		int polyCount = 0;
		for (size_t i = 0; i < g_chunkManager.chunksCount; i++) {
			Chunk* chunk = &g_chunkManager.chunks[i];
			if (chunk->status == ChunkStatus::ReadyToRender)
				polyCount += chunk->mesh.faceCount;
		}
		sprintf(buf, "Chunks polycount: %d", polyCount);
		UI::Text(buf);
		sprintf(buf, "%d dropped items", g_gameWorld.droppedItems.count);
		UI::Text(buf);
		sprintf(buf, "pos x%.2f y%.2f z%.2f",  player.camera.pos.x,  player.camera.pos.y,  player.camera.pos.z);
		UI::Text(buf);
		{
			glm::ivec2 chunkPos = PosToChunkPos(player.camera.pos);
			sprintf(buf, "current chunk x%d z%d", chunkPos.x, chunkPos.y);
			UI::Text(buf);
		}
		sprintf(buf, "orient x%.2f y%.2f z%.2f",  player.camera.front.x,  player.camera.front.y,  player.camera.front.z);
		UI::Text(buf);
	}

	int cellsCount = INVENTORY_MAX_SIZE;
	float cellWidth = 70;
	float inventoryWidth = (cellsCount - 1) * cellWidth;
	UI::SetAnchor(uiAnchor::Bottom, 50);
	UI::SetAdvanceMode(AdvanceMode::None);
	UI::ShiftOrigin(-inventoryWidth / 2, 0);
	UI::SetMargin(false);
	for (size_t i = 0; i < player.inventory.cellsCount; i++)
	{
		InventoryCell* cell = &player.inventory.cells[i];

		uiElemType uiElemType = uiStoneBlock;
		uiElemType = GetItemInfo(cell->itemType)->uiElement;

		UI::DrawElement(&Assets.uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1), uiUV[uiElemType].scale, uiUV[uiElemType].offset);

		UI::ShiftOrigin(-FontGetSize(Assets.regularFont) / 2, 0);
		char buf[64];
		sprintf(buf, "%d", cell->itemsCount);
		UI::Text(buf);
		UI::ShiftOrigin(FontGetSize(Assets.regularFont) / 2, 0);
		
		UI::ShiftOrigin(cellWidth, 0);
	}

	UI::SetAdvanceMode(AdvanceMode::Right);
	UI::SetAnchor(uiAnchor::Bottom, 50);
	UI::ShiftOrigin(-inventoryWidth / 2, 0);
	for (size_t i = 0; i < cellsCount; i++)
	{
		UI::DrawElement(&Assets.uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1),
			uiUV[uiInventoryCell].scale, uiUV[uiInventoryCell].offset); // inventory
	}

	UI::SetAnchor(uiAnchor::Bottom, 50);
	UI::ShiftOrigin(-inventoryWidth / 2, 0);
	UI::ShiftOrigin(cellWidth* player.inventory.selectedIndex, 0);
	UI::DrawElement(&Assets.uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1), 
		uiUV[uiInventorySelectCell].scale, uiUV[uiInventorySelectCell].offset);

	int heartsCount = 8;
	float heartWidth = 35;
	float heartsWidth = (heartsCount - 1) * heartWidth;
	UI::SetAnchor(uiAnchor::Bottom, 110);
	UI::ShiftOrigin(-heartsWidth / 2, 0);
	for (size_t i = 0; i < heartsCount; i++)
	{
		UI::DrawElement(&Assets.uiAtlas, glm::vec3(0, 0, 0), glm::vec3(heartWidth, heartWidth, 1), uiUV[uiHeart].scale, uiUV[uiHeart].offset); // health bar
	}

	if (g_inventoryOpened) {
		UI::SetMargin(true);
		UI::SetAnchor(uiAnchor::Center, 0);
		UI::ShiftOrigin(100, 0);
		UI::SetAdvanceMode(AdvanceMode::Up);
		UI::Text("Craft items");
		for (size_t i = 0; i < ArraySize(itemInfoTable); i++)
		{

			ItemInfo* itemInfo = &itemInfoTable[i];

			if (!itemInfo->craftable)
				continue;

			Inventory inventoryCopy = player.inventory;
			bool canCraft = true;

			for (size_t j = 0; j < ArraySize(itemInfo->craftScheme); j++)
			{
				if (itemInfo->craftScheme[j] == ItemType::None)
					continue;

				int cellIndex = InventoryFindItem(&inventoryCopy, itemInfo->craftScheme[j]);
				if (cellIndex != -1) {
					InventoryDropItem(&inventoryCopy, cellIndex, 1);
				}
				else {
					canCraft = false;
					break;
				}
			}

			if (canCraft) {
				char craftButtonText[128];
				sprintf(craftButtonText, "Craft %s", itemInfo->name);
				if (UI::Button(craftButtonText)) {
					player.inventory = inventoryCopy;
					InventoryAddItem(&player.inventory, itemInfo->itemType, 1);
				}
			}
		}
	}

	// debug textures
#if 0
	UI::SetAnchor(uiAnchor::Center, 0);
	UI::ShiftOrigin(-400, 0);
	UI::DrawElement(&Assets.uiAtlas, { 0,0,0 }, { 400,400,400 }, { 1,1 }, { 0,0 });
	UI::DrawElement(&Assets.textureAtlas, { 0,0,0 }, { 400,400,400 }, { 1,1 }, { 0,0 });
	UI::DrawElement(FontGetTextureAtlas(Assets.regularFont), { 0,0,0 }, { 400,400,400 }, { 1,1 }, { 0,0 });
#endif

	// shadows debug
#if 0
	UI::SetAnchor(uiAnchor::Top, 200);
	UI::DrawElement(&Assets.depthMapFBO.textures[0], { 0,0,0, }, { 400, 400, 1 }, { 1,1 }, { 0,0 });
#endif

	UI::End();
#pragma endregion

}