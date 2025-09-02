#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec2 aTexCoord;
//layout (location = 3) in vec3 aNormal;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main()
{
	gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}

#type fragment
#version 330 core

void main()
{             
}