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

Frustum FrustumCreate(
	glm::vec3 pos, glm::vec3 front, glm::vec3 up,
	float aspect, float fovY, float zNear, float zFar)
{
	fovY = glm::radians(fovY);

	front = glm::normalize(front);
	glm::vec3 right = glm::normalize(glm::cross(front, {0,1,0}));
	up = glm::normalize(glm::cross(right, front));

	float halfVSide = tanf(fovY * .5f) * zFar;
	float halfHSide = halfVSide * aspect;

	Frustum f;

	glm::vec3 farCenter = front * zFar; // центр дальней плоскости
	glm::vec3 nearCenter = front * zNear; // центр ближней плоскости

	f.nearFace.normal = front;
	f.nearFace.d = -glm::dot(f.nearFace.normal, pos + nearCenter);

	f.farFace.normal = -front;
	f.farFace.d = -glm::dot(f.farFace.normal, pos + farCenter);
	
	glm::vec3 farRight = farCenter + right * halfHSide; // вектор к правому краю дальней плоскости
	f.rightFace.normal = glm::normalize(glm::cross(up, farRight));
	f.rightFace.d = -glm::dot(f.rightFace.normal, pos);

	glm::vec3 farLeft = farCenter - right * halfHSide;
	f.leftFace.normal = glm::normalize(glm::cross(farLeft, up));
	f.leftFace.d = -glm::dot(f.leftFace.normal, pos);

	glm::vec3 farTop = farCenter + up * halfVSide;
	f.topFace.normal = glm::normalize(glm::cross(farTop, right));
	f.topFace.d = -glm::dot(f.topFace.normal, pos);

	glm::vec3 farBottom = farCenter - up * halfVSide;
	f.bottomFace.normal = glm::normalize(glm::cross(right, farBottom));
	f.bottomFace.d = -glm::dot(f.bottomFace.normal, pos);

	return f;
}

float FrustumSphereIntersection(Frustum* frustum, glm::vec3 sphereCenter, float radius)
{
	for (size_t i = 0; i < 6; i++) {
		if ((glm::dot(sphereCenter, frustum->planes[i].normal) + frustum->planes[i].d + radius) <= 0)
		{
			return false;
		}
	}

	return true;
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
