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

layout(std430, binding = 6) buffer palette_buffer
{
    vec4 palette[];
};

layout(std430,binding = 7) buffer xform_counter_buffer
{
	uint xform_invoke_count[];
};

layout(std430, binding = 8) buffer bins_buffer
{
	vec4 bins[];
};

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

uniform bool random_read; // use the shuffle buffer to read
uniform bool random_write; // use the shuffle buffer to write
uniform bool first_run;

uniform bool do_draw;
uniform uvec2 bin_dims;

uniform vec2 win_min;
uniform float scale;

uniform bool make_xform_dist;
uniform uint shuf_buf_idx_in;
uniform uint shuf_buf_idx_out;
uniform float rand_seed;



const float PI = 3.141592653589793;
const float EPS = (1e-10);

vec2 sincos(float v) {
	return vec2(sin(v), cos(v));
}

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}



// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

vec3 randstate = vec3(0.0);

float randf() {
	randstate.z += 1.120491121512120124;
	randstate.z *= rand_seed;
    return random(randstate);
}

$varsource

uint get_in_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_in];
}

uint get_out_shuf_idx() {
	return shuf_buf[gl_GlobalInvocationID.x + gl_WorkGroupSize.x * gl_NumWorkGroups.x * shuf_buf_idx_out];
}

void main() {
	randstate = vec3(sin(gl_GlobalInvocationID.x), rand_seed, 1.241225239120831);

	uint i_idx = (random_read)? get_in_shuf_idx(): gl_GlobalInvocationID.x;
	uint o_idx = (random_write)? get_out_shuf_idx(): gl_GlobalInvocationID.x;

	vec4 part_state = pos_in[i_idx];
	vec4 result = dispatch(part_state.xyz);
	pos_out[o_idx] = vec4(result.xyz, 0.0);

	if(do_draw) {
		vec2 pos = (apply_xform_final(result.xy) - win_min) * scale;
		ivec2 coords = ivec2(floor(pos));

		if( coords.x >= 0 && coords.y >= 0 && coords.x < bin_dims.x && coords.y < bin_dims.y && result.w > 0 ) {
			bins[coords.y * bin_dims.x + coords.x] += vec4(palette[min(255, uint(ceil(result.z * 255.0)))].rgb, result.w);
		}
	}
}