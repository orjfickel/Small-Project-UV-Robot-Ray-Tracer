#version 330 core

in vec3 worldPos;
out vec4 f;

void main(){
	f = vec4(vec3(worldPos.z * 1.0f), 1.0f);
}