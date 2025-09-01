#pragma once
#include <windows.h>
#include "Typedefs.h"
#include "Mesh.h"
#include "DataStructures.h"

// chunk sizes
#define CHUNK_SX 16
#define CHUNK_SZ 16
#define CHUNK_SY 24
#define CHUNK_SIZE (CHUNK_SX * CHUNK_SZ * CHUNK_SY)

enum BlockType : u16 {
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

	Block();
	Block(BlockType t);
};

struct Chunk {
	int posx, posz;
	Block* blocks;
	BlockMesh mesh;
	bool generated;

	void generateMesh();
};

struct ChunkGenTask {
	int posx;
	int posz;
	int index;
};
struct WorkingThread {
	HANDLE handle;
	int threadID;
};

extern WorkQueue chunkGenQueue;
extern ChunkGenTask chunkGenTasks[];
extern WorkingThread* chunkGenThreads;

DWORD chunkGenThreadProc(WorkingThread* args);
void updateChunk(int chunkIndex, int posx, int posz);