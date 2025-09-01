#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include "Typedefs.h"
#include "Renderer.h"
#include <stb_truetype.h>

// TODO: переименовать поля
struct CharData {
	u16 x0, y0, x1, y1;
	float xoff, yoff, xadvance;
};

struct Font {
	stbtt_fontinfo fontInfo;
	Texture atlas;
	CharData* charData;
	u32 firstChar, charsCount;
	float size; // pixel height
};

Font loadFont(const char* fontPath, float fontSize);

void uiInit();
void uiStart(GLFWwindow* window, Font* defaultFont);
void uiEnd();

void uiUseFont(Font* font);
void uiDrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, glm::vec2 uvScale, glm::vec2 uvShift);
bool uiButton(const char* text, glm::vec2 size = glm::vec2(0.0, 0.0));
bool uiSliderFloat(const char* text, float* value, float minValue, float maxValue);
bool uiSliderInt(const char* text, int* value, int minValue, int maxValue);
float uiGetTextWidth(const char* text);
void uiText(const char* text);

enum class uiAnchor {
	Center,
	Left,
	Right,
	Top,
	Bottom
};

enum class AdvanceMode {
	Right, Left, Up, Down
};

void uiSetAnchor(uiAnchor anchor, float offset);
void uiShiftOrigin(float offsetX, float offsetY);
void uiSetOrigin(float x, float y);
void uiSetAdvanceMode(AdvanceMode mode);
void uiSetMargin(bool enabled);

struct UiStyle {
	glm::vec4 buttonColor;
	glm::vec4 buttonHoveredColor;
	glm::vec4 buttonActiveColor;

	glm::vec2 sliderBarSize;
	glm::vec4 sliderBarColor;

	glm::vec2 sliderKnobSize;
	glm::vec4 sliderKnobColor;
	glm::vec4 sliderKnobHoveredColor;
	glm::vec4 sliderKnobActiveColor;


	float padding;	// внутренний отступ
	float margin;	// внешний отступ
};
