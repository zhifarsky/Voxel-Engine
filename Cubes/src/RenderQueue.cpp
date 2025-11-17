#include <glad/glad.h>
#include "RenderQueue.h"

/* TODO:
* привести переменные шейдеров к одному виду, перевести рендеринг мешей на RenderQueue
* устанавливать переменные при помощи буферов
* кэшировать uniform location для каждого шейдера
* батчинг отрисовки? для ui элементов будет эффективнее инстансинга
*/

enum class RenderEntryType {
	Camera,
	Lighting,
	PolymeshShadow,
	PolymeshShadowInstanced,
	Polymesh,
	PolymeshInstanced,
	Sprite,
};

void RenderQueueInit(RenderQueue* queue, void* memory, u64 capacity, FrameBuffer* screenFBO, FrameBuffer* depthMapFBO) {
	*queue = {};

	u32 renderGroupsCount = ArrayCount(queue->renderGroups);
	u64 groupCapacity = capacity / renderGroupsCount;
	u8* groupMemory = (u8*)memory;
	for (size_t i = 0; i < renderGroupsCount; i++)
	{
		queue->renderGroups[i].mem = groupMemory;
		queue->renderGroups[i].capacity = groupCapacity;
		groupMemory += groupCapacity;
	}

	queue->context.screenFBO = screenFBO;
	queue->context.depthMapFBO = depthMapFBO;
}

void RenderQueueClear(RenderQueue* queue) {
	for (size_t i = 0; i < ArrayCount(queue->renderGroups); i++)
	{
		queue->renderGroups[i].size = 0;
	}
	queue->context = {};
}

bool RenderGroupCanPush(RenderGroup* group, u64 size) {
	return group->size + size <= group->capacity;
}

void RenderGroupPush(RenderGroup* group, RenderEntryType commandType, void* command, u32 commandSize) {
	if (RenderGroupCanPush(group, sizeof(commandType) + commandSize)) {
		memcpy(&group->mem[group->size], &commandType, sizeof(RenderEntryType));
		group->size += sizeof(RenderEntryType);
		memcpy(&group->mem[group->size], command, commandSize);
		group->size += commandSize;
	}
	else
		dbgprint("[RENDER QUEUE] queue is full\n");
}

// TODO: сделать макросами?
// * push должен возвращать указатель на память, которую потом можно заполнить, чтобы избежать копирования памяти
void RenderQueuePush(RenderQueue* queue, RenderEntryCamera* command) {
	RenderGroupPush(&queue->initPass, RenderEntryType::Camera, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryLighting* command) {
	RenderGroupPush(&queue->initPass, RenderEntryType::Lighting, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryMeshShadow* command) {
	RenderGroupPush(&queue->shadowPass, RenderEntryType::PolymeshShadow, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryMeshShadowInstanced* command) {
	RenderGroupPush(&queue->shadowPass, RenderEntryType::PolymeshShadowInstanced, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMesh* command) {
	RenderGroupPush(&queue->mainPass, RenderEntryType::Polymesh, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMeshInstanced* command) {
	RenderGroupPush(&queue->mainPass, RenderEntryType::PolymeshInstanced, command, sizeof(*command));
}

// NOTE: если нужно отрисовать на фоне, рисуем перед основным проходом
// сортировки команд нет, поэтому спрайт должен быть запушен после камеры
void RenderQueuePush(RenderQueue* queue, RenderEntrySprite* command) {
	if (command->drawOnBackground)
		RenderGroupPush(&queue->initPass, RenderEntryType::Sprite, command, sizeof(*command));
	else
		RenderGroupPush(&queue->mainPass, RenderEntryType::Sprite, command, sizeof(*command));
}

void RenderSprite(RenderQueue* queue, RenderEntrySprite* entry) {
	Renderer::BindShader(entry->shader);
	Renderer::BindTexture(entry->texture);

	// TDOO: объеденить в одну матрицу
	Renderer::setUniformMatrix4(entry->shader, "view", glm::value_ptr(queue->context.view));
	Renderer::setUniformMatrix4(entry->shader, "projection", glm::value_ptr(queue->context.projection));

	Renderer::setUniformFloat(entry->shader, "scale", entry->scale);
	Renderer::setUniformInt(entry->shader, "spherical", entry->spherical == true ? 1 : 0);

	Renderer::setUniformMatrix4(entry->shader, "model", glm::value_ptr(entry->transform));

	if (entry->drawOnBackground)
		glDepthMask(GL_FALSE);

	Renderer::DrawGeometry(entry->VAO, entry->triangleCount);

	if (entry->drawOnBackground)
		glDepthMask(GL_TRUE);
}

u32 RenderQueueExecute(RenderQueue* queue) {
	u32 commandsExecuted = 0;
	
	//
	// Initial Pass
	//

	{
		RenderGroup* group = &queue->initPass;
		u8* i = group->mem;

		while (i < group->mem + group->size) {
			RenderEntryType entryType = *(RenderEntryType*)i;
			i += sizeof(entryType);
			commandsExecuted++;

			switch (entryType)
			{

			case RenderEntryType::Camera: {
				RenderEntryCamera* entry = (RenderEntryCamera*)i;
				i += sizeof(*entry);

				queue->context.frustum = entry->frustum;
				queue->context.projection = entry->projection;
				queue->context.view = entry->view;
			} break;

			case RenderEntryType::Lighting: {
				RenderEntryLighting* entry = (RenderEntryLighting*)i;
				i += sizeof(*entry);

				queue->context.lightSpaceMatrix = entry->lightSpaceMatrix;
				queue->context.directLightDirection = entry->directLightDirection;
				queue->context.directLightColor = entry->directLightColor;
				queue->context.ambientLightColor = entry->ambientLightColor;
			} break;

			case RenderEntryType::Sprite: {
				RenderEntrySprite* entry = (RenderEntrySprite*)i;
				i += sizeof(*entry);

				RenderSprite(queue, entry);
			} break;
			}
		}
	}

	//
	// Shadow Pass
	//

	{
		RenderGroup* group = &queue->shadowPass;
		u8* i = group->mem;

		Renderer::BindFrameBuffer(queue->context.depthMapFBO);
		Renderer::SetViewportDimensions(queue->context.depthMapFBO->textures[0].width, queue->context.depthMapFBO->textures[0].height);

		while (i < group->mem + group->size) {
			RenderEntryType entryType = *(RenderEntryType*)i;
			i += sizeof(entryType);
			commandsExecuted++;

			switch (entryType) {

			case RenderEntryType::PolymeshShadow: {
				RenderEntryMeshShadow* entry = (RenderEntryMeshShadow*)i;
				i += sizeof(*entry);

				Renderer::BindShader(entry->shader);

				Renderer::setUniformMatrix4(entry->shader, "lightSpaceMatrix", glm::value_ptr(queue->context.lightSpaceMatrix));
				Renderer::setUniformMatrix4(entry->shader, "model", glm::value_ptr(entry->transform));

				Renderer::DrawGeometry(entry->VAO, entry->triangleCount);

				//Renderer::unbindShader();
			} break;

			case RenderEntryType::PolymeshShadowInstanced: {
				RenderEntryMeshShadowInstanced* entry = (RenderEntryMeshShadowInstanced*)i;
				i += sizeof(*entry);

				Renderer::BindShader(entry->shader);

				Renderer::setUniformMatrix4(entry->shader, "lightSpaceMatrix", glm::value_ptr(queue->context.lightSpaceMatrix));
				Renderer::setUniformMatrix4(entry->shader, "model", glm::value_ptr(entry->transform));

				Renderer::DrawGeometryInstanced(entry->VAO, entry->triangleCount * 3, entry->instanceCount);

				//Renderer::unbindShader();
			} break;
	
			}
		}

		Renderer::UnbindFrameBuffer();
	}

	//
	// Main Pass
	//

	{
		RenderGroup* group = &queue->mainPass;
		u8* i = group->mem;

		Renderer::BindFrameBuffer(queue->context.screenFBO);
		Renderer::SetViewportDimensions(queue->context.screenFBO->textures[0].width, queue->context.screenFBO->textures[0].height);

		while (i < group->mem + group->size) {
			RenderEntryType entryType = *(RenderEntryType*)i;
			i += sizeof(entryType);
			commandsExecuted++;

			switch (entryType) {
			case RenderEntryType::Polymesh: {
				RenderEntryTexturedMesh* entry = (RenderEntryTexturedMesh*)i;
				i += sizeof(*entry);
				
				Renderer::BindShader(entry->shader);
				Renderer::BindTexture(entry->texture, 0);
				Renderer::BindTexture(entry->shadowMap, 1);

				// TODO: это точно нужно?
				Renderer::setUniformInt(entry->shader, "texture1", 0);
				Renderer::setUniformInt(entry->shader, "shadowMap", 1);

				glm::mat4 viewProjection = queue->context.projection * queue->context.view;

				Renderer::setUniformMatrix4(entry->shader, "viewProjection", glm::value_ptr(viewProjection));
				Renderer::setUniformMatrix4(entry->shader, "model", glm::value_ptr(entry->transform));
				Renderer::setUniformMatrix4(entry->shader, "lightSpaceMatrix", glm::value_ptr(queue->context.lightSpaceMatrix));

				Renderer::setUniformFloat(entry->shader, "overrideColor", entry->overrideColor);
				Renderer::setUniformFloat3(entry->shader, "color", entry->color);

				Renderer::setUniformFloat3(entry->shader, "sunDir", queue->context.directLightDirection);
				Renderer::setUniformFloat3(entry->shader, "sunColor", queue->context.directLightColor);
				Renderer::setUniformFloat3(entry->shader, "ambientColor", queue->context.ambientLightColor);

				if (entry->wireframeMode) {
					glDepthMask(GL_FALSE);
					glDepthFunc(GL_LEQUAL);
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}
				
				Renderer::DrawGeometry(entry->VAO, entry->triangleCount);

				if (entry->wireframeMode) {
					glDepthMask(GL_TRUE);
					glDepthFunc(GL_LESS);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}

				//Renderer::UnbindTexture(0);
				//Renderer::UnbindTexture(1);
				//Renderer::unbindShader();
			} break;

			case RenderEntryType::PolymeshInstanced: {
				RenderEntryTexturedMeshInstanced* entry = (RenderEntryTexturedMeshInstanced*)i;
				i += sizeof(*entry);

				Renderer::BindShader(entry->shader);
				Renderer::BindTexture(entry->texture, 0);
				Renderer::BindTexture(entry->shadowMap, 1);

				// TODO: это точно нужно?
				Renderer::setUniformInt(entry->shader, "texture1", 0);
				Renderer::setUniformInt(entry->shader, "shadowMap", 1);

				glm::mat4 viewProjection = queue->context.projection * queue->context.view;

				Renderer::setUniformMatrix4(entry->shader, "viewProjection", glm::value_ptr(viewProjection));
				Renderer::setUniformMatrix4(entry->shader, "model", glm::value_ptr(entry->transform));
				Renderer::setUniformMatrix4(entry->shader, "lightSpaceMatrix", glm::value_ptr(queue->context.lightSpaceMatrix));

				Renderer::setUniformFloat(entry->shader, "overrideColor", entry->overrideColor);
				Renderer::setUniformFloat3(entry->shader, "color", entry->color);

				Renderer::setUniformFloat3(entry->shader, "sunDir", queue->context.directLightDirection);
				Renderer::setUniformFloat3(entry->shader, "sunColor", queue->context.directLightColor);
				Renderer::setUniformFloat3(entry->shader, "ambientColor", queue->context.ambientLightColor);

				if (entry->wireframeMode) {
					glDepthMask(GL_FALSE);
					glDepthFunc(GL_LEQUAL);
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}

				Renderer::DrawGeometryInstanced(entry->VAO, entry->triangleCount * 3, entry->instanceCount);

				if (entry->wireframeMode) {
					glDepthMask(GL_TRUE);
					glDepthFunc(GL_LESS);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}

				//Renderer::UnbindTexture(0);
				//Renderer::UnbindTexture(1);
				//Renderer::unbindShader();
			} break;

			case RenderEntryType::Sprite: {
				RenderEntrySprite* entry = (RenderEntrySprite*)i;
				i += sizeof(*entry);

				RenderSprite(queue, entry);
			} break;

			}
		}

		Renderer::UnbindFrameBuffer();
	}


	return commandsExecuted;
}

RenderEntryMeshShadow MakeRenderEntryMeshShadow(u32 VAO, u32 triangleCount, const glm::mat4 transform, Shader shader)
{
	RenderEntryMeshShadow entry;

	entry.VAO = VAO;
	entry.triangleCount = triangleCount;
	entry.transform = transform;
	entry.shader = shader;

	return entry;
}

RenderEntryMeshShadowInstanced MakeRenderEntryMeshShadowInstanced(u32 VAO, u32 triangleCount, u32 instanceCount, const glm::mat4 transform, Shader shader)
{
	RenderEntryMeshShadowInstanced entry;

	entry.VAO = VAO;
	entry.triangleCount = triangleCount;
	entry.instanceCount = instanceCount;
	entry.transform = transform;
	entry.shader = shader;

	return entry;
}

RenderEntryTexturedMesh MakeRenderEntryTexturedMesh(
	u32 VAO, u32 triangleCount, 
	const glm::mat4& transform, 
	Shader shader, Texture* texture, Texture* shadowMap,
	bool overrideColor, glm::vec3 color,
	bool wireframeMode)
{
	RenderEntryTexturedMesh entry;

	entry.VAO = VAO;
	entry.triangleCount = triangleCount;
	entry.transform = transform;
	entry.shader = shader;
	entry.texture = texture;
	entry.shadowMap = shadowMap;
	entry.overrideColor = overrideColor;
	entry.color = color;
	entry.wireframeMode = wireframeMode;

	return entry;
}

RenderEntryTexturedMeshInstanced MakeRenderEntryTexturedMeshInstanced(
	u32 VAO, u32 triangleCount, u32 instanceCount,
	const glm::mat4& transform,
	Shader shader, Texture* texture, Texture* shadowMap,
	bool overrideColor, glm::vec3 color, 
	bool wireframeMode)
{
	RenderEntryTexturedMeshInstanced entry;

	entry.VAO = VAO;
	entry.triangleCount = triangleCount;
	entry.instanceCount = instanceCount;
	entry.transform = transform;
	entry.shader = shader;
	entry.texture = texture;
	entry.shadowMap = shadowMap;
	entry.overrideColor = overrideColor;
	entry.color = color;
	entry.wireframeMode = wireframeMode;

	return entry;
}
