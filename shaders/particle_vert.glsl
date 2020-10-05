#version 460

uniform mat4 projection;

layout (location = 0) in vec4 val;
out float col_val;

void main()
{
    col_val = val.z;
    gl_Position = projection * vec4(val.xy, 0.0, 1.0);
}