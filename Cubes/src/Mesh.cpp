#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Mesh.h"
#include "Files.h"
#include "Tools.h"

Shader
	cubeInstancedShader = NULL,
	shadowShader = NULL,
	polyMeshShader = NULL,
	polyMeshShadowShader = NULL,
	flatShader = NULL, 
	spriteShader = NULL,
	uiShader = NULL,
	screenShader = NULL;

Shader recompileShader(Shader shader, const char* fileName) {
	Shader newShader = Renderer::createShaderFromFile(fileName);
	if (newShader) {
		Renderer::deleteShader(shader);
		return newShader;
	}
	else return shader;
}

void initShaders() {
	using namespace Renderer;
	cubeInstancedShader = recompileShader(cubeInstancedShader, SHADER_FOLDER "block.glsl");
	shadowShader = recompileShader(shadowShader, SHADER_FOLDER "blockShadow.glsl");
	
	polyMeshShader = recompileShader(polyMeshShader, SHADER_FOLDER "polyMesh.glsl");
	polyMeshShadowShader = recompileShader(polyMeshShadowShader, SHADER_FOLDER "polyMeshShadow.glsl");
	
	flatShader = recompileShader(flatShader, SHADER_FOLDER "flat.glsl");
	spriteShader = recompileShader(spriteShader, SHADER_FOLDER "sprite.glsl");

	uiShader = recompileShader(uiShader, SHADER_FOLDER "uiElement.glsl");

	screenShader = recompileShader(screenShader, SHADER_FOLDER "screen.glsl");
}

#pragma region Sprite
void setupSprite(Sprite& sprite) {
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	sprite.VAO = VAO;
	sprite.VBO = VBO;
	sprite.EBO = EBO;

	// выбираем буфер как текущий
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * sprite.vertexCount, sprite.vertices, GL_STATIC_DRAW); // отправка вершин в память видеокарты
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * sprite.triangleCount, sprite.triangles, GL_STATIC_DRAW); // загружаем индексы

	// "объясняем" как необходимо прочитать массив с вершинами
	GLint stride = sizeof(Vertex);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos)); // pos
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, uv)); // tex coord

	glBindVertexArray(0);
}

// установить параметры шейдера спрайта перед отрисовкой
void useSpriteShader(glm::mat4 projection, glm::mat4 view) {
	GLuint shader = spriteShader;
	Renderer::bindShader(shader);

	// установка переменных в вершинном шейдере
	glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
}

// локальная трансформация для спрайта
void spriteApplyTransform(glm::vec3 pos, float scale, bool spherical) {
	glUniform1f(glGetUniformLocation(spriteShader, "scale"), scale);
	if (spherical)
		glUniform1i(glGetUniformLocation(spriteShader, "spherical"), 1);
	else
		glUniform1i(glGetUniformLocation(spriteShader, "spherical"), 0);

	glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
	model = glm::translate(model, pos);
	model = glm::scale(model, glm::vec3(scale, scale, scale));
	glUniformMatrix4fv(glGetUniformLocation(spriteShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
}


void drawSprite(Sprite& sprite, GLuint texture) {

	glBindVertexArray(sprite.VAO);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawElements(GL_TRIANGLES, sprite.triangleCount * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
#pragma endregion

#pragma region Flat
void useFlatShader(glm::mat4 projection, glm::mat4 view) {
	GLuint shader = flatShader;
	Renderer::bindShader(shader);
	
	// установка переменных в вершинном шейдере
	glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
}

// локальная трансформация для модели
void flatApplyTransform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale) {
	glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
	model = glm::translate(model, pos);
	model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1.0, 0.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.z), glm::vec3(0.0, 0.0, 1.0));
	model = glm::scale(model, scale);
	glUniformMatrix4fv(glGetUniformLocation(flatShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
}

void drawFlat(Geometry* mesh, glm::vec3 color) {
	glUniform4f(glGetUniformLocation(flatShader, "color"), color.r, color.g, color.b, 1);

	glBindVertexArray(mesh->VAO);
	glDrawElements(GL_TRIANGLES, mesh->triangleCount * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void createSprite(Sprite& sprite, float scaleX, float scaleY, glm::vec2& uv, float sizeU, float sizeV) {
	static Vertex vertices[] = {
		Vertex(-0.5, -0.5, 0, uv.x, uv.y),
		Vertex(0.5, -0.5, 0, uv.x + sizeU, uv.y),
		Vertex(0.5, 0.5, 0, uv.x + sizeU, uv.y + sizeV),
		Vertex(-0.5, 0.5, 0, uv.x, uv.y + sizeV),
	};

	Triangle* triangles = new Triangle[2] {
		Triangle(0,1,2),
		Triangle(0,2,3)
	};

	sprite = {0};
	sprite.vertices = vertices;
	sprite.triangles = triangles;
	sprite.vertexCount = 4;
	sprite.triangleCount = 2;
	setupSprite(sprite);
}
#pragma endregion
