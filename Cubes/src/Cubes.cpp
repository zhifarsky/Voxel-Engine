#pragma region external dependencies
#define NOMINMAX
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//#include <glad/glad.h>
//#include <SOIL/SOIL.h>
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
#include "Typedefs.h"
#include "Tools.h"
#include "Mesh.h"
#include "ui.h"
#include "Directories.h"
#include "Entity.h"
#include "DataStructures.h"
#include "World.h"
#include "Input.h"
#include "Renderer.h"
#include "ChunkManager.h"
#pragma endregion

#define rotateXYZ(matrix, x, y, z)\
matrix = glm::rotate(matrix, glm::radians(x), glm::vec3(1.0, 0.0, 0.0));\
matrix = glm::rotate(matrix, glm::radians(y), glm::vec3(0.0, 1.0, 0.0));\
matrix = glm::rotate(matrix, glm::radians(z), glm::vec3(0.0, 0.0, 1.0));

float near_plane = 1.0f, far_plane = 500.0f;
float shadowLightDist = 100;

Player player;
Input input[2] = { 0 };
Input* newInput = &input[0];
Input* oldInput = &input[1];

static float lastX = 400, lastY = 300; // позиция курсора
static float yaw = 0, pitch = 0;
static bool cursorMode = false; // TRUE для взаимодействия с UI, FALSE для перемещения камеры

static bool vsyncOn = false;

static DynamicArray<Entity> entities;

// mouse input
static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (cursorMode == true) {
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
		lastX = xpos;
		lastY = ypos;
		return;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
	lastX = xpos;
	lastY = ypos;


	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	//if (cursorMode == true) {
	//	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	//	return;
	//}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_Q && action == GLFW_RELEASE) {
		if (cursorMode == false) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			cursorMode = true;
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cursorMode = false;
		}
	}
	else if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
		rebuildShaders();
	}

	// переключение между ячейками инвентаря
	else if (
		key >= GLFW_KEY_1 &&
		key < GLFW_KEY_1 + g_gameWorld.inventory.count &&
		action == GLFW_PRESS) {
		g_gameWorld.inventoryIndex = key - GLFW_KEY_1;
	}
}

enum uiElemType : u16 {
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

float g_time = 0, g_deltaTime = 0, g_prevTime = 0;

int display_w, display_h;

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
	Geometry defaultBox, entityMesh;
	Sprite sunSprite, moonSprite;
	FrameBuffer depthMapFBO;
	Texture depthMap;
	Font bigFont, regularFont;
} Assets;

void RenderMainMenu(GLFWwindow* window);
void RenderPauseMenu(GLFWwindow* window);
void RenderGame(GLFWwindow* window);

void CubesMainGameLoop(GLFWwindow* window) {
	g_gameWorld.init(0, 8);
	g_gameWorld.gameState = gsMainMenu;

	//chunks = g_gameWorld.chunks;

	player.camera.pos = glm::vec3(8, 30, 8);
	player.camera.front = glm::vec3(0, 0, -1);
	player.camera.up = glm::vec3(0, 1, 0);
	player.maxSpeed = 25;


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

		for (uiElemType elemType : {uiInventoryCell, uiHeart, uiCross, uiGroundBlock, uiStoneBlock, uiSnowBlock})
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
		Vertex* vertices = new Vertex[8]{
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
		Triangle* triangles = new Triangle[12]{
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


	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

	// shadow framebuffer
	{
		Assets.depthMap = Renderer::createTexture(
			4096, 4096, NULL,
			PixelFormat::DepthMap, PixelFormat::DepthMap,
			TextureWrapping::ClampToBorder, TextureFiltering::Nearest);

		Assets.depthMapFBO = Renderer::createDepthMapFrameBuffer(&Assets.depthMap);
	}

	// MAIN GAME LOOP
	while (!glfwWindowShouldClose(window)) {
		g_time = glfwGetTime();
		g_deltaTime = g_time - g_prevTime;
		g_prevTime = g_time;

		glfwGetFramebufferSize(window, &display_w, &display_h);
		Renderer::setViewportDimensions(display_w, display_h);

		// input processing
		{
			ProcessButtonInput(&oldInput->startGame, &newInput->startGame, IsKeyReleased(window, GLFW_KEY_ENTER));
			ProcessButtonInput(&oldInput->switchExitMenu, &newInput->switchExitMenu, IsKeyReleased(window, GLFW_KEY_ESCAPE));

			ProcessButtonInput(&oldInput->attack, &newInput->attack, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_LEFT));
			ProcessButtonInput(&oldInput->placeBlock, &newInput->placeBlock, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_RIGHT));

			ProcessButtonInput(&oldInput->forward, &newInput->forward, IsKeyReleased(window, GLFW_KEY_W));
			ProcessButtonInput(&oldInput->backwards, &newInput->backwards, IsKeyReleased(window, GLFW_KEY_S));
			ProcessButtonInput(&oldInput->left, &newInput->left, IsKeyReleased(window, GLFW_KEY_A));
			ProcessButtonInput(&oldInput->right, &newInput->right, IsKeyReleased(window, GLFW_KEY_D));
		}

		if (newInput->startGame.halfTransitionsCount) {
			if (g_gameWorld.gameState == gsMainMenu)
				g_gameWorld.gameState = gsInGame;
		}
		if (newInput->switchExitMenu.halfTransitionsCount) {
			if (g_gameWorld.gameState == gsInGame)
				g_gameWorld.gameState = gsExitMenu;
			else if (g_gameWorld.gameState == gsExitMenu)
				g_gameWorld.gameState = gsInGame;
		}

		switch (g_gameWorld.gameState)
		{
		case gsMainMenu:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			RenderMainMenu(window);
			break;
		case gsExitMenu:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			RenderPauseMenu(window);
			break;
		case gsInGame:
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			RenderGame(window);
			break;
		}

		// ImGui
		//drawDebugGui(
		//	&wireframe_cb, &debugView_cb, &fov_slider, 
		//	&player.speed, &chunksUpdated, 
		//	&sunDir, &sunColor, 
		//	&ambientColor, 
		//	glm::vec2(currentChunkPosX, currentChunkPosZ));

		// swap old and new inputs
		Input* tempInput = oldInput;
		oldInput = newInput;
		newInput = tempInput;

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void RenderMainMenu(GLFWwindow* window) {
	Renderer::clear(0.6, 0.6, 0.6);

	uiStart(window, &Assets.regularFont);
	uiSetAnchor(uiAnchor::Center, 0);
	uiSetAdvanceMode(AdvanceMode::Down);
	if (uiButton("Start game", glm::vec2(0.0, 0.0))) {
		g_gameWorld.gameState = gsInGame;
	}

	const char* caption = "Main menu";
	uiSetAnchor(uiAnchor::Top, 100);
	uiUseFont(&Assets.bigFont);
	uiShiftOrigin(-uiGetTextWidth(caption) / 2, 0);
	uiText(caption);
	uiEnd();
}

void RenderPauseMenu(GLFWwindow* window) {
	enum class PauseMenuState {
		MainMenu,
		SettingsMenu
	};
	static PauseMenuState menuState = PauseMenuState::MainMenu;

	Renderer::clear(0.6, 0.6, 0.6);

	uiStart(window, &Assets.regularFont);
	uiSetAdvanceMode(AdvanceMode::Down);

	switch (menuState)
	{
	case PauseMenuState::MainMenu: {
		uiSetAnchor(uiAnchor::Center, 0);
		if (uiButton("Return", glm::vec2(0.0, 0.0))) {
			g_gameWorld.gameState = gsInGame;
		}
		if (uiButton("Settings")) {
			menuState = PauseMenuState::SettingsMenu;
		}
		if (uiButton("Exit")) {
			glfwSetWindowShouldClose(window, true);
		}
	} break;
	case PauseMenuState::SettingsMenu: {
		uiSetAnchor(uiAnchor::Center, 0);
		uiShiftOrigin(0, 200);

		uiSliderFloat("FOV", &player.camera.FOV, 40, 120);

		int renderDistanceSlider = GetRenderDistance(g_chunkManager.chunksCount);
		if (uiSliderInt("Render distance", &renderDistanceSlider, MIN_RENDER_DISTANCE, MAX_RENDER_DISTANCE)) {
			if (GetRenderDistance(g_chunkManager.chunksCount) != renderDistanceSlider) {
				ChunkManagerReleaseChunks(&g_chunkManager);
				ChunkManagerAllocChunks(&g_chunkManager, renderDistanceSlider);
			}
		}

		if (uiButton("Switch wireframe mode"))
			wireframe_cb = !wireframe_cb;

		if (uiButton("Switch debug view"))
			debugView_cb = !debugView_cb;

		if (uiButton("Switch vsync")) {
			if (vsyncOn)
				glfwSwapInterval(1);
			else
				glfwSwapInterval(0);
			vsyncOn = !vsyncOn;
		}

		if (uiButton("Back")) {
			menuState = PauseMenuState::MainMenu;
		}
	} break;
	default:
		break;
	}

	uiEnd();
}

void RenderGame(GLFWwindow* window) {
	{
		// update player
		float cameraSpeedAdj = player.maxSpeed * g_deltaTime; // делаем скорость камеры независимой от FPS
		if (ButtonHeldDown(newInput->forward))
			player.speedVector += cameraSpeedAdj * player.camera.front;
			//player.camera.pos += cameraSpeedAdj * player.camera.front;
		if (ButtonHeldDown(newInput->backwards))
			player.speedVector -= cameraSpeedAdj * player.camera.front;
			//player.camera.pos -= cameraSpeedAdj * player.camera.front;
		if (ButtonHeldDown(newInput->left))
			player.speedVector -= glm::normalize(glm::cross(player.camera.front, player.camera.up)) * cameraSpeedAdj;;
			//player.camera.pos -= glm::normalize(glm::cross(player.camera.front, player.camera.up)) * cameraSpeedAdj;
		if (ButtonHeldDown(newInput->right))
			player.speedVector += glm::normalize(glm::cross(player.camera.front, player.camera.up)) * cameraSpeedAdj;
			//player.camera.pos += glm::normalize(glm::cross(player.camera.front, player.camera.up)) * cameraSpeedAdj;

		player.camera.pos += player.speedVector;
		player.speedVector *= 0.8; // TODO: deltatime

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
	if (newInput->placeBlock.halfTransitionsCount) {
		ChunkManagerPlaceBlock(&g_chunkManager, g_gameWorld.inventory[g_gameWorld.inventoryIndex], player.camera.pos, player.camera.front, 128);
	}
	else if (newInput->attack.halfTransitionsCount) {
		ChunkManagerPlaceBlock(&g_chunkManager, BlockType::btAir, player.camera.pos, player.camera.front, 128);
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
	glm::vec3 direction;
	glm::mat4 view, projection;
	{
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		player.camera.front = glm::normalize(direction);


		view = glm::lookAt(player.camera.pos, player.camera.pos + player.camera.front, player.camera.up);

		// матрица проекции (перспективная/ортогональная проекция)
		projection = glm::mat4(1.0);
		projection = glm::perspective(glm::radians(player.camera.FOV), (float)display_w / (float)display_h, 0.1f, 1000.0f);
	}
#pragma endregion

	// draw shadow maps
	glm::mat4 lightSpaceMatrix;
	{
		//glCullFace(GL_FRONT);

		//float projDim = (float)Assets.depthMap.width / 16.0f;
		float projDim = (float)Assets.depthMap.width / 16.0f / 4;
		glm::mat4 lightProjection = glm::ortho(-projDim, projDim, -projDim, projDim, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(
			shadowLightDist * directLightDir + player.camera.pos * glm::vec3(1, 0, 1),
			player.camera.pos * glm::vec3(1, 0, 1),
			glm::vec3(0.0f, 1.0f, 0.0f));

		lightSpaceMatrix = lightProjection * lightView;

		Renderer::setViewportDimensions(Assets.depthMap.width, Assets.depthMap.height);
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
		Renderer::setViewportDimensions(display_w, display_h);
	}

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
	glDepthMask(TRUE);

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
		Renderer::bindTexture(&Assets.depthMap, 1);

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
		Renderer::bindTexture(&Assets.depthMap, 1);

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

	// draw ui
	uiStart(window, &Assets.regularFont);
	uiSetAnchor(uiAnchor::Center, 0);
	uiDrawElement(&Assets.uiAtlas, glm::vec3(0, 0, 0), glm::vec3(64, 64, 1), uiUV[uiCross].scale, uiUV[uiCross].offset); // cursor

	//uiSetAnchor(uiTopAnchor, 360/2);
	//uiDrawElement(depthMap, { 0,0,0 }, { 360,360,1 }, { 1,1 }, { 0,0 }); // depthmap (shadow)

	// debug info
	{
		uiSetAnchor(uiAnchor::Left, 10);
		uiSetAdvanceMode(AdvanceMode::Down);
		char buf[128];
		sprintf(buf, "FPS: %d", (int)(1.0f / g_deltaTime));
		uiText(buf);
		sprintf(buf, "Frametime: %.3f", g_deltaTime);
		uiText(buf);
		sprintf(buf, "%d chunks", g_chunkManager.chunksCount);
		uiText(buf);
		sprintf(buf, "%d blocks", g_chunkManager.chunksCount * CHUNK_SX * CHUNK_SY * CHUNK_SZ);
		uiText(buf);
	}

	int cellsCount = 8;
	float cellWidth = 70;
	float inventoryWidth = (cellsCount - 1) * cellWidth;
	uiSetAnchor(uiAnchor::Bottom, 50);
	uiSetAdvanceMode(AdvanceMode::Right);
	uiShiftOrigin(-inventoryWidth / 2, 0);
	uiSetMargin(false);
	for (size_t i = 0; i < g_gameWorld.inventory.count; i++)
	{
		uiElemType uiElemType;

		switch (g_gameWorld.inventory[i])
		{
		case BlockType::btGround:	uiElemType = uiGroundBlock; break;
		case BlockType::btStone:	uiElemType = uiStoneBlock;	break;
		case BlockType::btSnow:		uiElemType = uiSnowBlock;	break;
		default:
			break;
		}

		uiDrawElement(&Assets.uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1), uiUV[uiElemType].scale, uiUV[uiElemType].offset);
	}

	uiSetAnchor(uiAnchor::Bottom, 50);
	uiShiftOrigin(-inventoryWidth / 2, 0);
	for (size_t i = 0; i < cellsCount; i++)
	{
		uiDrawElement(&Assets.uiAtlas, glm::vec3(0), glm::vec3(cellWidth, cellWidth, 1), uiUV[uiInventoryCell].scale, uiUV[uiInventoryCell].offset); // inventory
	}

	int heartsCount = 8;
	float heartWidth = 35;
	float heartsWidth = (heartsCount - 1) * heartWidth;
	uiSetAnchor(uiAnchor::Bottom, 110);
	uiShiftOrigin(-heartsWidth / 2, 0);
	for (size_t i = 0; i < heartsCount; i++)
	{
		uiDrawElement(&Assets.uiAtlas, glm::vec3(0, 0, 0), glm::vec3(heartWidth, heartWidth, 1), uiUV[uiHeart].scale, uiUV[uiHeart].offset); // health bar
	}
	uiEnd();
#pragma endregion

}

static void drawDebugGui(bool* wireframe_cb, bool* debugView_cb, float* fov_slider, float* camspeed_slider,
	int* chunksUpdated,
	glm::vec3* sunDir, glm::vec3* sunColor, glm::vec3* skyColor,
	glm::vec2 currentChunkPos)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Button("Rebuild shaders")) {
		initShaders();
	}
	ImGui::Separator();

	ImGui::SliderFloat("FOV", fov_slider, 0, 179);
	ImGui::SliderFloat("Cam speed", camspeed_slider, 1, 60);
	ImGui::Separator();
	ImGui::Checkbox("wireframe", wireframe_cb);
	ImGui::Checkbox("Debug view", debugView_cb);
	ImGui::Separator();
	ImGui::SliderFloat3("Sun position", (float*)sunDir, -1, 1);
	ImGui::ColorEdit3("Sun color", (float*)sunColor);
	ImGui::ColorEdit3("Ambient color", (float*)skyColor);
	ImGui::Separator();
	ImGui::InputFloat3("Camera pos", (float*)&player.camera.pos);
	ImGui::InputFloat2("Chunk pos", (float*)&currentChunkPos);
	ImGui::InputFloat3("Camera front", (float*)&player.camera.front);
	ImGui::Separator();
	ImGui::SliderFloat("Shadow near plane", &near_plane, 0.1, 1000);
	ImGui::SliderFloat("Shadow far plane", &far_plane, 0.1, 1000);
	ImGui::SliderFloat("Shadow light dist", &shadowLightDist, 0, 50);

	ImGui::Separator();

	if (vsyncOn) ImGui::Text("VSync ON");
	else ImGui::Text("VSync OFF");

	if (ImGui::Button("VSync")) {
		vsyncOn = !vsyncOn;
		typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALPROC)(int);
		PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = 0;

		const char* extensions = (char*)glGetString(GL_EXTENSIONS);

		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");

		if (wglSwapIntervalEXT) {
			if (vsyncOn) wglSwapIntervalEXT(1);
			else wglSwapIntervalEXT(0);
		}
	}
	ImGui::Separator();
	if (ImGui::TreeNodeEx("Chunks")) {
		for (size_t i = 0; i < g_chunkManager.chunksCount; i++)
		{
			ImGui::PushID(i);
			int pos[2] = { g_chunkManager.chunks[i].posx, g_chunkManager.chunks[i].posz };
			ImGui::InputInt2("", pos);
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}