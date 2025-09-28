#type vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in int instancePackedOffset;
layout (location = 2) in int instanceFaceDirection;
layout (location = 3) in int instanceTextureID;
layout (location = 4) in int instanceSizeX;
layout (location = 5) in int instanceSizeZ;

uniform mat4 lightSpaceMatrix;

uniform ivec2 chunkPos;

const int CHUNK_SX = 16, CHUNK_SZ = 16, CHUNK_SY = 24;

void main() {
    vec3 pos = aPos;

    // y+
    if (instanceFaceDirection == 0) {
        pos.xz = pos.zx;
        pos.x *= float(instanceSizeX);
        pos.z *= float(instanceSizeZ);
        pos.y++;
    }
    // x+
    else if (instanceFaceDirection == 2) {
        pos.xz = pos.zx;
        pos.xy = pos.yx;
    }
    // x-
    else if (instanceFaceDirection == 3) {
        pos.xy = pos.yx;
        pos.x++;
    }
    // z+
    else if (instanceFaceDirection == 4) {
        pos.xz = pos.zx;
        pos.zy = pos.yz;
    }
    // z-
    else if (instanceFaceDirection == 5) {
        pos.zy = pos.yz;
        pos.z++;
    }
    // y-
    else {
    }
    
    // распаковываем offset
    vec3 offset = vec3(
	    instancePackedOffset % CHUNK_SX,
	    instancePackedOffset / (CHUNK_SX * CHUNK_SZ),
	    instancePackedOffset / CHUNK_SX % CHUNK_SZ
    );

    vec3 vertexPos = pos + offset + vec3(chunkPos.x, 0, chunkPos.y);

	gl_Position = lightSpaceMatrix * vec4(vertexPos, 1.0);
}

#type fragment
#version 330 core

void main()
{             
}