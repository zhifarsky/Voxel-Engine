#pragma once
#include <iostream>
#include <glm.hpp>
#include <glad/glad.h>
#include "Typedefs.h"
#include "Renderer.h"

// SPRITE
Sprite createSprite(float scaleX, float scaleY, UV& uv);
//void useSpriteShader(Shader shader, glm::mat4 projection, glm::mat4 view);
//void spriteApplyTransform(Shader shader, glm::vec3 pos, float scale, bool spherical = true);
//void drawSprite(
//	Sprite& sprite, Shader shader, GLuint texture,
//	glm::mat4& projection, glm::mat4& view,
//	glm::vec3& pos, float scale, bool spherical);

// FLAT
//void useFlatShader(glm::mat4 projection, glm::mat4 view);
void ApplyTransform(Shader shader, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
//void drawFlat(Shader shader, Geometry* mesh, glm::vec3 color, float alpha = 1.0f);