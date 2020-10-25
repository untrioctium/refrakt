#pragma once

#include <array>
#include <optional>
#include <map>
#include <cstddef>
#include <vector>
#include <memory>

#include "buffer_objects.hpp"
#include "shaders.hpp"

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

    std::array<unsigned int, 2> size;
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
        if (final_xform) func(-1, final_xform.value());
    }

    static void set_num_particles(std::size_t new_size) {
        swap_buffer_.reset(new pos_buffer_t{ new_size });
    }

    enum class counters {
        drawn,
        NUM_COUNTERS
    };

private:

    using pos_buffer_t = storage_buffer<std::array<float, 4>>;
    using param_buffer_t = storage_buffer<float>;

    static std::size_t num_temporal_samples_;

    static std::unique_ptr<pos_buffer_t> swap_buffer_;
    //std::unique_ptr<pos_buffer_t> local_buffer_ = std::make_unique<pos_buffer_t>(swap_buffer_->size());

    std::unique_ptr<compute_shader> iterate_cs_;
    std::unique_ptr<compute_shader> animate_cs_;

    storage_buffer<unsigned int> counters_ = { static_cast<int>(flame::counters::NUM_COUNTERS) };


};