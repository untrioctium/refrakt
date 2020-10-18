 #version 460

 out vec4 outColor;
 flat in vec4 color;
 flat in int radius;
 flat in vec2 center;

 void main()
 {
    if(radius == 0) {
        outColor = color;
        return;
    }
    float distance = length(gl_PointCoord * 2.0 - 1.0);
    if(distance > 1) discard;
    outColor = color * (1.0 - distance * distance) * 0.63661977236/float(radius * radius);
 }