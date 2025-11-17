#pragma once
#include <iostream>
#include <glm.hpp>
#include <glad/glad.h>
#include "Typedefs.h"
#include "Renderer.h"

// SPRITE
Sprite createSprite(float scaleX, float scaleY, UV& uv);

// FLAT
void ApplyTransform(Shader shader, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);