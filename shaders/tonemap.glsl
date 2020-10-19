#version 460

layout( local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 8) buffer bins_buffer
{
	vec4 bins[];
};

layout (rgba32f) uniform image2D image_in;
layout (rgba32f) uniform image2D image_out;

uniform float gamma = 4.0;
uniform float scale_constant = 4.0;
uniform float brightness = 100.0;
uniform float vibrancy = 1.0;

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	vec4 color = imageLoad(image_in, pos);
	//vec4 color = bins[pos.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + pos.x];

	//float factor = (color.a) * log(color.a + 0.00001);
	color *= .5 * brightness * log(1.0 + color.a * scale_constant) * 0.434294481903251827651128918916 / color.a;

	float inv_gamma = 1.0/gamma;
	float z= pow(color.a, inv_gamma);
	float gamma_factor = z / color.a;
	vec3 inv_gamma_p = vec3(inv_gamma);

	//color.r = clamp(mix(pow(color.r, inv_gamma), gamma_factor * color.r, vibrancy), 0.0, 1.0);
	//color.g = clamp(mix(pow(color.g, inv_gamma), gamma_factor * color.g, vibrancy), 0.0, 1.0);
	//color.b = clamp(mix(pow(color.b, inv_gamma), gamma_factor * color.b, vibrancy), 0.0, 1.0);
	color.rgb = clamp(mix(pow(color.rgb, inv_gamma_p), gamma_factor * color.rgb, vibrancy), 0.0, 1.0);
	color.a = clamp(color.a, 0.0, 1.0);

	imageStore(image_out, pos, color);
}