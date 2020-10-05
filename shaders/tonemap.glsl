#version 460

layout( local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba32f) uniform image2D image;

void main()
{
	ivec2 pos = ivec2(gl_WorkGroupID.xy);
	vec4 color = imageLoad(image, pos);
	float factor = log(color.a + 0.00001)/(color.a + 0.00001);
	color *= factor;
	color.r = pow(color.r, .25);
	color.g = pow(color.g, .25);
	color.b = pow(color.b, .25);
	color.a = 1.0;
	imageStore(image, pos, color);
}