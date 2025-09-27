#pragma region external dependencies
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/noise.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
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

float near_plane = 1.0f, far_plane = 500.0f;
float shadowLightDist = 100;

Player player;

static bool cursorMode = false; // TRUE для взаимодействия с UI, FALSE для перемещения камеры

static bool g_vsyncOn = false;

static DynamicArray<Entity> entities;

enum uiElemType : u16 {
	uiInventorySelectCell,
	uiInventoryCell,
	uiHeart,
	uiCross,
	uiGroundBlock,
	uiStoneBlock,
	uiSnowBlock,
	uiCOUNT
};

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
	Geometry screenQuad, defaultBox, entityMesh;
	Sprite sunSprite, moonSprite;
	FrameBuffer depthMapFBO;
	// рендерим в screenFBO с MSAA, копируем результат в intermediateFBO и делаем постобработку
	FrameBuffer screenFBO, intermediateFBO; 
	//Texture depthMap;
	Font bigFont, regularFont;
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
	initShaders(); // компиляция шейдеров
	UI::Init();

	Settings settings;
	SettingsLoad(&settings);
	g_shadowQuality = settings.shadowQuality;
	g_MSAAFactor = (Renderer::MSAAFactor)settings.antiAliasingQuality;
	g_vsyncOn = settings.vsync;
	SetVsync(settings.vsync);

	g_gameWorld.init(0, settings.renderDistance);
	g_gameWorld.gameState = gsMainMenu;

	player.camera.pos = glm::vec3(8, 30, 8);
	player.camera.front = glm::vec3(0, 0, -1);
	player.camera.up = glm::vec3(0, 1, 0);
	player.camera.FOV = settings.FOV;
	player.maxSpeed = 15;
	player.inventory = InventoryCreate();

	// загрузка текстур
	Assets.testTexture = Renderer::createTextureFromFile(TEX_FOLDER "uv.png", PixelFormat::RGBA);
	Assets.textureAtlas = Renderer::createTextureFromFile(TEX_FOLDER "TextureAtlas.png", PixelFormat::RGBA);
	{
		texUSize = 16.0 / (float)Assets.textureAtlas.width;
		texVSize = 16.0 / (float)Assets.textureAtlas.height;
		blocksUV[(int)BlockType::btGround].offset = { 0, 0 };
		blocksUV[(int)BlockType::btGround].scale = { texUSize, texVSize };
		blocksUV[(int)BlockType::btStone].offset = { texUSize, 0 };
		blocksUV[(int)BlockType::btStone].scale = { texUSize, texVSize };
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

		for (uiElemType elemType : {uiInventorySelectCell, uiInventoryCell, uiHeart, uiCross, uiGroundBlock, uiStoneBlock, uiSnowBlock})
		{
			uiUV[elemType] = UVOffset(tileOrigin, tileSize);
			tileOrigin.x += tileSize.x;
		}
	}

	Assets.entityTexture = Renderer::createTextureFromFile(TEX_FOLDER "zombie_temp.png", PixelFormat::RGBA);

	// загрузка шрифтов
	Assets.regularFont = loadFont(FONT_FOLDER "DigitalPixel.otf", 30);
	Assets.bigFont = loadFont(FONT_FOLDER "DigitalPixel.otf", 60);

	{
		static Vertex vertices[] = {
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
			vertices[i].normal = { 0,1,0 };
		}
		static Triangle triangles[] = {
			Triangle(0, 2, 1), Triangle(0, 3, 2),
			Triangle(4, 5, 7), Triangle(5, 6, 7),
			Triangle(0, 4, 3), Triangle(4, 7, 3),
			Triangle(1, 2, 6), Triangle(1, 6, 5),
			Triangle(2, 3, 6), Triangle(3, 7, 6),
			Triangle(0, 1, 4), Triangle(1, 5, 4)
		};

		Assets.defaultBox = Renderer::createGeometry(vertices, 8, triangles, 12);
		Assets.entityMesh = Renderer::createGeometryFromFile(MESH_FOLDER "zombie.mesh");

		createSprite(Assets.sunSprite, 1, 1, blocksUV[(int)BlockType::texSun].offset, texUSize, texVSize);
		createSprite(Assets.moonSprite, 1, 1, blocksUV[(int)BlockType::texMoon].offset, texUSize, texVSize);
	}

	{
		static Vertex vertices[] = {
			Vertex(-1,-1, 0, 0,0),
			Vertex(1,-1, 0, 1,0),
			Vertex(1, 1, 0, 1,1),
			Vertex(-1, 1, 0, 0,1),
		};
		static Triangle triangles[] = {
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

void GameUpdateAndRender(float time, Input* newInput, FrameBufferInfo* frameBufferInfo) {
	g_time = time;
	g_deltaTime = g_time - g_prevTime;
	g_prevTime = g_time;

	fbInfo = frameBufferInfo;
	static FrameBufferInfo lastFBInfo = { 0 };
	if (lastFBInfo.sizeX != frameBufferInfo->sizeX || lastFBInfo.sizeY != frameBufferInfo->sizeY) {
		InitFramebuffers(frameBufferInfo->sizeX, frameBufferInfo->sizeY, g_MSAAFactor);
	}

	if (ButtonClicked(newInput->startGame)) {
		if (g_gameWorld.gameState == gsMainMenu)
			g_gameWorld.gameState = gsInGame;
	}
	if (ButtonClicked(newInput->switchExitMenu)) {
		if (g_gameWorld.gameState == gsInGame)
			g_gameWorld.gameState = gsExitMenu;
		else if (g_gameWorld.gameState == gsExitMenu) {
			// save settings to file
			{
				Settings settings;
				settings.FOV = player.camera.FOV;
				settings.shadowQuality = g_shadowQuality;
				settings.renderDistance = GetRenderDistance(g_chunkManager.chunksCount);
				settings.antiAliasingQuality = (int)g_MSAAFactor;
				settings.vsync = g_vsyncOn;
				SettingsSave(&settings);
			}

			g_gameWorld.gameState = gsInGame;
		}
	}
	if (ButtonClicked(newInput->rebuildShaders)) {
		dbgprint("Rebuilding shaders...\n");
		initShaders();
		dbgprint("Shaders rebuild done!\n");
	}
	


	switch (g_gameWorld.gameState)
	{
	case gsMainMenu:
		SetCursorMode(true);
		RenderMainMenu(newInput);
		break;
	case gsExitMenu:
		SetCursorMode(true);
		RenderPauseMenu(newInput);
		break;
	case gsInGame:
		SetCursorMode(false);
		RenderGame(newInput);
		break;
	}

}

void RenderMainMenu(Input* input) {
	Renderer::clear(0.6, 0.6, 0.6);

	UI::Start(input, &Assets.regularFont, fbInfo);
	UI::SetAnchor(uiAnchor::Center, 0);
	UI::SetAdvanceMode(AdvanceMode::Down);
	
	const char* buttonText = "Start game";
	UI::ShiftOrigin(-UI::GetButtonWidth(buttonText) / 2, 0);
	if (UI::Button("Start game", glm::vec2(0.0, 0.0))) {
		g_gameWorld.gameState = gsInGame;
	}

	const char* caption = "Main menu";
	UI::SetAnchor(uiAnchor::Top, 100);
	UI::UseFont(&Assets.bigFont);
	UI::ShiftOrigin(-UI::GetTextWidth(caption) / 2, 0);
	UI::Text(caption);
	UI::End();
}

void RenderPauseMenu(Input* input) {
	enum class PauseMenuState {
		MainMenu,
		SettingsMenu
	};
	static PauseMenuState menuState = PauseMenuState::MainMenu;

	Renderer::clear(0.6, 0.6, 0.6);

	UI::Start(input, &Assets.regularFont, fbInfo);
	UI::SetAdvanceMode(AdvanceMode::Down);
	float elemWidth = 400;

	switch (menuState)
	{
	case PauseMenuState::MainMenu: {
		UI::SetAnchor(uiAnchor::Center, 0);
		UI::ShiftOrigin(-elemWidth / 2, 0);
		if (UI::Button("Return", glm::vec2(elemWidth, 0))) {
			g_gameWorld.gameState = gsInGame;
		}
		if (UI::Button("Settings", glm::vec2(elemWidth, 0))) {
			menuState = PauseMenuState::SettingsMenu;
		}
		if (UI::Button("Exit", glm::vec2(elemWidth, 0))) {
			CloseWindow();
		}
	} break;
	case PauseMenuState::SettingsMenu: {
		UI::SetAnchor(uiAnchor::Center, 0);
		UI::ShiftOrigin(-elemWidth / 2, 200);

		UI::SliderFloat("FOV", &player.camera.FOV, 40, 120, elemWidth);

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

		UI::CheckBox("Wireframe mode", &wireframe_cb);
		
		UI::CheckBox("Debug view", &debugView_cb);

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

void RenderGame(Input* input) {
	{
		// update player rotation
		{
			static float lastX = 0, lastY = 0; // позиция курсора
			static float yaw = 0, pitch = 0;
			static const float sensitivity = 0.1f; // TODO: перенести в настройки

			double xpos, ypos;
			GetCursorPos(&xpos, &ypos);

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
			if (ButtonHeldDown(input->forward))
				player.speedVector += cameraSpeedAdj * player.camera.front;
			if (ButtonHeldDown(input->backwards))
				player.speedVector -= cameraSpeedAdj * player.camera.front;
			if (ButtonHeldDown(input->left))
				player.speedVector -= glm::normalize(glm::cross(player.camera.front, player.camera.up)) * cameraSpeedAdj;;
			if (ButtonHeldDown(input->right))
				player.speedVector += glm::normalize(glm::cross(player.camera.front, player.camera.up)) * cameraSpeedAdj;

			player.camera.pos += player.speedVector;
			player.speedVector *= 0.8; // TODO: deltatime
		}

		// player inventory
		{
			for (size_t i = 0; i < INVENTORY_MAX_SIZE; i++)
			{
				if (ButtonClicked(input->inventorySlots[i])) {
					InventorySelectItem(&player.inventory, i);
				}
			}

			if (ButtonClicked(input->scrollUp)) {
				InventorySelectItem(&player.inventory, player.inventory.selectedIndex + 1);
			}
			if (ButtonClicked(input->scrollDown)) {
				InventorySelectItem(&player.inventory, player.inventory.selectedIndex - 1);
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
		sunDir.x = cos((g_time + 10) * sunSpeed);
		sunDir.y = sin((g_time + 10) * sunSpeed);
		sunDir.z = 0;
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


	int currentChunkPosX = (int)(player.camera.pos.x / CHUNK_SX) * CHUNK_SX;
	int currentChunkPosZ = (int)(player.camera.pos.z / CHUNK_SZ) * CHUNK_SZ;
	if (player.camera.pos.x < 0)
		currentChunkPosX -= CHUNK_SX;
	if (player.camera.pos.z < 0)
		currentChunkPosZ -= CHUNK_SZ;

	ChunkManagerBuildChunks(&g_chunkManager, player.camera.pos.x, player.camera.pos.z);

	// destroying / building blocks
	if (ButtonClicked(input->placeBlock)) {
		InventoryCell *cell = &player.inventory.cells[player.inventory.selectedIndex];
		if (cell->itemsCount > 0) {
			ChunkManagerPlaceBlock(&g_chunkManager, cell->itemType, player.camera.pos, player.camera.front, 128);
			InventoryDropItem(&player.inventory, player.inventory.selectedIndex, 1);
		}
	}
	else if (ButtonClicked(input->attack)) {
		PlaceBlockResult res = ChunkManagerPlaceBlock(&g_chunkManager, BlockType::btAir, player.camera.pos, player.camera.front, 128);
		if (res.success) {
			Item drop = {};
			drop.type = res.typePrev;
			drop.pos = { res.pos.x + 0.5, res.pos.y + 0.5, res.pos.z + 0.5 };
			drop.count = 1;
			g_gameWorld.droppedItems.append(drop);
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
		do {
			belowBlock = ChunkManagerPeekBlockFromPos(&g_chunkManager, entity.pos.x, entity.pos.y, entity.pos.z);
			if (belowBlock && belowBlock->type != BlockType::btAir)
				entity.pos.y = (int)entity.pos.y + 1;
			else
				break;
		} while (belowBlock);

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
#pragma region трансформации
	
	// трансформация вида (камера)
	glm::mat4 view, projection;
	{
		view = glm::lookAt(player.camera.pos, player.camera.pos + player.camera.front, player.camera.up);

		// матрица проекции (перспективная/ортогональная проекция)
		projection = glm::mat4(1.0);
		projection = glm::perspective(glm::radians(player.camera.FOV), (float)fbInfo->sizeX / (float)fbInfo->sizeY, 0.1f, 1000.0f);
	}
#pragma endregion

	// draw shadow maps
	glm::mat4 lightSpaceMatrix;
	{
		//glCullFace(GL_FRONT);

		//float projDim = (float)Assets.depthMap.width / 16.0f;
		//float projDim = (float)Assets.depthMapFBO.textures[0].width / 16.0f / 4;
		float projDim = 128;
		glm::mat4 lightProjection = glm::ortho(-projDim, projDim, -projDim, projDim, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(
			shadowLightDist * directLightDir + player.camera.pos * glm::vec3(1, 0, 1),
			player.camera.pos * glm::vec3(1, 0, 1),
			glm::vec3(0.0f, 1.0f, 0.0f));

		lightSpaceMatrix = lightProjection * lightView;

		Renderer::setViewportDimensions(Assets.depthMapFBO.textures[0].width, Assets.depthMapFBO.textures[0].height);
		Renderer::bindFrameBuffer(&Assets.depthMapFBO);
		//glClear(GL_DEPTH_BUFFER_BIT);
		Renderer::clear(0, 0, 0);

		Renderer::bindShader(shadowShader);
		// установка переменных в вершинном шейдере
		Renderer::setUniformMatrix4(shadowShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));


		// chunks shadowmaps
		for (size_t c = 0; c < g_chunkManager.chunksCount; c++)
		{
			Chunk& chunk = g_chunkManager.chunks[c];
			if (chunk.generated && !chunk.mesh.needUpdate) {
				// render 1 chunk
				Renderer::setUniformInt2(shadowShader, "chunkPos", chunk.posx, chunk.posz);
				Renderer::setUniformInt2(shadowShader, "atlasSize", Assets.textureAtlas.width, Assets.textureAtlas.height);

				Renderer::drawInstancedGeo(chunk.mesh.VAO, 6, chunk.mesh.faceCount);
			}
		}

		// entities shadowmaps
		Renderer::bindShader(polyMeshShadowShader);
		Renderer::setUniformMatrix4(polyMeshShadowShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
		for (size_t i = 0; i < entities.count; i++)
		{
			glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
			model = glm::translate(model, entities[i].pos);
			model = glm::rotate(model, glm::radians(entities[i].rot.x), glm::vec3(1.0, 0.0, 0.0));
			model = glm::rotate(model, glm::radians(entities[i].rot.y), glm::vec3(0.0, 1.0, 0.0));
			model = glm::rotate(model, glm::radians(entities[i].rot.z), glm::vec3(0.0, 0.0, 1.0));
			model = glm::scale(model, glm::vec3(1, 2, 1));
			Renderer::setUniformMatrix4(polyMeshShadowShader, "model", glm::value_ptr(model));

			// TODO: биндить нужный меш в зависимости от типа entity
			Renderer::drawGeometry(&Assets.entityMesh);
		}

		Renderer::unbindFrameBuffer();
		//glCullFace(GL_BACK);
		//Renderer::setViewportDimensions(fbInfo->sizeX, fbInfo->sizeY);
	}

	Renderer::bindFrameBuffer(&Assets.screenFBO);
	Renderer::setViewportDimensions(fbInfo->sizeX, fbInfo->sizeY);
	Renderer::clear(ambientLightColor.r, ambientLightColor.g, ambientLightColor.b, 1.0);

	if (wireframe_cb) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// draw sun and moon
	glDepthMask(GL_FALSE); // render on background
	useSpriteShader(projection, view);
	spriteApplyTransform(player.camera.pos + sunDir, 0.3, true);
	drawSprite(Assets.sunSprite, Assets.textureAtlas.ID);
	spriteApplyTransform(player.camera.pos + moonDir, 0.3, true);
	drawSprite(Assets.moonSprite, Assets.textureAtlas.ID);
	glDepthMask(GL_TRUE);

	// set global uniforms
	{
		// set chunks uniforms
		{
			Renderer::bindShader(cubeInstancedShader);

			Renderer::setUniformFloat3(cubeInstancedShader, "sunDir", directLightDir.x, directLightDir.y, directLightDir.z);
			Renderer::setUniformFloat3(cubeInstancedShader, "sunColor", directLightColor.x, directLightColor.y, directLightColor.z);
			Renderer::setUniformFloat3(cubeInstancedShader, "ambientColor", ambientLightColor.x, ambientLightColor.y, ambientLightColor.z);

			Renderer::setUniformMatrix4(cubeInstancedShader, "viewProjection", (float*)glm::value_ptr(projection * view));
			Renderer::setUniformMatrix4(cubeInstancedShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

			Renderer::setUniformInt(cubeInstancedShader, "texture1", 0);
			Renderer::setUniformInt(cubeInstancedShader, "shadowMap", 1);
		}
		// set polymesh uniforms
		{
			Renderer::bindShader(polyMeshShader);

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

		Renderer::unbindShader();
	}

	// можно отрисовывать
	// draw chunks
	{
		Renderer::bindShader(cubeInstancedShader);
		Renderer::bindTexture(&Assets.textureAtlas, 0);
		Renderer::bindTexture(&Assets.depthMapFBO.textures[0], 1);

		for (size_t c = 0; c < g_chunkManager.chunksCount; c++)
		{
			Chunk* chunk = &g_chunkManager.chunks[c];
			if (chunk->generated && !chunk->mesh.needUpdate) {
				Renderer::setUniformInt2(cubeInstancedShader, "chunkPos", chunk->posx, chunk->posz);
				Renderer::setUniformInt2(cubeInstancedShader, "atlasSize", Assets.textureAtlas.width, Assets.textureAtlas.height);

				Renderer::drawInstancedGeo(chunk->mesh.VAO, 6, chunk->mesh.faceCount);
			}
		}

		Renderer::unbindTexture(0);
		Renderer::unbindTexture(1);
	}

	// draw entities
	{
		Renderer::bindShader(polyMeshShader);
		Renderer::bindTexture(&Assets.entityTexture, 0);
		Renderer::bindTexture(&Assets.depthMapFBO.textures[0], 1);

		for (size_t i = 0; i < entities.count; i++)
		{
			Entity* e = &entities.items[i];
			glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
			model = glm::translate(model, e->pos);
			rotateXYZ(model, e->rot.x, e->rot.y, e->rot.z);
			model = glm::scale(model, { 1,1,1 });
			Renderer::setUniformMatrix4(polyMeshShader, "model", glm::value_ptr(model));

			Renderer::drawGeometry(&Assets.entityMesh);
		}
		Renderer::unbindShader();
		Renderer::unbindTexture(0);
		Renderer::unbindTexture(1);
	}

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

#define GRAVITY 0
#if GRAVITY
	// gravity, ground collision
	player.camera.pos.y -= 20 * g_deltaTime;
	Block* belowBlock;
	do {
		belowBlock = g_gameWorld.peekBlockFromPos(glm::vec3(player.camera.pos.x - chunk.posx, player.camera.pos.y - 2, player.camera.pos.z - chunk.posz));
		if (belowBlock && belowBlock->type != (int)BlockType::(int)BlockType::btAir)
			player.camera.pos.y = (int)player.camera.pos.y + 1;
		else
			break;
	} while (belowBlock);
#endif


	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// axis
	useFlatShader(projection, view); // TODO: uniform переменные в шейдере можно установить один раз за кадр? Также можно испоьзовать uniform buffer
	flatApplyTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1.5, 0.2, 0.2));
	drawFlat(&Assets.defaultBox, glm::vec3(1, 0, 0));
	flatApplyTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.2, 1.5, 0.2));
	drawFlat(&Assets.defaultBox, glm::vec3(0, 1, 0));
	flatApplyTransform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.2, 0.2, 1.5));
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
		Renderer::drawGeometry(&Assets.screenQuad); // TEST
		Renderer::unbindTexture();

		Renderer::unbindShader();
		Renderer::switchDepthTest(true);
	}

	// draw ui
	UI::Start(input, &Assets.regularFont, fbInfo);
	UI::SetAnchor(uiAnchor::Center, 0);
	UI::DrawElement(&Assets.uiAtlas, glm::vec3(0, 0, 0), glm::vec3(64, 64, 1), uiUV[uiCross].scale, uiUV[uiCross].offset); // cursor

	// debug info
	{
		UI::SetAnchor(uiAnchor::Left, 10);
		UI::SetAdvanceMode(AdvanceMode::Down);
		char buf[128];
		sprintf(buf, "FPS: %d", (int)(1.0f / g_deltaTime));
		UI::Text(buf);
		sprintf(buf, "Frametime: %.3f", g_deltaTime);
		UI::Text(buf);
		sprintf(buf, "%d chunks", g_chunkManager.chunksCount);
		UI::Text(buf);
		sprintf(buf, "%d blocks", g_chunkManager.chunksCount * CHUNK_SX * CHUNK_SY * CHUNK_SZ);
		UI::Text(buf);
		sprintf(buf, "%d dropped items", g_gameWorld.droppedItems.count);
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
		uiElemType uiElemType = uiStoneBlock;
		InventoryCell* cell = &player.inventory.cells[i];

		switch (cell->itemType)
		{
		case BlockType::btGround:	uiElemType = uiGroundBlock; break;
		case BlockType::btStone:	uiElemType = uiStoneBlock;	break;
		case BlockType::btSnow:		uiElemType = uiSnowBlock;	break;
		default:
			break;
		}

		UI::DrawElement(&Assets.uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1), uiUV[uiElemType].scale, uiUV[uiElemType].offset);

		UI::ShiftOrigin(-Assets.regularFont.size / 2, 0);
		char buf[64];
		sprintf(buf, "%d", cell->itemsCount);
		UI::Text(buf);
		UI::ShiftOrigin(Assets.regularFont.size / 2, 0);
		
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
	UI::End();
#pragma endregion

}