#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model;

void main()
{
	gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;
uniform vec2 UVScale;
uniform vec2 UVShift;
uniform vec4 color;
uniform float colorWeight;

void main()
{
	vec4 texColor = texture(texture1, TexCoord * UVScale + UVShift);
	FragColor = mix(texColor, color, colorWeight);
}