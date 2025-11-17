#pragma once
#include "Renderer.h"

struct RenderEntryCamera {
	Frustum frustum;
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
	u32 VAO, triangleCount;
	Shader shader;
};

RenderEntryMeshShadow MakeRenderEntryMeshShadow(
	u32 VAO, u32 triangleCount, const glm::mat4 transform, Shader shader);


struct RenderEntryMeshShadowInstanced {
	glm::mat4 transform;
	u32 VAO, triangleCount, instanceCount;
	Shader shader;
};

RenderEntryMeshShadowInstanced MakeRenderEntryMeshShadowInstanced(
	u32 VAO, u32 triangleCount, u32 instanceCount, const glm::mat4 transform, Shader shader);

struct RenderEntryTexturedMesh {
	glm::mat4 transform;
	glm::vec3 color;

	Texture* texture, * shadowMap;
	
	u32 VAO, triangleCount;
	
	Shader shader;

	bool overrideColor;
	bool wireframeMode;
};

RenderEntryTexturedMesh MakeRenderEntryTexturedMesh(
	u32 VAO, u32 triangleCount,
	const glm::mat4& transform,
	Shader shader, Texture* texture = NULL, Texture* shadowMap = NULL,
	bool overrideColor = false, glm::vec3 color = glm::vec3(0, 0, 0), 
	bool wireframeMode = false);

struct RenderEntryTexturedMeshInstanced {
	glm::mat4 transform;
	glm::vec3 color;

	Texture* texture, * shadowMap;

	u32 VAO, triangleCount, instanceCount;
	
	Shader shader;

	bool overrideColor;
	bool wireframeMode;
};

RenderEntryTexturedMeshInstanced MakeRenderEntryTexturedMeshInstanced(
	u32 VAO, u32 triangleCount, u32 instanceCount,
	const glm::mat4& transform,
	Shader shader, Texture* texture = NULL, Texture* shadowMap = NULL,
	bool overrideColor = false, glm::vec3 color = glm::vec3(0, 0, 0), 
	bool wireframeMode = false);

struct RenderEntrySprite {
	glm::mat4 transform;

	Texture* texture;

	u32 VAO, triangleCount;

	float scale;

	Shader shader;

	bool spherical, drawOnBackground;
};

struct RenderGroup {
	u8* mem;
	u64 size, capacity;
};

struct RenderQueue {
	struct {
		FrameBuffer* screenFBO, * depthMapFBO;

		Frustum frustum;
		glm::mat4 projection, view, lightSpaceMatrix;

		glm::vec3
			directLightDirection,
			directLightColor,
			ambientLightColor;
	} context;

	union {
		struct {
			RenderGroup initPass;
			RenderGroup shadowPass;
			RenderGroup mainPass;
			RenderGroup postPass; // TODO: рендеринг дебаг геометрии и тд (wireframe чанка)
		};

		RenderGroup renderGroups[3];
	};
};

void RenderQueueInit(RenderQueue* queue, void* memory, u64 capacity, FrameBuffer* screenFBO, FrameBuffer* depthMapFBO);
u32 RenderQueueExecute(RenderQueue* queue);
void RenderQueueClear(RenderQueue* queue);

void RenderQueuePush(RenderQueue* queue, RenderEntryCamera* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryLighting* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryMeshShadow* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryMeshShadowInstanced* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMesh* command);
void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMeshInstanced* command);
void RenderQueuePush(RenderQueue* queue, RenderEntrySprite* command);
