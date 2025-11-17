#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Mesh.h"
#include "Files.h"
#include "Tools.h"
#include "Cubes.h"

#pragma region Sprite
#pragma endregion

#pragma region Flat
//void useFlatShader(glm::mat4 projection, glm::mat4 view) {
//	GLuint shader = flatShader;
//	Renderer::bindShader(shader);
//	
//	// установка переменных в вершинном шейдере
//	glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
//	glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
//}

// локальная трансформация для модели
void ApplyTransform(Shader shader, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale) {
	glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
	model = glm::translate(model, pos);
	model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1.0, 0.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.z), glm::vec3(0.0, 0.0, 1.0));
	model = glm::scale(model, scale);
	glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
}

void drawFlat(Shader shader, Geometry* mesh, glm::vec3 color, float alpha) {
	glUniform4f(glGetUniformLocation(shader, "color"), color.r, color.g, color.b, alpha);

	glBindVertexArray(mesh->VAO);
	glDrawElements(GL_TRIANGLES, mesh->triangleCount * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}



Sprite createSprite(float scaleX, float scaleY, UV& uv) {
	Vertex vertices[] = {
		Vertex(-0.5, -0.5, 0, uv.offset.x, uv.offset.y),
		Vertex(0.5, -0.5, 0, uv.offset.x + uv.scale.x, uv.offset.y),
		Vertex(0.5, 0.5, 0, uv.offset.x + uv.scale.x, uv.offset.y + uv.scale.y),
		Vertex(-0.5, 0.5, 0, uv.offset.x, uv.offset.y + uv.scale.y),
	};

	Triangle triangles[] = {
		Triangle(0,1,2),
		Triangle(0,2,3)
	};

	Sprite sprite = {0};
	sprite.vertexCount = ArrayCount(vertices);
	sprite.triangleCount = ArrayCount(triangles);
	
	{
		glGenVertexArrays(1, &sprite.VAO);
		glGenBuffers(1, &sprite.VBO);
		glGenBuffers(1, &sprite.EBO);

		// выбираем буфер как текущий
		glBindVertexArray(sprite.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, sprite.VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.EBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * sprite.vertexCount, vertices, GL_STATIC_DRAW); // отправка вершин в память видеокарты
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * sprite.triangleCount, triangles, GL_STATIC_DRAW); // загружаем индексы

		// "объясняем" как необходимо прочитать массив с вершинами
		GLint stride = sizeof(Vertex);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos)); // pos
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, uv)); // tex coord

		glBindVertexArray(0);
	}

	return sprite;
}
#pragma endregion
