#include <gtc/noise.hpp>
#include <gtc/matrix_transform.hpp>
#include "World.h"
#include "FastNoiseLite.h"
#include "ChunkManager.h"
#include "Tools.h"
#include "Files.h"

GameWorld g_gameWorld;

glm::mat4 getProjection(float FOV, int displayW, int displayH) {
	return glm::perspective(glm::radians(FOV), (float)displayW / (float)displayH, 0.1f, 1000.0f);
}

int getChunksCount(int renderDistance) {
	int chunkSide = renderDistance * 2 + 1;
	return chunkSide * chunkSide;
}

void Camera::update(float yaw, float pitch) {
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(direction);
}

Inventory InventoryCreate() {
	Inventory inventory = { 0 };
	return inventory;
}

void InventoryAddItem(Inventory* inventory, ItemType type, int count)
{
	for (size_t i = 0; i < inventory->cellsCount; i++)
	{
		if (inventory->cells[i].itemType == type) {
			inventory->cells[i].itemsCount += count;
			return;
		}
	}

	if (inventory->cellsCount == INVENTORY_MAX_SIZE)
		return;

	InventoryCell* cell = &inventory->cells[inventory->cellsCount];
	cell->itemType = type;
	cell->itemsCount = count;
	inventory->cellsCount++;
}

void InventoryDropItem(Inventory* inventory, int index, int count) {
	if (index < 0 || index >= inventory->cellsCount)
		return;


	InventoryCell* cell = &inventory->cells[index];
	cell->itemsCount -= count;

	if (cell->itemsCount <= 0) {
		for (size_t i = index; i < inventory->cellsCount - 1; i++)
		{
			inventory->cells[i] = inventory->cells[i + 1];
		}
		inventory->cellsCount--;
	}
}

void InventorySelectItem(Inventory* inventory, int index) {
	if (index < 0 || index >= inventory->cellsCount)
		return;

	inventory->selectedIndex = index;
}

InventoryCell InventoryGetCurrentItem(Inventory* inventory) {
	return inventory->cells[inventory->selectedIndex];
}

int InventoryFindItem(Inventory* inventory, ItemType item) {
	for (size_t i = 0; i < inventory->cellsCount; i++)
	{
		InventoryCell* cell = &inventory->cells[i];
		if (cell->itemsCount > 0 && cell->itemType == item) {
			return i;
		}
	}
	return -1;
}

void GetWorldPath(char* buffer, const char* worldname) {
	sprintf(buffer, "%s%s", WORLDS_FOLDER, worldname);
}

void GameWorld::init(GameWorldInfo* info, u32 renderDistance) {
	this->info = *info;
	
	ChunkManagerCreate(this->info.seed);
	ChunkManagerAllocChunks(&g_chunkManager, renderDistance);

	//inventory.clear();
	//this->inventory.append(BlockType::btGround);
	//this->inventory.append(BlockType::btStone);
	//this->inventory.append(BlockType::btSnow);
}
