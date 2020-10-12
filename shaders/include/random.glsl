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

#define rot32(x,k) (((x)<<(k))|((x)>>(32-(k))))
uint ranval() {
    uint off = gl_WorkGroupID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x + gl_GlobalInvocationID.x;
    uvec4 x = rs[off];

    uint e = x.x - rot32(x.y, 27);
    x.x = x.y ^ rot32(x.z, 17);
    x.y = x.z + x.w;
    x.z = x.w + e;
    x.w = e + x.x;
    rs[off] = x;
    return x.x;
}

float randf() {
    return floatConstruct(ranval());
}

bool randbit() {
    return (ranval() & 1) == 0;
}