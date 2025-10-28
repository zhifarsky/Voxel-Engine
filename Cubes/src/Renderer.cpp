#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <Windows.h>
#include <stb_image.h>
#include "Renderer.h"
#include "Tools.h"
#include "Files.h"

const char* VertexShaderToken = "#type vertex";
const char* FragmentShaderToken = "#type fragment";

using namespace Renderer;


const GLenum PixelFormatTable[] = {
	GL_RGB, GL_RGBA, GL_RED, GL_DEPTH_COMPONENT
};

const GLenum TextureWrappingTable[] = {
	GL_REPEAT, GL_MIRRORED_REPEAT,
	GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
};

const GLenum TextureFilteringTable[] = {
	GL_NEAREST, GL_LINEAR,
};



namespace Renderer {
	bool init(LoadProc loadProc) {
		int res = gladLoadGLLoader((GLADloadproc)loadProc);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return res;
	}

	void clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void setViewportDimensions(int width, int height, int x, int y)
	{
		glViewport(x, y, width, height);
	}

	bool checkShaderCompileErrors(Shader shader) {
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
			char* buf = (char*)malloc(maxLength * sizeof(GLchar));
			glGetShaderInfoLog(shader, maxLength, &maxLength, buf);
			dbgprint("[SHADER ERROR] %s\n", buf);
			//DEBUG_BREAK;
			free(buf);
			return false;
		}
		return true;
	}
	bool checkShaderLinkErrors(Shader shader) {
		GLint isLInked = 0;
		glGetProgramiv(shader, GL_LINK_STATUS, &isLInked);
		if (isLInked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
			char* buf = (char*)malloc(maxLength * sizeof(GLchar));
			glGetProgramInfoLog(shader, maxLength, &maxLength, buf);
			dbgprint("[SHADER ERROR] %s\n", buf);
			//DEBUG_BREAK;
			free(buf);
			return false;
		}
		return true;
	}

	Shader createShader(const char* vertexSource, const char* fragmentSource) {
		GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vShader, 1, &vertexSource, NULL);
		glCompileShader(vShader);
		if (!checkShaderCompileErrors(vShader)) {
			glDeleteShader(vShader);
			return 0; // TODO: возвращать дебаг-шейдер
		}

		GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fShader, 1, &fragmentSource, NULL);
		glCompileShader(fShader);
		if (!checkShaderCompileErrors(fShader)) {
			glDeleteShader(vShader);
			glDeleteShader(fShader);
			return 0; // TODO: возвращать дебаг-шейдер
		}

		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vShader);
		glAttachShader(shaderProgram, fShader);
		glLinkProgram(shaderProgram);
		if (!checkShaderLinkErrors(shaderProgram)) {
			glDeleteShader(vShader);
			glDeleteShader(fShader);
			glDeleteProgram(shaderProgram);
			return 0; // TODO: возвращать дебаг-шейдер
		}

		glDeleteShader(vShader);
		glDeleteShader(fShader);

		return shaderProgram;
	}

	Shader createShaderFromFile(const char* vertexShaderFilename, const char* fragmentShaderFilename) {
		u32 vFileSize = 0, fFileSize = 0;
		char* vertexSource = (char*)readEntireFile(vertexShaderFilename, &vFileSize, FileType::text);
		char* fragmentSource = (char*)readEntireFile(fragmentShaderFilename, &fFileSize, FileType::text);
		if (vertexSource == 0 || fragmentSource == 0) {
			FatalError("Error on loading shaders from disk");
		}

		Shader shaderProgram = Renderer::createShader(vertexSource, fragmentSource);

		free(vertexSource);
		free(fragmentSource);

		return shaderProgram;
	}

	Shader Renderer::createShaderFromFile(const char* fileName)
	{
		u32 vFileSize = 0, fFileSize = 0;
		char* source = (char*)readEntireFile(fileName, &vFileSize, FileType::text);
		if (!source) {
			FatalError("Error on loading shaders from disk");
		}

		char* vertexSource = strstr(source, VertexShaderToken);
		char* fragmentSource = strstr(source, FragmentShaderToken);

		if (!vertexSource || !fragmentSource)
			FatalError("Unknown shader format");

		char* split = std::max(vertexSource, fragmentSource) - 2;
		*split = '\0';

		vertexSource += strlen(VertexShaderToken);
		fragmentSource += strlen(FragmentShaderToken);

		Shader shader = Renderer::createShader(vertexSource, fragmentSource);

		free(source);

		return shader;
	}

	void deleteShader(Shader shader) {
		glDeleteProgram(shader);
	}

	void bindShader(Shader shader) {
		glUseProgram(shader);
	}

	void unbindShader() {
		glUseProgram(0);
	}

	// TODO: сделать setUniform макросами?

	void setUniformMatrix4(Shader shader, const char* name, float* values, bool transpose) {
		glUniformMatrix4fv(glGetUniformLocation(shader, name), 1, transpose, values);
	}

	void setUniformInt(Shader shader, const char* name, int x) {
		glUniform1i(glGetUniformLocation(shader, name), x);
	}

	void setUniformInt2(Shader shader, const char* name, int x, int y) {
		glUniform2i(glGetUniformLocation(shader, name), x, y);
	}

	void setUniformFloat(Shader shader, const char* name, float x) {
		glUniform1f(glGetUniformLocation(shader, name), x);
	}

	void setUniformFloat2(Shader shader, const char* name, float x, float y) {
		glUniform2f(glGetUniformLocation(shader, name), x, y);
	}

	void setUniformFloat3(Shader shader, const char* name, float x, float y, float z) {
		glUniform3f(glGetUniformLocation(shader, name), x, y, z);
	}

	void setUniformFloat4(Shader shader, const char* name, float x, float y, float z, float w) {
		glUniform4f(glGetUniformLocation(shader, name), x, y, z, w);
	}
	void setUniformFloat4(Shader shader, const char* name, glm::vec4 v) {
		glUniform4f(glGetUniformLocation(shader, name), v.x, v.y, v.z, v.w);
	}

	void createMSAAFrameBuffer(FrameBuffer* fb, u32 width, u32 height, MSAAFactor samplesCount) {
		*fb = {0};
		Texture* texture = &fb->textures[0];

		u32 samples = (u32)samplesCount;
		
		glGenFramebuffers(1, &fb->ID);
		glBindFramebuffer(GL_FRAMEBUFFER, fb->ID);

		// текстура для RGB буфера
		glGenTextures(1, &fb->textures[0].ID);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture->ID);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture->ID, 0);

		// RBO для Detph и Stencil буферов
		glGenRenderbuffers(1, &fb->RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, fb->RBO);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->RBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			FatalError("Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void createColorFrameBuffer(FrameBuffer* fb, u32 width, u32 height) {
		*fb = { 0 };
		Texture* texture = &fb->textures[0];

		glGenFramebuffers(1, &fb->ID);
		glBindFramebuffer(GL_FRAMEBUFFER, fb->ID);

		// текстура для RGB буфера
		glGenTextures(1, &texture->ID);
		glBindTexture(GL_TEXTURE_2D, texture->ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->ID, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			FatalError("Framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void createDepthMapFrameBuffer(FrameBuffer* fb, u32 size) {
		*fb = { 0 };
		
		Texture* depthMap = &fb->textures[0];
		*depthMap = Renderer::createTexture(
			size, size, NULL,
			PixelFormat::DepthMap, PixelFormat::DepthMap,
			TextureWrapping::ClampToBorder, TextureFiltering::Nearest);

		glGenFramebuffers(1, &fb->ID);
		glBindFramebuffer(GL_FRAMEBUFFER, fb->ID);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap->ID, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void releaseFrameBuffer(FrameBuffer* fb) {
		for (size_t i = 0; i < FRAMEBUFFER_MAX_TEXTURES; i++)
		{
			if (fb->textures[i].ID)
				glDeleteTextures(1, &fb->textures[i].ID);
		}

		if (fb->RBO)
			glDeleteRenderbuffers(1, &fb->RBO);

		if (fb->ID)
			glDeleteFramebuffers(1, &fb->ID);

		*fb = { 0 };
	}

	void bindFrameBuffer(FrameBuffer* fb) {
		glBindFramebuffer(GL_FRAMEBUFFER, fb->ID);
	}

	void unbindFrameBuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	Texture createTexture(
		int width, int height, void* data,
		PixelFormat inputFormat, PixelFormat outputFormat,
		TextureWrapping wrapping, TextureFiltering filtering)
	{
		Texture texture;
		texture.width = width;
		texture.height = height;
		texture.pixelformat = outputFormat;
		
		glGenTextures(1, &texture.ID);
		glBindTexture(GL_TEXTURE_2D, texture.ID);
		
		GLenum type = GL_UNSIGNED_BYTE;

		// if shadow map
		if (outputFormat == PixelFormat::DepthMap) {
			type = GL_FLOAT;
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		}

		glTexImage2D(GL_TEXTURE_2D, 0,
			PixelFormatTable[(int)inputFormat],
			texture.width, texture.height, 0,
			PixelFormatTable[(int)outputFormat], type, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// texture filtering
		GLenum glFiltering = TextureFilteringTable[(int)filtering];
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFiltering);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFiltering);

		// texture wrapping
		GLenum glWrapping = TextureWrappingTable[(int)wrapping];
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapping);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapping);

		return texture;
	}

	Texture createTextureFromFile(const char* path, PixelFormat pixelFormat, TextureWrapping wrapping, TextureFiltering filtering) {
		int forceChannels;
		switch (pixelFormat) {
		case PixelFormat::RGBA:
			forceChannels = 4; 
			break;
		default:
			FatalError("Unknown texture type");
		}

		s32 width, height, channels;
		u8* image = stbi_load(path, &width, &height, &channels, forceChannels);
		if (!image) {
			FatalError("Error on loading texture from disk");
		}

		Texture tex = Renderer::createTexture(width, height, image, pixelFormat, pixelFormat, wrapping, filtering);
		stbi_image_free(image);
		return tex;
	}

	void deleteTexture(Texture* texture) {
		glDeleteTextures(1, &texture->ID);
	}

	void bindTexture(Texture* texture, int textureSlot) {
		glActiveTexture(GL_TEXTURE0 + textureSlot);
		glBindTexture(GL_TEXTURE_2D, texture->ID);
		glActiveTexture(GL_TEXTURE0);
	}

	void unbindTexture(int textureSlot) {
		glActiveTexture(GL_TEXTURE0 + textureSlot);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
	}

	Geometry createGeometry(Vertex* vertices, u32 verticesCount, Triangle* triangles, u32 triangleCount) {
		Geometry geo;
		geo.vertexCapacity = geo.vertexCount = verticesCount;
		geo.triangleCapacity = geo.triangleCount = triangleCount;
		geo.vertices = vertices;
		geo.triangles = triangles;

		glGenVertexArrays(1, &geo.VAO);
		glGenBuffers(1, &geo.VBO);
		glGenBuffers(1, &geo.EBO);

		glBindVertexArray(geo.VAO);
		glBindBuffer(GL_ARRAY_BUFFER, geo.VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.EBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * verticesCount, vertices, GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * triangleCount, triangles, GL_STATIC_DRAW);

		GLint stride = sizeof(Vertex);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos)); // pos
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, uv)); // tex coord
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal)); // normal

		glBindVertexArray(0);

		return geo;
	}

	/* работает только с .obj форматом
	работает только с триангулированными мешами 
	TODO: загрузка нормалей */
	Geometry createGeometryFromFile(const char* fileName) {
		struct Face {
			int vertex[4];
			int uv[4];
			int normal[4];
			int vertexCount;
		};
		
		FILE* file = fopen(fileName, "rt");
		if (!file)
			FatalError("Error on opening file");
		
		int vCount = 0, uvCount = 0, faceCount = 0, normalCount = 0;

		char line[256];
		while (fgets(line, 256, file)) {
			if (line[0] == 'v') {
				if (line[1] == ' ')
					vCount++;
				else if (line[1] == 't')
					uvCount++;
				else if (line[1] == 'n')
					normalCount++;
			}
			else if (line[0] == 'f') {
				faceCount++;
			}
		}

		glm::vec3* positions = (glm::vec3*)malloc(sizeof(glm::vec3) * vCount);
		glm::vec2* uvs = (glm::vec2*)malloc(sizeof(glm::vec2) * uvCount);
		glm::vec3* normals = (glm::vec3*)malloc(sizeof(glm::vec3) * normalCount);
		Face* faces = (Face*)malloc(sizeof(Face) * faceCount);

		rewind(file);

		int vIndex = 0, vtIndex = 0, fIndex = 0, nIndex = 0;

		while (fgets(line, 256, file)) {
			//strncmp(line, "v ", 2);
			if (line[0] == 'v') {
				if (line[1] == ' ') {
					sscanf(line, "v %f %f %f", &positions[vIndex].x, &positions[vIndex].y, &positions[vIndex].z);
					vIndex++;
				}
				else if (line[1] == 't') {
					sscanf(line, "vt %f %f", &uvs[vtIndex].x, &uvs[vtIndex].y);
					vtIndex++;
				}
				else if (line[1] == 'n') {
					sscanf(line, "vn %f %f %f", &normals[nIndex].x, &normals[nIndex].y, &normals[nIndex].z);
					nIndex++;
				}
			}
			// TODO: вариант парсинга, если есть нормали
			else if (line[0] == 'f') {
				int v, vt, n;
				int elements = sscanf(line, "f %d/%d/%d", &v, &vt, &n);
				if (elements == 2) {
					sscanf(line, "f %d/%d %d/%d %d/%d",
						&faces[fIndex].vertex[0], &faces[fIndex].uv[0],
						&faces[fIndex].vertex[1], &faces[fIndex].uv[1],
						&faces[fIndex].vertex[2], &faces[fIndex].uv[2]);
				}
				else if (elements == 3) {
					sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
						&faces[fIndex].vertex[0], &faces[fIndex].uv[0], &faces[fIndex].normal[0],
						&faces[fIndex].vertex[1], &faces[fIndex].uv[1], &faces[fIndex].normal[1],
						&faces[fIndex].vertex[2], &faces[fIndex].uv[2], &faces[fIndex].normal[2]);
				}
				fIndex++;
			}
		}

		// TODO: оптимизировать (создает по 3 вершины на полигон, вместо использования index-буфера)
		vIndex = 0;
		Vertex* vertices = (Vertex*)calloc(sizeof(Vertex), faceCount * 3);
		Triangle* tris = (Triangle*)calloc(sizeof(Triangle), faceCount);
		for (int i = 0; i < faceCount; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				vertices[vIndex].pos = positions[faces[i].vertex[j] - 1];
				vertices[vIndex].uv = uvs[faces[i].uv[j] - 1];
				vertices[vIndex].normal = normals[faces[i].normal[j] - 1];
				tris[i].indices[j] = vIndex;
				vIndex++;
			}
		}
		
		free(uvs);
		free(faces);
		free(positions);
		free(normals);
		fclose(file);

		return createGeometry(vertices, faceCount * 3, tris, faceCount);;
	}

	// удаляет только ресурсы с GPU
	void deleteGeometry(Geometry* geo) {
		glDeleteBuffers(1, &geo->VBO);
		glDeleteBuffers(1, &geo->EBO);
		glDeleteVertexArrays(1, &geo->VAO);
	}

	//void bindGeometry(Geometry* geo) {
	//	glBindVertexArray(geo->VAO);
	//}

	//void unbindGeometry() {
	//	glBindVertexArray(0);
	//}

	void drawGeometry(Geometry* geo) {
		glBindVertexArray(geo->VAO);
		glDrawElements(GL_TRIANGLES, geo->triangleCount * 3, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void drawInstancedGeo(u32 VAO, u32 elementsCount, u32 instancesCount) {
		glBindVertexArray(VAO);
		glDrawElementsInstanced(GL_TRIANGLES, elementsCount, GL_UNSIGNED_INT, 0, instancesCount);
		glBindVertexArray(0);
	}

	void switchDepthTest(bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}
}

Vertex::Vertex() {}
Vertex::Vertex(float x, float y, float z) {
	pos.x = x;
	pos.y = y;
	pos.z = z;
	uv = { 0,0 };
}
Vertex::Vertex(float x, float y, float z, float u, float v) {
	pos.x = x;
	pos.y = y;
	pos.z = z;
	uv.x = u;
	uv.y = v;
}
Vertex::Vertex(float x, float y, float z, float u, float v, glm::vec3 color, glm::vec3 normal) {
	pos.x = x;
	pos.y = y;
	pos.z = z;
	uv.x = u;
	uv.y = v;
	this->normal = normal;
}

Triangle::Triangle() { }
Triangle::Triangle(int a, int b, int c) {
	indices[0] = a;
	indices[1] = b;
	indices[2] = c;
}

BlockFaceInstance::BlockFaceInstance(int pos, BlockFace face, TextureID textureID, u8 sizeA, u8 sizeB) {
	this->pos = pos;
	this->face = face;
	this->textureID = textureID;
	this->sizeA = sizeA;
	this->sizeB = sizeB;
}

BlockFaceInstance::BlockFaceInstance(int pos, BlockFace face, TextureID textureID, u8 sizeY, u8 sizeZ, u8 sizeX) {
	this->pos = pos;
	this->face = face;
	this->textureID = textureID;

	switch (face)
	{
	case faceXNeg:
	case faceXPos:
		sizeA = sizeY; sizeB = sizeZ; break;
		break;
	case faceYNeg:
	case faceYPos:
		sizeA = sizeX; sizeB = sizeZ; break;
		break;
	case faceZNeg:
	case faceZPos:
		sizeA = sizeX; sizeB = sizeY; break;
		break;
	default:
		sizeA = 0; sizeB = 0; break;
	}
}