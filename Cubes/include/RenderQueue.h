#pragma once
#include "Renderer.h"

struct RenderEntryCamera {
	glm::mat4 view, projection;
};

struct RenderEntryLighting {
	glm::mat4 lightSpaceMatrix; // TODO: вычислять из позиции и направления источника света?
	glm::vec3
		directLightDirection,
		directLightColor,
		ambientLightColor;
};

struct RenderEntryMeshShadow {
	glm::mat4 transform;
	Geometry* geometry;
	Shader shader;
};

struct RenderEntryTexturedMesh {
	glm::mat4 transform;
	glm::vec3 color;

	Texture* texture, * shadowMap;
	Geometry* geometry;
	Shader shader;

	bool overrideColor;
};

struct RenderEntryTexturedMeshInstanced {
	glm::mat4 transform;
	glm::vec3 color;

	Texture* texture, * shadowMap;
	GeometryInstanced geometry;
	Shader shader;

	bool overrideColor;
};

struct RenderGroup {
	u8* mem;
	u64 size, capacity;
};

struct RenderQueue {
	struct {
		FrameBuffer* screenFBO, * depthMapFBO;

		glm::mat4 projection, view, lightSpaceMatrix;

		glm::vec3
			directLightDirection,
			directLightColor,
			ambientLightColor;
	} context;

	u8* mem;
	u64 size, capacity;

	// TODO: доделать
	RenderGroup initPass;
	RenderGroup shadowPass;
	RenderGroup mainPass;
};

void RenderQueueInit(RenderQueue* queue, void* memory, u64 capacity, FrameBuffer* screenFBO, FrameBuffer* depthMapFBO);
u32 RenderQueueExecute(RenderQueue* queue);
void RenderQueueClear(RenderQueue* queue);

void RenderQueuePush(RenderQueue* queue, RenderEntryCamera* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryLighting* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryMeshShadow* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMesh* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMeshInstanced* command);
