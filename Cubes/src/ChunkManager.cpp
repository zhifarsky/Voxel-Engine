#include <FastNoiseLite.h>
#include <glad/glad.h>
#include <glm.hpp>
#include "ChunkManager.h"
#include "Tools.h"

ChunkManager g_chunkManager;

float Remap(float value, float oldMin, float oldMax, float newMin, float newMax) {
	return newMin + (value - oldMin) * (newMax - newMin) / (oldMax - oldMin);
}

static void setupBlockMesh(BlockMesh* mesh, bool staticMesh) {
	static glm::vec3 faceVerts[] = {
	glm::vec3(0,0,0),
	glm::vec3(1,0,0),
	glm::vec3(1,0,1),
	glm::vec3(0,0,1),
	};

	static int faceIndices[] = {
		0, 1, 2, 2, 3, 0
	};

	// instanced face
	static GLuint VBO, EBO;
	static bool firstRun = true;
	if (firstRun) {
		firstRun = false;
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		
		glBufferData(GL_ARRAY_BUFFER, sizeof(faceVerts), faceVerts, GL_STATIC_DRAW); // отправка вершин в память видеокарты
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faceIndices), faceIndices, GL_STATIC_DRAW); // загружаем индексы
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	GLuint VAO, instanceVBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &instanceVBO);

	mesh->VAO = VAO;
	mesh->VBO = VBO;
	mesh->EBO = EBO;
	mesh->instanceVBO = instanceVBO;

	int bufferMode;
	if (staticMesh) bufferMode = GL_STATIC_DRAW;
	else			bufferMode = GL_DYNAMIC_DRAW;

	// выбираем буфер как текущий
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	
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

void deleteBlockMesh(BlockMesh* mesh) {
	//glDeleteBuffers(1, &mesh->VBO);
	glDeleteBuffers(1, &mesh->instanceVBO);
	//glDeleteBuffers(1, &mesh->EBO);
	glDeleteVertexArrays(1, &mesh->VAO);
	mesh->EBO = mesh->VBO = mesh->instanceVBO = 0;
	mesh->needUpdate = false;
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
				height *= Remap(
					PerlinNoise(seed + 1, glm::vec2(x + posx, z + posz) * (noiseScale / 5)),
					-1, 1, 0, 1.5);

				// generate rivers between biomes
				if (temperature > biomeEdge && temperature - riverWidth <= biomeEdge) {
					height *= 0.15;
					height += 0.2;
				}

				Block* block = &blocks[y][z][x];
				block->used = false;

				// generate height
				if (y > height * 23.0f)
					block->type = BlockType::btAir;
				else if (y > height * 15.0f) {
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

void ChunkGenerateBlocks_DEBUG(Chunk* chunk, int posx, int posz, int seed) {
	for (s32 y = 0; y < CHUNK_SY; y++) {
		for (s32 z = 0; z < CHUNK_SZ; z++) {
			for (s32 x = 0; x < CHUNK_SX; x++) {
				chunk->blocks[y][z][x].type = BlockType::btAir;
				chunk->blocks[y][z][x].used = false;
			}
		}
	}
	

	// столбы в углах
	//for (size_t y = 0; y  < CHUNK_SY; y ++)
	//{
	//	chunk->blocks[y][0][CHUNK_SX - 1].type = BlockType::btGround;
	//	chunk->blocks[y][CHUNK_SZ - 1][0].type = BlockType::btStone;
	//	chunk->blocks[y][CHUNK_SZ - 1][CHUNK_SX - 1].type = BlockType::btSnow;
	//}

	//chunk->blocks[0][0][0].type = BlockType::btGround;
	//chunk->blocks[0][0][1].type = BlockType::btGround;
	//chunk->blocks[0][0][2].type = BlockType::btGround;
	//chunk->blocks[0][0][3].type = BlockType::btStone;
	//
	//chunk->blocks[0][1][0].type = BlockType::btGround;
	//chunk->blocks[0][1][1].type = BlockType::btGround;
	//chunk->blocks[0][1][2].type = BlockType::btGround;
	//chunk->blocks[0][1][3].type = BlockType::btStone;

	//chunk->blocks[0][2][0].type = BlockType::btStone;
	//chunk->blocks[0][2][1].type = BlockType::btStone;
	//chunk->blocks[0][2][2].type = BlockType::btStone;
	//chunk->blocks[0][2][3].type = BlockType::btStone;

	for (size_t z = 0; z < CHUNK_SZ; z++)
	{
		for (size_t x = 0; x < CHUNK_SZ; x++)
		{
			chunk->blocks[0][z][x].type = BlockType::btGround;
		}
	}
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

#define GREEDY 1
					// TODO: Greedy meshing для остальных сторон
					// TODO: проверить, в правильные ли стороны расширяет sizeX и sizeZ на РАЗНЫХ сторонах куба
					// top
#if GREEDY
					{
						int sizeX = 0, sizeZ = 0;

						for (int i = z; i < CHUNK_SZ; i++) // TODO: остальные стороны блока проверяются в этом же цикле
						{
							// проверяем блоки по оси Z
							if (!blocks[y][i][x].used &&
								(blocks[y][z][x].type == blocks[y][i][x].type) &&
								(y == CHUNK_SY - 1 ||
									blocks[y + 1][i][x].type == BlockType::btAir))
							{
								sizeZ++;

								// проверяем блоки по оси X
								// TODO: на каждой итерации проверяем текущий блок, хотя он уже проверен выше 
								int sizeX_current = 1;
								for (int j = x + 1; j < CHUNK_SX; j++)
								{
									if (!(blocks[y][i][j].used) &&
										(blocks[y][z][x].type == blocks[y][i][j].type) &&
										(y == CHUNK_SY - 1 ||
											blocks[y + 1][i][j].type == BlockType::btAir))
									{
										sizeX_current++;
									}
									else
										break;
								}

								if (sizeX == 0)
									sizeX = sizeX_current;
								else
									sizeX = std::min(sizeX, sizeX_current);
							}
							else
								break;
						}

						for (int i = z; i < z + sizeZ; i++)
						{
							for (int j = x; j < x + sizeX; j++)
							{
								blocks[y][i][j].used = true;
							}
						}

						if (sizeX && sizeZ)
							faces[faceCount++] = BlockFaceInstance(blockIndex, faceYPos, texID, sizeX, sizeZ);
					}
#else					
					// top
					if (y == CHUNK_SY - 1 || blocks[y + 1][z][x].type == BlockType::btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceYPos, texID, 1, 1);
					}

#endif
#if 1
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
#endif
				}
				blockIndex++;
			}
		}
	}


	// TODO: изменить хранение переменных used, чтобы проще было очищать
	for (size_t y = 0; y < CHUNK_SY; y++) {
		for (size_t z = 0; z < CHUNK_SZ; z++) {
			for (size_t x = 0; x < CHUNK_SX; x++) {
				chunk->blocks[y][z][x].used = false;
			}
		}
	}

	chunk->mesh.faceCount = faceCount;
	chunk->mesh.needUpdate = true;
}
 
u32 chunkGenThreadProc(WorkingThread* args);

void ChunkManagerCreate(u32 threadsCount) {
	g_chunkManager.seed = 0;
	g_chunkManager.workQueue = WorkQueueCreate(GetChunksCount(MAX_RENDER_DISTANCE));
	g_chunkManager.chunkGenTasks = (ChunkGenTask*)calloc(GetChunksCount(MAX_RENDER_DISTANCE), sizeof(ChunkGenTask));
	g_chunkManager.threads.alloc(threadsCount);
	for (size_t i = 0; i < threadsCount; i++) 
	{
		WorkingThread* thread = WorkingThreadCreate(i, chunkGenThreadProc, &g_chunkManager.threads[i]);
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
	for (size_t i = 0; i < manager->chunksCount; i++)
	{
		deleteBlockMesh(&manager->chunks[i].mesh);
	}
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
	
	ChunkGenTask* task = &manager->chunkGenTasks[WorkQueueGetTasksCount(manager->workQueue)];
	task->index = index;
	WorkQueueAddTask(manager->workQueue);
}

u32 chunkGenThreadProc(WorkingThread* args) {
	for (;;) {
		QueueTaskItem queueItem = WorkQueueGetNextTask(g_chunkManager.workQueue);
		if (queueItem.valid) {
			ChunkGenTask* task = &g_chunkManager.chunkGenTasks[queueItem.taskIndex];

			Chunk* chunk = &g_chunkManager.chunks[task->index];

			ChunkGenerateBlocks(chunk, chunk->posx, chunk->posz, g_chunkManager.seed);
			//ChunkGenerateBlocks_DEBUG(chunk, chunk->posx, chunk->posz, g_chunkManager.seed);
			ChunkGenerateMesh(chunk);

			WorkQueueSetTaskCompleted(g_chunkManager.workQueue);
		}
		else
			WorkQueueThreadWaitForNextTask(g_chunkManager.workQueue, WAIT_INFINITE);
			//WaitForSingleObject(g_chunkManager.workQueue.semaphore, INFINITE);
	}

	return 0;
}

// TODO: многопоточность
void ChunkManagerBuildChunks(ChunkManager* manager, float playerPosX, float playerPosZ) {
	static bool printed = false;
	printed = false;
	
	WorkQueueClearTasks(manager->workQueue);

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

	WorkQueueWaitAndClear(manager->workQueue);

	// обновляем чанки на ГПУ
	for (size_t i = 0; i < manager->chunksCount; i++)
	{
		Chunk* chunk = &manager->chunks[i];
		if (chunk->generated && chunk->mesh.needUpdate) {
			updateBlockMesh(&chunk->mesh);
		}
	}
}


// TDOO: временное решение, сделать лучше
BlockSide CubeGetSideFromRay(glm::ivec3& cubePos, glm::vec3& rayPos, glm::vec3& rayDir) {
	glm::vec3 localCameraPos = rayPos - (glm::vec3)cubePos;

	glm::vec3 fromCubeCenter = glm::normalize(localCameraPos - glm::vec3(0.5f));

	float maxComponent = 0.0f;
	int dominantAxis = -1;

	for (int i = 0; i < 3; ++i) {
		if (abs(fromCubeCenter[i]) > abs(maxComponent)) {
			maxComponent = fromCubeCenter[i];
			dominantAxis = i;
		}
	}

	if (dominantAxis == 0) { // X-axis
		return maxComponent > 0 ? BlockSide::XPos : BlockSide::XNeg;
	}
	else if (dominantAxis == 1) { // Y-axis
		return maxComponent > 0 ? BlockSide::YPos : BlockSide::YNeg;
	}
	else { // Z-axis
		return maxComponent > 0 ? BlockSide::ZPos : BlockSide::ZNeg;
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
	if (relativePos.x >= CHUNK_SX || relativePos.y >= CHUNK_SY || relativePos.z >= CHUNK_SZ ||
		relativePos.x < 0 || relativePos.y < 0 || relativePos.z < 0) {
		return NULL;
	}

	if (outChunkIndex)
		*outChunkIndex = chunkIndex;

	return &manager->chunks[chunkIndex].blocks[relativePos.y][relativePos.z][relativePos.x];
}

PeekBlockResult ChunkManagerPeekBlockFromRay(ChunkManager* manager, glm::vec3 rayPos, glm::vec3 rayDir, u8 maxDist) {
	PeekBlockResult res = { };
	
	if (glm::length(rayDir) == 0)
		return res;
	float deltaDist = 1;
	float dist = 0;

	glm::vec3 norm = glm::normalize(rayDir);
	while (dist < maxDist) {
		glm::vec3 currentPos = rayPos + (norm * dist);
		int chunkIndex = -1;

		Block* block = ChunkManagerPeekBlockFromPos(manager, currentPos.x, currentPos.y, currentPos.z, &chunkIndex);

		if (block != NULL && block->type != BlockType::btAir) {
			res.block = block;
			res.blockPos = glm::floor(currentPos);
			res.chunkIndex = chunkIndex;
			res.success = true;

			return res;
		}

		dist += deltaDist;
	}

	return res;
}

bool inline IsBlockIndexValid(glm::ivec3& index) {
	return (
		index.x >= 0 && index.x < CHUNK_SX &&
		index.y >= 0 && index.y < CHUNK_SY &&
		index.z >= 0 && index.z < CHUNK_SZ);
}

#define ChunkGetBlock(chunk, index) (chunk->blocks[index.y][index.z][index.x])

PlaceBlockResult ChunkManagerPlaceBlock(ChunkManager* manager, BlockType blockType, glm::vec3 pos, glm::vec3 direction, u8 maxDist) {
	PlaceBlockResult res = { };
	PeekBlockResult peekRes = ChunkManagerPeekBlockFromRay(manager, pos, direction, maxDist);
	if (peekRes.success) {
		Chunk* chunk = &manager->chunks[peekRes.chunkIndex];
		
		// позиция относительно чанка
		glm::ivec3 newBlockIndex = 
			glm::vec3(peekRes.blockPos.x, peekRes.blockPos.y, peekRes.blockPos.z) - 
			glm::vec3(chunk->posx, 0, chunk->posz);
		
		BlockSide side = CubeGetSideFromRay(peekRes.blockPos, pos, direction);
		switch (side)
		{
		case BlockSide::YPos:
			newBlockIndex.y += 1;
			break;
		case BlockSide::YNeg:
			newBlockIndex.y -= 1;
			break;
		case BlockSide::XPos:
			newBlockIndex.x += 1;
			break;
		case BlockSide::XNeg:
			newBlockIndex.x -= 1;
			break;
		case BlockSide::ZPos:
			newBlockIndex.z += 1;
			break;
		case BlockSide::ZNeg:
			newBlockIndex.z -= 1;
			break;
		}

		if (IsBlockIndexValid(newBlockIndex)) {
			Block* newBlock = &ChunkGetBlock(chunk, newBlockIndex);
			if (newBlock->type == BlockType::btAir) {
				newBlock->type = blockType;

				ChunkGenerateMesh(chunk);
				updateBlockMesh(&chunk->mesh);

				res.typePrev = peekRes.block->type;
				res.typeNew = blockType;
				res.pos = peekRes.blockPos;
				res.success = true;

				return res;
			}
		}

	}

	res.success = false;
	return res;
}

PlaceBlockResult ChunkManagerDestroyBlock(ChunkManager* manager, glm::vec3 pos, glm::vec3 direction, u8 maxDist) {
	PlaceBlockResult res = {};
	PeekBlockResult peekRes = ChunkManagerPeekBlockFromRay(manager, pos, direction, maxDist);
	if (peekRes.success) {
		res.typePrev = peekRes.block->type;
		res.typeNew = BlockType::btAir;
		res.pos = peekRes.blockPos;
		res.success = true;

		peekRes.block->type = BlockType::btAir;
		Chunk* chunk = &manager->chunks[peekRes.chunkIndex];
		ChunkGenerateMesh(chunk);
		updateBlockMesh(&chunk->mesh);

		return res;
	}

	res.success = false;
	return res;
}