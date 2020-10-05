#include <cstddef>
#include <vector>

constexpr uint32_t ReverseBits32(uint32_t n) {
    n = (n << 16) | (n >> 16);
    n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
    n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
    n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
    n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
    return n;
}

constexpr uint32_t flip_bits(uint32_t v, uint32_t radix) {
    return ReverseBits32(v) >> (32 - radix);
}

std::uint32_t next_pow2(std::uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

std::vector<float> make_sample_points(std::uint32_t count) {
    std::vector<float> ret{};
    ret.resize(count * std::size_t{ 2 });

    std::uint32_t max = (count % 2 == 0) ? count : next_pow2(count);
    float inv_max = 1.0f / max;

    unsigned int v = max;
    unsigned r = 0;

    while (v >>= 1) {
        r++;
    }
    for (std::uint32_t i = 0; i < count; i++) {
        ret[i * 2] = (i * inv_max) * 2.0 - 1.0;
        ret[i * 2 + 1] = (flip_bits(i, r) * inv_max) * 2.0 - 1.0;
    }

    return ret;
}
