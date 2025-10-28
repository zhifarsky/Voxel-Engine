#include <FastNoiseLite.h>
#include <glad/glad.h>
#include <glm.hpp>
#include "ChunkManager.h"
#include "World.h"
#include "Tools.h"
#include "Files.h"

#define DEBUG_CHUNKS 0

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
	glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, sizeA));
	glVertexAttribDivisor(4, 1);
	// size z
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_BYTE, sizeof(BlockFaceInstance), (void*)offsetof(BlockFaceInstance, sizeB));
	glVertexAttribDivisor(5, 1);

	glBindVertexArray(0);
}

void deleteBlockMesh(Chunk* chunk) {
	BlockMesh* mesh = &chunk->mesh;
	//glDeleteBuffers(1, &mesh->VBO);
	glDeleteBuffers(1, &mesh->instanceVBO);
	//glDeleteBuffers(1, &mesh->EBO);
	glDeleteVertexArrays(1, &mesh->VAO);
	mesh->EBO = mesh->VBO = mesh->instanceVBO = 0;
	chunk->status = ChunkStatus::Initialized; // TODO: в каких случаях удаляем буферы и какой должен быть статус?
}

// обновить геометрию в ГПУ
static void updateBlockMesh(Chunk* chunk) {
	BlockMesh* mesh = &chunk->mesh;
	glBindBuffer(GL_ARRAY_BUFFER, mesh->instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(mesh->faces), mesh->faces);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	chunk->status = ChunkStatus::ReadyToRender;
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

bool inline IsBlockIndexValid(glm::ivec3& index) {
	return (
		index.x >= 0 && index.x < CHUNK_SX &&
		index.y >= 0 && index.y < CHUNK_SY &&
		index.z >= 0 && index.z < CHUNK_SZ);
}

#define ChunkGetBlock(chunk, index) (chunk->blocks[index.y][index.z][index.x])

void ChunkGenerateBlocks(Chunk* chunk, int posx, int posz, int seed) {
	chunk->posx = posx;
	chunk->posz = posz;

	dbgprint("[CHUNK GEN] %d %d\n", posx, posz);

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
}

void ChunkGenerateBlocks_DEBUG(Chunk* chunk, int posx, int posz, int seed) {
	chunk->posx = posx;
	chunk->posz = posz;

	// cleanup
	for (s32 y = 0; y < CHUNK_SY; y++) {
		for (s32 z = 0; z < CHUNK_SZ; z++) {
			for (s32 x = 0; x < CHUNK_SX; x++) {
				chunk->blocks[y][z][x].type = BlockType::btAir;
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

	//chunk->blocks[0][0][1].type = BlockType::btGround;
	//chunk->blocks[1][0][1].type = BlockType::btGround;

	//chunk->blocks[0][1][1].type = BlockType::btGround;
	//
	//chunk->blocks[0][2][1].type = BlockType::btGround;
	//chunk->blocks[1][2][1].type = BlockType::btGround;


	for (size_t z = 0; z < 3; z++)
	{
		for (size_t x = 0; x < 3; x++)
		{
			chunk->blocks[0][z][x].type = BlockType::btGround;
		}
	}

	chunk->blocks[0][1][1].type = BlockType::btAir;
}

bool IsXPosFaceVisible(Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX], int y, int z, int x) {
	return x == CHUNK_SX - 1 || blocks[y][z][x + 1].type == BlockType::btAir;
}
bool IsXNegFaceVisible(Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX], int y, int z, int x) {
	return x == 0 || blocks[y][z][x - 1].type == BlockType::btAir;
}
bool IsYPosFaceVisible(Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX], int y, int z, int x) {
	return y == CHUNK_SY - 1 || blocks[y + 1][z][x].type == BlockType::btAir;
}
bool IsYNegFaceVisible(Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX], int y, int z, int x) {
	return y == 0 || blocks[y - 1][z][x].type == BlockType::btAir;
}
bool IsZPosFaceVisible(Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX], int y, int z, int x) {
	return z == CHUNK_SZ - 1 || blocks[y][z + 1][x].type == BlockType::btAir;
}
bool IsZNegFaceVisible(Block blocks[CHUNK_SY][CHUNK_SZ][CHUNK_SX], int y, int z, int x) {
	return z == 0 || blocks[y][z - 1][x].type == BlockType::btAir;
}

void ChunkGenerateMesh(Chunk* chunk) {
	static int layerStride = CHUNK_SX * CHUNK_SZ;
	static int stride = CHUNK_SX;
	int faceCount = 0;
	int blockIndex = 0;
	
	BlockFaceInstance* faces = chunk->mesh.faces;
	auto blocks = chunk->blocks;

	int inc = 1;

#define GREEDY 1

#if GREEDY == 1
	bool usedXPos[CHUNK_SY][CHUNK_SZ][CHUNK_SX] = { false };
	bool usedXNeg[CHUNK_SY][CHUNK_SZ][CHUNK_SX] = { false };
	bool usedYPos[CHUNK_SY][CHUNK_SZ][CHUNK_SX] = { false };
	bool usedYNeg[CHUNK_SY][CHUNK_SZ][CHUNK_SX] = { false };
	bool usedZPos[CHUNK_SY][CHUNK_SZ][CHUNK_SX] = { false };
	bool usedZNeg[CHUNK_SY][CHUNK_SZ][CHUNK_SX] = { false };
#endif

	memset(faces, 0, sizeof(chunk->mesh.faces));
	
	for (size_t y = 0; y < CHUNK_SY; y += inc) {
		for (size_t z = 0; z < CHUNK_SZ; z += inc) {
			for (size_t x = 0; x < CHUNK_SX; x += inc) {
				
				BlockType blockType = chunk->blocks[y][z][x].type;
				if (blockType != BlockType::btAir) {
					const BlockInfo* blockInfo = GetBlockInfo(blockType);
					TextureID texID = blockInfo->textureID;

#if GREEDY == 1
					//
					// try grow every cube face 
					//

					struct Run {
						enum Axis { AxisY = 0, AxisZ = 1, AxisX = 2 };
						bool (*used)[CHUNK_SZ][CHUNK_SX];
						BlockFace face;
						Axis axisA, axisB;
					};

					Run runs[] = { 
						{usedXNeg, faceXNeg, Run::AxisY, Run::AxisZ},
						{usedXPos, faceXPos, Run::AxisY, Run::AxisZ},
						{usedYNeg, faceYNeg, Run::AxisZ, Run::AxisX},
						{usedYPos, faceYPos, Run::AxisZ, Run::AxisX},
						{usedZNeg, faceZNeg, Run::AxisY, Run::AxisX},
						{usedZPos, faceZPos, Run::AxisY, Run::AxisX},
					};

					for (Run& run : runs) {
						int chunkSize[3] = { CHUNK_SY, CHUNK_SZ, CHUNK_SX };
						int startIndex[3] = { y, z, x };
						BlockType firstBlockType = blocks[y][z][x].type;

						auto IsVisible = IsXNegFaceVisible;
						switch (run.face)
						{
						case faceXNeg: IsVisible = IsXNegFaceVisible; break;
						case faceXPos: IsVisible = IsXPosFaceVisible; break;
						case faceYNeg: IsVisible = IsYNegFaceVisible; break;
						case faceYPos: IsVisible = IsYPosFaceVisible; break;
						case faceZNeg: IsVisible = IsZNegFaceVisible; break;
						case faceZPos: IsVisible = IsZPosFaceVisible; break;
						}

						// grow face on A and B axis
						int lenA = 0, lenB = 0;
						// go A
						for (int i = startIndex[run.axisA]; i < chunkSize[run.axisA]; i++)
						{
							int accessA[] = { startIndex[0], startIndex[1], startIndex[2] };
							accessA[run.axisA] = i;

							bool* currentUsed = &run.used[accessA[0]][accessA[1]][accessA[2]];
							BlockType currentBlockType = blocks[accessA[0]][accessA[1]][accessA[2]].type;

							if (
								*currentUsed == false &&
								currentBlockType == firstBlockType &&
								IsVisible(blocks, accessA[0], accessA[1], accessA[2]))
							{
								int currentLenB = 0;
								// go B
								for (int j = startIndex[run.axisB]; j < chunkSize[run.axisB]; j++)
								{
									int accessB[] = { startIndex[0], startIndex[1], startIndex[2] };
									accessB[run.axisA] = i;
									accessB[run.axisB] = j;

									if (lenB != 0 && currentLenB >= lenB) {
										break;
									}

									bool* currentUsed = &run.used[accessB[0]][accessB[1]][accessB[2]];
									BlockType currentBlockType = blocks[accessB[0]][accessB[1]][accessB[2]].type;

									if (
										*currentUsed == false &&
										currentBlockType == firstBlockType &&
										IsVisible(blocks, accessB[0], accessB[1], accessB[2]))
									{
										currentLenB++;
									}
									else {
										break;
									}
								}
								if (lenB == 0)
									lenB = currentLenB; // first run
								else
									lenB = std::min(currentLenB, lenB);

								lenA++;
							}
							else {
								break;
							}
						}
						// set blocks as used
						for (int i = 0; i < lenA; i++) {
							for (int j = 0; j < lenB; j++) {
								int access[] = { startIndex[0], startIndex[1], startIndex[2] };
								access[run.axisA] += i;
								access[run.axisB] += j;

								run.used[access[0]][access[1]][access[2]] = true;
							}
						}

						// push result to faces array
						if (lenA > 0 && lenB > 0) {
							int faceSize[3] = { 1 };
							faceSize[run.axisA] = lenA;
							faceSize[run.axisB] = lenB;
							faces[faceCount++] = BlockFaceInstance(blockIndex, run.face, texID, faceSize[0], faceSize[1], faceSize[2]);
						}
					}

#else				
					glm::ivec3 pos(x, y, z);
					static glm::ivec3 chunkSize(CHUNK_SX, CHUNK_SY, CHUNK_SZ);
					// x/y/z
					for (size_t i = 0; i < 3; i++)
					{
						glm::ivec3 neighbourPosA = pos;
						glm::ivec3 neighbourPosB = pos;
						neighbourPosA[i] -= 1;
						neighbourPosB[i] += 1;
						
						// neg
						if (pos[i] == 0 ||
							blocks[neighbourPosA.y][neighbourPosA.z][neighbourPosA.x].type == BlockType::btAir)
						{
							faces[faceCount++] = BlockFaceInstance(blockIndex, (BlockFace)(i * 2), texID, inc, inc);
						}

						// pos
						if (pos[i] == chunkSize[i] - 1 || 
							blocks[neighbourPosB.y][neighbourPosB.z][neighbourPosB.x].type == BlockType::btAir)
						{
							faces[faceCount++] = BlockFaceInstance(blockIndex, (BlockFace)(i * 2 + 1), texID, inc, inc);
						}
					}
#endif

				}
				blockIndex++;
			}
		}
	}

	chunk->mesh.faceCount = faceCount;
	chunk->status = ChunkStatus::Generated;
}

void ChunkManagerCreate(int seed) {
	g_chunkManager.seed = seed;
	g_chunkManager.queue = new TaskQueue();
}

void ChunkManagerAllocChunks(ChunkManager* manager, u32 renderDistance) {
	int chunksCount = GetChunksCount(renderDistance);
	manager->chunks = (Chunk*)calloc(chunksCount, sizeof(Chunk));
	manager->chunksCount = chunksCount;

	for (size_t i = 0; i < chunksCount; i++)
	{
		setupBlockMesh(&manager->chunks[i].mesh, false);
	}

#if (!DEBUG_CHUNKS)
	manager->queue->Start(std::max(1, GetThreadsCount()));
#else
	manager->queue->Start(1);
#endif
}

void ChunkManagerReleaseChunks(ChunkManager* manager) {
	// дожидаемся выполнения всех задач, чтобы потоки не использовали освобожденную память
	manager->queue->StopAndJoin(); 

	for (size_t i = 0; i < manager->chunksCount; i++)
	{
		deleteBlockMesh(&manager->chunks[i]);
	}
	free(manager->chunks);
	manager->chunks = NULL;
	manager->chunksCount = 0;

}



void GetChunkPath(char* buffer, const char* worldname, int posx, int posz) {
	sprintf(buffer, "%s%s/%d %d", WORLDS_FOLDER, worldname, posx, posz);
}

// TDOO: асинхнонная загрузка и сохранение чанков

void ChunkSaveToDisk(Chunk* chunk, const char* worldname, int posx, int posz) {
#if DEBUG_CHUNKS
	return;
#endif

	char filename[128];

	{
		CreateNewDirectory(WORLDS_FOLDER);
		char worldpath[128];
		GetWorldPath(worldpath, worldname);
		CreateNewDirectory(worldpath);
	}

	GetChunkPath(filename, worldname, posx, posz);

	FILE* f = fopen(filename, "wb");
	if (f) {
		dbgprint("[CHUNK SAVE] %s\n", filename);
		fwrite(chunk->blocks, 1, sizeof(Block) * CHUNK_SIZE, f);
		fclose(f);
	}
	else {
		dbgprint("[CHUNK SAVE FAIL] %s\n", filename);
	}
}

void ChunksSaveToDisk(Chunk* chunks, int chunksCount, const char* worldName) {
	for (size_t i = 0; i < chunksCount; i++)
	{
		if (chunks[i].generationInProgress)
			continue;

		switch (chunks[i].status)
		{
		case ChunkStatus::Generated:
		case ChunkStatus::ReadyToRender:
			ChunkSaveToDisk(&chunks[i], worldName, chunks[i].posx, chunks[i].posz);
			break;
		}
	}
}


bool ChunkLoadFromDisk(Chunk* chunk, const char* worldname, int posx, int posz) {
#if DEBUG_CHUNKS
	return false;
#endif
	
	char filename[128];
	GetChunkPath(filename, worldname, chunk->posx, chunk->posz);
	if (IsFileExists(filename)) {
		FILE* f = fopen(filename, "rb");
		if (f) {
			dbgprint("[CHUNK LOAD] %s\n", filename);
			fread(chunk->blocks, 1, sizeof(Block) * CHUNK_SIZE, f);
			fclose(f);
			chunk->posx = posx;
			chunk->posz = posz;
			return true;
		}
		else {
			dbgprint("[CHUNK LOAD FAIL] %s\n", filename);
			return false;
		}
	}
	else {
		//dbgprint("[CHUNK LOAD NOT FOUND] %s\n", filename);
		return false;
	}
}

// TODO: улучшить решение
BlockSide CubeGetSideFromRay(glm::ivec3& cubePos, glm::vec3& rayOrigin, glm::vec3& rayDir) {
	auto RayIntersection = [](glm::vec3& rayOrigin, glm::vec3& rayDir, u32 plane, float planeOffset) -> glm::vec3 {
		float epsilon = 1e-6;
		if (plane > 2 ||
			abs(rayDir[plane]) < epsilon)
		{
			return { 0,0,0 };
		}

		float t = -(rayOrigin[plane] - planeOffset) / rayDir[plane];
		glm::vec3 res = rayOrigin + (t * rayDir);
		res[plane] = planeOffset;

		return res;
		};

	int j = 0, i = 0;
	bool success = false;
	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < 3; i++)
		{
			glm::vec3 rayOriginRelative = rayOrigin - glm::vec3(cubePos);
			glm::vec3 p = RayIntersection(rayOriginRelative, rayDir, i, j);
			if (p.length() > 0) {
				if (j == 0 && rayOriginRelative[i] < 0 ||
					j == 1 && rayOriginRelative[i] > 1)
				{
					if (p.x >= 0 && p.x <= 1 &&
						p.y >= 0 && p.y <= 1 &&
						p.z >= 0 && p.z <= 1)
					{
						success = true;
						goto search_end;
					}
				}
			}
		}
	}

search_end:

	// dbgprint("[GETSIDE] success %d i %d j %d\n", success, i, j);

	if (success) {
		if (i == 0) {
			if (j == 0)
				return BlockSide::XNeg;
			else if (j == 1)
				return BlockSide::XPos;
		}
		else if (i == 1) {
			if (j == 0)
				return BlockSide::YNeg;
			else if (j == 1)
				return BlockSide::YPos;
		}
		else if (i == 2) {
			if (j == 0)
				return BlockSide::ZNeg;
			else if (j == 1)
				return BlockSide::ZPos;
		}
	}

	return BlockSide::None;
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
				updateBlockMesh(chunk);

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
		updateBlockMesh(chunk);

		return res;
	}

	res.success = false;
	return res;
}

glm::ivec2 PosToChunkPos(glm::vec3 pos)
{
	return {
		(int)floor(pos.x / CHUNK_SX)* CHUNK_SX,
		(int)floor(pos.z / CHUNK_SZ)* CHUNK_SZ,
	};
}

void ChunkManagerBuildChunks(ChunkManager* manager, float playerPosX, float playerPosZ) {
	static bool printed = false;
	printed = false;

	int centerPosX = (int)(playerPosX / CHUNK_SX) * CHUNK_SX;
	int centerPosZ = (int)(playerPosZ / CHUNK_SZ) * CHUNK_SZ;
	if (playerPosX < 0) centerPosX -= CHUNK_SX;
	if (playerPosZ < 0) centerPosZ -= CHUNK_SZ;

	s32 renderDistance = GetRenderDistance(manager->chunksCount);

	for (int z = -renderDistance; z <= renderDistance; z++) 
	{
		for (int x = -renderDistance; x <= renderDistance; x++) 
		{
			int chunkPosX = centerPosX + x * CHUNK_SX;
			int chunkPosZ = centerPosZ + z * CHUNK_SZ;

			// check if already generated
			bool alreadyGenerated = false;
			for (size_t i = 0; i < manager->chunksCount; i++)
			{
				Chunk* chunk = &manager->chunks[i];
				
				if (chunk->status != ChunkStatus::Uninitalized && 
					chunk->posx == chunkPosX && chunk->posz == chunkPosZ)
				{
					alreadyGenerated = true;
					break;
				}
			}
			if (alreadyGenerated)
				continue;

			// find place for new chunk
			int chunkToReplaceIndex = -1;
			for (size_t i = 0; i < manager->chunksCount; i++)
			{
				Chunk* chunk = &manager->chunks[i];

				if (chunk->generationInProgress)
					continue;

				// если чанк еще не сгенерирован
				if (chunk->status == ChunkStatus::Uninitalized) {
					chunkToReplaceIndex = i;
					break;
				}

				// если чанк за пределом видимости
				if (abs(chunk->posx - centerPosX) > renderDistance * CHUNK_SX ||
					abs(chunk->posz - centerPosZ) > renderDistance * CHUNK_SZ) {
					chunkToReplaceIndex = i;
					break;
				}
			}
			if (chunkToReplaceIndex == -1)
				continue;


			// sync gen
			{
				//ChunkGenerateBlocks(&manager->chunks[chunkToReplaceIndex], chunkPosX, chunkPosZ, 0);
				//ChunkGenerateMesh(&manager->chunks[chunkToReplaceIndex]);
			}

			// async gen
			// add to queue
			Chunk* chunkToReplace = &manager->chunks[chunkToReplaceIndex];
			chunkToReplace->generationInProgress = true;
			bool saveBeforeReplacing =
				(chunkToReplace->status == ChunkStatus::Generated ||
					chunkToReplace->status == ChunkStatus::ReadyToRender);
			
			chunkToReplace->status = ChunkStatus::Initialized;

			int oldChunkPosX = chunkToReplace->posx;
			int oldChunkPosZ = chunkToReplace->posz;

			chunkToReplace->posx = chunkPosX;
			chunkToReplace->posz = chunkPosZ;

			manager->queue->AddTask([saveBeforeReplacing, chunkToReplace, oldChunkPosX, oldChunkPosZ, chunkPosX, chunkPosZ]() {
				// save chunk before replacing
				if (saveBeforeReplacing)
				{
					ChunkSaveToDisk(chunkToReplace, g_gameWorld.info.name, oldChunkPosX, oldChunkPosZ);
				}
				// try loading from disk
				if (!ChunkLoadFromDisk(chunkToReplace, g_gameWorld.info.name, chunkPosX, chunkPosZ)) {
#if !DEBUG_CHUNKS

					// if failed, generate chunk
					ChunkGenerateBlocks(chunkToReplace, chunkPosX, chunkPosZ, 0);
#else
					ChunkGenerateBlocks_DEBUG(chunkToReplace, chunkPosX, chunkPosZ, 0);
#endif

				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(200)); // test

				//ChunkGenerateBlocks(chunkToReplace, chunkPosX, chunkPosZ, 0);
				ChunkGenerateMesh(chunkToReplace);
				chunkToReplace->generationInProgress = false;
			});
		}
	}

	//int updates = 0;
	//int maxUpdates = 5;
	for (size_t i = 0; i < manager->chunksCount; i++)
	{
		//if (updates > maxUpdates)
		//	break;

		Chunk* chunk = &manager->chunks[i];
		if (!chunk->generationInProgress && chunk->status == ChunkStatus::Generated) {
			updateBlockMesh(chunk);
			//updates++;
		}
	}
}