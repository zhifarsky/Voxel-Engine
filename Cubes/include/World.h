#pragma once
#include <glm.hpp>
#include "Typedefs.h"
#include "Entity.h"
#include "Mesh.h"
#include "DataStructures.h"
#include "ChunkManager.h"

struct GameWorld;
extern GameWorld g_gameWorld;

enum GameState {
	gsMainMenu, gsExitMenu, gsInGame, gsDebug,
	gsCount
};

glm::mat4 getProjection(float FOV, int displayW, int displayH);
int getChunksCount(int renderDistance);

struct Camera {
	glm::vec3 pos = glm::vec3(0, 0, 0);
	glm::vec3 front = glm::vec3(0, 0, -1);
	glm::vec3 up = glm::vec3(0, 1, 0);
	float FOV = 80;

	void update(float yaw, float pitch);
};

struct Item {
	glm::vec3 pos;
	u32 count;
	BlockType type;
};

struct InventoryCell {
	u32 itemsCount;
	BlockType itemType;
};

#define INVENTORY_MAX_SIZE 8

struct Inventory {
	InventoryCell cells[INVENTORY_MAX_SIZE];
	int cellsCount;
	int selectedIndex;
};

Inventory InventoryCreate();
void InventoryAddItem(Inventory* inventory, BlockType type, int count);
void InventorySelectItem(Inventory* inventory, int index);
void InventoryDropItem(Inventory* inventory, int index, int count);

struct Player {
	Inventory inventory;
	Camera camera;
	glm::vec3 speedVector;
	float maxSpeed;
};

struct GameWorld {
	DynamicArray<Entity> entities;
	DynamicArray<Item> droppedItems;

	// TDOO: сделать обычным массивом, чтобы при инициализации игры 
	// у инвентаря сразу был нужный размер
	//DynamicArray<BlockType> inventory; 
	//u32 inventoryIndex;

	GameState gameState;

	void init(u32 seed, u32 renderDistance);
};