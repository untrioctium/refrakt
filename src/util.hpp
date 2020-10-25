#pragma once

#include <string>
#include <set>
#include <sstream>
#include <chrono>

std::string replace_macro(const std::string& str, const std::string& macro, const std::string& value);
std::set<std::string> find_macros(const std::string& str);
std::string read_file(const std::string& path);

template<typename T, std::size_t Size, typename Conv>
std::array<T, Size> parse_strings(const std::string&& in, Conv&& cv) {
    std::array<T, Size> ret{};
    std::istringstream ss{ in };

    for (int i = 0; i < Size && ss; i++) {
        std::string v;
        ss >> v;
        if (!v.size()) break;
        ret[i] = cv(v);
    }

    return ret;
}

template <std::size_t Samples>
class moving_average {
public:

    template<typename T>
    float add(T v) {
        val = (1.0 - 1.0 / float(Samples)) * val + 1.0 / float(Samples) * static_cast<float>(v);
        return val;
        /*head++;
        if (head == Samples) head = 0;
        samples_[head] = static_cast<float>(v);

        float accum = 0.0;
        for (auto& v : samples_) accum += v;
        return accum / float(Samples);*/
    }

private:
    std::size_t head = 0;
    float val = 0;
    std::array<float, Samples> samples_ = { 0.0 };
};

class timer {
public:
    using ms = std::chrono::milliseconds;
    using us = std::chrono::microseconds;
    using ns = std::chrono::nanoseconds;

    timer() {
        reset();
    }

    void reset() {
        mark_ = clock::now();
    }

    template<typename Resolution>
    auto time() {
        auto now = clock::now();
        return std::chrono::duration_cast<Resolution>(now - mark_).count();
    }

private:
    using clock = std::chrono::high_resolution_clock;
    decltype(clock::now()) mark_;
};

namespace jsf32 {

    using u4 = std::uint32_t;
    struct ctx { u4 a; u4 b; u4 c; u4 d; };

    #define rot32(x,k) (((x)<<(k))|((x)>>(32-(k))))
    inline u4 ranval(ctx& x) {
        u4 e = x.a - rot32(x.b, 27);
        x.a = x.b ^ rot32(x.c, 17);
        x.b = x.c + x.d;
        x.c = x.d + e;
        x.d = e + x.a;
        return x.d;
    }

    inline void warmup_ctx(ctx& x, u4 seed) {
        x.a = 0xf1ea5eed;
        x.b = x.c = x.d = seed;

        for (int i = 0; i < 20; i++) (void) ranval(x);
    }

}