#pragma once
#include <glm.hpp>
#include "Typedefs.h"
#include "Entity.h"
#include "Mesh.h"
#include "DataStructures.h"
#include "ChunkManager.h"

struct GameWorld;
extern GameWorld g_gameWorld;

glm::mat4 getProjection(float FOV, int displayW, int displayH);
int getChunksCount(int renderDistance);

struct Camera {
	glm::vec3 pos;
	glm::vec3 front;
	glm::vec3 up;
	float FOV;

	void update(float yaw, float pitch);
};

struct InventoryCell {
	u32 itemsCount;
	ItemType itemType;
};

#define INVENTORY_MAX_SIZE 8

struct Inventory {
	InventoryCell cells[INVENTORY_MAX_SIZE];
	int cellsCount;
	int selectedIndex;
};

Inventory InventoryCreate();
void InventoryAddItem(Inventory* inventory, ItemType type, int count);
void InventorySelectItem(Inventory* inventory, int index);
InventoryCell InventoryGetCurrentItem(Inventory* inventory);
void InventoryDropItem(Inventory* inventory, int index, int count);
int InventoryFindItem(Inventory* inventory, ItemType item);

struct Player {
	Inventory inventory;
	Camera camera;
	glm::vec3 speedVector;
	float maxSpeed;
};

struct GameWorldInfo {
	char name[128];
	int seed;
};

void GetWorldPath(char* buffer, const char* worldname);
void EnumerateWorlds(DynamicArray<GameWorldInfo>* infos);

struct GameWorld {
	GameWorldInfo info;
	//DynamicArray<Item> droppedItems;

	Player player;

	void init(GameMemory* memory, GameWorldInfo* info, u32 renderDistance);
};