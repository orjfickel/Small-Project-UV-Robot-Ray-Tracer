#version 330 core

in vec2 uv;

out vec4 f;

uniform sampler2D tex;

void main(){
	f = texture(tex, uv);
}