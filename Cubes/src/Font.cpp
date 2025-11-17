#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <algorithm>
#include "Font.h"
#include "Files.h"

struct Font {
	stbtt_fontinfo fontInfo;
	Texture atlas;
	CharData* charData;
	u8* fileBuffer;
	u32 firstChar, charsCount;
	float size; // pixel height
};

Font* loadFont(GameMemory* memory, const char* path, float fontSize) {
	Font* font = (Font*)calloc(1, sizeof(Font));

	u32 fileSize = 0;
	// не освобождаем fileBuffer, так как его использует stbtt
	font->fileBuffer = readEntireFile(&memory->permStorage, path, &fileSize, FileType::binary);

	u32 atlasWidth = 512, atlasHeight = 512;
	u8* tempBitmap = (u8*)memory->tempStorage.push(atlasWidth * atlasHeight);

	font->size = fontSize;
	font->firstChar = ' '; 
	char lastChar = '~';
	font->charsCount = lastChar - font->firstChar + 1;

	font->charData = (CharData*)memory->permStorage.push(font->charsCount * sizeof(stbtt_bakedchar));

	// render font to atlas
	stbtt_BakeFontBitmap(
		font->fileBuffer, 
		0, 
		font->size, 
		tempBitmap, 
		atlasWidth, atlasHeight, 
		font->firstChar, font->charsCount, 
		(stbtt_bakedchar*)font->charData);

	// get font info
	stbtt_InitFont(
		&font->fontInfo, 
		font->fileBuffer, 
		stbtt_GetFontOffsetForIndex(font->fileBuffer, 0));

	u8* rgbaBitmap = (u8*)memory->tempStorage.push(atlasWidth * atlasHeight * 4);
	for (int i = 0; i < atlasWidth * atlasHeight; i++) {
		rgbaBitmap[i * 4 + 0] = 255; // R
		rgbaBitmap[i * 4 + 1] = 255; // G
		rgbaBitmap[i * 4 + 2] = 255; // B
		rgbaBitmap[i * 4 + 3] = tempBitmap[i]; // A
	}
	
	font->atlas = Renderer::CreateTexture(
		atlasWidth, atlasHeight, rgbaBitmap,
		PixelFormat::RGBA, PixelFormat::RGBA,
		TextureWrapping::ClampToEdge, TextureFiltering::Linear
	);

	return font;
}

float FontGetSize(Font* font) {
	return font->size;
}

CharData FontGetCharData(Font* font, char c) {
	s32 i = c - font->firstChar;
	
	// если неподдерживаемый символ
	if (i < 0 && i > font->charsCount)
		i = '?' - font->firstChar;

	return font->charData[i];
}

float FontGetKernInPixels(Font* font, s32 codepointA, s32 codepointB) {
	int glyph1 = stbtt_FindGlyphIndex(&font->fontInfo, codepointA);
	int glyph2 = stbtt_FindGlyphIndex(&font->fontInfo, codepointB);

	int kern = stbtt_GetGlyphKernAdvance(&font->fontInfo, glyph1, glyph2);
	return (float)kern * stbtt_ScaleForPixelHeight(&font->fontInfo, font->size);
}

Texture* FontGetTextureAtlas(Font* font)
{
	return &font->atlas;
}
