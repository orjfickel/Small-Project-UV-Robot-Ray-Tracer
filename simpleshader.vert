#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tuv;
layout (location = 2) in vec3 col;

out vec2 uv;

uniform mat4 view;
uniform mat4 projection;

void main(){
	gl_Position = projection * view * vec4(pos, 1.0f);
	uv = vec2(tuv.x, tuv.y);
}