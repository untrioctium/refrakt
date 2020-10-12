#pragma once

#include <array>
#include <optional>
#include <map>
#include <cstddef>
#include <vector>

struct motion_info {
    float freq;
    std::string function;
    float amplitude;
};

struct flame_xform {
    using affine_t = std::array<float, 6>;

    affine_t affine;
    std::optional<affine_t> post;
    std::map<std::string, float> variations;
    std::map<std::string, float> var_param;

    float weight;
    float color;
    float color_speed;

    bool animate;
    float opacity;

    std::map<std::string, motion_info> motion;
};

struct flame {
    std::vector<flame_xform> xforms;
    std::array<std::array<float, 4>, 256> palette;

    std::array<float, 2> center;
    float scale;
    float rotate;

    std::optional<flame_xform> final_xform;

    int estimator_min;
    int estimator_radius;
    float estimator_curve;

    float gamma;
    float vibrancy;
    float brightness;

    template<typename Func> void for_each_xform(Func&& func) {
        int idx = 0;
        for (auto& x : xforms) {
            func(idx++, x);
        }
    }
};