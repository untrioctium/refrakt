 #version 460

layout(std430, binding = 6) buffer palette_buffer
{
    vec4 palette[];
};

layout(std430, binding = 10) buffer flame_counters
{
	uint flame_atomic_counters[];
};

 out vec4 outColor;
 in vec2 col_val;

 void main()
 {
    if(col_val.y > 0.0) {
        atomicAdd(flame_atomic_counters[0], 1);
        uint upper_col = min(256, uint(ceil(col_val.x * 255.0)));

        outColor = vec4(palette[min(256, uint(ceil(col_val.x * 255.0)))].rgb, col_val.y);
    } else discard;
 }