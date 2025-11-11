#type vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in int instancePackedOffset;
layout (location = 2) in int instanceFaceDirection;
layout (location = 3) in int instanceTextureID;
layout (location = 4) in int instanceSizeX;
layout (location = 5) in int instanceSizeZ;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

const int CHUNK_SX = 16, CHUNK_SZ = 16, CHUNK_SY = 48;

void main() {
    vec3 pos = aPos;

    // x-
    if (instanceFaceDirection == 0) {
        pos.x *= float(instanceSizeZ);
        pos.z *= float(instanceSizeX);

        pos.xz = pos.zx; // переворачиваем полигон (для верной работы FACE_CULL)
        pos.xy = pos.yx;
    }
    // x+
    else if (instanceFaceDirection == 1) {
        pos.x *= float(instanceSizeX);
        pos.z *= float(instanceSizeZ);

        pos.xy = pos.yx;
        pos.x++;
    }
    // y-
    else if (instanceFaceDirection == 2){
        pos.x *= float(instanceSizeX);
        pos.z *= float(instanceSizeZ);
    }
    // y+
    if (instanceFaceDirection == 3) {
        pos.x *= float(instanceSizeZ);
        pos.z *= float(instanceSizeX);

        pos.xz = pos.zx; 
        pos.y++;
    }
    // z-
    else if (instanceFaceDirection == 4) {
        pos.x *= float(instanceSizeZ);
        pos.z *= float(instanceSizeX);
        
        pos.xz = pos.zx;
        pos.zy = pos.yz;
    }
    // z+
    else if (instanceFaceDirection == 5) {
        pos.x *= float(instanceSizeX);
        pos.z *= float(instanceSizeZ);

        pos.zy = pos.yz;
        pos.z++;
    }
    
    // распаковываем offset
    vec3 offset = vec3(
	    instancePackedOffset % CHUNK_SX,
	    instancePackedOffset / (CHUNK_SX * CHUNK_SZ),
	    instancePackedOffset / CHUNK_SX % CHUNK_SZ
    );

    vec3 vertexPos = pos + offset;

	gl_Position =  lightSpaceMatrix * model * vec4(vertexPos, 1.0);
}

#type fragment
#version 330 core

void main()
{             
}