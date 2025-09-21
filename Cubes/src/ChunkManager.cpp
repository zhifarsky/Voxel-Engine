#include <FastNoiseLite.h>
#include <glad/glad.h>
#include <glm.hpp>
#include "ChunkManager.h"
#include "Tools.h"

ChunkManager g_chunkManager;

static void setupBlockMesh(BlockMesh* mesh, bool staticMesh) {
	const glm::vec3 faceVerts[] = {
	glm::vec3(0,0,0),
	glm::vec3(1,0,0),
	glm::vec3(1,0,1),
	glm::vec3(0,0,1),
	};

	const int faceIndices[] = {
		0, 1, 2, 2, 3, 0
	};

	GLuint VAO, VBO, instanceVBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &instanceVBO);
	glGenBuffers(1, &EBO);

	mesh->VAO = VAO;
	mesh->VBO = VBO;
	mesh->EBO = EBO;
	mesh->instanceVBO = instanceVBO;

	int bufferMode;
	if (staticMesh) bufferMode = GL_STATIC_DRAW;
	else			bufferMode = GL_DYNAMIC_DRAW;

	// выбираем буфер как текущий
	glBindVertexArray(VAO);

	// instanced face
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(faceVerts), faceVerts, GL_STATIC_DRAW); // отправка вершин в память видеокарты
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faceIndices), faceIndices, GL_STATIC_DRAW); // загружаем индексы

	// "объясняем" как необходимо прочитать массив с вершинами
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0); // pos
	glEnableVertexAttribArray(0);


	// instances
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mesh->faces), mesh->faces, GL_STATIC_DRAW);

	// pos
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_SHORT, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, pos));
	glVertexAttribDivisor(1, 1); // первая переменная - location атрибута в шейдере
	// face direction
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_BYTE, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, face));
	glVertexAttribDivisor(2, 1); // первая переменная - location атрибута в шейдере
	// texture id
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_SHORT, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, textureID));
	glVertexAttribDivisor(3, 1); // первая переменная - location атрибута в шейдере

	// size X 
	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, sizeX));
	glVertexAttribDivisor(4, 1);
	// size z
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, sizeZ));
	glVertexAttribDivisor(5, 1);

	glBindVertexArray(0);
}

// обновить геометрию в ГПУ
static void updateBlockMesh(BlockMesh* mesh) {
	glBindBuffer(GL_ARRAY_BUFFER, mesh->instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mesh->faces), mesh->faces);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	mesh->needUpdate = false;
}

//static FastNoiseLite noise; // TODO: должно быть по 1 на поток?

float PerlinNoise(int seed, float x, float y, float z) {
	FastNoiseLite noise(seed);
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	return noise.GetNoise(x, y, z);
}

float PerlinNoise(int seed, float x, float y) {
	FastNoiseLite noise(seed);
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	return noise.GetNoise(x, y);
}

float PerlinNoise(int seed, glm::vec3 pos) {
	FastNoiseLite noise(seed);
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	return noise.GetNoise(pos.x, pos.y, pos.z);
}

float PerlinNoise(int seed, glm::vec2 pos) {
	FastNoiseLite noise(seed);
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	return noise.GetNoise(pos.x, pos.y);
}

void ChunkGenerateBlocks(Chunk* chunk, int posx, int posz, int seed) {
	chunk->posx = posx;
	chunk->posz = posz;

	float noiseScale = 6.0f;
	float caveNoiseScale = 5.0f;
	float temperatureNoiseScale = 0.7f;

	// определение типов блоков
	auto blocks = chunk->blocks;
	for (s32 y = 0; y < CHUNK_SY; y++) {
		for (s32 z = 0; z < CHUNK_SZ; z++) {
			for (s32 x = 0; x < CHUNK_SX; x++) {
				// biome
				BlockType groundBlockType;
				float temperature = PerlinNoise(seed, glm::vec2(x + posx, z + posz) * temperatureNoiseScale);
				float biomeEdge = 0.5;
				float riverWidth = 0.06;
				temperature = (temperature + 1.0f) / 2.0f;
				if (temperature > biomeEdge)
					groundBlockType = BlockType::btGround;
				else
					groundBlockType = BlockType::btSnow;

				// height
				float height = PerlinNoise(seed, glm::vec2(x + posx, z + posz) * noiseScale);
				height = (height + 1.0f) / 2.0f;

				// generate rivers between biomes
				if (temperature > biomeEdge && temperature - riverWidth <= biomeEdge) {
					height *= 0.15;
					height += 0.2;
				}

				Block* block = &blocks[y][z][x];

				// generate height
				if (y > height * 23.0f)
					block->type = BlockType::btAir;
				else if (y > height * 20.0f) {
					block->type = groundBlockType;
				}
				else
				{
					block->type = BlockType::btStone;

					float ironOre = PerlinNoise(seed, glm::vec3(x + posx, y, z + posz) * 30.0f);
					ironOre = (ironOre + 1.0f) / 2;
					if (ironOre > 0.7)
						block->type = BlockType::btIronOre;

					// generate caves
					float cave = PerlinNoise(seed, glm::vec3(x + posx, y, z + posz) * caveNoiseScale);
					float caveWidth = 0.06;
					cave = (cave + 1.0f) / 2.0f;
					if (cave > 0.5 && cave < 0.5 + caveWidth) {
						block->type = BlockType::btAir;
					}
				}

				// bedrock
				if (y == 0) {
					block->type = BlockType::btStone;
				}
			}
		}
	}

	chunk->generated = true;
}

void ChunkGenerateMesh(Chunk* chunk) {
	int layerStride = CHUNK_SX * CHUNK_SZ;
	int stride = CHUNK_SX;
	int faceCount = 0;

	BlockFaceInstance* faces = chunk->mesh.faces;
	auto blocks = chunk->blocks;

	memset(faces, 0, sizeof(chunk->mesh.faces));

	int blockIndex = 0;
	for (size_t y = 0; y < CHUNK_SY; y++) {
		for (size_t z = 0; z < CHUNK_SZ; z++) {
			for (size_t x = 0; x < CHUNK_SX; x++) {
				Block* block = &chunk->blocks[y][z][x];
				BlockType blockType = chunk->blocks[y][z][x].type;
				if (blockType != BlockType::btAir) {
					TextureID texID;
					switch (blockType)
					{
					case BlockType::btGround:	texID = tidGround;	break;
					case BlockType::btStone:	texID = tidStone;	break;
					case BlockType::btSnow:		texID = tidSnow;	break;
					case BlockType::btIronOre:	texID = tidIronOre;	break;
					default:						texID = tidGround;	break;
					}

					// TODO: Greedy meshing
					// top
					if (y == CHUNK_SY - 1 || blocks[y + 1][z][x].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceYPos, texID);
					}
					// bottom
					if (y == 0 || blocks[y - 1][z][x].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceYNeg, texID);
					}
					// front
					if (z == 0 || blocks[y][z - 1][x].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceZPos, texID);
					}
					// back
					if (z == CHUNK_SZ - 1 || blocks[y][z + 1][x].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceZNeg, texID);
					}
					// left
					if (x == 0 || blocks[y][z][x - 1].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceXPos, texID);
					}
					// right
					if (x == CHUNK_SX - 1 || blocks[y][z][x + 1].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceXNeg, texID);
					}
				}
				blockIndex++;
			}
		}
	}

	chunk->mesh.faceCount = faceCount;
	chunk->mesh.needUpdate = true;
}
 
DWORD chunkGenThreadProc(WorkingThread* args);

void ChunkManagerCreate(u32 threadsCount) {
	g_chunkManager.seed = 0;
	g_chunkManager.workQueue = WorkQueue::Create(GetChunksCount(MAX_RENDER_DISTANCE));
	g_chunkManager.chunkGenTasks = (ChunkGenTask*)calloc(GetChunksCount(MAX_RENDER_DISTANCE), sizeof(ChunkGenTask));
	g_chunkManager.threads.alloc(threadsCount);
	for (size_t i = 0; i < threadsCount; i++) 
	{
		WorkingThread thread;
		thread.threadID = i;
		thread.handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)chunkGenThreadProc, &g_chunkManager.threads[i], 0, 0);
		g_chunkManager.threads.append(thread);
	}
}

void ChunkManagerAllocChunks(ChunkManager* manager, u32 renderDistance) {
	int chunksCount = GetChunksCount(renderDistance);
	manager->chunks = (Chunk*)calloc(chunksCount, sizeof(Chunk));
	manager->chunksCount = chunksCount;

	for (size_t i = 0; i < chunksCount; i++)
	{
		setupBlockMesh(&manager->chunks[i].mesh, false);
	}
}

void ChunkManagerReleaseChunks(ChunkManager* manager) {
	free(manager->chunks);
	manager->chunks = NULL;
	manager->chunksCount = 0;
}

// singlethreaded
//void ChunkManagerBuildChunk(ChunkManager* manager, int index, int posX, int posZ) {
//	ChunkGenerateBlocks(&manager->chunks[index], posX, posZ, manager->seed);
//	ChunkGenerateMesh(&manager->chunks[index]);
//	//updateBlockMesh(&manager->chunks[index].mesh);
//}

// multithreaded
void ChunkManagerBuildChunk(ChunkManager* manager, int index, int posX, int posZ) {
	manager->chunks[index].generated = true; // чтобы несколько потоков не генерировали один и тот же чанк
	manager->chunks[index].posx = posX;
	manager->chunks[index].posz = posZ;
	
	ChunkGenTask* task = &manager->chunkGenTasks[manager->workQueue.taskCount];
	task->index = index;
	manager->workQueue.addTask();
}

DWORD chunkGenThreadProc(WorkingThread* args) {
	for (;;) {
		QueueTaskItem queueItem = g_chunkManager.workQueue.getNextTask();
		if (queueItem.valid) {
			ChunkGenTask* task = &g_chunkManager.chunkGenTasks[queueItem.taskIndex];
			
			Chunk* chunk = &g_chunkManager.chunks[task->index];

			ChunkGenerateBlocks(chunk, chunk->posx, chunk->posz, g_chunkManager.seed);
			ChunkGenerateMesh(chunk);

			g_chunkManager.workQueue.setTaskCompleted();
		}
		else
			WaitForSingleObject(g_chunkManager.workQueue.semaphore, INFINITE);
	}

	return 0;
}

// TODO: многопоточность
void ChunkManagerBuildChunks(ChunkManager* manager, float playerPosX, float playerPosZ) {
	static bool printed = false;
	printed = false;
	
	manager->workQueue.clearTasks();

	int centerPosX = (int)(playerPosX / CHUNK_SX) * CHUNK_SX;
	int centerPosZ = (int)(playerPosZ / CHUNK_SZ) * CHUNK_SZ;
	if (playerPosX < 0) centerPosX -= CHUNK_SX;
	if (playerPosZ < 0) centerPosZ -= CHUNK_SZ;

	s32 renderDistance = GetRenderDistance(manager->chunksCount);
	
	for (int z = -renderDistance; z <= renderDistance; z++) {
		for (int x = -renderDistance; x <= renderDistance; x++) {
			int chunkPosX = centerPosX + x * CHUNK_SX;
			int chunkPosZ = centerPosZ + z * CHUNK_SZ;

			// выясняем, нужно ли генерировать этот чанк
			bool alreadyGenerated = false;
			for (size_t i = 0; i < manager->chunksCount; i++) {
				if (manager->chunks[i].generated &&
					manager->chunks[i].posx == chunkPosX && manager->chunks[i].posz == chunkPosZ)
				{
					alreadyGenerated = true;
					break;
				}
			}
			if (alreadyGenerated)
				continue;

			// находим чанк, который можно заменить
			int chunkToReplaceIndex = -1;
			for (size_t i = 0; i < manager->chunksCount; i++) {
				// если чанк еще не сгенерирован
				if (!manager->chunks[i].generated) {
					chunkToReplaceIndex = i;
					break;
				}
				// если чанк за пределом видимости
				if (abs(manager->chunks[i].posx - centerPosX) > renderDistance * CHUNK_SX ||
					abs(manager->chunks[i].posz - centerPosZ) > renderDistance * CHUNK_SZ) {
					chunkToReplaceIndex = i;
					break;
				}
			}
			if (chunkToReplaceIndex == -1)
				continue;

			ChunkManagerBuildChunk(manager, chunkToReplaceIndex, chunkPosX, chunkPosZ);
		}
	}

	manager->workQueue.waitAndClear();

	// обновляем чанки на ГПУ
	for (size_t i = 0; i < manager->chunksCount; i++)
	{
		Chunk* chunk = &manager->chunks[i];
		if (chunk->generated && chunk->mesh.needUpdate) {
			updateBlockMesh(&chunk->mesh);
		}
	}
}

Block* ChunkManagerPeekBlockFromPos(ChunkManager* manager, float posX, float posY, float posZ, int* outChunkIndex) {
	int chunkIndex = -1;

	// поиск чанка, в котором находится этот блок
	for (size_t i = 0; i < manager->chunksCount; i++)
	{
		if (posX >= manager->chunks[i].posx && posX < manager->chunks[i].posx + CHUNK_SX &&
			posZ >= manager->chunks[i].posz && posZ < manager->chunks[i].posz + CHUNK_SZ) {
			chunkIndex = i;
			break;
		}
	}
	if (chunkIndex == -1)
		return NULL;

	glm::ivec3 relativePos(
		posX - manager->chunks[chunkIndex].posx,
		posY,
		posZ - manager->chunks[chunkIndex].posz);

	// если блок не в пределах чанка
	if (relativePos.x >= CHUNK_SX || relativePos.y >= CHUNK_SY || relativePos.z >= CHUNK_SZ) {
		return NULL;
	}

	if (outChunkIndex)
		*outChunkIndex = chunkIndex;

	return &manager->chunks[chunkIndex].blocks[relativePos.y][relativePos.z][relativePos.x];
}

Block* ChunkManagerPeekBlockFromRay(ChunkManager* manager, glm::vec3 rayPos, glm::vec3 rayDir, u8 maxDist, glm::ivec3* outBlockPos, int* outChunkIndex) {
	if (glm::length(rayDir) == 0)
		return NULL;
	float deltaDist = 1;
	float dist = 0;

	Block* block;

	glm::vec3 norm = glm::normalize(rayDir);
	while (dist < maxDist) {
		glm::vec3 currentPos = rayPos + (norm * dist);
		int chunkIndex = -1;

		block = ChunkManagerPeekBlockFromPos(manager, currentPos.x, currentPos.y, currentPos.z, &chunkIndex);

		if (block != NULL && block->type != BlockType::btAir) {
			if (outBlockPos)
				*outBlockPos = glm::ivec3(currentPos.x, currentPos.y, currentPos.z);
			if (outChunkIndex)
				*outChunkIndex = chunkIndex;
			return block;
		}

		dist += deltaDist;
	}
	return NULL;
}

PlaceBlockResult ChunkManagerPlaceBlock(ChunkManager* manager, BlockType blockType, glm::vec3 pos, glm::vec3 direction, u8 maxDist) {
	PlaceBlockResult res = {};
	int chunkIndex = -1;
	glm::ivec3 blockPos;
	Block* block = ChunkManagerPeekBlockFromRay(manager, pos, direction, maxDist, &blockPos, &chunkIndex);
	if (block) {
		res.typePrev = block->type;
		res.typeNew = blockType;
		res.pos = blockPos;
		res.success = true;

		block->type = blockType;
		Chunk* chunk = &manager->chunks[chunkIndex];
		ChunkGenerateMesh(chunk);
		updateBlockMesh(&chunk->mesh);

		return res;
	}

	res.success = false;
	return res;
}