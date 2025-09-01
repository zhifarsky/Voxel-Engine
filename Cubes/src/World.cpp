#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <gtc/noise.hpp>
#include <gtc/matrix_transform.hpp>
#include "World.h"
#include "FastNoiseLite.h"

GameWorld gameWorld;

int renderDistance = 12; // дистанция прорисовки чанков

static FastNoiseLite noise;

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

void GameWorld::init(u32 seed, u32 chunksCount) {
	this->seed = seed;

	//chunks = (Chunk*)calloc(chunksCount, sizeof(Chunk));
	//this->chunksCount = chunksCount;
	//for (size_t i = 0; i < chunksCount; i++)
	//{
	//	// chunks[i].blocks = new Block[CHUNK_SX * CHUNK_SY * CHUNK_SZ]; // TODO: аллокация в арене
	//	chunks[i].blocks = (Block*)calloc(CHUNK_SX * CHUNK_SY * CHUNK_SZ, sizeof(Block)); // TODO: аллокация в арене
	//	chunks[i].mesh.faceSize = CHUNK_SIZE * 6;
	//	chunks[i].mesh.faces = (BlockFaceInstance*)calloc(chunks[i].mesh.faceSize, sizeof(BlockFaceInstance)); // 6 сторон по 4 вершины
	//	setupBlockMesh(chunks[i].mesh, false, true);
	//}
	
	// потоки для создания чанков
	{
		int threadCount = 16;
		chunkGenThreads = (WorkingThread*)malloc(sizeof(WorkingThread) * threadCount);
		for (size_t i = 0; i < threadCount; i++) {
			chunkGenThreads[i].threadID = i;
			chunkGenThreads[i].handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)chunkGenThreadProc, &chunkGenThreads[i], 0, 0);
		}
	}

	// генерация начальных чанков
	initChunks(chunksCount);

	inventory.clear();
	this->inventory.append(btGround);
	this->inventory.append(btStone);
	this->inventory.append(btSnow);

	noise = FastNoiseLite(seed);
}

// TDOO: утечка ГПУ памяти, если вызвать дважды
// необходимо переиспользовать VAO, VBO, EBO или удалять их
void GameWorld::initChunks(int chunksCount) {
	for (size_t i = 0; i < this->chunksCount; i++) {
		free(chunks[i].mesh.faces);
		free(chunks[i].blocks);
	}
	free(chunks);

	chunks = (Chunk*)calloc(chunksCount, sizeof(Chunk));

	this->chunksCount = chunksCount;

	for (size_t i = 0; i < chunksCount; i++)
	{
		chunks[i].blocks = (Block*)calloc(CHUNK_SX * CHUNK_SY * CHUNK_SZ, sizeof(Block)); // TODO: аллокация в арене
		chunks[i].mesh.faceSize = CHUNK_SIZE * 6;
		chunks[i].mesh.faces = (BlockFaceInstance*)calloc(chunks[i].mesh.faceSize, sizeof(BlockFaceInstance)); // 6 сторон по 4 вершины
		setupBlockMesh(chunks[i].mesh, false, true);
	}

	int renderDistance = (sqrt(this->chunksCount) - 1) / 2;

	int chunkNum = 0;
	for (int z = -renderDistance; z <= renderDistance; z++) {
		for (int x = -renderDistance; x <= renderDistance; x++) {
			updateChunk(chunkNum, x * CHUNK_SX, z * CHUNK_SZ);
			chunkNum++;
		}
	}
	chunkGenQueue.waitAndClear();

	for (size_t i = 0; i < this->chunksCount; i++) {
		if (chunks[i].mesh.needUpdate) {
			updateBlockMesh(gameWorld.chunks[i].mesh);
		}
	}
}

void GameWorld::reallocChunks(u32 chunksCount) {
	chunks = (Chunk*)realloc(chunks, sizeof(Chunk) * chunksCount);
	this->chunksCount = chunksCount;
}

float GameWorld::perlinNoise(glm::vec2 pos, int seedShift) {
	if (seedShift != 0)
		noise.SetSeed(seed + seedShift);
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	return noise.GetNoise(pos.x, pos.y);
}

float GameWorld::perlinNoise(glm::vec3 pos, int seedShift) {
	if (seedShift != 0)
		noise.SetSeed(seed + seedShift);
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	return noise.GetNoise(pos.x, pos.y, pos.z);
}

void GameWorld::generateChunk(int index, int posx, int posz) {
	Chunk& chunk = chunks[index];
	chunk.generated = false;

	chunk.posx = posx;
	chunk.posz = posz;

	float noiseScale = 6.0f;
	float caveNoiseScale = 5.0f;
	float temperatureNoiseScale = 0.7f;
	
	// определение типов блоков
	Block* blocks = chunk.blocks;
	int blockIndex = 0;
	for (size_t y = 0; y < CHUNK_SY; y++) {
		for (size_t z = 0; z < CHUNK_SZ; z++) {
			for (size_t x = 0; x < CHUNK_SX; x++) {
				// biome
				BlockType groundBlockType;
				float temperature = perlinNoise(glm::vec2((int)x + posx, (int)z + posz) * temperatureNoiseScale);
				float biomeEdge = 0.5;
				float riverWidth = 0.06;
				temperature = (temperature + 1.0f) / 2.0f;
				if (temperature > biomeEdge)
					groundBlockType = btGround;
				else
					groundBlockType = btSnow;
				
				// height
				float height = perlinNoise(glm::vec2((int)x + posx, (int)z + posz) * noiseScale);
				height = (height + 1.0f) / 2.0f;

				// generate rivers between biomes
				if (temperature > biomeEdge && temperature - riverWidth <= biomeEdge) {
					height *= 0.15;
					height += 0.2;
				}

				// generate height
				if (y > height * 23.0f)
					blocks[blockIndex].type = btAir;
				else if (y > height * 20.0f) {
					blocks[blockIndex].type = groundBlockType;
				}
				else
				{
					blocks[blockIndex].type = btStone;
					
					float ironOre = perlinNoise(glm::vec3((int)x + posx, (int)y, (int)z + posz) * 30.0f, 1);
					ironOre = (ironOre + 1.0f) / 2;
					if (ironOre > 0.7)
						blocks[blockIndex].type = btIronOre;

					// generate caves
					float cave = perlinNoise(glm::vec3((int)x + posx, y, (int)z + posz) * caveNoiseScale);
					float caveWidth = 0.06;
					cave = (cave + 1.0f) / 2.0f;
					if (cave > 0.5 && cave < 0.5 + caveWidth) {
						blocks[blockIndex].type = btAir;
					}
				}

				// bedrock
				if (y == 0) {
					blocks[blockIndex].type = btStone;
				}

				//blocks[blockIndex].brightness = (float)y / (float)chunkSizeY;
				blockIndex++;
			}
		}
	}

	chunk.generated = true;
}

Block* GameWorld::peekBlockFromPos(glm::vec3 pos, int *outChunkIndex) {
	int chunkIndex = -1;
	
	// поиск чанка, в котором находится этот блок
	// TODO: более эффективный метод, чем перебор?
	for (size_t i = 0; i < chunksCount; i++)
	{
		if (pos.x >= chunks[i].posx && pos.x < chunks[i].posx + CHUNK_SX &&
			pos.z >= chunks[i].posz && pos.z < chunks[i].posz + CHUNK_SZ) {
			chunkIndex = i;
			break;
		}
	}
	if (chunkIndex == -1) 
		return NULL;

	if (outChunkIndex)
		*outChunkIndex = chunkIndex;

	glm::vec3 relativePos(
		pos.x - chunks[chunkIndex].posx, 
		pos.y, 
		pos.z - chunks[chunkIndex].posz);

	glm::floor(relativePos);

	static int strideZ = CHUNK_SX;
	static int strideY = CHUNK_SX * CHUNK_SZ;
	int shift = (int)relativePos.x + (int)relativePos.z * strideZ + (int)relativePos.y * strideY;
	if (shift >= CHUNK_SIZE) { // выход за пределы чанка
		return NULL;
	}

	return &chunks[chunkIndex].blocks[shift];
}

Block* GameWorld::peekBlockFromRay(glm::vec3 rayPos, glm::vec3 rayDir, u8 maxDist, glm::vec3* outBlockPos) {
	if (glm::length(rayDir) == 0)
		return NULL;
	float deltaDist = 1;
	float dist = 0;

	Block* block;

	glm::vec3 norm = glm::normalize(rayDir);
	while (dist < maxDist) {
		glm::vec3 currentPos = rayPos + (norm * dist);

		block = peekBlockFromPos(
			glm::vec3(
			currentPos.x,
			currentPos.y,
			currentPos.z));

		if (block != NULL && block->type != btAir) {
			if (outBlockPos)
				*outBlockPos = glm::floor(glm::vec3(currentPos.x, currentPos.y, currentPos.z));
			return block;
		}

		dist += deltaDist;
	}
	return NULL;	
}

BlockFace blockSideCastRay(glm::vec3& rayOrigin, glm::vec3& rayDir, glm::vec3& blockPos) {
	glm::vec3 invSlope = 1.0f / rayDir;
	glm::vec3 boxMin = blockPos;
	glm::vec3 boxMax = blockPos + 1.0f;

	float tx1 = (boxMin.x - rayOrigin.x) * invSlope.x;
	float tx2 = (boxMax.x - rayOrigin.x) * invSlope.x;

	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);

	float ty1 = (boxMin.y - rayOrigin.y) * invSlope.y;
	float ty2 = (boxMax.y - rayOrigin.y) * invSlope.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));


	float tz1 = (boxMin.z - rayOrigin.z) * invSlope.z;
	float tz2 = (boxMax.z - rayOrigin.z) * invSlope.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	glm::vec3 pos(
		(tmin * rayDir.x) + rayOrigin.x,
		(tmin * rayDir.y) + rayOrigin.y,
		(tmin * rayDir.z) + rayOrigin.z
	);

	glm::vec3 relPos = pos - blockPos;

	float e = 0.001;

	if (relPos.y > 1 - e)
		return faceYNeg;
	if (relPos.y < 0 + e)
		return faceYPos;
	if (relPos.z < 0 + e)
		return faceZNeg;
	if (relPos.z > 1 - e)
		return faceZPos;
	if (relPos.x < 0 + e)
		return faceXNeg;
	if (relPos.x > 1 - e)
		return faceXPos;

	return faceNone;
}

bool GameWorld::placeBlock(BlockType type, glm::vec3 pos, glm::vec3 direction, u8 maxDist) {
	if (glm::length(direction) == 0)
		return false;
	float deltaDist = 1;
	float dist = 0;

	Block* block;

	glm::vec3 norm = glm::normalize(direction);
	while (dist < maxDist) {
		glm::vec3 currentPos = pos + (norm * dist);

		int chunkIndex = -1;

		block = peekBlockFromPos(
			glm::vec3(
				currentPos.x,
				currentPos.y,
				currentPos.z),
			&chunkIndex);

		// если нашли блок на пути луча
		if (block != NULL && block->type != btAir) {
			block->type = type;
			chunks[chunkIndex].generateMesh();
			updateBlockMesh(chunks[chunkIndex].mesh);
			return true;
		}

		dist += deltaDist;
	}
	return false;
}