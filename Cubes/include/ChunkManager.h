#pragma once
#include "Renderer.h"
#include "DataStructures.h"

#define CHUNK_SX 16
#define CHUNK_SZ 16
#define CHUNK_SY 24
#define CHUNK_SIZE (CHUNK_SX * CHUNK_SZ * CHUNK_SY)
#define MAX_RENDER_DISTANCE 16
#define GetChunksSideCount(renderDistance) (renderDistance * 2 + 1)
#define GetChunksCount(renderDistance) (GetChunksSideCount(renderDistance) * GetChunksSideCount(renderDistance))
#define GetRenderDistance(chunksCount) (((int)sqrt(chunksCount) - 1) / 2)

struct ChunkManager;
extern ChunkManager g_chunkManager;

enum class BlockType : u16 {
	btGround,
	btStone,
	btSnow,
	btIronOre,
	texSun,
	texMoon,
	btAir,
	btCOUNT // должен быть последним. обозначает количество типов блоков
};

struct Block {
	BlockType type;
	bool used;
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

struct ChunkGenTask {
	int posx;
	int posz;
	int index;
};

struct ChunkManager {
	Chunk* chunks;
	Array<ChunkGenTask> chunkGenTasks;
	Array<WorkingThread> threads;
	WorkQueue workQueue;
	int chunksCount;
	int seed;
};

void ChunkManagerCreate(u32 threadsCount);
void ChunkManagerAllocChunks(ChunkManager* manager, u32 renderDistance);
void ChunkManagerBuildChunk(ChunkManager* manager, int index, int posX, int posZ);
void ChunkManagerBuildChunks(ChunkManager* manager, float playerPosX, float playerPosZ);
Block* ChunkManagerPeekBlockFromPos(ChunkManager* manager, float posX, float posY, float posZ, int* outChunkIndex = NULL);
Block* ChunkManagerPeekBlockFromRay(ChunkManager* manager, glm::vec3 rayPos, glm::vec3 rayDir, u8 maxDist, glm::ivec3* outBlockPos, int* outChunkIndex = NULL);
bool ChunkManagerPlaceBlock(ChunkManager* manager, BlockType blockType, glm::vec3 pos, glm::vec3 direction, u8 maxDist);