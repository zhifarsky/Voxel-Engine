#pragma once

//
// common
//

#define Kilobytes(v) ((v) * 1024LL)
#define Megabytes(v) ((Kilobytes(v)) * 1024LL)
#define Gigabytes(v) (Megabytes(v) * 1024LL)

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

//
// includes
//

#include "Typedefs.h"
#include "Input.h"
#include "DataStructures.h"
#include "Entity.h"
#include "Items.h"


//
// platform layer
//

struct GameMemory {
	Arena permStorage;
	Arena tempStorage;
	Arena chunkStorage;

	bool isInitialized;
};

struct FrameBufferInfo {
	int sizeX, sizeY;
};

enum class WindowMode {
	Windowed,
	WindowedFullScreen
};

bool GetVsync();
void SetVsync(bool vsyncOn);
void GetCursorPos(double* xpos, double* ypos);
void SetCursorMode(bool enabled);
void WindowSwitchMode(WindowMode windowMode);
WindowMode WindowGetCurrentMode();
void CloseWindow();

//
// game
//

enum GameStatus : u8 {
	gsMainMenu, gsExitMenu, gsInGame, gsDebug,
	gsCount
};

struct GameState {
	GameStatus status;

	Array<Entity> entities;
	Array<Item> droppedItems;

	bool inventoryOpened;
	
	float time, deltaTime;

	float nearPlane, farPlane;

	float shadowRenderDistance;
	float shadowLightDistance;

	int AASamplesCount;
	int shadowQuality;

	glm::vec3 sunDirection;
	//glm::vec3 lightColor;
	//glm::vec3 ambientColor;
};

//void GameInit();
void GameUpdateAndRender(GameMemory* memory, float time, Input* input, FrameBufferInfo* frameBufferInfo);
