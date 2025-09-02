#type vertex
#version 330 core
// TDOO: объеденить flat и polymesh шейдеры?
// TDOO: оптимизировать https://youtu.be/vtqCWvmnGTs?t=2276
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
	ourNormal = aNormal;
	FragPos = vec3(model * vec4(aPos, 1.0));
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);

	FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

//uniform sampler2D texture1;
//uniform float col_mix;
uniform vec4 color;

void main()
{
	FragColor = color;
}