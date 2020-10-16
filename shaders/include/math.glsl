const float PI = 3.141592653589793;
const float EPS = (1e-10);

vec2 sincos(float v) {
	return vec2(sin(v), cos(v));
}

float mod2(float x, float y) {
	return x - y * trunc(x / y);
}

float log10(float x) {
	return log(x) * 0.434294481903251827651128918916;
}

#define badval(x) (((x)!=(x))||((x)>1e10)||((x)<-1e10))