#pragma once
#include <glm.hpp>
#include "Typedefs.h"
#include "Entity.h"
#include "Mesh.h"
#include "DataStructures.h"
#include "Chunk.h"

struct GameWorld;
extern GameWorld gameWorld;

extern int renderDistance;
#define СhunksSide (renderDistance * 2 + 1)
#define СhunksCount (СhunksSide * СhunksSide)

enum GameState {
	gsMainMenu, gsExitMenu, gsInGame,
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

struct Player {
	Camera camera;
	float speed = 20;
};

struct GameWorld {
	u32 seed;
	
	Chunk* chunks;
	u32 chunksCount;
	
	DynamicArray<Entity> entities;
	u32 entitiesCount;

	// TDOO: сделать обычным массивом, чтобы при инициализации игры 
	// у инвентаря сразу был нужный размер
	DynamicArray<BlockType> inventory; 
	u32 inventoryIndex;

	GameState gameState;

	void init(u32 seed, u32 chunksCount);
	void initChunks(int chunksCount);
	void reallocChunks(u32 chunksCount);
	float perlinNoise(glm::vec2 pos, int seedShift = 0);
	float perlinNoise(glm::vec3 pos, int seedShift = 0);
	void generateChunk(int index, int posx, int posz);
	Block* peekBlockFromPos(glm::vec3 pos, int* outChunkIndex = NULL);
	Block* peekBlockFromRay(glm::vec3 rayPos, glm::vec3 rayDir, u8 maxDist, glm::vec3* outBlockPos = NULL);
	bool placeBlock(BlockType type, glm::vec3 pos, glm::vec3 direction, u8 maxDist);
};