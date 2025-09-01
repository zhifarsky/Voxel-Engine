#include <glad/glad.h>
#include <Windows.h>
#include <SOIL/SOIL.h>
#include "Renderer.h"
#include "Tools.h"

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
	static void readShaderProgramErrors(GLuint shaderProgram);
	
	bool init(LoadProc loadProc) {
		int res = gladLoadGLLoader((GLADloadproc)loadProc);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
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

	Shader createShader(const char* vertexSource, const char* fragmentSource) {
		GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vShader, 1, &vertexSource, NULL);
		glCompileShader(vShader);
		//readShaderProgramErrors(vShader);

		GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fShader, 1, &fragmentSource, NULL);
		glCompileShader(fShader);
		//readShaderProgramErrors(fShader);

		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vShader);
		glAttachShader(shaderProgram, fShader);
		glLinkProgram(shaderProgram);
		//readShaderProgramErrors(shaderProgram);

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

	FrameBuffer createDepthMapFrameBuffer(Texture* depthMap) {
		FrameBuffer fb;

		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap->ID, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return fb;
	}

	void bindFrameBuffer(FrameBuffer* frameBuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, *frameBuffer);
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
			forceChannels = SOIL_LOAD_RGBA; break;
		default:
			FatalError("Unknown texture type");
		}

		s32 width, height;
		u8* image = SOIL_load_image(path, &width, &height, 0, forceChannels);

		if (!image) {
			FatalError("Error on loading texture from disk");
		}

		Texture tex = Renderer::createTexture(width, height, image, pixelFormat, pixelFormat, wrapping, filtering);
		SOIL_free_image_data(image);
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

	void drawBlockMesh(BlockMesh* geo) {
		glBindVertexArray(geo->VAO);
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, geo->faceCount);
		glBindVertexArray(0);
	}

	void switchDepthTest(bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	static void readShaderProgramErrors(GLuint shaderProgram) {
		int  success;
		char infoLog[1024];
		glGetProgramiv(shaderProgram, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
			//std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;
			OutputDebugStringA("ERROR::SHADER_PROGRAM::COMPILATION_FAILED\n");
			OutputDebugStringA(infoLog);
			OutputDebugStringA("\n");
			FatalError(infoLog);
		}
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

Triangle::Triangle() {
}
Triangle::Triangle(int a, int b, int c) {
	indices[0] = a;
	indices[1] = b;
	indices[2] = c;
}

BlockFaceInstance::BlockFaceInstance(int pos, BlockFace face, TextureID textureID) {
	this->pos = pos;
	this->face = face;
	this->textureID = textureID;
}

BlockMesh::BlockMesh() {
	faces = 0;
	faceCount = faceSize = 0;
	VAO = VBO = 0;

}
