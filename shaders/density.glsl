#version 460

layout( local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba32f) uniform image2D in_hist;
layout (rgba32f) uniform image2D out_hist;

uniform int estimator_radius;
uniform int estimator_min;
uniform float estimator_curve;

void main() {
	ivec2 pos = ivec2(gl_WorkGroupID.xy);
	vec4 input_val = imageLoad(in_hist, pos);

	int radius = max(
		estimator_min, 
		min(
			estimator_radius, 
			int(
				estimator_radius 
				/ pow(input_val.a + 0.000001, estimator_curve)
			)
		)
	);

	if( radius == 0  ) {
		imageStore(out_hist, pos, input_val);
		return;
	}

	vec4 accum = vec4(0.0);
	float kernel_accum = 0.0;
	float norm = 0.63661977236 / (radius * radius);

	for( int y = -radius; y < radius; y++ ) {
		for( int x = -radius; x < radius; x++ ) {
			float sample_dist = length(vec2(x/float(radius),y/float(radius)));
			if(sample_dist > 1.0f) continue;

			ivec2 sample_pos = pos + ivec2(x, y);
			accum += (1 - sample_dist * sample_dist) * imageLoad(in_hist, sample_pos);
		}
	}

	imageStore(out_hist, pos, accum * norm);
}