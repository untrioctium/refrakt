 #version 460

layout(std430, binding = 6) buffer palette_buffer
{
    vec4 palette[];
};

 out vec4 outColor;
 in float col_val;

 void main()
 {
    uint upper_col = min(256, uint(ceil(col_val * 256.0)));

    outColor = vec4(palette[min(256, upper_col)].rgb, 1.0);
 }