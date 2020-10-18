#version 460

uniform mat4 projection;
uniform float ss_affine[6];

layout(std430, binding = 12) buffer particle_buffer_draw
{
	vec4 pos_draw[];
};

out vec2 col_val;

void main()
{
    vec4 val = pos_draw[gl_VertexID];
    col_val = val.zw;
    gl_Position = projection * vec4(val.xy, 0.0, 1.0);
}