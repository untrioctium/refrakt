#version 430
out vec2 pos;

const vec2 verts[4] = vec2[](
	vec2(-1.0f, -1.0f),
	vec2(-1.0f, 1.0f),
	vec2(1.0f, -1.0f),
	vec2(1.0f, 1.0f)
);

void main() {
	pos = verts[gl_VertexID] * 0.5 + 0.5;
	gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
}