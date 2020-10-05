#version 430
in vec2 pos;

layout(location=0) out vec4 color;

uniform sampler2D tex;

void main() {
	color = texture(tex, pos);
}