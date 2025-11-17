#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Typedefs.h"
#include "Cubes.h"

#define rotateXYZ(matrix, x, y, z)\
matrix = glm::rotate(matrix, glm::radians(x), glm::vec3(1.0, 0.0, 0.0));\
matrix = glm::rotate(matrix, glm::radians(y), glm::vec3(0.0, 1.0, 0.0));\
matrix = glm::rotate(matrix, glm::radians(z), glm::vec3(0.0, 0.0, 1.0));

#define FRAMEBUFFER_MAX_TEXTURES 1

struct RendererStats {
	u32 drawCallsCount;
	u32 drawCallsInstancedCount;
	u32 trianglesRendered;
};

extern RendererStats g_RendererStats;

struct Plane {
	glm::vec3 normal;
	float d;
};

struct Frustum {
	union {
		struct {
			Plane
				topFace, bottomFace,
				rightFace, leftFace,
				farFace, nearFace;
		};
		Plane planes[6];
	};
};

Frustum FrustumCreate(
	glm::vec3 pos, glm::vec3 front, glm::vec3 up,
	float aspect, float fovY, float zNear, float zFar);

// true если сфера внутри frustum
float FrustumSphereIntersection(Frustum* frustum, glm::vec3 sphereCenter, float radius);

glm::mat4 GetTransform(glm::vec3 pos, glm::vec3 rot = glm::vec3(0), glm::vec3 scale = glm::vec3(1));

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

struct FrameBuffer {
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
	Vertex(float x, float y, float z, float u, float v, glm::vec3 normal);
};
#pragma pack(pop)

struct Triangle {
	union {
		int indices[3];
		struct { int a, b, c; };
	};

	Triangle();
	Triangle(int a, int b, int c);
};

struct Geometry {
	u32 vertexCount, triangleCount;
	u32 VAO, VBO, EBO;
};

typedef Geometry Sprite;

struct GeometryInstanced {
	u32 vertexCount, triangleCount, instanceCount;
	u32 VAO, VBO, instanceVBO, EBO;
};

enum BlockFace : u8 {
	faceXNeg = 0,
	faceXPos = 1,
	faceYNeg = 2,
	faceYPos = 3,
	faceZNeg = 4,
	faceZPos = 5,
	faceNone,
};

#pragma pack(push, 1)
struct BlockFaceInstance {
	u16 pos;
	TextureID textureID;
	BlockFace face;
	// первая и вторая координата грани куба
	// т.е. один из вариантов в зависимости от грани: XY/XZ/YZ
	u8 sizeA, sizeB;

	BlockFaceInstance(int pos, BlockFace face, TextureID textureID, u8 sizeA = 1, u8 sizeB = 1);
	BlockFaceInstance(int pos, BlockFace face, TextureID textureID, u8 sizeY, u8 sizeZ, u8 sizeX);
};
#pragma pack(pop)    

struct UV {
	glm::vec2 offset;
	glm::vec2 scale;
};

UV GetUVFromAtlas(Texture* atlas, s32 tileIndex, glm::ivec2 tileSize);

//
// Asset Management
//

enum class ShaderAssetID : u8 {
	CubeShader, CubeShadowShader,
	PolymeshShader, PolymeshShadowShader,
	FlatShader,
	SpriteShader,
	UIShader,
	ScreenShader,
	COUNT
};

enum class TextureAssetID : u8 {
	EnvTexture,
	UITexture,
	EntityTexture,
	TestTexture,
	COUNT
};
enum class MeshAssetID {
	EntityMesh,
	COUNT
};

// TODO: вместо структуры с union разные стркутуры с одним заголовком
struct Asset {
	const char* path;

	bool IsInitialized;
	u64 fileWriteTime;

	union {
		Texture texture;
		Shader shader;
		Geometry mesh;
	};
};

void InvalidateShaders();

//Asset* GetAsset(AssetID id);
Shader GetShader(ShaderAssetID id);
Geometry* GetMesh(MeshAssetID id);
Texture* GetTexture(TextureAssetID id);

//
// Renderer
//

namespace Renderer {
	typedef void* (*LoadProc)(const char* name);

	// возвращает true, если успешно
	bool Init(LoadProc);

	void Begin(Arena* tempStorage);
	
	void Clear(float r, float g, float b, float a = 1);

	void SetViewportDimensions(int width, int height, int x = 0, int y = 0);

	Shader CreateShader(const char* vertexSource, const char* fragmentSource);
	// oldShader можно использовать для рекомпиляции шейдера
	Shader CreateShaderFromFile(Arena* tempStorage, const char* fileName, Shader oldShader = 0);
	void DeleteShader(Shader shader);	
	void BindShader(Shader shader);
	void UnbindShader();

	s32 GetUniformLocation(Shader shader, const char* name);
	void setUniformMatrix4(Shader shader, const char* name, float* values, bool transpose = false);
	void setUniformInt(Shader shader, const char* name, int x);
	void setUniformInt2(Shader shader, const char* name, int x, int y);
	void setUniformFloat(Shader shader, const char* name, float x);
	void setUniformFloat2(Shader shader, const char* name, float x, float y);
	void setUniformFloat3(Shader shader, const char* name, float x, float y, float z);
	void setUniformFloat3(Shader shader, const char* name, const glm::vec3& v);
	void setUniformFloat4(Shader shader, const char* name, glm::vec4 v);

	int GetMaxAASamples();
	void CreateMSAAFrameBuffer(FrameBuffer* fb, u32 width, u32 height, int samplesCount);
	void CreateColorFrameBuffer(FrameBuffer* frameBuffer, u32 width, u32 height);
	void CreateDepthMapFrameBuffer(FrameBuffer* frameBuffer, u32 size);
	
	void ReleaseFrameBuffer(FrameBuffer* frameBuffer);

	void BindFrameBuffer(FrameBuffer* frameBuffer);
	void UnbindFrameBuffer();

	Texture CreateTexture(
		int width, int height, void* data,
		PixelFormat inputFormat = PixelFormat::RGBA,
		PixelFormat outputFormat = PixelFormat::RGBA,
		TextureWrapping wrapping = TextureWrapping::Repeat,
		TextureFiltering filtering = TextureFiltering::Nearest);
	Texture CreateTextureFromFile(
		const char* path,
		PixelFormat pixelFormat,
		TextureWrapping wrapping = TextureWrapping::Repeat,
		TextureFiltering filtering = TextureFiltering::Nearest
	);
	void DeleteTexture(Texture* texture);
	void BindTexture(Texture* texture, int textureSlot = 0);
	void UnbindTexture(int textureSlot = 0);

	Geometry CreateGeometry(Vertex* vertices, u32 verticesCount, Triangle* triangles, u32 triangleCount);
	Geometry CreateGeometryFromFile(Arena* tempStorage, const char* fileName);
	void DeleteGeometry(Geometry* geo);
	void DrawGeometry(Geometry* geo);
	void DrawGeometry(u32 VAO, u32 triangleCount);
	void DrawGeometryInstanced(u32 VAO, u32 elementsCount, u32 instancesCount);

	void SwitchDepthTest(bool enabled);
}