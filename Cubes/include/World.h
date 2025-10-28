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
	glm::vec3 pos;
	glm::vec3 front;
	glm::vec3 up;
	float FOV;

	void update(float yaw, float pitch);
};

struct Plane {
	glm::vec3 normal;
	float d;
};

struct Frustum {
	union {
		struct {
			Plane
				topFace, bottomFace,
				rightFace, leftFace,
				farFace, nearFace;
		};
		Plane planes[6];
	};
};

Frustum FrustumCreate(
	glm::vec3 pos, glm::vec3 front, glm::vec3 up,
	float aspect, float fovY, float zNear, float zFar);

// true если сфера внутри frustum
float FrustumSphereIntersection(Frustum* frustum, glm::vec3 sphereCenter, float radius);

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
void InventoryDropItem(Inventory* inventory, int index, int count);

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
	DynamicArray<Entity> entities;
	DynamicArray<Item> droppedItems;

	Player player;

	// TDOO: сделать обычным массивом, чтобы при инициализации игры 
	// у инвентаря сразу был нужный размер
	//DynamicArray<BlockType> inventory; 
	//u32 inventoryIndex;

	void init(GameWorldInfo* info, u32 renderDistance);
};