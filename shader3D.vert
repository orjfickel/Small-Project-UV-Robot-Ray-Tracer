#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

uniform mat4 view;
uniform mat4 projection;

out vec4 f_color;

void main()
{
	gl_Position = projection * view * vec4(pos, 1.0f);
	f_color = vec4(col, 1.0f);
}