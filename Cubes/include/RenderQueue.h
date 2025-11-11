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

struct RenderEntryTexturedMesh {
	glm::mat4 transform;
	glm::vec3 color;

	Texture* texture, * shadowMap;
	Geometry* geometry;
	Shader shader;

	bool overrideColor;
};

struct RenderQueue {
	struct {
		glm::mat4 projection, view, lightSpaceMatrix;

		glm::vec3
			directLightDirection,
			directLightColor,
			ambientLightColor;
	} context;

	u8* mem;
	u64 size, capacity;
};

void RenderQueueInit(RenderQueue* queue, void* memory, u64 capacity);
void RenderQueueExecute(RenderQueue* queue);
void RenderQueueClear(RenderQueue* queue);

void RenderQueuePushCamera(RenderQueue* queue, RenderEntryCamera* command);
void RenderQueuePushLighting(RenderQueue* queue, RenderEntryLighting* command);
void RenderQueuePushTexturedMesh(RenderQueue* queue, RenderEntryTexturedMesh* command);