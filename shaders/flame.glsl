#version 460

layout(std430, binding = 0) buffer particle_buffer_in
{
	vec4 pos_in[];
};

layout(std430, binding = 1) buffer particle_buffer_out
{
	vec4 pos_out[];
};

layout(std430, binding = 4) buffer shuffle_buffer
{
	uint shuf_buf[];
};

layout(std430,binding = 5) buffer parameter_buffer
{
	float fp[];
};

layout(std430,binding = 7) buffer xform_counter_buffer
{
	uint xform_invoke_count[];
};

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

uniform bool random_read; // use the shuffle buffer to read
uniform bool random_write; // use the shuffle buffer to write
uniform bool first_run;
uniform bool make_xform_dist;
uniform uint shuf_buf_idx_in;
uniform uint shuf_buf_idx_out;
uniform float rand_seed;

const float PI = 3.141592653589793;
const float EPS = 0.00000001;

vec2 sincos(float v) {
	return vec2(sin(v), cos(v));
}
vec2 randstate = vec2(0.0, 0.0);

float randf() {
	randstate.y += rand_seed;
    return fract(sin(dot(randstate.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

$varsource

uint get_in_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_in];
}

uint get_out_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_out];
}

void main() {
	randstate = vec2(gl_GlobalInvocationID.x/float(gl_WorkGroupSize.x * gl_NumWorkGroups.x), rand_seed);

	uint i_idx = (random_read)? get_in_shuf_idx(): gl_GlobalInvocationID.x;
	uint o_idx = (random_write)? get_out_shuf_idx(): gl_GlobalInvocationID.x;

	vec4 part_state = pos_in[i_idx];
	vec4 result = dispatch(part_state.xy);
	pos_out[o_idx] = vec4(result.xy, result.w * result.z + (1.0 - result.w) * ((first_run)? randf(): part_state.z), 0.0);
}