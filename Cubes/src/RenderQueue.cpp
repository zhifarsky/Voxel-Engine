#include "RenderQueue.h"

enum class RenderEntryType {
	Camera,
	Lighting,
	MeshShadow,
	TexturedMesh,
	TexturedMeshInstanced,
};

void RenderQueueInit(RenderQueue* queue, void* memory, u64 capacity, FrameBuffer* screenFBO, FrameBuffer* depthMapFBO) {
	queue->mem = (u8*)memory;
	queue->size = 0;
	queue->capacity = capacity;
	queue->context = {};
	queue->context.screenFBO = screenFBO;
	queue->context.depthMapFBO = depthMapFBO;
}

void RenderQueueClear(RenderQueue* queue) {
	queue->size = 0;
	queue->context = {};
}

void RenderQueuePush(RenderQueue* queue, void* command, u32 commandSize) {
	if (queue->size + commandSize > queue->capacity) {
		dbgprint("[RENDER QUEUE] queue is full\n");
		return;
	}

	memcpy(&queue->mem[queue->size], command, commandSize);
	queue->size += commandSize;
}

// TODO: сделать макросами?
// * push должен возвращать указатель на память, которую потом можно заполнить, чтобы избежать копирования памяти
void RenderQueuePush(RenderQueue* queue, RenderEntryCamera* command) {
	RenderEntryType type = RenderEntryType::Camera;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryLighting* command) {
	RenderEntryType type = RenderEntryType::Lighting;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryMeshShadow* command) {
	RenderEntryType type = RenderEntryType::MeshShadow;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMesh* command) {
	RenderEntryType type = RenderEntryType::TexturedMesh;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueuePush(RenderQueue* queue, RenderEntryTexturedMeshInstanced* command) {
	RenderEntryType type = RenderEntryType::TexturedMeshInstanced;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

u32 RenderQueueExecute(RenderQueue* queue) {
	u32 commandsExecuted = 0;
	u8* i = queue->mem;
	
	while (i < queue->mem + queue->size) {
		RenderEntryType entryType = *(RenderEntryType*)i;
		i += sizeof(entryType);
		commandsExecuted++;

		switch (entryType)
		{
		case RenderEntryType::Camera: {
			RenderEntryCamera* entry = (RenderEntryCamera*)i;
			i += sizeof(*entry);
			
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
		
		/* TODO:
		* привести переменные шейдеров к одному виду, перевести рендеринг мешей на RenderQueue
		* устанавливать переменные при помощи буферов
		* батчинг отрисовки?
		* сначала shadow pass, потом остальное в не зависимости от порядка команд
		*/

		case RenderEntryType::MeshShadow: {
			RenderEntryMeshShadow* entry = (RenderEntryMeshShadow*)i;
			i += sizeof(*entry);

			Renderer::bindFrameBuffer(queue->context.depthMapFBO);
			Renderer::setViewportDimensions(queue->context.depthMapFBO->textures[0].width, queue->context.depthMapFBO->textures[0].height);

			Renderer::bindShader(entry->shader);

			Renderer::setUniformMatrix4(entry->shader, "lightSpaceMatrix", glm::value_ptr(queue->context.lightSpaceMatrix));
			Renderer::setUniformMatrix4(entry->shader, "model", glm::value_ptr(entry->transform));

			Renderer::drawGeometry(entry->geometry);

			Renderer::unbindShader();
			
			Renderer::unbindFrameBuffer();

		} break;

		case RenderEntryType::TexturedMesh: {
			RenderEntryTexturedMesh* entry = (RenderEntryTexturedMesh*)i;
			i += sizeof(*entry);

			Renderer::bindFrameBuffer(queue->context.screenFBO);
			Renderer::setViewportDimensions(queue->context.screenFBO->textures[0].width, queue->context.screenFBO->textures[0].height);

			Renderer::bindShader(entry->shader);
			Renderer::bindTexture(entry->texture, 0);
			Renderer::bindTexture(entry->shadowMap, 1);
			
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

			Renderer::drawGeometry(entry->geometry);

			Renderer::unbindTexture(0);
			Renderer::unbindTexture(1);
			Renderer::unbindShader();

			Renderer::unbindFrameBuffer();
		} break;
		
		case RenderEntryType::TexturedMeshInstanced: {
			RenderEntryTexturedMeshInstanced* entry = (RenderEntryTexturedMeshInstanced*)i;
			i += sizeof(*entry);

			Renderer::bindFrameBuffer(queue->context.screenFBO);
			Renderer::setViewportDimensions(queue->context.screenFBO->textures[0].width, queue->context.screenFBO->textures[0].height);

			Renderer::bindShader(entry->shader);
			Renderer::bindTexture(entry->texture, 0);
			Renderer::bindTexture(entry->shadowMap, 1);

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

			Renderer::drawInstancedGeo(entry->geometry.VAO, entry->geometry.triangleCount * 3, entry->geometry.instanceCount);

			Renderer::unbindTexture(0);
			Renderer::unbindTexture(1);
			Renderer::unbindShader();

			Renderer::unbindFrameBuffer();
		} break;

		default:
			break;
		}
	}

	return commandsExecuted;
}