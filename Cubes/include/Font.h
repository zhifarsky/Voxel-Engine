#pragma once
#include "Renderer.h"
#include "Cubes.h"

struct Font;
// NOTE: полностью повторяет stbtt_bakedchar
// если изменить структуру, придется преобразовывать в stbtt_backedchar при работе с stbtt
struct CharData {
	u16 x0, y0, x1, y1;
	float xoff, yoff, xadvance;
};

Font* loadFont(GameMemory* memory, const char* fontPath, float fontSize);
float FontGetSize(Font* font);
CharData FontGetCharData(Font* font, char c);
float FontGetKernInPixels(Font* font, s32 codepointA, s32 codepointB);
Texture* FontGetTextureAtlas(Font* font);