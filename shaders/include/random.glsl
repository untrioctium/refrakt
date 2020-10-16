layout(std430, binding = 11) buffer rand_states
{
	uvec4 rs[];
};

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

uvec4 local_random_state;

void load_random_state() {
    local_random_state = rs[gl_WorkGroupID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x];
}

void save_random_state() {
    rs[gl_WorkGroupID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x] = local_random_state;
}

#define rot32(x,k) (((x)<<(k))|((x)>>(32-(k))))
uint ranval() {
    uint e = local_random_state.x - rot32(local_random_state.y, 27);
    local_random_state.x = local_random_state.y ^ rot32(local_random_state.z, 17);
    local_random_state.y = local_random_state.z + local_random_state.w;
    local_random_state.z = local_random_state.w + e;
    local_random_state.w = e + local_random_state.x;
    return local_random_state.x;
}

float randf() {
    return clamp(float(ranval())/4294967295.0f, 0.0, 1.0);
}

double randd() {
    return clamp(float(ranval())/4294967295.0lf, 0.0, 1.0);
}

bool randbit() {
    return (ranval() & 1) == 0;
}