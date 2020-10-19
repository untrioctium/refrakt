#version 460

uniform int estimator_radius;
uniform int estimator_min;
uniform float estimator_curve;
uniform int row_width;
uniform int height;

uniform float gamma = 4.0;
uniform float scale_constant = 4.0;
uniform float brightness = 100.0;
uniform float vibrancy = 1.0;

uniform mat4 projection;

flat out int radius;
flat out vec4 color;
flat out vec2 offset;
flat out float norm_factor;

layout(std430, binding = 8) buffer bins_buffer
{
	vec4 bins[];
};

void main() {
	ivec2 pos = ivec2(gl_VertexID % row_width, gl_VertexID / row_width); 
	color = bins[pos.y * row_width + pos.x];

	if( color.a == 0.0 ) {
		gl_Position = vec4(0.0, 0.0, -100.0, 1.0);
		return;
	}

	radius = max(
		estimator_min, 
		min(
			estimator_radius, 
			int(
				estimator_radius 
				/ pow(color.a, estimator_curve)
			)
		)
	);

	gl_Position = projection * vec4(vec2(float(pos.x), float(pos.y)), 0.0, 1.0);
	gl_PointSize = float(radius) * 2.0 + 1.0;
	
	float half_width = 1.0/(4.0 * float(radius) + 2.0);
	offset = vec2(half_width, -half_width);

	/*color *= .5 * brightness * log(1.0 + color.a * scale_constant) * 0.434294481903251827651128918916 / color.a;

	float inv_gamma = 1.0/gamma;
	float z= pow(color.a, inv_gamma);
	float gamma_factor = z / color.a;
	vec3 inv_gamma_p = vec3(inv_gamma);

	color.rgb = clamp(mix(pow(color.rgb, inv_gamma_p), gamma_factor * color.rgb, vibrancy), 0.0, 1.0);
	color.a = clamp(color.a, 0.0, 1.0);*/
	norm_factor = 0.63661977236 / float(radius * radius);
}