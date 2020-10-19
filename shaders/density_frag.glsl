#version 460

out vec4 outColor;
flat in vec4 color;
flat in int radius;
flat in vec2 offset;
flat in float norm_factor;

void main()
{
   if(radius == 0) {
       outColor = color;
       return;
   }
   vec2 normed = 2.0 * (gl_PointCoord + offset) - 1.0;
   float distance = dot(normed, normed);
   if(distance > 1) discard;
   else outColor = color * (1 - distance) * norm_factor;
}