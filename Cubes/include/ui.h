#pragma once
#include <glm.hpp>
#include "Cubes.h"
#include "Input.h"
#include "Typedefs.h"
#include "Renderer.h"
//#include <stb_truetype.h>
#include "Font.h"

struct UVOffset {
	glm::vec2 offset;
	glm::vec2 scale;
};

enum class uiAnchor {
	Center,
	Left,
	LeftTop,
	LeftBottom,
	Right,
	RightTop,
	RightBottom,
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

namespace UI {
	void Init();
	void Begin(Arena* tempStorage, Input* input, Font* defaultFont, FrameBufferInfo* fbInfo);
	void End();

	UiStyle* GetStyle();

	void UseFont(Font* font);
	void DrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, glm::vec2 uvScale, glm::vec2 uvShift);
	// рассчитывает uv элемента в атласе
	void DrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, s32 tileIndex, glm::ivec2 tileSize);
	bool Button(const char* text, glm::vec2 size = { 0, 0 }, bool centerX = false);
	float GetButtonWidth(const char* text);
	bool CheckBox(const char* text, bool* value, glm::vec2 size = { 0,0 });
	bool SliderFloat(const char* text, float* value, float minValue, float maxValue, float width = 0);
	bool SliderInt(const char* text, int* value, int minValue, int maxValue, float width = 0);
	float GetTextWidth(const char* text);
	void Text(const char* text, bool centerX = false);

	void SetAnchor(uiAnchor anchor, float offset);
	void ShiftOrigin(float offsetX, float offsetY);
	void SetOrigin(float x, float y);
	void SetAdvanceMode(AdvanceMode mode);
	void SetMargin(bool enabled);
}
