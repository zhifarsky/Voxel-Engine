#pragma once
#include <glm.hpp>
#include "Typedefs.h"

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

typedef u32 Shader;
typedef u32 FrameBuffer;

struct Texture {
	u32 ID;
	PixelFormat pixelformat;
	u32 width, height;
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

	BlockFaceInstance(int pos, BlockFace face, TextureID textureID);
};
#pragma pack(pop)

struct BlockMesh {
	BlockFaceInstance* faces;

	u32 faceCount;
	u32 faceSize;

	u32 VAO, VBO, instanceVBO, EBO;

	bool needUpdate;

	BlockMesh();
};


namespace Renderer {
	typedef void* (*LoadProc)(const char* name);

	//bool init(void* (*loadProc)(const char* name));
	// возвращает true, если успешно
	bool init(LoadProc);

	void clear(float r, float g, float b, float a = 1);

	void setViewportDimensions(int width, int height, int x = 0, int y = 0);

	Shader createShader(const char* vertexSource, const char* fragmentSource);
	Shader createShaderFromFile(const char* vertexShaderFilename, const char* fragmentShaderFilename);
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

	FrameBuffer createDepthMapFrameBuffer(Texture* depthMap);
	void bindFrameBuffer(FrameBuffer* frameBuffer);
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
	void deleteGeometry(Geometry* geo);
	void drawGeometry(Geometry* geo);
	void drawBlockMesh(BlockMesh* geo);

	void switchDepthTest(bool enabled);
}