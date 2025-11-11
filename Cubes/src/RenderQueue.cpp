#include "RenderQueue.h"

enum class RenderEntryType {
	Camera,
	Lighting,
	TexturedMesh,
};

void RenderQueueInit(RenderQueue* queue, void* memory, u64 capacity) {
	queue->mem = (u8*)memory;
	queue->size = 0;
	queue->capacity = capacity;
	queue->context = {};
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
void RenderQueuePushCamera(RenderQueue* queue, RenderEntryCamera* command) {
	RenderEntryType type = RenderEntryType::Camera;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueuePushLighting(RenderQueue* queue, RenderEntryLighting* command) {
	RenderEntryType type = RenderEntryType::Lighting;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueuePushTexturedMesh(RenderQueue* queue, RenderEntryTexturedMesh* command) {
	RenderEntryType type = RenderEntryType::TexturedMesh;
	RenderQueuePush(queue, &type, sizeof(type));
	RenderQueuePush(queue, command, sizeof(*command));
}

void RenderQueueExecute(RenderQueue* queue) {
	u8* i = queue->mem;
	while (i < queue->mem + queue->size) {
		RenderEntryType entryType = *(RenderEntryType*)i;
		i += sizeof(entryType);

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
		*/

		case RenderEntryType::TexturedMesh: {
			RenderEntryTexturedMesh* entry = (RenderEntryTexturedMesh*)i;
			i += sizeof(*entry);

			Renderer::bindShader(entry->shader);
			Renderer::bindTexture(entry->texture, 0);
			Renderer::bindTexture(entry->shadowMap, 1);
			
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
		} break;
		
		default:
			break;
		}
	}
}