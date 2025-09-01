#include "Chunk.h"
#include "World.h"

//WorkQueue chunkGenQueue(CHUNK_SIZE);
//ChunkGenTask chunkGenTasks[CHUNK_SIZE];
//WorkingThread* chunkGenThreads = 0;
WorkQueue chunkGenQueue(4 * 1024);
ChunkGenTask chunkGenTasks[4 * 1024];
WorkingThread* chunkGenThreads = 0;


Block::Block() {}

Block::Block(BlockType t) {
	this->type = t;
}

void Chunk::generateMesh() {
	int layerStride = CHUNK_SX * CHUNK_SZ;
	int stride = CHUNK_SX;
	int faceCount = 0;

	BlockFaceInstance* faces = mesh.faces;

	memset(faces, 0, mesh.faceSize * sizeof(BlockFaceInstance));

	int blockIndex = 0;
	glm::vec3 color(1, 1, 1);
	for (size_t y = 0; y < CHUNK_SY; y++) {
		for (size_t z = 0; z < CHUNK_SZ; z++) {
			for (size_t x = 0; x < CHUNK_SX; x++) {
				BlockType blockType = blocks[blockIndex].type;
				if (blockType != btAir) {
					TextureID texID;
					switch (blockType)
					{
					case btGround:	texID = tidGround;	break;
					case btStone:	texID = tidStone;	break;
					case btSnow:	texID = tidSnow;	break;
					case btIronOre:	texID = tidIronOre;	break;
					default:	texID = tidGround;	break;
					}


					// top
					if (y == CHUNK_SY - 1 || blocks[blockIndex + layerStride].type == btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceYPos, texID);
					}

					// bottom
					if (y == 0 || blocks[blockIndex - layerStride].type == btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceYNeg, texID);
					}

					// front
					if (z == 0 || blocks[blockIndex - stride].type == btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceZPos, texID);
					}

					// back
					if (z == CHUNK_SZ - 1 || blocks[blockIndex + stride].type == btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceZNeg, texID);
					}

					// left
					if (x == 0 || blocks[blockIndex - 1].type == btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceXPos, texID);
					}

					// right
					if (x == CHUNK_SX - 1 || blocks[blockIndex + 1].type == btAir) {
						faces[faceCount++] = BlockFaceInstance(blockIndex, faceXNeg, texID);
					}
				}
				blockIndex++;
			}
		}
	}

	mesh.faceCount = faceCount;
}

DWORD chunkGenThreadProc(WorkingThread* args) {
	for (;;) {
		QueueTaskItem queueItem = chunkGenQueue.getNextTask();
		if (queueItem.valid) {
			ChunkGenTask* task = &chunkGenTasks[queueItem.taskIndex];

			gameWorld.generateChunk(task->index, task->posx, task->posz);
			gameWorld.chunks[task->index].generateMesh();
			gameWorld.chunks[task->index].mesh.needUpdate = true; // необходимо отправить новый меш на ГПУ в основоном потоке

			chunkGenQueue.setTaskCompleted();
		}
		else
			WaitForSingleObject(chunkGenQueue.semaphore, INFINITE);
	}

	return 0;
}

void updateChunk(int chunkIndex, int posx, int posz) {
	//dbgprint("generating chunk %d: (%d,%d)\n", posx, posz);
	chunkGenTasks[chunkGenQueue.taskCount].posx = posx;
	chunkGenTasks[chunkGenQueue.taskCount].posz = posz;
	chunkGenTasks[chunkGenQueue.taskCount].index = chunkIndex;
	chunkGenQueue.addTask();
}
