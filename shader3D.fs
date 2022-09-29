#version 330 core
out vec4 f;

in vec2 uv;

uniform sampler2D tex;

void main()
{
	f = texture(tex, uv);
}