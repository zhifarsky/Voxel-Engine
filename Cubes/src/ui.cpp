#include "ui.h"
#include <unordered_map>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Mesh.h"
#include "Files.h"
#include "Tools.h"

using namespace UI;

struct ElementState {
	bool wasActive;
};

std::unordered_map<std::string, ElementState> elementsState;

static Sprite face;

Font* g_Font;
Shader g_UIShader;

Input* input;
float originX = 0, originY = 0;
float left, right, bottom, top;
int displayW, displayH;
glm::mat4 projection;
AdvanceMode advanceMode;

UiStyle uiStyle;
bool marginEnabled;

// TODO: кэширование атласа шрифта

namespace UI {
	float TextInternal(const char* text, float posX, float posY);

void Init() {
	// TODO: запечь вершины в вершинный шейдер вместо создания их на CPU
	//static Vertex faceVerts[] = {
	//	Vertex(-0.5,-0.5,0, 0,0),
	//	Vertex(0.5, -0.5,0, 1,0),
	//	Vertex(0.5, 0.5,0, 1,1),
	//	Vertex(-0.5,0.5,0, 0,1),
	//};
	//static Vertex faceVerts[] = {
	//	Vertex(0,0,0, 0,0),
	//	Vertex(1,0,0, 1,0),
	//	Vertex(1,1,0, 1,1),
	//	Vertex(0,1,0, 0,1),
	//};	
	static Vertex faceVerts[] = {
		Vertex(0,0,0, 0,0),
		Vertex(1,0,0, 1,0),
		Vertex(1,1,0, 1,1),
		Vertex(0,1,0, 0,1),
	};
	static Triangle faceTris[] = {
		Triangle(0,1,2),
		Triangle(0,2,3)
	};

	face = Renderer::createGeometry(faceVerts, 4, faceTris, 2);

	// style
	{
		uiStyle.padding = 10;
		uiStyle.margin = 20; 

		uiStyle.buttonColor = { 0.5, 0.2, 0.2, 1 };
		uiStyle.buttonHoveredColor = { 0.7, 0.2, 0.2, 1 };
		uiStyle.buttonActiveColor = { 1, 0, 0, 1 };

		uiStyle.sliderBarSize = { 300, 10 };
		uiStyle.sliderBarColor = { 0.7, 0.7, 0.7, 1 };

		uiStyle.sliderKnobSize = { 20, 40 };
		uiStyle.sliderKnobColor = { 1, 1, 1, 0.5 };
		uiStyle.sliderKnobHoveredColor = { 1, 1, 1, 0.7 };
		uiStyle.sliderKnobActiveColor = { 1, 1, 1, 1 };
	}
}

UiStyle* GetStyle() {
	return &uiStyle;
}

void UI::Begin(Arena* tempStorage, Input* currentInput, Font* defaultFont, FrameBufferInfo* fbInfo) {
	g_Font = defaultFont;
	g_UIShader = GetShader(ShaderAssetID::UIShader);

	//window = currentWindow;
	input = currentInput;
	displayW = fbInfo->sizeX;
	displayH = fbInfo->sizeY;

	originX = originY = 0;

	advanceMode = AdvanceMode::Right;
	marginEnabled = true;

	projection = glm::mat4(1.0);
	right = displayW;
	top = displayH;
	left = 0;
	bottom = 0;
	projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);

	// use ui shader
	Renderer::bindShader(g_UIShader);
	Renderer::setUniformMatrix4(g_UIShader, "projection", glm::value_ptr(projection));
	Renderer::switchDepthTest(false);
}

void End() {
	Renderer::unbindShader();
	Renderer::switchDepthTest(true);
}

void SetAnchor(uiAnchor anchor, float offset) {
	switch (anchor) {
	case uiAnchor::Center:
		originX = displayW / 2.0f;
		originY = displayH / 2.0f;
		break;
	case uiAnchor::Left:
		originY = displayH / 2.0f;
		originX = left + offset;
		break;
	case uiAnchor::LeftTop:
		originY = top - offset;
		originX = left + offset;
		break;
	case uiAnchor::LeftBottom:
		originY = bottom + offset;
		originX = left + offset;
		break;
	case uiAnchor::Right:
		originY = displayH / 2.0f;
		originX = right - offset;
		break;
	case uiAnchor::RightTop:
		originY = top - offset;
		originX = right - offset;
		break;
	case uiAnchor::RightBottom:
		originY = bottom + offset;
		originX = right - offset;
		break;
	case uiAnchor::Top:
		originX = displayW / 2.0f;
		originY = top - offset;
		break;
	case uiAnchor::Bottom:
		originX = displayW / 2.0f;
		originY = bottom + offset;
		break;
	}
}

void SetOrigin(float x, float y) {
	originX = x;
	originY = y;
}

void ShiftOrigin(float offsetX, float offsetY) {
	originX += offsetX;
	originY += offsetY;
}

void SetAdvanceMode(AdvanceMode mode) {
	advanceMode = mode;
}

void SetMargin(bool enabled) {
	marginEnabled = enabled;
}

void Advance(float offsetX, float offsetY) {
	switch (advanceMode)
	{
	case AdvanceMode::Right:
		originX += offsetX + uiStyle.margin * marginEnabled;
		break;
	case AdvanceMode::Left:
		originX -= offsetX + uiStyle.margin * marginEnabled;
		break;
	case AdvanceMode::Up:
		originY += offsetY + uiStyle.margin * marginEnabled;
		break;
	case AdvanceMode::Down:
		originY -= offsetY + uiStyle.margin * marginEnabled;
		break;
	case AdvanceMode::None:
		break;
	}
}

void UseFont(Font* newFont)
{
	g_Font = newFont;
}

// NOTE: элементы отцентрованы при помощи translate. отстальные элементы не отцентрованы
void DrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, glm::vec2 uvScale, glm::vec2 uvShift) {
	Renderer::bindTexture(texture);

	glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
	model = glm::translate(model, glm::vec3(originX, originY, 0));
	model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1.0, 0.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0, 1.0, 0.0));
	model = glm::scale(model, scale);
	model = glm::translate(model, glm::vec3(-.5, -.5, 0)); // отцентровывание
	
	// т.к. текстура была перевернута
	uvShift.y += uvScale.y;
	uvScale.y = -uvScale.y;

	Renderer::setUniformMatrix4(g_UIShader, "model", value_ptr(model));
	Renderer::setUniformFloat2(g_UIShader, "UVScale", uvScale.x, uvScale.y);
	Renderer::setUniformFloat2(g_UIShader, "UVShift", uvShift.x, uvShift.y);
	Renderer::setUniformFloat(g_UIShader, "colorWeight", 0);

	Renderer::drawGeometry(&face);

	Renderer::unbindTexture();

	Advance(scale.x, scale.y);
}

void DrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, s32 tileIndex, glm::ivec2 tileSize) {
	UV uv = GetUVFromAtlas(texture, tileIndex, tileSize);
	DrawElement(texture, rot, scale, uv.scale, uv.offset);
}

bool Button(const char* text, glm::vec2 size, bool centerX) {
	float textWidth = GetTextWidth(text);
	
	if (size.x == 0)
		size.x = textWidth + uiStyle.padding * 2;
	if (size.y == 0)
		size.y = FontGetSize(g_Font) + uiStyle.padding * 2;

	glm::vec2 pos = { originX, originY };
	if (centerX)
		pos.x -= size.x / 2;

	ElementState state = {0};
	if (elementsState.count(text))
		state = elementsState[text];

	double px, py;
	GetCursorPos(&px, &py);
	py = displayH - py;
	bool clicked = ButtonClicked(input->uiClick);
	bool hovered = pointRectCollision(
		px, py,
		pos.x, pos.y, 
		pos.x + size.x, 
		pos.y + size.y);

	bool res = hovered && clicked && !state.wasActive;
	state.wasActive = hovered && clicked;
	elementsState[text] = state;

	glm::vec4 color = uiStyle.buttonColor;
	if (hovered && clicked)
		color = uiStyle.buttonActiveColor;
	else if (hovered) 
		color = uiStyle.buttonHoveredColor;

	{
		glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
		model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
		model = glm::scale(model, glm::vec3(size, 1));

		Renderer::setUniformMatrix4(g_UIShader, "model", glm::value_ptr(model));
		Renderer::setUniformFloat4(g_UIShader, "color", color);
		Renderer::setUniformFloat(g_UIShader, "colorWeight", 1);

		Renderer::drawGeometry(&face);
	}

	TextInternal(text, pos.x - textWidth / 2 + size.x / 2, pos.y + size.y / 2);
	Advance(size.x, size.y);

	return res;
}

float GetButtonWidth(const char* text) {
	return GetTextWidth(text) + uiStyle.padding * 2;
}

bool CheckBox(const char* text, bool* value, glm::vec2 size)
{
	if (size.x * size.y == 0)
		size = glm::vec2(FontGetSize(g_Font), FontGetSize(g_Font)) * 2.0f;

	ElementState state = { 0 };
	if (elementsState.count(text))
		state = elementsState[text];
	
	double px, py;
	GetCursorPos(&px, &py);
	py = displayH - py;
	bool clicked = ButtonClicked(input->uiClick);
	bool hovered = pointRectCollision(
		px, py,
		originX, originY,
		originX + size.x,
		originY + size.y);

	bool res = hovered && clicked && !state.wasActive;
	state.wasActive = hovered && clicked;
	elementsState[text] = state;

	if (res)
		*value = !*value;

	glm::vec4 color = uiStyle.buttonColor;
	if (*value)
		color = uiStyle.buttonActiveColor;
	else if (hovered)
		color = uiStyle.buttonHoveredColor;

	{
		glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
		model = glm::translate(model, glm::vec3(originX, originY, 0.0f));
		model = glm::scale(model, glm::vec3(size, 1));

		Renderer::setUniformMatrix4(g_UIShader, "model", glm::value_ptr(model));
		Renderer::setUniformFloat4(g_UIShader, "color", color);
		Renderer::setUniformFloat(g_UIShader, "colorWeight", 1);

		Renderer::drawGeometry(&face);
	}

	TextInternal(text, originX + size.x + uiStyle.margin, originY + FontGetSize(g_Font) / 2);

	Advance(size.x, size.y);

	return res;
}

bool SliderInternal(const char* name, const char* text, float* value, float minValue, float maxValue, float width) {
	
	float sliderRange = maxValue - minValue;
	float normalizedValue = (*value - minValue) / sliderRange;

	float barWidth = width == 0 ? uiStyle.sliderBarSize.x : width;
	float barHeight = uiStyle.sliderBarSize.y;
	float knobWidth = uiStyle.sliderKnobSize.x;
	float knobHeight = uiStyle.sliderKnobSize.y;
	glm::vec2 knobPos(originX + barWidth * normalizedValue, originY);

	double px, py;
	GetCursorPos(&px, &py);
	py = displayH - py;
	bool clicked = ButtonHeldDown(input->uiClick);
	bool hovered = pointRectCollision(
		px, py,
		originX, knobPos.y - knobHeight / 2,
		originX + barWidth, knobPos.y + knobHeight / 2
	);

	if (elementsState.count(name)) {
		if (elementsState[name].wasActive)
			hovered = true;
	}

	bool res = false;
	if (hovered && clicked) {
		res = true;
		float newNormalizedValue = glm::clamp((px - originX) / barWidth, 0.0, 1.0);
		float newValue = newNormalizedValue * sliderRange + minValue;
		*value = newValue;

		if (!elementsState.count(name)) {
			elementsState[name] = {};
		}
		elementsState[name].wasActive = true;
	}
	else {
		if (!elementsState.count(name)) {
			elementsState[name] = {};
		}
		elementsState[name].wasActive = false;
	}

	{
		TextInternal(text, originX, originY + knobHeight / 2 + uiStyle.padding);

		glm::mat4 barModel = glm::mat4(1.0f);
		barModel = glm::translate(barModel, glm::vec3(originX, originY - barHeight / 2, 0));
		barModel = glm::scale(barModel, glm::vec3(barWidth, barHeight, 1));

		glm::mat4 knobModel = glm::mat4(1.0f);
		knobModel = glm::translate(knobModel, glm::vec3(knobPos.x - knobWidth / 2, originY - knobHeight / 2, 0));
		knobModel = glm::scale(knobModel, glm::vec3(knobWidth, knobHeight, 1));

		Renderer::setUniformFloat(g_UIShader, "colorWeight", 1);

		Renderer::setUniformFloat4(g_UIShader, "color", uiStyle.sliderBarColor);
		Renderer::setUniformMatrix4(g_UIShader, "model", glm::value_ptr(barModel));
		Renderer::drawGeometry(&face);

		if (hovered && clicked)
			Renderer::setUniformFloat4(g_UIShader, "color", uiStyle.sliderKnobActiveColor);
		else if (hovered)
			Renderer::setUniformFloat4(g_UIShader, "color", uiStyle.sliderKnobHoveredColor);
		else
			Renderer::setUniformFloat4(g_UIShader, "color", uiStyle.sliderKnobColor);
		Renderer::setUniformMatrix4(g_UIShader, "model", glm::value_ptr(knobModel));
		Renderer::drawGeometry(&face);
	}
	Advance(barWidth, knobHeight + FontGetSize(g_Font));
	return res;
}

bool SliderFloat(const char* text, float* value, float minValue, float maxValue, float width) {
	char buf[128];
	sprintf(buf, "%s %.1f", text, *value);
	return SliderInternal(text, buf, value, minValue, maxValue, width);
}

bool SliderInt(const char* text, int* value, int minValue, int maxValue, float width) {
	char buf[128];
	sprintf(buf, "%s %d", text, *value);
	float v = *value;

	bool res = SliderInternal(text, buf, &v, minValue, maxValue, width);
	*value = v;
	return res;
}

float GetTextWidth(const char* text) {
	u32 len = strlen(text);
	
	float width = 0;

	for (size_t i = 0; i < len; i++)
	{
		CharData charData = FontGetCharData(g_Font, text[i]);

		width += charData.xoff + charData.xadvance;

		if (i > 0) {
			width += FontGetKernInPixels(g_Font, text[i - 1], text[i]);;
		}
	}

	return width;
}

float TextInternal(const char* text, float posX, float posY) {
	u32 len = strlen(text);
	
	Renderer::bindTexture(FontGetTextureAtlas(g_Font));
	Renderer::setUniformFloat(g_UIShader, "colorWeight", 0);
	float x = posX;

	for (size_t i = 0; i < len; i++)
	{
		char c = text[i];
		CharData charData = FontGetCharData(g_Font, c);

		float cHeight = charData.y1 - charData.y0;
		float cWidth = charData.x1 - charData.x0;
		float cAspect = cWidth / cHeight;
		float cHeightFactor = cHeight / FontGetSize(g_Font);
		
		float coordX = x + charData.xoff;
		float coordY = posY - cHeight - charData.yoff;

		// kerning
		if (i > 0) {
			coordX += FontGetKernInPixels(g_Font, text[i-1], text[i]);
		}

		glm::mat4 model(1); // единичная матрица (1 по диагонали)	
		model = glm::translate(model, glm::vec3((int)coordX, (int)coordY, 0));
		model = glm::scale(model, glm::vec3((int)cWidth, (int)cHeight, 1));
		Renderer::setUniformMatrix4(g_UIShader, "model", glm::value_ptr(model));

		Renderer::setUniformFloat2(g_UIShader, "UVShift",
			(float)charData.x0 / FontGetTextureAtlas(g_Font)->width,
			(float)charData.y1 / FontGetTextureAtlas(g_Font)->height);
		Renderer::setUniformFloat2(g_UIShader, "UVScale",
			(float)(charData.x1 - charData.x0) / FontGetTextureAtlas(g_Font)->width,
			-(float)(charData.y1 - charData.y0) / FontGetTextureAtlas(g_Font)->height);

		Renderer::drawGeometry(&face);
		
		x += charData.xadvance;
	}
	return x - posX;
}

void Text(const char* text, bool centerX) {
	glm::vec2 pos(originX, originY);
	if (centerX)
		pos.x -= GetTextWidth(text) / 2; 
	
	float width = TextInternal(text, pos.x, pos.y);
	Advance(width, FontGetSize(g_Font));
}

}
