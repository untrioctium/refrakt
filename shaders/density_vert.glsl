#version 460

uniform int estimator_radius;
uniform int estimator_min;
uniform float estimator_curve;
uniform int row_width;
uniform int height;

uniform mat4 projection;

flat out int radius;
flat out vec4 color;
flat out vec2 center;

layout(std430, binding = 8) buffer bins_buffer
{
	vec4 bins[];
};

void main() {
	ivec2 pos = ivec2(gl_VertexID % row_width, gl_VertexID / row_width); 
	color = bins[pos.y * row_width + pos.x];

	if( color.w < 1.0 ) {
		gl_Position = vec4(0.0, 0.0, -100.0, 1.0);
		return;
	}

	radius = max(
		estimator_min, 
		min(
			estimator_radius, 
			int(
				estimator_radius 
				/ pow(color.a + 1.0, estimator_curve)
			)
		)
	);

	center = vec2(float(pos.x), float(pos.y));
	gl_Position = projection * vec4(center, 0.0, 1.0);
	gl_PointSize = float(radius) * 2.0 + 1.0;
	center = center;
}