#pragma once
#include <glm.hpp>
#include "Typedefs.h"

#define FRAMEBUFFER_MAX_TEXTURES 1

enum class PixelFormat {
	RGB, RGBA, Grayscale, DepthMap,
	COUNT
};

enum class TextureWrapping {
	Repeat, MirroredRepeat,
	ClampToEdge, ClampToBorder,
	COUNT
};

enum class TextureFiltering {
	Nearest, Linear,
	COUNT
};

struct Texture {
	u32 ID;
	PixelFormat pixelformat;
	u32 width, height;
};

typedef u32 Shader;

struct FrameBuffer_new {
	Texture textures[FRAMEBUFFER_MAX_TEXTURES];
	u32 ID, RBO;
};

#pragma pack(push, 1)
struct Vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;

	Vertex();
	Vertex(float x, float y, float z);
	Vertex(float x, float y, float z, float u, float v);
	Vertex(float x, float y, float z, float u, float v, glm::vec3 color, glm::vec3 normal);
};
#pragma pack(pop)

struct Triangle {
	union {
		int indices[3];
		int a, b, c;
	};

	Triangle();
	Triangle(int a, int b, int c);
};

struct Geometry {
	Vertex* vertices;
	Triangle* triangles;

	u32 vertexCapacity, vertexCount;
	u32 triangleCapacity, triangleCount;

	u32 VAO, VBO, EBO;
};

typedef Geometry Sprite;

struct GeometryInstanced {
	Vertex* vertices;
	Triangle* triangles;

	u32 vertexCapacity, vertexCount;
	u32 triangleCapacity, triangleCount;

	u32 instanceCount;

	u32 VAO, VBO, instanceVBO, EBO;
};


// в том же порядке, что и в атласе
enum TextureID : u16 {
	tidGround,
	tidStone,
	tidSun,
	tidMoon,
	tidSnow,
	tidIronOre,
	tidAir,
	tidCount
};

enum BlockFace : u8 {
	faceYPos = 0,
	faceYNeg = 1,
	faceXPos = 2,
	faceXNeg = 3,
	faceZPos = 4,
	faceZNeg = 5,
	faceNone,
};

#pragma pack(push, 1)
struct BlockFaceInstance {
	u16 pos;
	TextureID textureID;
	BlockFace face;
	u8 sizeX, sizeZ;

	BlockFaceInstance(int pos, BlockFace face, TextureID textureID, u8 sizeX = 1, u8 sizeZ = 1);
};
#pragma pack(pop)

namespace Renderer {
	typedef void* (*LoadProc)(const char* name);
	enum class MSAAFactor { NONE = 1, X2 = 2, X4 = 4, X8 = 8, X16 = 16 };


	//bool init(void* (*loadProc)(const char* name));
	// возвращает true, если успешно
	bool init(LoadProc);

	void clear(float r, float g, float b, float a = 1);

	void setViewportDimensions(int width, int height, int x = 0, int y = 0);

	Shader createShader(const char* vertexSource, const char* fragmentSource);
	Shader createShaderFromFile(const char* vertexShaderFilename, const char* fragmentShaderFilename);
	Shader createShaderFromFile(const char* fileName);
	void deleteShader(Shader shader);	
	void bindShader(Shader shader);
	void unbindShader();

	void setUniformMatrix4(Shader shader, const char* name, float* values, bool transpose = false);
	void setUniformInt(Shader shader, const char* name, int x);
	void setUniformInt2(Shader shader, const char* name, int x, int y);
	void setUniformFloat(Shader shader, const char* name, float x);
	void setUniformFloat2(Shader shader, const char* name, float x, float y);
	void setUniformFloat3(Shader shader, const char* name, float x, float y, float z);
	void setUniformFloat4(Shader shader, const char* name, float x, float y, float z, float w);
	void setUniformFloat4(Shader shader, const char* name, glm::vec4 v);

	void createMSAAFrameBuffer(FrameBuffer_new* frameBuffer, u32 width, u32 height, MSAAFactor samplesCount);
	void createColorFrameBuffer(FrameBuffer_new* frameBuffer, u32 width, u32 height);
	void createDepthMapFrameBuffer(FrameBuffer_new* frameBuffer, u32 size);
	
	void releaseFrameBuffer(FrameBuffer_new* frameBuffer);

	void bindFrameBuffer(FrameBuffer_new* frameBuffer);
	void unbindFrameBuffer();

	Texture createTexture(
		int width, int height, void* data,
		PixelFormat inputFormat = PixelFormat::RGBA,
		PixelFormat outputFormat = PixelFormat::RGBA,
		TextureWrapping wrapping = TextureWrapping::Repeat,
		TextureFiltering filtering = TextureFiltering::Nearest);
	Texture createTextureFromFile(
		const char* path,
		PixelFormat pixelFormat,
		TextureWrapping wrapping = TextureWrapping::Repeat,
		TextureFiltering filtering = TextureFiltering::Nearest
	);
	void deleteTexture(Texture* texture);
	void bindTexture(Texture* texture, int textureSlot = 0);
	void unbindTexture(int textureSlot = 0);

	Geometry createGeometry(Vertex* vertices, u32 verticesCount, Triangle* triangles, u32 triangleCount);
	Geometry createGeometryFromFile(const char* fileName);
	void deleteGeometry(Geometry* geo);
	void drawGeometry(Geometry* geo);
	void drawInstancedGeo(u32 VAO, u32 elementsCount, u32 instancesCount);

	void switchDepthTest(bool enabled);
}