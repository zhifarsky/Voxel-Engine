#type vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in int instancePackedOffset;
layout (location = 2) in int instanceFaceDirection;
layout (location = 3) in int instanceTextureID;

uniform mat4 viewProjection;
uniform mat4 lightSpaceMatrix;

uniform ivec2 atlasSize;
uniform ivec2 chunkPos;

out vec3 ourColor;
out vec3 ourNormal;
out vec2 ourUV;
out vec3 FragPos;
out vec4 FragPosLightSpace;

const vec2 uvs[4] = vec2[4](
    vec2(0.0,  0.0),
    vec2(0.0,  1.0),
    vec2(1.0,  1.0),
    vec2(1.0,  0.0)
);

void main() {
    int CHUNK_SX = 16;
    int CHUNK_SZ = 16;
    int CHUNK_SY = 24;
    vec3 pos = aPos;
    
    int texSize = 16;

    // y+
    if (instanceFaceDirection == 0) {
        pos.xz = pos.zx; // переворачиваем полигон (для верной работы FACE_CULL)
        pos.y++;
        ourNormal = vec3(0,1,0);
    }
    // x+
    else if (instanceFaceDirection == 2) {
        ourNormal = vec3(-1,0,0);
        pos.xz = pos.zx;
        pos.xy = pos.yx;
    }
    // x-
    else if (instanceFaceDirection == 3) {
        ourNormal = vec3(1,0,0);
        pos.xy = pos.yx;
        pos.x++;
    }
    // z+
    else if (instanceFaceDirection == 4) {
        ourNormal = vec3(0,0,-1);
        pos.xz = pos.zx;
        pos.zy = pos.yz;
    }
    // z-
    else if (instanceFaceDirection == 5) {
        ourNormal = vec3(0,0,1);
        pos.zy = pos.yz;
        pos.z++;
    }
    // y-
    else {
        ourNormal = vec3(0,-1,0);        
    }
    
    // распаковываем offset
    vec3 offset = vec3(
	    instancePackedOffset % CHUNK_SX,
	    instancePackedOffset / (CHUNK_SX * CHUNK_SZ),
	    instancePackedOffset / CHUNK_SX % CHUNK_SZ
    );

    // todo: вычислить uv из instanceTextureID и atlasSize
    //float u = uvs[gl_VertexID].x / float(atlasSize.x) + float((instanceTextureID * texSize) % atlasSize.x) / float(atlasSize.x);
    //float v = uvs[gl_VertexID].y / float(atlasSize.x) + float(instanceTextureID * texSize / atlasSize.x * texSize) / float(atlasSize.y);
    float uvSize = (float(atlasSize) / float(texSize));
    float u = uvs[gl_VertexID].x / uvSize + (1.0 / uvSize) * float(instanceTextureID); // TODO: доделать (сейчас максимум один ряд текстур)
    float v = uvs[gl_VertexID].y / uvSize;
    ourUV = vec2(u,v);

    vec3 vertexPos = pos + offset + vec3(chunkPos.x, 0, chunkPos.y);
    FragPos = vertexPos;

    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

	gl_Position = viewProjection * vec4(vertexPos, 1.0);
}

#type fragment
#version 330 core

out vec4 FragColor;

in vec3 ourColor;
in vec3 ourNormal;
in vec2 ourUV;
in vec3 FragPos;
in vec4 FragPosLightSpace;

uniform sampler2D texture1;
uniform sampler2D shadowMap;

uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 ambientColor;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    //float bias = 0.0001;
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0002);  

    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;  
    return shadow;
}  

float ShadowCalculationSmooth(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    //float bias = 0.005;
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0002);  
    
	int shadowBlur = 1;
	
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -shadowBlur; x <= shadowBlur; ++x)
    {
        for(int y = -shadowBlur; y <= shadowBlur; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= (shadowBlur * 2 + 1) * (shadowBlur * 2 + 1);
    return shadow;
}  

void main() {
	vec4 texColor = texture(texture1, ourUV);

	vec3 norm = normalize(ourNormal);
	vec3 lightDir = normalize(sunDir);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * sunColor;

    //float shadow = ShadowCalculation(FragPosLightSpace, norm, sunDir);
    float shadow = ShadowCalculationSmooth(FragPosLightSpace, norm, sunDir);

	vec3 result = (ambientColor + diffuse * (1.0 - shadow)) * vec3(texColor.r, texColor.g, texColor.b);
	FragColor = vec4(result, 1.0);
}