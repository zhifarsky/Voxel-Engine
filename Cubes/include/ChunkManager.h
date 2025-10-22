#pragma once
#include "Renderer.h"
#include "DataStructures.h"
#include "Items.h"

#define CHUNK_SX 16
#define CHUNK_SZ 16
#define CHUNK_SY 24
#define CHUNK_SIZE (CHUNK_SX * CHUNK_SZ * CHUNK_SY)
#define MIN_RENDER_DISTANCE 1
#define MAX_RENDER_DISTANCE 24
#define GetChunksSideCount(renderDistance) (renderDistance * 2 + 1)
#define GetChunksCount(renderDistance) (GetChunksSideCount(renderDistance) * GetChunksSideCount(renderDistance))
#define GetRenderDistance(chunksCount) (((int)sqrt(chunksCount) - 1) / 2)

struct ChunkManager;
extern ChunkManager g_chunkManager;


struct Block {
	BlockType type;
	bool used;
};

enum class BlockSide {
	None = 0, YPos, YNeg, XPos, XNeg, ZPos, ZNeg
};

struct BlockMesh {
	BlockFaceInstance faces[CHUNK_SIZE * 6];
	u32 faceCount;
	u32 VAO, VBO, instanceVBO, EBO;
	bool needUpdate;
};

struct Chunk {
	Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX];
	BlockMesh mesh;
	int posx, posz;
	bool generated;
};

void ChunkGenerateBlocks(Chunk* chunk, int posx, int posz, int seed);
void ChunkGenerateMesh(Chunk* chunk);
void ChunksSaveToDisk(Chunk* chunks, int chunksCount, const char* worldName);

struct ChunkGenTask {
	//int posx;
	//int posz;
	int index;
};

struct ChunkManager {
	Chunk* chunks;
	ChunkGenTask* chunkGenTasks;
	Array<WorkingThread*> threads;
	WorkQueue* workQueue;
	int chunksCount;
	int seed;
};

struct PlaceBlockResult {
	glm::ivec3 pos;
	BlockType typePrev;
	BlockType typeNew;
	bool success;
};

struct PeekBlockResult {
	Block* block; // NOTE: небезопасно хранить результат, т.к. указатель может быть невалиден
	glm::ivec3 blockPos;
	int chunkIndex;
	bool success;
};

void ChunkManagerCreate(u32 threadsCount, int seed);
void ChunkManagerAllocChunks(ChunkManager* manager, u32 renderDistance);
void ChunkManagerReleaseChunks(ChunkManager* manager);
void ChunkManagerBuildChunk(ChunkManager* manager, int index, int posX, int posZ);
void ChunkManagerBuildChunks(ChunkManager* manager, float playerPosX, float playerPosZ);
Block* ChunkManagerPeekBlockFromPos(ChunkManager* manager, float posX, float posY, float posZ, int* outChunkIndex = NULL);
PeekBlockResult ChunkManagerPeekBlockFromRay(ChunkManager* manager, glm::vec3 rayPos, glm::vec3 rayDir, u8 maxDist);
PlaceBlockResult ChunkManagerPlaceBlock(ChunkManager* manager, BlockType blockType, glm::vec3 pos, glm::vec3 direction, u8 maxDist);
PlaceBlockResult ChunkManagerDestroyBlock(ChunkManager* manager, glm::vec3 pos, glm::vec3 direction, u8 maxDist);