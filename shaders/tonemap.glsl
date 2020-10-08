#version 460

layout( local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba32f) uniform image2D image;

uniform float gamma = 4.0;
uniform float scale_constant = 1.0;
uniform float brightness = 100.0;
uniform float vibrancy = 1.0;

void main()
{
	ivec2 pos = ivec2(gl_WorkGroupID.xy);
	vec4 color = imageLoad(image, pos);

	//float factor = (color.a) * log(color.a + 0.00001);
	color *= .5 * brightness * log(1.0 + color.a * scale_constant) / color.a;

	float inv_gamma = 1.0/gamma;
	float z= pow(color.a, inv_gamma);
	float gamma_factor = z / color.a;

	color.r = clamp(mix(pow(color.r, inv_gamma), gamma_factor * color.r, vibrancy), 0.0, 1.0);
	color.g = clamp(mix(pow(color.g, inv_gamma), gamma_factor * color.g, vibrancy), 0.0, 1.0);
	color.b = clamp(mix(pow(color.b, inv_gamma), gamma_factor * color.b, vibrancy), 0.0, 1.0);
	color.a = clamp(color.a, 0.0, 1.0);

	imageStore(image, pos, color);
}