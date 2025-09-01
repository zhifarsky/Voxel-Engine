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