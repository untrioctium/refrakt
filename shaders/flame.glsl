#version 460

$include_buffers
$include_random
$include_math

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

uniform bool random_read; // use the shuffle buffer to read
uniform bool random_write; // use the shuffle buffer to write
uniform bool first_run;
uniform int total_params;

uniform bool do_draw;
uniform uvec2 bin_dims;

uniform vec2 win_min;
uniform float scale;

uniform bool make_xform_dist;
uniform uint shuf_buf_idx_in;
uniform uint shuf_buf_idx_out;

$varsource

uint get_in_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_in];
}

uint get_out_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_out];
}

void main() {

	uint i_idx = (random_read)? get_in_shuf_idx(): gl_GlobalInvocationID.x;
	uint o_idx = (random_write)? get_out_shuf_idx(): gl_GlobalInvocationID.x;

	vec4 part_state = pos_in[i_idx];
	vec4 result = dispatch(part_state.xyz);
	pos_out[o_idx] = vec4(result.xyz, 0.0);

	if(do_draw) {
		vec2 pos = (result.xy - win_min) * scale;
		ivec2 coords = ivec2(floor(pos));

		if( coords.x >= 0 && coords.y >= 0 && coords.x < bin_dims.x && coords.y < bin_dims.y && result.w > 0 ) {
			// 100% not thread safe in any manner
			// it just werks though?
			bins[(bin_dims.y - coords.y) * bin_dims.x + coords.x] += vec4(palette[min(255, uint(ceil(result.z * 255.0)))].rgb, result.w);
			atomicAdd(xform_invoke_count[0], 1);
		}
	}
}