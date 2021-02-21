#include "pugixml.hpp"
#include "flame.hpp"
#include "variation_table.hpp"

#include <inja/inja.hpp>

#include "buffer_cache.hpp"

#include <random>

std::vector<std::uint32_t> create_shuffle_buffer(std::uint32_t size);
void make_shuffle_buffers(std::uint32_t size, std::size_t count);
std::vector<std::array<float,4>> make_sample_points(std::uint32_t count);

#define BLOCK_WIDTH 256

bool flame::do_common_init(const flame_compiler& fc)
{
    make_shader_buffer_map();

    auto shader_src = replace_macro(read_file("shaders/flame.glsl"), "varsource", fc.compile_flame_xforms(*this));
    shader_src = replace_macro(shader_src, "block_width", std::to_string(BLOCK_WIDTH));
    shader_src = replace_macro(shader_src, "final_xform_call", (final_xform) ? "result = dispatch(result.xyz, -1) * vec4(1.0, 1.0, 1.0, result.w);" : "");

    iterate_cs_ = std::make_unique<compute_shader>(shader_src);
    reset_animation();

    palette_.update_all(palette.data());

    return iterate_cs_->is_good() && animate_cs_->is_good();
}

void flame::make_shader_buffer_map()
{
    nlohmann::json buffer_map{};
    int counter = 0;
    buffer_map["xforms"] = nlohmann::json::array();

    auto make_xform_map = [&counter](const flame_xform& xform) {
        nlohmann::json xform_map = nlohmann::json::object();
        int start = counter;
        xform_map["meta"]["start"] = start;
        xform_map["weight"] = counter++;

        xform_map["affine"] = std::vector{ counter++, counter++, counter++, counter++, counter++, counter++ };
        if (xform.post) xform_map["post"] = std::vector{ counter++, counter++, counter++, counter++, counter++, counter++ };
        xform_map["variations"] = nlohmann::json::object();
        for (auto& [k, v] : xform.variations) xform_map["variations"][k] = counter++;
        xform_map["param"] = nlohmann::json::object();
        for (auto& [k, v] : xform.var_param) xform_map["param"][k] = counter++;
        xform_map["color"] = counter++;
        xform_map["color_speed"] = counter++;
        xform_map["opacity"] = counter++;
        xform_map["rotation_frequency"] = counter++;

        xform_map["meta"]["end"] = counter - 1;
        xform_map["meta"]["size"] = counter - start;

        return xform_map;
    };

    for (const auto& xform : this->xforms) {
        buffer_map["xforms"].push_back(make_xform_map(xform));
    }

    if (this->final_xform) buffer_map["final_xform"] = make_xform_map(this->final_xform.value());

    buffer_map["size"] = counter++;

    buffer_map_ = buffer_map;
}

using fp_buffer_t = std::array<float, 1024>;
void flame::copy_flame_data_to_buffer()
{
    fp_buffer_t buf;

    float normal_weight = 0.0;
    for (auto& x : this->xforms) normal_weight += x.weight;

    auto push_xform = [&](const auto& xform, const auto& xmap) {
        buf[xmap["weight"]] = xform.weight / normal_weight;
        for (int a = 0; a < 6; a++) buf[xmap["affine"][a]] = xform.affine[a];
        for (auto& [n, w] : xform.variations) buf[xmap["variations"][n]] = w;
        for (auto& [n, v] : xform.var_param) buf[xmap["param"][n]] = v;
        if (xform.post) for (int a = 0; a < 6; a++) buf[xmap["post"][a]] = xform.post.value()[a];
        buf[xmap["color"]] = xform.color;
        buf[xmap["opacity"]] = xform.opacity;
        buf[xmap["color_speed"]] = xform.color_speed;
        buf[xmap["rotation_frequency"]] = xform.rotation_frequency;
    };

    for (int i = 0; i < this->xforms.size(); i++) {
        const auto& xform = this->xforms.at(i);
        const auto& xmap = buffer_map_["xforms"][i];

        push_xform(xform, xmap);
    }

    if (this->final_xform) push_xform(this->final_xform.value(), buffer_map_["final_xform"]);

    param_buffer_.update_all(buf.data());
}

void flame::set_sim_parameters(std::size_t total_particles, std::size_t temporal_samples, std::size_t shuffle_count)
{
    swap_buffer_.reset(new pos_buffer_t(total_particles));

    num_temporal_samples_ = temporal_samples;
    num_shuffle_buffers_ = shuffle_count;

    auto points_per_ts = total_particles / num_temporal_samples_;

    auto shuf_cache_group = buffer_cache::buffer_group("shuffle", std::to_string(points_per_ts));

    while (shuf_cache_group.cached_buffers().size() < num_shuffle_buffers_) {
        make_shuffle_buffers(points_per_ts, 8);
    }

    auto shuf_buffers = shuf_cache_group.cached_buffers();
    auto shuf_shuf = create_shuffle_buffer(shuf_buffers.size());

    shuffle_buffers_ = std::make_unique<storage_buffer<std::uint32_t>>( num_shuffle_buffers_ * points_per_ts );

    for (int i = 0; i < num_shuffle_buffers_; i++) {
        std::string name = shuf_buffers[shuf_shuf[i]];
        shuffle_buffers_->update_many(points_per_ts * i, points_per_ts, shuf_cache_group.read_buffer<std::uint32_t>(name).data());
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, shuffle_buffers_->name());

    rand_states_ = std::make_unique<storage_buffer<jsf32::ctx>>(total_particles);
    auto rand_state_group = buffer_cache::buffer_group("rand_state", std::to_string(total_particles));

    if (rand_state_group.cached_buffers().size() < 1) {
        std::vector<jsf32::ctx> states{};
        states.resize(total_particles);

        for (jsf32::u4 i = 0; i < total_particles; i++) {
            jsf32::warmup_ctx(states[i], i);
        }

        rand_state_group.write_buffer(states);
    }

    auto buf_name = rand_state_group.cached_buffers().at(0);
    auto rs_buf = rand_state_group.read_buffer<jsf32::ctx>(buf_name);
    rand_states_->update_all(rs_buf.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, rand_states_->name());

    sample_buffer_ = std::make_unique<pos_buffer_t>(make_sample_points(points_per_ts));

    // invalidate existing flames
    for (auto f : active_flames_) {
        f->local_buffer_.reset();
        f->inflated_buffer_.reset();
    }
}

std::unique_ptr<flame> flame::load_flame(const std::string& path, const flame_compiler& vt) {
    pugi::xml_document doc;
    auto result = doc.load_file(path.c_str());

    auto f = new flame{};//std::make_unique<flame>();
    auto flame_node = doc.child("flame");
    f->center = parse_strings<float, 2>(std::string{ flame_node.attribute("center").value() }, [](auto& v) {return std::stof(v); });

    f->scale = flame_node.attribute("scale").as_float();
    f->rotate = flame_node.attribute("rotate").as_float();
    f->estimator_curve = flame_node.attribute("estimator_curve").as_float();
    f->estimator_min = flame_node.attribute("estimator_min").as_int();
    f->estimator_radius = flame_node.attribute("estimator_radius").as_int();
    f->brightness = flame_node.attribute("brightness").as_float();
    f->gamma = flame_node.attribute("gamma").as_float();
    f->vibrancy = flame_node.attribute("vibrancy").as_float();;
    f->size = parse_strings<unsigned int, 2>(std::string{ flame_node.attribute("size").value() }, [](auto& v) { return (unsigned int)std::stoi(v); });

    bool is_bad = false;

    for (auto node : flame_node.children()) {
        std::string node_name = node.name();
        if (node_name == "xform" || node_name == "finalxform") {
            flame_xform xform{};
            for (auto attr : node.attributes()) {
                std::string name = attr.name();

                if (name == "weight") xform.weight = attr.as_float();
                else if (name == "color") xform.color = attr.as_float();
                else if (name == "color_speed") xform.color_speed = attr.as_float();
                else if (name == "animate") xform.rotation_frequency = (attr.as_float() > 0 && node_name != "final_xform") ? 1.0: 0.0;
                else if (name == "opacity") xform.opacity = attr.as_float();
                else if (vt.is_param(name)) xform.var_param[name] = attr.as_float();
                else if (vt.is_variation(name)) xform.variations[name] = attr.as_float();
                else if (name == "coefs") xform.affine = parse_strings<float, 6>(std::string{ attr.value() }, [](auto& v) {return (float) std::stod(v); });
                else if (name == "post") xform.post = parse_strings<float, 6>(std::string{ attr.value() }, [](auto& v) {return (float) std::stod(v); });
                else { std::cout << "Unknown attribute " << name << " in flame " << path << std::endl; is_bad = true; }
            }

            /*for (auto motion : node.children("motion")) {
                motion_info m{};
                m.freq = motion.attribute("motion_frequency").as_float();
                m.function = motion.attribute("motion_function").as_string();

                for (auto a : motion.attributes()) {
                    if (a.name() != "motion_frequency" && a.name() != "motion_function") {
                        m.freq = a.as_float();
                        xform.motion[a.name()] = m;
                    }
                }
            }*/

            if (node_name == "finalxform") { f->final_xform = xform; }
            else { f->xforms.push_back(xform); }
        }
        else if (node_name == "color") {
            auto idx = node.attribute("index").as_ullong();
            f->palette[idx] = parse_strings<float, 4>(std::string{ node.attribute("rgb").value() }, [](auto& v) { return std::stoi(v) / 256.0f; });
            f->palette[idx][3] = 1.0f;
        }
    }

    if (!is_bad && f->do_common_init(vt)) {
        return std::unique_ptr<flame>{ f };
    }
    else return nullptr;
}

void flame::warmup(std::size_t num_passes, float tss_width)
{

    static std::random_device rd;
    static std::mt19937 generator{ rd() };
    std::uniform_int_distribution shuf_dist(0, int(num_shuffle_buffers_ - 1));

    needs_update = false;
    if (!local_buffer_) local_buffer_ = std::make_unique<pos_buffer_t>(swap_buffer_->size());
    if (!inflated_buffer_) inflated_buffer_ = std::make_unique<storage_buffer<float>>(int(buffer_map_["size"]) * num_temporal_samples_);

    copy_flame_data_to_buffer();
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    //glFinish();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, param_buffer_.name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, inflated_buffer_->name());

    glUseProgram(animate_cs_->name());
    animate_cs_->set_uniform<float>("temporal_sample_width", tss_width);
    glDispatchCompute(num_temporal_samples_ / 32, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    //glFinish();

    glUseProgram(iterate_cs_->name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sample_buffer_->name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, local_buffer_->name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, palette_.name());
    iterate_cs_->set_uniform<int>("total_params", buffer_map_["size"]);
    iterate_cs_->set_uniform<bool>("random_read", true);
    iterate_cs_->set_uniform<bool>("random_write", true);
    iterate_cs_->set_uniform<bool>("first_run", true);
    iterate_cs_->set_uniform<bool>("do_draw", false);
    iterate_cs_->set_uniform<unsigned int>("shuf_buf_idx_in", shuf_dist(generator));
    iterate_cs_->set_uniform<unsigned int>("shuf_buf_idx_out", shuf_dist(generator));
    glDispatchCompute((swap_buffer_->size() / num_temporal_samples_) / BLOCK_WIDTH, num_temporal_samples_, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    //glFinish();

    GLuint names[2] = { local_buffer_->name(), swap_buffer_->name() };

    iterate_cs_->set_uniform<bool>("first_run", false);
    for (int i = 0; i < num_passes; i++) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, names[i % 2]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, names[(i + 1) % 2]);
        iterate_cs_->set_uniform<unsigned int>("shuf_buf_idx_in", shuf_dist(generator));
        iterate_cs_->set_uniform<unsigned int>("shuf_buf_idx_out", shuf_dist(generator));
        glDispatchCompute((swap_buffer_->size() / num_temporal_samples_) / BLOCK_WIDTH, num_temporal_samples_, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        //glFinish();
    }

    if (num_passes % 2) local_buffer_.swap(swap_buffer_);
}

std::size_t flame::draw_to_bins(bin_t& bins, std::size_t bins_width, int num_iter)
{
    static std::random_device rd;
    static std::mt19937 generator{ rd() };
    std::uniform_int_distribution shuf_dist(0, int(num_shuffle_buffers_ - 1));

    // create screen space transform
    std::size_t target_dims[2] = { bins_width, bins.size() / bins_width };
    flame_xform::affine_t base{ 1, 0, 0, 1, 0, 0 };

    base = translate_affine(base, { target_dims[0] / 2.0f, target_dims[1] / 2.0f });
    base = scale_affine(base, scale * float(target_dims[1]) / float(size[1]));
    base = rotate_affine(base, rotate);
    base = translate_affine(base, { -center[0], -center[1] });

    // set parameters
    glUseProgram(iterate_cs_->name());
    iterate_cs_->set_uniform<int>("total_params", buffer_map_["size"]);
    iterate_cs_->set_uniform<bool>("random_read", true);
    iterate_cs_->set_uniform<bool>("random_write", false);
    iterate_cs_->set_uniform<bool>("first_run", false);
    iterate_cs_->set_uniform<bool>("do_draw", true);
    iterate_cs_->set_uniform<glm::uvec2>("bin_dims", glm::uvec2(target_dims[0], target_dims[1]));
    iterate_cs_->set_uniform<std::array<float, 6>>("ss_affine", base);

    // bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, palette_.name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, bins.name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, inflated_buffer_->name());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, counters_.name());
    counters_.zero_out();

    GLuint names[2] = { local_buffer_->name(), swap_buffer_->name() };

    for (int i = 0; i < num_iter; i++) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, names[i % 2]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, names[(i + 1) % 2]);
        iterate_cs_->set_uniform<unsigned int>("shuf_buf_idx_in", shuf_dist(generator));
        iterate_cs_->set_uniform<unsigned int>("shuf_buf_idx_out", shuf_dist(generator));
        glDispatchCompute((swap_buffer_->size() / num_temporal_samples_) / BLOCK_WIDTH, num_temporal_samples_, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        //glFinish();
    }

    if (num_iter % 2) local_buffer_.swap(swap_buffer_);

    return std::size_t{ counters_.get_one(0) };
}

void flame::reset_animation()
{
    animate_cs_ = std::make_unique<compute_shader>(inja::render(read_file("shaders/templates/animate.tpl.glsl"), buffer_map_));
    needs_update = true;
}
