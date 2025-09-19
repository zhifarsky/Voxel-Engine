#type vertex
#version 330 core
// TDOO: �������������� https://youtu.be/vtqCWvmnGTs?t=2276
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;


out vec2 TexCoord;
out vec3 ourNormal;
out vec3 FragPos;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	ourNormal = mat3(transpose(inverse(model))) * aNormal;
	FragPos = vec3(model * vec4(aPos, 1.0));
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);

	FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 ourNormal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

// texture sampler
uniform sampler2D texture1;
uniform sampler2D shadowMap;

uniform float lightingFactor;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 ambientColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
	float bias = 0.005;
	// check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{
	// vec3 norm = normalize(ourNormal);
	vec3 norm = ourNormal;
	vec3 lightDir = normalize(sunDir);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * sunColor;

	vec4 texColor = texture(texture1, TexCoord);
	float shadow = ShadowCalculation(FragPosLightSpace);
	vec3 result = (ambientColor + diffuse * (1.0 - shadow)) * vec3(texColor.r, texColor.g, texColor.b);
	FragColor = vec4(result, 1.0);
}