#pragma once

#include <array>
#include <optional>
#include <map>
#include <cstddef>
#include <vector>
#include <memory>
#include <set>
#include <nlohmann/json.hpp>

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

    float rotation_frequency;
    float opacity;

    std::map<std::string, motion_info> motion;
};

class flame_compiler;

struct flame {
    using bin_t = storage_buffer<std::array<float, 4>>;

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

    /*static void set_num_particles(std::size_t new_size) {
        swap_buffer_.reset(new pos_buffer_t{ new_size });
    }

    static void set_num_temporal_samples(std::size_t new_ts);
    static void set_num_shuffle_buffers(std::size_t num_sb);*/

    static void set_sim_parameters(std::size_t total_particles, std::size_t temporal_samples, std::size_t shuffle_count);

    enum class counters {
        drawn,
        NUM_COUNTERS
    };

    static std::unique_ptr<flame> load_flame(const std::string& path, const flame_compiler& vt);

    bool needs_warmup() { return needs_update || !local_buffer_ || !inflated_buffer_; }

    void warmup(std::size_t num_passes, float tss_width);
    std::size_t draw_to_bins(bin_t& bins, std::size_t bins_width, int num_iter);

    ~flame() {
        active_flames_.erase(this);
    }

    void reset_animation();

    static flame_xform::affine_t rotate_affine(const flame_xform::affine_t& a, float deg) {
        float rad = 0.01745329251f * deg;
        float sino = sinf(rad);
        float coso = cosf(rad);

        flame_xform::affine_t ret = a;
        ret[0] = a[0] * coso + a[2] * sino;
        ret[1] = a[1] * coso + a[3] * sino;
        ret[2] = a[2] * coso - a[0] * sino;
        ret[3] = a[3] * coso - a[1] * sino;

        return ret;
    }

    static flame_xform::affine_t scale_affine(const flame_xform::affine_t& a, float scale) {
        flame_xform::affine_t ret = a;

        ret[0] = a[0] * scale;
        ret[1] = a[1] * scale;
        ret[2] = a[2] * scale;
        ret[3] = a[3] * scale;

        return ret;
    }

    static flame_xform::affine_t translate_affine(const flame_xform::affine_t& a, const std::array<float, 2>& t) {
        flame_xform::affine_t ret = a;

        ret[4] = ret[0] * t[0] + ret[2] * t[1] + a[4];
        ret[5] = ret[1] * t[0] + ret[3] * t[1] + a[5];
        return ret;
    }

    void print_debug_info() {
        std::cout << buffer_map_.dump(1) << std::endl;
    }

private:

    flame() {
        active_flames_.insert(this);
    };

    bool do_common_init(const flame_compiler& fc);

    friend class flame_compiler;

    void make_shader_buffer_map();
    void copy_flame_data_to_buffer();

    using pos_buffer_t = storage_buffer<std::array<float, 4>>;
    using param_buffer_t = storage_buffer<float>;

    static inline std::size_t num_temporal_samples_ = 1024;
    static inline std::size_t num_shuffle_buffers_ = 1024;

    static inline std::unique_ptr<storage_buffer<std::uint32_t>> shuffle_buffers_;
    static inline std::unique_ptr<pos_buffer_t> sample_buffer_;
    static inline std::unique_ptr<pos_buffer_t> swap_buffer_;
    static inline std::unique_ptr<storage_buffer<jsf32::ctx>> rand_states_;

    std::unique_ptr<pos_buffer_t> local_buffer_;// = std::make_unique<pos_buffer_t>(swap_buffer_->size());

    std::unique_ptr<compute_shader> iterate_cs_;
    std::unique_ptr<compute_shader> animate_cs_;

    storage_buffer<unsigned int> counters_ = { static_cast<int>(flame::counters::NUM_COUNTERS) };

    nlohmann::json buffer_map_;
    storage_buffer<float> param_buffer_ = { 1024 };
    std::unique_ptr<storage_buffer<float>> inflated_buffer_;

    storage_buffer<std::array<float, 4>> palette_ = { 256 };

    bool needs_update = true;

    static inline std::set<flame*> active_flames_ = {};
};