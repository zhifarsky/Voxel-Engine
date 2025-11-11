#pragma region external dependencies
#include <glm.hpp>
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
#include "RenderQueue.h"
#pragma endregion

// TDOO: перенести глобальные переменные

FrameBufferInfo* fbInfo;

bool g_DebugView = false;
bool g_RenderWireframe = false;
bool g_ShowDebugInfo = true;

bool g_UpdateSun = true;
bool g_GenChunks = true;

int g_maxInteractionDistance = 12;

struct SceneLighting {
	glm::vec3 directLightDirection;
	glm::vec3 directLightColor;
	glm::vec3 ambientLightColor;
};

struct {
	Geometry screenQuad, defaultQuad, defaultBox;
	Sprite sunSprite, moonSprite;
	FrameBuffer depthMapFBO;
	FrameBuffer screenFBO, intermediateFBO; // рендерим в screenFBO с MSAA, копируем результат в intermediateFBO и делаем постобработку
	Font* bigFont, *regularFont;
} Assets;

void InitFramebuffers(int width, int height, int samplesCount) {
	if ((width * height) == 0)
		return;

	Renderer::releaseFrameBuffer(&Assets.screenFBO);
	Renderer::releaseFrameBuffer(&Assets.intermediateFBO);

	Renderer::createMSAAFrameBuffer(&Assets.screenFBO, width, height, samplesCount);
	Renderer::createColorFrameBuffer(&Assets.intermediateFBO, width, height);
	Renderer::setViewportDimensions(width, height);
}

void InitShadowMapBuffer(u32 size) {
	Renderer::releaseFrameBuffer(&Assets.depthMapFBO);
	Renderer::createDepthMapFrameBuffer(&Assets.depthMapFBO, size);
}

void RenderMainMenu(GameMemory* memory, GameState* gameState, Input* input);
void RenderPauseMenu(GameMemory* memory, GameState* gameState, Input* input);
void RenderGame(GameState* gameState, GameMemory* memory, Input* input);

void GameInit(GameState* gameState, GameMemory* memory) {
	dbgprint("GL VERSION %s\n", glGetString(GL_VERSION));
	
	UI::Init();

	// загрузка шрифтов
	Assets.regularFont = loadFont(memory, FONT_FOLDER "DigitalPixel.otf", 25);
	Assets.bigFont = loadFont(memory, FONT_FOLDER "DigitalPixel.otf", 50);

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

		UV sunUV = GetUVFromAtlas(GetTexture(AssetID::EnvTexture), tidSun, {16,16});
		UV moonUV = GetUVFromAtlas(GetTexture(AssetID::EnvTexture), tidMoon, { 16,16 });
		Assets.sunSprite = createSprite(1, 1, sunUV);
		Assets.moonSprite = createSprite(1, 1, moonUV);
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
		gameState->entities.append(entity);

		entity.type = entityZombie;
		entity.state = entityStateIdle;
		entity.pos = { 5, CHUNK_SY - 2, 5 };
		gameState->entities.append(entity);
		
		entity.pos = { -15, CHUNK_SY - 2, -15 };
		gameState->entities.append(entity);
	}

	{
		// shadow framebuffer
		InitShadowMapBuffer(pow(2, gameState->shadowQuality));
	}
}

void GameExit() {
	ChunksSaveToDisk(g_chunkManager.chunks, g_chunkManager.chunksCount, g_gameWorld.info.name);
	CloseWindow();
}

// time - время в секундах
void GameUpdateAndRender(GameMemory* memory, float time, Input* input, FrameBufferInfo* frameBufferInfo) {
	//
	// memory initialisaiton
	//

	if (!memory->isInitialized) {
		GameState* gameState = (GameState*)memory->permStorage.pushZero(sizeof(GameState));

		gameState->time = time;

		Settings settings;
		SettingsLoad(&settings);
		gameState->shadowQuality = settings.shadowQuality;
		gameState->AASamplesCount = settings.antiAliasingQuality;
		SetVsync(settings.vsync);

		gameState->entities.alloc(&memory->permStorage, 1024);
		gameState->droppedItems.alloc(&memory->permStorage, 1024);

		gameState->nearPlane = 0.1;
		gameState->farPlane = 1000;
		
		gameState->shadowRenderDistance = 128;
		gameState->shadowLightDistance = 128;

		GameInit(gameState, memory);
		gameState->status = gsMainMenu;

		memory->isInitialized = true;
	}

	GameState* gameState = (GameState*)memory->permStorage.mem;

	gameState->deltaTime = time - gameState->time; // текущее время - предыдущее время
	gameState->time = time;

	fbInfo = frameBufferInfo;
	static FrameBufferInfo lastFBInfo = { 0 };
	if (lastFBInfo.sizeX != frameBufferInfo->sizeX || lastFBInfo.sizeY != frameBufferInfo->sizeY) {
		InitFramebuffers(frameBufferInfo->sizeX, frameBufferInfo->sizeY, gameState->AASamplesCount);
	}

	if (ButtonClicked(input->startGame)) {
		if (gameState->status == gsMainMenu)
			gameState->status = gsInGame;
	}
	if (ButtonClicked(input->switchExitMenu)) {
		if (gameState->status == gsInGame)
			gameState->status = gsExitMenu;
		else if (gameState->status == gsExitMenu) {
			// save settings to file
			{
				Settings settings;
				settings.FOV = g_gameWorld.player.camera.FOV;
				settings.shadowQuality = gameState->shadowQuality;
				settings.renderDistance = GetRenderDistance(g_chunkManager.chunksCount);
				settings.antiAliasingQuality = gameState->AASamplesCount;
				settings.vsync = GetVsync();
				SettingsSave(&settings);
			}

			gameState->status = gsInGame;
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
		InvalidateShaders();
		dbgprint("Shaders rebuild done!\n");
	}
	if (ButtonClicked(input->showDebugInfo)) {
		g_ShowDebugInfo = !g_ShowDebugInfo;
	}

	Renderer::Begin(&memory->tempStorage);

	switch (gameState->status)
	{
	case gsMainMenu:
		SetCursorMode(true);
		RenderMainMenu(memory, gameState, input);
		break;
	case gsExitMenu:
		SetCursorMode(true);
		RenderPauseMenu(memory, gameState, input);
		break;
	case gsInGame:
		RenderGame(gameState, memory, input);
		break;
	}

	// в полноэкранном окне не отрисовывается системный курсор, поэтому отрисовываем его самостоятельно 
	if (gameState->status == gsMainMenu ||
		gameState->status == gsExitMenu)
	{
		UI::Begin(&memory->tempStorage, input, Assets.regularFont, fbInfo);
		double xpos = 0, ypos = 0;
		GetCursorPos(&xpos, &ypos);
		UI::SetOrigin(xpos, fbInfo->sizeY - ypos);
		UI::DrawElement(GetTexture(AssetID::UITexture), glm::vec3(0, 0, 0), glm::vec3(64, 64, 1), uiCross, {16, 16});
		UI::End();
	}
}

void RenderMainMenu(GameMemory* memory, GameState* gameState, Input* input) {
	enum class MainMenuStatus {
		Main,
		SelectWorld
	};
	static MainMenuStatus menuStatus = MainMenuStatus::Main;

	Renderer::clear(0.6, 0.6, 0.6);

	UI::Begin(&memory->tempStorage, input, Assets.regularFont, fbInfo);
	UI::SetAnchor(uiAnchor::Center, 0);
	UI::SetAdvanceMode(AdvanceMode::Down);
	
	switch (menuStatus)
	{
	case MainMenuStatus::Main:
	{
		if (UI::Button("Start game", {0,0}, true)) {
			menuStatus = MainMenuStatus::SelectWorld;
		}
	} break;
	case MainMenuStatus::SelectWorld:
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

		auto StartGame = [](GameMemory* memory, GameState* gameState, GameWorldInfo* worldInfo) {
			Settings settings;
			SettingsLoad(&settings);

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

			g_gameWorld.init(memory, worldInfo, settings.renderDistance);

			gameState->status = gsInGame;
		};
		
		// load existing world
		for (size_t i = 0; i < worldsList.count; i++)
		{
			if (UI::Button(worldsList[i].name, {0,0}, true)) {
				StartGame(memory, gameState, &worldsList.items[i]);
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

			StartGame(memory, gameState, &worldInfo);
		}
	} break;
	}

	UI::SetAnchor(uiAnchor::Top, 100);
	UI::UseFont(Assets.bigFont);
	UI::Text("Main menu", true);
	UI::End();
}

void RenderPauseMenu(GameMemory* memory, GameState* gameState, Input* input) {
	enum class PauseMenuState {
		MainMenu,
		SettingsMenu
	};
	static PauseMenuState menuState = PauseMenuState::MainMenu;

	Renderer::clear(0.6, 0.6, 0.6);

	UI::Begin(&memory->tempStorage, input, Assets.regularFont, fbInfo);
	UI::SetAdvanceMode(AdvanceMode::Down);
	float elemWidth = 400;

	switch (menuState)
	{
	case PauseMenuState::MainMenu: {
		UI::SetAnchor(uiAnchor::Center, 0);
		if (UI::Button("Return", glm::vec2(elemWidth, 0), true)) {
			gameState->status = gsInGame;
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
				ChunkManagerReleaseChunks(memory, &g_chunkManager);
				ChunkManagerAllocChunks(memory, &g_chunkManager, renderDistanceSlider);
			}
		}

		if (UI::SliderInt("Shadow quality", &gameState->shadowQuality, 10, 14, elemWidth)) {
			InitShadowMapBuffer(pow(2, gameState->shadowQuality));
		}

		if (UI::SliderInt("Anti Aliasing quality", &gameState->AASamplesCount, 1, Renderer::GetMaxAASamples(), elemWidth)) {
			InitFramebuffers(fbInfo->sizeX, fbInfo->sizeY, gameState->AASamplesCount);
		}

		UI::CheckBox("Update sun", &g_UpdateSun);
		if (!g_UpdateSun) {
			static float sunSlider = 0;
			UI::SliderFloat("Sun direction", &sunSlider, -2, 2);
			gameState->sunDirection.x = cos(sunSlider);
			gameState->sunDirection.y = sin(sunSlider);
			gameState->sunDirection.z = 0;
		}

		UI::CheckBox("Wireframe mode", &g_RenderWireframe);
		
		UI::CheckBox("Debug view", &g_DebugView);
		UI::CheckBox("Debug info", &g_ShowDebugInfo);
		UI::CheckBox("Generate chunks", &g_GenChunks);

		bool vsyncOn = GetVsync();
		if (UI::CheckBox("Vsync", &vsyncOn)) {
			SetVsync(vsyncOn);
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

void DrawSprite(Sprite& sprite, Shader shader, Texture* texture,
	glm::mat4& projection, glm::mat4& view,
	glm::vec3& pos, float scale, bool spherical)
{
	Renderer::bindShader(shader);
	Renderer::bindTexture(texture);

	Renderer::setUniformMatrix4(shader, "view", glm::value_ptr(view));
	Renderer::setUniformMatrix4(shader, "projection", glm::value_ptr(projection));

	Renderer::setUniformFloat(shader, "scale", scale);
	Renderer::setUniformInt(shader, "spherical", spherical == true ? 1 : 0);

	glm::mat4 model(1); // единичная матрица (1 по диагонали)
	model = glm::translate(model, pos);
	model = glm::scale(model, glm::vec3(scale));
	Renderer::setUniformMatrix4(shader, "model", glm::value_ptr(model));

	Renderer::drawGeometry(&sprite);

	Renderer::unbindTexture();
}

void DrawFlat(Geometry& mesh, Shader shader,
	glm::vec3 color, float alpha,
	glm::mat4& model,
	glm::mat4& view, glm::mat4& projection) {
	Renderer::bindShader(shader);

	Renderer::setUniformMatrix4(shader, "model", glm::value_ptr(model));
	Renderer::setUniformMatrix4(shader, "view", glm::value_ptr(view));
	Renderer::setUniformMatrix4(shader, "projection", glm::value_ptr(projection));

	Renderer::setUniformFloat4(shader, "color", { color, alpha });

	Renderer::drawGeometry(&mesh);
}

void DrawChunksShadow(Chunk* chunks, int chunksCount, FrameBuffer* depthMapFBO, Frustum* frustum, glm::mat4& lightSpaceMatrix) {
	Shader shader = GetShader(AssetID::CubeShadowShader);
	
	Renderer::bindShader(shader);
	Renderer::setUniformMatrix4(shader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (size_t c = 0; c < chunksCount; c++)
	{
		Chunk* chunk = &chunks[c];
		if (!chunk->generationInProgress && chunk->status == ChunkStatus::ReadyToRender) {
			// frustum culling
			glm::vec3 chunkCenter(chunk->posx + CHUNK_SX / 2.0f, CHUNK_SY / 2.0f, chunk->posz + CHUNK_SZ / 2.0f);
			if (!FrustumSphereIntersection(frustum, chunkCenter, g_chunkRadius)) {
				continue;
			}
			
			glm::mat4 model(1);
			model = glm::translate(model, { chunk->posx, 0, chunk->posz });
			Renderer::setUniformMatrix4(shader, "model", glm::value_ptr(model));

			Renderer::drawInstancedGeo(chunk->mesh.VAO, 6, chunk->mesh.faceCount);
		}
	}
}

// TODO:
// можно убрать атрибут "chunkPos" и использовать viewProjectionModel матрицу
// таким образом отрисовка чанка будет не будет отличаться от отрисовки инстанцированного меша
// можно будет отправлять в RenderQueue наряду с другими объектами

// draw to screen
int DrawChunks(Chunk* chunks,
	int chunksCount, FrameBuffer* depthMapFBO,
	FrameBuffer* screenFBO, SceneLighting* lighting,
	Frustum* frustum,
	glm::mat4& lightSpaceMatrix,
	glm::mat4& view, glm::mat4& projection, bool overrideColor = false)
{
	Shader cubeShader = GetShader(AssetID::CubeShader);
	Shader flatShader = GetShader(AssetID::FlatShader);
	Texture* texture = GetTexture(AssetID::EnvTexture);

	Renderer::bindShader(cubeShader);
	Renderer::bindTexture(texture, 0);
	Renderer::bindTexture(&depthMapFBO->textures[0], 1);

	{
		if (!overrideColor) {
			Renderer::setUniformInt(cubeShader, "overrideColor", 0);
		}
		else {
			Renderer::setUniformInt(cubeShader, "overrideColor", 1);
			Renderer::setUniformFloat3(cubeShader, "color", 0, 0, 0);
		}

		Renderer::setUniformFloat3(cubeShader, "sunDir", lighting->directLightDirection.x, lighting->directLightDirection.y, lighting->directLightDirection.z);
		Renderer::setUniformFloat3(cubeShader, "sunColor", lighting->directLightColor.x, lighting->directLightColor.y, lighting->directLightColor.z);
		Renderer::setUniformFloat3(cubeShader, "ambientColor", lighting->ambientLightColor.x, lighting->ambientLightColor.y, lighting->ambientLightColor.z);
		
		glm::mat4 viewProjection = projection * view;
		Renderer::setUniformMatrix4(cubeShader, "viewProjection", glm::value_ptr(viewProjection));
		Renderer::setUniformMatrix4(cubeShader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

		Renderer::setUniformInt(cubeShader, "texture1", 0);
		Renderer::setUniformInt(cubeShader, "shadowMap", 1);
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

			Renderer::bindShader(cubeShader);

			glm::mat4 model = glm::translate(glm::mat4(1), {chunk->posx, 0, chunk->posz});
			Renderer::setUniformMatrix4(cubeShader, "model", glm::value_ptr(model));

			Renderer::drawInstancedGeo(chunk->mesh.VAO, 6, chunk->mesh.faceCount);

			chunksRendered++;
		}
#if _DEBUG
		// draw debug info
		else {
			
			Renderer::bindShader(flatShader);
			
			glm::mat4 model = GetTransform({ chunk->posx, 0, chunk->posz }, glm::vec3(0), { CHUNK_SX, CHUNK_SY, CHUNK_SZ });
			
			if (chunk->status == ChunkStatus::ReadyToRender) {
				DrawFlat(Assets.defaultBox, flatShader, { 1, 0, 0 }, 1, model, view, projection);
			}
			else if (chunk->status == ChunkStatus::Generated) {
				DrawFlat(Assets.defaultBox, flatShader, { 0, 1, 0 }, 1, model, view, projection);
			}
			else if (chunk->status == ChunkStatus::Initialized) {
				DrawFlat(Assets.defaultBox, flatShader, { 0, 0, 0 }, 1, model, view, projection);
			}
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

	Shader shader = GetShader(AssetID::PolymeshShadowShader);
	Geometry* mesh = GetMesh(AssetID::EntityMesh);

	Renderer::bindShader(shader);
	Renderer::setUniformMatrix4(shader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

	for (size_t i = 0; i < entitiesCount; i++)
	{
		glm::mat4 model(1); // единичная матрица (1 по диагонали)
		model = glm::translate(model, entities[i].pos);
		model = glm::rotate(model, glm::radians(entities[i].rot.x), glm::vec3(1.0, 0.0, 0.0));
		model = glm::rotate(model, glm::radians(entities[i].rot.y), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(entities[i].rot.z), glm::vec3(0.0, 0.0, 1.0));
		model = glm::scale(model, glm::vec3(1, 1, 1));
		Renderer::setUniformMatrix4(shader, "model", glm::value_ptr(model));

		// TODO: биндить нужный мэш в зависимости от типа entity
		Renderer::drawGeometry(mesh);
	}

	//glCullFace(GL_BACK);
}

void RenderGame(GameState* gameState, GameMemory* memory, Input* input) {
	Player& player = g_gameWorld.player;

	if (gameState->inventoryOpened)
		SetCursorMode(true);
	else
		SetCursorMode(false);

	// update player rotation
	if (!gameState->inventoryOpened)
	{
		static float lastX = 0, lastY = 0; // позиция курсора
		static float yaw = 0, pitch = 0;
		static const float sensitivity = 0.1f; // TODO: перенести в настройки

		double xpos, ypos;
		GetCursorPos(&xpos, &ypos);

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
		float cameraSpeedAdj = player.maxSpeed * gameState->deltaTime; // делаем скорость камеры независимой от FPS
		float jumpSpeedAdj = 100 * gameState->deltaTime;
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
		player.camera.pos.y -= 20 * gameState->deltaTime; // gravity
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
			gameState->inventoryOpened = !gameState->inventoryOpened;
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
	for (size_t i = 0; i < gameState->droppedItems.count; i++)
	{
		Item* item = &gameState->droppedItems[i];
		if (item->count < 1)
			continue;

		if (glm::distance(player.camera.pos, item->pos) < collectRadius) {
			InventoryAddItem(&player.inventory, item->type, item->count);
			item->count = 0; 
		}
	}

	// update sun and moon

	SceneLighting lighting;
	glm::vec3 moonDir;
	{
		float sunSpeed = 0.05;
		glm::vec3 sunColor(1, 0.9, 0.8);
		glm::vec3 moonColor(0.6, 0.6, 0.9);
		glm::vec3 skyColor(0.4, 0.4, 0.8);
		
		if (g_UpdateSun) {
			gameState->sunDirection.x = cos((gameState->time + 10) * sunSpeed);
			gameState->sunDirection.y = sin((gameState->time + 10) * sunSpeed);
			gameState->sunDirection.z = 0;
		}
	
		moonDir = -gameState->sunDirection;

		// если день
		if (gameState->sunDirection.y > 0) {
			float sunIntencity = glm::max(gameState->sunDirection.y, 0.0f);
			lighting.ambientLightColor = skyColor * glm::max(sunIntencity, 0.2f);
			lighting.directLightColor = sunColor * sunIntencity;

			lighting.directLightDirection = gameState->sunDirection;
		}
		// если ночь
		else {
			float moonIntencity = glm::max(moonDir.y, 0.0f) * 0.3;
			lighting.directLightColor = moonColor * moonIntencity;
			lighting.ambientLightColor = skyColor * glm::max(moonIntencity, 0.2f);

			lighting.directLightDirection = moonDir;
		}
	}

	Frustum frustum = FrustumCreate(
		player.camera.pos, player.camera.front, player.camera.up,
		(float)fbInfo->sizeX / fbInfo->sizeY,
		player.camera.FOV,
		gameState->nearPlane, gameState->farPlane);

	int currentChunkPosX = (int)(player.camera.pos.x / CHUNK_SX) * CHUNK_SX;
	int currentChunkPosZ = (int)(player.camera.pos.z / CHUNK_SZ) * CHUNK_SZ;
	if (player.camera.pos.x < 0)
		currentChunkPosX -= CHUNK_SX;
	if (player.camera.pos.z < 0)
		currentChunkPosZ -= CHUNK_SZ;

	if (g_GenChunks) {
		if (ButtonClicked(input->regenerateChunks)) {
			//g_chunkManager.queue->StopAndJoin();
			//for (size_t i = 0; i < g_chunkManager.chunksCount; i++)
			//{
			//	g_chunkManager.chunks[i].status = ChunkStatus::Uninitalized;
			//}
			//g_chunkManager.queue->Start(std::max(1, GetThreadsCount()));
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
			auto res = ChunkManagerPlaceBlock(&memory->tempStorage, &g_chunkManager, blockType, player.camera.pos, player.camera.front, g_maxInteractionDistance);
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
				static float startTime = gameState->time;
				glm::ivec3 blockPos = res.blockPos;
				if (blockPos == lastBlokPos) {
					float blockHardness = GetBlockInfo(res.block->type)->hardness;
					float itemDamage = GetItemInfo(InventoryGetCurrentItem(&player.inventory).itemType)->blockDamage;
					float timeToDestroyBlock = blockHardness / itemDamage;

					if (gameState->time - startTime > timeToDestroyBlock) {
						PlaceBlockResult res = ChunkManagerDestroyBlock(&memory->tempStorage, &g_chunkManager, player.camera.pos, player.camera.front, g_maxInteractionDistance);
						if (res.success) {
							Item drop = { 
								.pos = glm::vec3(res.pos.x + 0.5, res.pos.y + 0.5, res.pos.z + 0.5), 
								.count = 1, 
								.type = GetBlockInfo(res.typePrev)->itemType 
							};
							gameState->droppedItems.append(drop);

						}
						startTime = gameState->time;
					}
				}
				else {
					startTime = gameState->time;
				}
				lastBlokPos = blockPos;
			}
		}
	}

	// update entities
	for (size_t i = 0; i < gameState->entities.count; i++)
	{
		Entity& entity = gameState->entities[i];

		if (entity.type == entityNull)
			continue;

		// gravity
		entity.pos.y -= 5 * gameState->deltaTime;
		entity.pos += entity.speed * gameState->deltaTime;
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

		switch (gameState->entities[i].state) {
		case entityStateChasing:
			glm::vec3 toPlayer = (player.camera.pos - entity.pos) * glm::vec3(1, 0, 1);
			if (glm::length(toPlayer) > 0) {
				gameState->entities[i].pos += glm::normalize(toPlayer) * 3.0f * gameState->deltaTime;

				glm::vec3 dir = glm::normalize(toPlayer);
				float yaw = atan2f(dir.x, dir.z);
				float pitch = asinf(-dir.y);

				gameState->entities[i].rot = glm::degrees(glm::vec3(pitch, yaw, 0.0f));
			}
			break;
		case entityStateIdle:
			break;
		}
	}

#pragma region Rendering
	
	// clear framebuffers & bind Screen Framebuffer
	{
		Renderer::bindFrameBuffer(&Assets.depthMapFBO);
		Renderer::setViewportDimensions(Assets.depthMapFBO.textures[0].width, Assets.depthMapFBO.textures[0].height);
		Renderer::clear(0, 0, 0);
		
		Renderer::bindFrameBuffer(&Assets.screenFBO);
		Renderer::setViewportDimensions(fbInfo->sizeX, fbInfo->sizeY);
		Renderer::clear(lighting.ambientLightColor.r, lighting.ambientLightColor.g, lighting.ambientLightColor.b);
	}

	// трансформация вида (камера)
	glm::mat4 view, projection;
	{
		view = glm::lookAt(player.camera.pos, player.camera.pos + player.camera.front, player.camera.up);

		// матрица проекции (перспективная/ортогональная проекция)
		projection = glm::mat4(1);
		projection = glm::perspective(glm::radians(player.camera.FOV), (float)fbInfo->sizeX / (float)fbInfo->sizeY, gameState->nearPlane, gameState->farPlane);
	}

	// трансформация тени
	glm::mat4 lightSpaceMatrix;
	{
		glm::mat4 lightProjection = glm::ortho(
			-gameState->shadowRenderDistance,
			gameState->shadowRenderDistance, 
			-gameState->shadowRenderDistance, 
			gameState->shadowRenderDistance, 
			gameState->nearPlane, gameState->farPlane);
		
		glm::mat4 lightView = glm::lookAt(
			gameState->shadowLightDistance * lighting.directLightDirection + player.camera.pos * glm::vec3(1, 0, 1),
			player.camera.pos * glm::vec3(1, 0, 1),
			glm::vec3(0.0f, 1.0f, 0.0f));

		lightSpaceMatrix = lightProjection * lightView;
	}

	//
	// init render queue
	//

	RenderQueue renderQueue;
	RenderQueueInit(&renderQueue, memory->tempStorage.push(Megabytes(1)), Megabytes(1));
	{
		RenderEntryCamera cameraEntry = { .view = view, .projection = projection };
		RenderQueuePushCamera(&renderQueue, &cameraEntry);
	}
	{
		RenderEntryLighting lightingEntry = {
			.lightSpaceMatrix = lightSpaceMatrix,
			.directLightDirection = lighting.directLightDirection,
			.directLightColor = lighting.directLightColor,
			.ambientLightColor = lighting.ambientLightColor
		};
		RenderQueuePushLighting(&renderQueue, &lightingEntry);
	}

	// draw sun and moon
	{
		Shader spriteShader = GetShader(AssetID::SpriteShader);
		Texture* texture = GetTexture(AssetID::EnvTexture);

		glm::vec3 sunPos = player.camera.pos + gameState->sunDirection;
		glm::vec3 moonPos = player.camera.pos + moonDir;

		glDepthMask(GL_FALSE); // render on background
		DrawSprite(Assets.sunSprite, spriteShader, texture,
			projection, view,
			sunPos, 0.3, true);
		DrawSprite(Assets.moonSprite, spriteShader, texture,
			projection, view,
			moonPos, 0.3, true);
		glDepthMask(GL_TRUE);
	}
	 
	// TEST
	{
		Shader flatShader = GetShader(AssetID::FlatShader);
		
		Renderer::bindShader(flatShader);
		Renderer::setUniformMatrix4(flatShader, "projection", glm::value_ptr(projection));
		Renderer::setUniformMatrix4(flatShader, "view", glm::value_ptr(view));

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
						
						// draw
						{
							glm::mat4 model(1);
							model = glm::translate(model, p);
							model = glm::scale(model, { 0.4, 0.4, 0.4 });
							model = glm::translate(model, { -.5,-.5,-.5 });
							
							DrawFlat(Assets.defaultBox, flatShader, color, 1, model, view, projection);
						}
					}
				}
			}
		}

		// cube
		if (false)
		{
			glm::mat4 model(1);
			model = glm::translate(model, { 0,0,0 });
			model = glm::scale(model, { 1, 1, 1 });
			DrawFlat(Assets.defaultBox, flatShader, { 0.5f, 0.5f, 0.5f }, 1, model, view, projection);
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
		DrawEntitiesShadow(gameState->entities.items, gameState->entities.count, &Assets.depthMapFBO, lightSpaceMatrix);


		// draw to screen
		Renderer::bindFrameBuffer(&Assets.screenFBO);
		Renderer::setViewportDimensions(fbInfo->sizeX, fbInfo->sizeY);
		chunksRendered = DrawChunks(g_chunkManager.chunks, g_chunkManager.chunksCount, &Assets.depthMapFBO, &Assets.screenFBO, &lighting, &frustum, lightSpaceMatrix, view, projection, false);
		
		// draw entities
		{
			Shader shader = GetShader(AssetID::PolymeshShader);
			Geometry* mesh = GetMesh(AssetID::EntityMesh);
			Texture* texture = GetTexture(AssetID::EntityTexture);

			for (size_t i = 0; i < gameState->entities.count; i++)
			{
				Entity* e = &gameState->entities[i];

				RenderEntryTexturedMesh entry = {};
				entry.shader = shader;
				entry.geometry = mesh;
				entry.texture = texture;
				entry.shadowMap = &Assets.depthMapFBO.textures[0];
				entry.transform = GetTransform(e->pos, e->rot);

				RenderQueuePushTexturedMesh(&renderQueue, &entry);
			}
		}

		// draw wireframe
		if (g_RenderWireframe) {
			glDepthMask(GL_FALSE);
			glDepthFunc(GL_LEQUAL);
			glLineWidth(2);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			DrawChunks(g_chunkManager.chunks, g_chunkManager.chunksCount, &Assets.depthMapFBO, &Assets.screenFBO, &lighting, &frustum, lightSpaceMatrix, view, projection, true);
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

		Shader shader = GetShader(AssetID::PolymeshShader);
		Texture* texture = GetTexture(AssetID::TestTexture);

		for (size_t i = 0; i < gameState->droppedItems.count; i++)
		{
			Item* item = &gameState->droppedItems[i];
			if (item->count < 1)
				continue;

			RenderEntryTexturedMesh entry = {};
			entry.shader = shader;
			entry.geometry = &Assets.defaultBox;
			entry.texture = texture;
			entry.shadowMap = &Assets.depthMapFBO.textures[0];
			
			glm::mat4 model(1);
			model = glm::translate(model, { 0.0f, sin(gameState->time) * 0.3, 0.0f });
			model = glm::translate(model, item->pos);
			rotateXYZ(model, 0.0f, gameState->time * 30, 0.0f);
			model = glm::scale(model, glm::vec3(0.3));
			model = glm::translate(model, glm::vec3(-0.5)); // center

			entry.transform = model;

			RenderQueuePushTexturedMesh(&renderQueue, &entry);
		}
	}

	//
	// Render Queue
	//

	RenderQueueExecute(&renderQueue);

	// draw debug geometry
	{
		Shader flatShader = GetShader(AssetID::FlatShader);
		Renderer::bindShader(flatShader);
		Renderer::setUniformMatrix4(flatShader, "projection", glm::value_ptr(projection));
		Renderer::setUniformMatrix4(flatShader, "view", glm::value_ptr(view));

		// chunk borders
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		if (g_DebugView) {
			for (size_t c = 0; c < g_chunkManager.chunksCount; c++)
			{
				Chunk& chunk = g_chunkManager.chunks[c];
				glm::mat4 model(1);
				model = glm::translate(model, { chunk.posx, 0, chunk.posz });
				model = glm::scale(model, { CHUNK_SX, CHUNK_SY, CHUNK_SZ });
				DrawFlat(Assets.defaultBox, flatShader, { 0,0,0 }, 1, model, view, projection);
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// axis
		glm::mat4 modelX = glm::scale(glm::mat4(1), {1.0, 0.2, 0.2});
		glm::mat4 modelY = glm::scale(glm::mat4(1), {0.2, 1.0, 0.2});
		glm::mat4 modelZ = glm::scale(glm::mat4(1), {0.2, 0.2, 1.0});
		DrawFlat(Assets.defaultBox, flatShader, glm::vec3(1, 0, 0), 1, modelX, view, projection);
		DrawFlat(Assets.defaultBox, flatShader, glm::vec3(0, 1, 0), 1, modelY, view, projection);
		DrawFlat(Assets.defaultBox, flatShader, glm::vec3(0, 0, 1), 1, modelZ, view, projection);
	}

	// post processing & draw to default buffer
	{
		Renderer::switchDepthTest(false);
		Renderer::bindShader(GetShader(AssetID::ScreenShader));

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
	UI::Begin(&memory->tempStorage, input, Assets.regularFont, fbInfo);
	Texture* uiAtlas = GetTexture(AssetID::UITexture);
	UI::SetAnchor(uiAnchor::Center, 0);
	{
		UI::DrawElement(uiAtlas, glm::vec3(0, 0, 0), glm::vec3(64, 64, 1), uiCross, {16,16}); // cursor
	}

	// debug info
	if (g_ShowDebugInfo)
	{
		UI::SetAnchor(uiAnchor::LeftTop, 25);
		UI::SetAdvanceMode(AdvanceMode::Down);
		char buf[128];
		sprintf(buf, "FPS: %d", (int)(1.0f / gameState->deltaTime));
		UI::Text(buf);
		sprintf(buf, "Frametime : %.3fms", gameState->deltaTime * 1000);
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
		sprintf(buf, "%d dropped items", gameState->droppedItems.count);
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
		sprintf(buf, "light dir x%.2f y%.2f z%.2f", lighting.directLightDirection.x, lighting.directLightDirection.y, lighting.directLightDirection.z);
		UI::Text(buf);
		sprintf(buf, "memory perm %.1f/%.1fMB, temp %.1f/%.1fMB, chunks %.1f/%.1fMB", 
			(float)memory->permStorage.size / Megabytes(1),
			(float)memory->permStorage.capacity / Megabytes(1),
			(float)memory->tempStorage.size / Megabytes(1),
			(float)memory->tempStorage.capacity / Megabytes(1),
			(float)memory->chunkStorage.size / Megabytes(1),
			(float)memory->chunkStorage.capacity / Megabytes(1)
		);
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

		UI::DrawElement(uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1), uiElemType, {16, 16}); // TDOO: размеры тайла должны браться из текстуры, а не передаваться

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
		UI::DrawElement(uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1),
			uiInventoryCell, {16, 16}); // inventory
	}

	UI::SetAnchor(uiAnchor::Bottom, 50);
	UI::ShiftOrigin(-inventoryWidth / 2, 0);
	UI::ShiftOrigin(cellWidth* player.inventory.selectedIndex, 0);
	UI::DrawElement(uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1),
		uiInventorySelectCell, {16,16});

	int heartsCount = 8;
	float heartWidth = 35;
	float heartsWidth = (heartsCount - 1) * heartWidth;
	UI::SetAnchor(uiAnchor::Bottom, 110);
	UI::ShiftOrigin(-heartsWidth / 2, 0);
	for (size_t i = 0; i < heartsCount; i++)
	{
		UI::DrawElement(uiAtlas, glm::vec3(0, 0, 0), glm::vec3(heartWidth, heartWidth, 1), uiHeart, {16,16}); // health bar
	}

	if (gameState->inventoryOpened) {
		UI::SetMargin(true);
		UI::SetAnchor(uiAnchor::Center, 0);
		UI::ShiftOrigin(100, 0);
		UI::SetAdvanceMode(AdvanceMode::Up);
		UI::Text("Craft items");
		for (size_t i = 0; i < ArrayCount(itemInfoTable); i++)
		{

			ItemInfo* itemInfo = &itemInfoTable[i];

			if (!itemInfo->craftable)
				continue;

			Inventory inventoryCopy = player.inventory;
			bool canCraft = true;

			for (size_t j = 0; j < ArrayCount(itemInfo->craftScheme); j++)
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

	// debug shadows
#if 0
	UI::SetAnchor(uiAnchor::Top, 200);
	UI::DrawElement(&Assets.depthMapFBO.textures[0], { 0,0,0, }, { 400, 400, 1 }, { 1,1 }, { 0,0 });
#endif

	UI::End();

#pragma endregion

}