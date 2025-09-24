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

enum class uiAnchor {
	Center,
	Left,
	Right,
	Top,
	Bottom
};

enum class AdvanceMode {
	Right, Left, Up, Down, None
};

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

Font loadFont(const char* fontPath, float fontSize);

namespace UI {
	void Init();
	void Start(GLFWwindow* window, Font* defaultFont);
	void End();

	void UseFont(Font* font);
	void DrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, glm::vec2 uvScale, glm::vec2 uvShift);
	bool Button(const char* text, glm::vec2 size = { 0, 0 });
	bool CheckBox(const char* text, glm::vec2 size = { 0,0 });
	bool SliderFloat(const char* text, float* value, float minValue, float maxValue);
	bool SliderInt(const char* text, int* value, int minValue, int maxValue);
	float GetTextWidth(const char* text);
	void Text(const char* text);

	void SetAnchor(uiAnchor anchor, float offset);
	void ShiftOrigin(float offsetX, float offsetY);
	void SetOrigin(float x, float y);
	void SetAdvanceMode(AdvanceMode mode);
	void SetMargin(bool enabled);
}
