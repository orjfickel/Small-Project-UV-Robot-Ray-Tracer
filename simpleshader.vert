#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

out vec3 worldPos;

uniform mat4 view;
uniform mat4 projection;

void main(){
	gl_Position = projection * view * vec4(pos, 1.0f);
	worldPos = vec3(view * vec4(pos,1.0f));
}