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

Font* font;

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

void Start(Input* currentInput, Font* defaultFont, FrameBufferInfo* fbInfo) {
	font = defaultFont;

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
	Renderer::bindShader(uiShader);
	Renderer::setUniformMatrix4(uiShader, "projection", glm::value_ptr(projection));
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
	font = newFont;
}

// NOTE: элементы отцентрованы при помощи translate. отстальные элементы не отцентрованы
void DrawElement(Texture* texture, glm::vec3 rot, glm::vec3 scale, glm::vec2 uvScale, glm::vec2 uvShift) {
	Renderer::bindTexture(texture);

	glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
	model = glm::translate(model, glm::vec3(originX, originY, 0));
	model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1.0, 0.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(rot.z+180), glm::vec3(0.0, 0.0, 1.0)); // +180 так как элемент отрисовывался вверх ногами
	model = glm::scale(model, scale);
	model = glm::translate(model, glm::vec3(-.5, -.5, 0)); // отцентровывание
	
	Renderer::setUniformMatrix4(uiShader, "model", value_ptr(model));
	Renderer::setUniformFloat2(uiShader, "UVScale", uvScale.x, uvScale.y);
	Renderer::setUniformFloat2(uiShader, "UVShift", uvShift.x, uvShift.y);
	Renderer::setUniformFloat(uiShader, "colorWeight", 0);

	Renderer::drawGeometry(&face);

	Renderer::unbindTexture();

	Advance(scale.x, scale.y);
}

bool Button(const char* text, glm::vec2 size) {
	float textWidth = GetTextWidth(text);
	
	if (size.x == 0)
		size.x = textWidth + uiStyle.padding * 2;
	if (size.y == 0)
		size.y = FontGetSize(font) + uiStyle.padding * 2;

	ElementState state = {0};
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

	glm::vec4 color = uiStyle.buttonColor;
	if (hovered && clicked)
		color = uiStyle.buttonActiveColor;
	else if (hovered) 
		color = uiStyle.buttonHoveredColor;

	{
		glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
		model = glm::translate(model, glm::vec3(originX, originY, 0.0f));
		model = glm::scale(model, glm::vec3(size, 1));

		Renderer::setUniformMatrix4(uiShader, "model", glm::value_ptr(model));
		Renderer::setUniformFloat4(uiShader, "color", color);
		Renderer::setUniformFloat(uiShader, "colorWeight", 1);

		Renderer::drawGeometry(&face);
	}

	TextInternal(text, originX - textWidth / 2 + size.x / 2, originY + size.y / 2);
	Advance(size.x, size.y);

	return res;
}

float GetButtonWidth(const char* text) {
	return GetTextWidth(text) + uiStyle.padding * 2;
}

bool CheckBox(const char* text, bool* value, glm::vec2 size)
{
	if (size.x * size.y == 0)
		size = glm::vec2(50, 50);

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

		Renderer::setUniformMatrix4(uiShader, "model", glm::value_ptr(model));
		Renderer::setUniformFloat4(uiShader, "color", color);
		Renderer::setUniformFloat(uiShader, "colorWeight", 1);

		Renderer::drawGeometry(&face);
	}

	TextInternal(text, originX + size.x + uiStyle.margin, originY + FontGetSize(font) / 2);

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

		Renderer::setUniformFloat(uiShader, "colorWeight", 1);

		Renderer::setUniformFloat4(uiShader, "color", uiStyle.sliderBarColor);
		Renderer::setUniformMatrix4(uiShader, "model", glm::value_ptr(barModel));
		Renderer::drawGeometry(&face);

		if (hovered && clicked)
			Renderer::setUniformFloat4(uiShader, "color", uiStyle.sliderKnobActiveColor);
		else if (hovered)
			Renderer::setUniformFloat4(uiShader, "color", uiStyle.sliderKnobHoveredColor);
		else
			Renderer::setUniformFloat4(uiShader, "color", uiStyle.sliderKnobColor);
		Renderer::setUniformMatrix4(uiShader, "model", glm::value_ptr(knobModel));
		Renderer::drawGeometry(&face);
	}
	Advance(barWidth, knobHeight + FontGetSize(font));
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
		CharData charData = FontGetCharData(font, text[i]);

		width += charData.xoff + charData.xadvance;

		if (i > 0) {
			width += FontGetKernInPixels(font, text[i - 1], text[i]);;
		}
	}

	return width;
}

float TextInternal(const char* text, float posX, float posY) {
	u32 len = strlen(text);
	
	Renderer::bindTexture(FontGetTextureAtlas(font));
	Renderer::setUniformFloat(uiShader, "colorWeight", 0);
	float x = posX;

	for (size_t i = 0; i < len; i++)
	{
		char c = text[i];
		CharData charData = FontGetCharData(font, c);

		float cHeight = charData.y1 - charData.y0;
		float cWidth = charData.x1 - charData.x0;
		float cAspect = cWidth / cHeight;
		float cHeightFactor = cHeight / FontGetSize(font);
		
		float coordX = x + charData.xoff;
		float coordY = posY - cHeight - charData.yoff;

		// kerning
		if (i > 0) {
			coordX += FontGetKernInPixels(font, text[i-1], text[i]);
		}

		glm::mat4 model = glm::mat4(1.0f); // единичная матрица (1 по диагонали)
		model = glm::translate(model, glm::vec3(coordX, coordY, 0));
		model = glm::scale(model, glm::vec3(
			cWidth, 
			cHeight,
			1));
		Renderer::setUniformMatrix4(uiShader, "model", glm::value_ptr(model));

		Renderer::setUniformFloat2(uiShader, "UVShift", 
			(float)charData.x0 / FontGetTextureAtlas(font)->width,
			(float)charData.y1 / FontGetTextureAtlas(font)->height);
		Renderer::setUniformFloat2(uiShader, "UVScale", 
			(float)(charData.x1 - charData.x0) / FontGetTextureAtlas(font)->width,
			-(float)(charData.y1 - charData.y0) / FontGetTextureAtlas(font)->height);

		Renderer::drawGeometry(&face);
		
		x += charData.xadvance;
	}
	return x - posX;
}

void Text(const char* text) {
	float width = TextInternal(text, originX, originY);
	Advance(width, FontGetSize(font));
}

}
