#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <gtc/noise.hpp>
#include <gtc/matrix_transform.hpp>
#include "World.h"
#include "FastNoiseLite.h"
#include "ChunkManager.h"

GameWorld g_gameWorld;

FastNoiseLite noise;

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

void GameWorld::init(u32 seed, u32 renderDistance) {
	this->seed = seed;

	ChunkManagerCreate(16);
	ChunkManagerAllocChunks(&g_chunkManager, renderDistance);

	inventory.clear();
	this->inventory.append(BlockType::btGround);
	this->inventory.append(BlockType::btStone);
	this->inventory.append(BlockType::btSnow);

	noise = FastNoiseLite(seed);
}
