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

layout(std430, binding = 6) buffer palette_buffer
{
    vec4 palette[];
};

layout(std430, binding = 8) buffer bins_buffer
{
	vec4 bins[];
};

layout(std430, binding = 9) buffer fp_inflated_buffer
{
	float fp_inflated[];
};

layout(std430, binding = 10) buffer flame_counters
{
	uint flame_atomic_counters[];
};

