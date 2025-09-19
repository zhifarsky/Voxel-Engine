#pragma once
#include <iostream>
#include <glm.hpp>
#include <glad/glad.h>
#include "Typedefs.h"
#include "Renderer.h"

extern Shader 
	cubeInstancedShader, shadowShader, 
	polyMeshShader, polyMeshShadowShader,
	flatShader, spriteShader, 
	uiShader;

void initShaders();
void rebuildShaders();

// SPRITE
void setupSprite(Sprite& sprite);
void createSprite(Sprite& sprite, float scaleX, float scaleY, glm::vec2& uv, float sizeU, float sizeV);
void useSpriteShader(glm::mat4 projection, glm::mat4 view);
void spriteApplyTransform(glm::vec3 pos, float scale, bool spherical = true);
void drawSprite(Sprite& sprite, GLuint texture);

// FLAT
void useFlatShader(glm::mat4 projection, glm::mat4 view);
void flatApplyTransform(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
void drawFlat(Geometry* mesh, glm::vec3 color);