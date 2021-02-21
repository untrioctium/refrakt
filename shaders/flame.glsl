#version 460

layout(local_size_x = $block_width, local_size_y = 1, local_size_z = 1) in;

$include_buffers
$include_random
$include_math
#line 9

uniform bool random_read; // use the shuffle buffer to read
uniform bool random_write; // use the shuffle buffer to write
uniform int num_shuf_bufs = 1024;
uniform bool first_run;
uniform int total_params;

uniform bool do_draw;
uniform uvec2 bin_dims;

uniform uint shuf_buf_idx_in;
uniform uint shuf_buf_idx_out;

uniform float ss_affine[6];

shared int xid;
shared int shuf_in;
shared int shuf_out;
shared float fp[1024];

$varsource

uint get_in_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_in];
}

uint get_out_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_out];
}



void main() {
	load_random_state();

	uint per_thread = total_params / gl_WorkGroupSize.x + 1;

	for( int i = 0; i < per_thread; i++ ) {
		fp[gl_LocalInvocationID.x * per_thread + i] = fp_inflated[total_params * gl_WorkGroupID.y + (gl_LocalInvocationID.x * per_thread + i) % total_params];
	}
	barrier();

	if( gl_LocalInvocationID.x == 0 ) {
		xid = get_xform_id(randf());
	}

	uint i_idx = (random_read)? get_in_shuf_idx(): gl_GlobalInvocationID.x;
	uint o_idx = (random_write)? get_out_shuf_idx(): gl_GlobalInvocationID.x;

	// if we are on the first run, all particles pull from the first buffer
	// which contains the sample points but jitter it a bit
	uint offset = ((first_run)? 0: (gl_WorkGroupID.y * gl_WorkGroupSize.x * gl_NumWorkGroups.x));
	vec4 part_state = pos_in[offset + i_idx];
	if(first_run) 
	{
		part_state.xy += randf() * .1 * PI * 2.0 * sincos(sqrt(randf()));;
	}

	barrier();
	vec4 result = dispatch(part_state.xyz, xid);

	//if(badval(result.x) || badval(result.y)) result.xyw = vec3(vec2(randf(), randf()) * 2.0 - 1.0, 0.0);
	
	pos_out[gl_WorkGroupID.y * gl_WorkGroupSize.x * gl_NumWorkGroups.x + o_idx] = vec4(result.xyz, 0.0);

	if(do_draw) {
		$final_xform_call
		//result.xy = vec2(fma(ss_affine[0], result.x, fma(ss_affine[2], result.y, ss_affine[4])), fma(ss_affine[1], result.x, fma(ss_affine[3], result.y, ss_affine[5])));
		//pos_draw[gl_WorkGroupID.y * gl_WorkGroupSize.x * gl_NumWorkGroups.x + gl_GlobalInvocationID.x] = result;
		vec2 pos = vec2(fma(ss_affine[0], result.x, fma(ss_affine[2], result.y, ss_affine[4])), fma(ss_affine[1], result.x, fma(ss_affine[3], result.y, ss_affine[5])));
		ivec2 coords = ivec2(floor(pos));

		if( coords.x >= 0 && coords.y >= 0 && coords.x < bin_dims.x && coords.y < bin_dims.y && result.w > 0 ) {
			// 100% not thread safe in any manner
			// it just werks though? possibly because only one block is ever executing at a time
			bins[(bin_dims.y - coords.y - 1) * bin_dims.x + coords.x] += vec4(palette[min(255, uint(ceil(result.z * 255.0)))].rgb, result.w);
			atomicAdd(flame_atomic_counters[0], 1);
		}
	}

	save_random_state();
}