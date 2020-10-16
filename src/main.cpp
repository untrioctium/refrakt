#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <GL/glew.h>    
#include <GLFW/glfw3.h>
#include <random>

#include <array>
#include <vector>
#include <iostream>
#include <optional>
#include <set>
#include <map>
#include <pugixml.hpp>
#include <nlohmann/json.hpp>
#include "buffer_objects.hpp"
#include "buffer_cache.hpp"
#include "variation_table.hpp"
#include "shaders.hpp"

#include "flame.hpp"

#include <inja/inja.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_image_resize.h>


void glDebugOutput(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar* message,const void* userParam);
std::vector<float> make_sample_points(std::uint32_t count);

std::vector<std::uint32_t> create_shuffle_buffer(std::uint32_t size);
void make_shuffle_buffers(std::uint32_t size, std::size_t count);


flame_xform::affine_t rotate_affine(const flame_xform::affine_t& a, float deg) {
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

flame_xform::affine_t scale_affine(const flame_xform::affine_t& a, float scale) {
    flame_xform::affine_t ret = a;

    ret[0] = a[0] * scale;
    ret[1] = a[1] * scale;
    ret[2] = a[2] * scale;
    ret[3] = a[3] * scale;

    return ret;
}

flame_xform::affine_t translate_affine(const flame_xform::affine_t& a, const std::array<float, 2>& t) {
    flame_xform::affine_t ret = a;

    ret[4] = ret[0] * t[0] + ret[2] * t[1] + a[4];
    ret[5] = ret[1] * t[0] + ret[3] * t[1] + a[5];
    return ret;
}

flame load_flame(const std::string& path, const variation_table& vt) {
    pugi::xml_document doc;
    auto result = doc.load_file(path.c_str());

    flame f{};
    auto flame_node = doc.child("flame");
    f.center = parse_strings<float, 2>(std::string{ flame_node.attribute("center").value() }, [](auto& v) {return std::stof(v); });

    f.scale = flame_node.attribute("scale").as_float();
    f.rotate = flame_node.attribute("rotate").as_float();
    f.estimator_curve = flame_node.attribute("estimator_curve").as_float();
    f.estimator_min = flame_node.attribute("estimator_min").as_int();
    f.estimator_radius = flame_node.attribute("estimator_radius").as_int();
    f.brightness = flame_node.attribute("brightness").as_float();
    f.gamma = flame_node.attribute("gamma").as_float();
    f.vibrancy = flame_node.attribute("vibrancy").as_float();;
    f.size = parse_strings<unsigned int, 2>(std::string{ flame_node.attribute("size").value() }, [](auto& v) { return (unsigned int)std::stoi(v); });

    for (auto node : flame_node.children()) {
        std::string node_name = node.name();
        if (node_name == "xform" || node_name == "finalxform") {
            flame_xform xform{};
            for (auto attr : node.attributes()) {
                std::string name = attr.name();

                if (name == "weight") xform.weight = attr.as_float();
                else if (name == "color") xform.color = attr.as_float();
                else if (name == "color_speed") xform.color_speed = attr.as_float();
                else if (name == "animate") xform.animate = attr.as_int() == 1;
                else if (name == "opacity") xform.opacity = attr.as_float();
                else if (vt.is_param(name)) xform.var_param[name] = attr.as_float();
                else if (vt.is_variation(name)) xform.variations[name] = attr.as_float();
                else if (name == "coefs") xform.affine = parse_strings<float, 6>(std::string{ attr.value() }, [](auto& v) {return std::stof(v); });
                else if (name == "post") xform.post = parse_strings<float, 6>(std::string{ attr.value() }, [](auto& v) {return std::stof(v); });
                else { std::cout << "Unknown attribute " << name << " in flame " << path << std::endl; }
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

            if (node_name == "finalxform") { f.final_xform = xform;}
            else { f.xforms.push_back(xform); }
        }
        else if (node_name == "color") {
            auto idx = node.attribute("index").as_ullong();
            f.palette[idx] = parse_strings<float, 4>(std::string{ node.attribute("rgb").value() }, [](auto& v) { return std::stoi(v)/256.0f; });
            f.palette[idx][3] = 1.0f;
        }
    }

    return f;
}

using buffer_map_t = nlohmann::json;

buffer_map_t make_shader_buffer_map(const flame& flame) {
    nlohmann::json buffer_map{};
    int counter = 0;
    buffer_map["xforms"] = nlohmann::json::array();

    auto make_xform_map = [&counter](const flame_xform& xform) {
        nlohmann::json xform_map = nlohmann::json::object();
        int start = counter;
        xform_map["meta"]["start"] = start;
        xform_map["meta"]["animated"] = xform.animate;
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

        xform_map["meta"]["end"] = counter - 1;
        xform_map["meta"]["size"] = counter - start;

        return xform_map;
    };

    for (const auto& xform : flame.xforms) {
        buffer_map["xforms"].push_back(make_xform_map(xform));
    }

    if (flame.final_xform) buffer_map["final_xform"] = make_xform_map(flame.final_xform.value());

    buffer_map["size"] = counter++;

    return buffer_map;
}

using fp_buffer_t = std::array<float, 1024>;
void copy_flame_data_to_buffer(const flame& f, const buffer_map_t& buf_map, storage_buffer<float>& pbuf) {

    fp_buffer_t buf;

    float normal_weight = 0.0;
    for (auto& x : f.xforms) normal_weight += x.weight;

    auto push_xform = [&](const auto& xform, const auto& xmap) {
        buf[xmap["weight"]] = xform.weight / normal_weight;
        for (int a = 0; a < 6; a++) buf[xmap["affine"][a]] = xform.affine[a];
        for (auto& [n, w] : xform.variations) buf[xmap["variations"][n]] = w;
        for (auto& [n, v] : xform.var_param) buf[xmap["param"][n]] = v;
        if (xform.post) for (int a = 0; a < 6; a++) buf[xmap["post"][a]] = xform.post.value()[a];
        buf[xmap["color"]] = xform.color;
        buf[xmap["opacity"]] = xform.opacity;
        buf[xmap["color_speed"]] = xform.color_speed;
    };

    for (int i = 0; i < f.xforms.size(); i++) {
        const auto& xform = f.xforms.at(i);
        const auto& xmap = buf_map["xforms"][i];

        push_xform(xform, xmap);
    }

    if (f.final_xform) push_xform(f.final_xform.value(), buf_map["final_xform"]);

    pbuf.update_all(buf.data());
}

struct gl_state {
    GLFWwindow* window;
    GLFWmonitor* primary_monitor;
};

std::pair<gl_state, int> init_gl(int w, int h, bool fullscreen) {
    
    gl_state rs{};

    glfwSetErrorCallback([](int error, const char* description) { fprintf(stderr, "Glfw Error %d: %s\n", error, description); });

    if (!glfwInit())
        return { rs, 1 };

    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    rs.primary_monitor = glfwGetPrimaryMonitor();
    rs.window = glfwCreateWindow(w, h, "flame", (fullscreen)? rs.primary_monitor : nullptr, nullptr);
    if (rs.window == nullptr) return { rs, 2 };

    glfwMakeContextCurrent(rs.window);
    glfwSwapInterval(1); // Enable vsync

    if (glewInit() != GLEW_OK) return { rs, 3 };

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMono-Regular.ttf", 15);
    //io.Fonts->GetTexDataAsRGBA32();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(rs.window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    return { rs, 0 };
}

struct sim_package {
};

struct sim_info {
    std::size_t num_points;
    std::size_t num_temporal_samples;
    
    int warmup_passes;
    int drawing_passes;

    int num_shuf_bufs;

    bool sample_random[2];
    bool warmup_random[2];
    bool draw_random[2];

    bool use_random_xform_selection;

    int block_width;

    std::size_t points_per_ts() const { return num_points / num_temporal_samples; }
};

int main(int, char**)
{
    auto [gl, error] = init_gl(1920, 1080, false);

    if (error) {
        std::cout << "Render init exited with code: " << error << std::endl;
    }

    std::random_device rd;
    std::mt19937 generator{rd()};
    std::uniform_real_distribution<float> dist(19232581.235235, 91212584.1241251);

    sim_info si{};

    si.num_points = 1024 * 1024;
    si.num_temporal_samples = 1024;

    si.block_width = 32;
    si.num_shuf_bufs = 1024;
    si.warmup_passes = 16;
    si.drawing_passes = 128;

    si.sample_random[0] = true;
    si.sample_random[1] = false;

    si.warmup_random[0] = true;
    si.warmup_random[1] = true;

    si.draw_random[0] = true;
    si.draw_random[1] = true;

    si.use_random_xform_selection = true;

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    storage_buffer<float> samples{ make_sample_points(si.points_per_ts()) };

    auto variations = variation_table{ "variations.yaml" };
    auto flame_def = load_flame("flames/electricsheep.247.23090.flam3", variations);
    auto buffer_map = make_shader_buffer_map(flame_def);
    auto shader_src = replace_macro(read_file("shaders/flame.glsl"), "varsource", variations.compile_flame_xforms(flame_def, buffer_map));
    shader_src = replace_macro(shader_src, "block_width", std::to_string(si.block_width));
    shader_src = replace_macro(shader_src, "final_xform_call", (flame_def.final_xform) ? "result = dispatch(result.xyz, -1) * vec4(1.0, 1.0, 1.0, result.w);" : "");




    auto animate_src = inja::render(read_file("shaders/templates/animate.tpl.glsl"), buffer_map);

    std::cout << "BUFFER MAP\n" << std::endl;
    std::cout << buffer_map.dump(1) << std::endl;

    std::cout << "SHADER SRC\n" << std::endl;
    std::cout << shader_src << std::endl;

    std::cout << "ANIMATE SRC\n" << std::endl;
    std::cout << animate_src << std::endl;

    auto cs = compute_shader(shader_src);
    auto tonemap_cs = compute_shader(read_file("shaders/tonemap.glsl"));
    auto quad_vf = vf_shader(read_file("shaders/quad_vert.glsl"), read_file("shaders/quad_frag.glsl"));
    auto density_cs = compute_shader(read_file("shaders/density.glsl"));
    auto animate_cs = compute_shader(animate_src);

    auto print_buffer = [](auto& buf, auto& name) { std::cout << name << ": " << buf.name() << std::endl; };

    storage_buffer<std::array<float, 4>> pos[2] = { {si.num_points},{si.num_points} };
    GLuint vao;
  
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    storage_buffer<float> fp{ 1024 };
    print_buffer(fp, "uninflated params");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, fp.name());

    storage_buffer<std::array<float, 4>> palette{ 256, flame_def.palette.data() };
    print_buffer(palette, "palette");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, palette.name());

    auto xform_atomic_init = std::vector<unsigned int>(flame_def.xforms.size(), 0);
    storage_buffer<unsigned int> xform_atomic_counters{ xform_atomic_init };
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, xform_atomic_counters.name());

    auto flame_stats_init = std::vector<unsigned int>(16, 0);
    storage_buffer<unsigned int> flame_atomic_counters(flame_stats_init);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, flame_atomic_counters.name());

    const std::size_t target_dims[2] = { 1920, 1080 };

    storage_buffer<std::array<float,4>> bins{ target_dims[0] * target_dims[1] };
    glClearNamedBufferData(bins.name(), GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, bins.name());

    storage_buffer<float> inflated_functions{ buffer_map["size"] * si.num_temporal_samples };
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, inflated_functions.name());

    auto shuf_cache_group = buffer_cache::buffer_group("shuffle", std::to_string(si.points_per_ts()));

    while (shuf_cache_group.cached_buffers().size() < si.num_shuf_bufs) {
        make_shuffle_buffers(si.points_per_ts(), 8);
    }

    auto shuf_buffers = shuf_cache_group.cached_buffers();
    auto shuf_shuf = create_shuffle_buffer(shuf_buffers.size());
    
    storage_buffer<std::uint32_t> shuf_buf{ si.num_shuf_bufs * si.points_per_ts() };

    for (int i = 0; i < si.num_shuf_bufs; i++) {
        std::string name = shuf_buffers[shuf_shuf[i]];
        shuf_buf.update_many(si.points_per_ts() * i, si.points_per_ts(), shuf_cache_group.read_buffer<std::uint32_t>(name).data());
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, shuf_buf.name());

    storage_buffer<jsf32::ctx> rand_states{ si.num_points };
    auto rand_state_group = buffer_cache::buffer_group("rand_state", std::to_string(si.num_points));

    if (rand_state_group.cached_buffers().size() < 1) {
        std::vector<jsf32::ctx> states{};
        states.resize(si.num_points);

        for (jsf32::u4 i = 0; i < si.num_points; i++) {
            jsf32::warmup_ctx(states[i], i);
        }

        rand_state_group.write_buffer(states);
    }

    auto buf_name = rand_state_group.cached_buffers().at(0);
    auto rs_buf = rand_state_group.read_buffer<jsf32::ctx>(buf_name);



    rand_states.update_all(rs_buf.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, rand_states.name());
    print_buffer(rand_states, "rand");
    const int in_buf_pos = 0;
    const int out_buf_pos = 1;
    const int in_buf_col = 2;
    const int out_buf_col = 3;

    texture<float> render_targets[2] = { {target_dims[0], target_dims[1]}, {target_dims[0], target_dims[1]} };
    frame_buffer fb;

    timer frame_timer;

    const float DEGREES_PER_SECOND = 72.0f;

    bool clear_parts = true;
    bool needs_clear = true;
    float scale_constant = 4;

    moving_average<16> fps;
    moving_average<100> pps;
    float pps_final = 0.0;

    bool animate = false;
    bool do_step = false;

    float tss_width = 1.2 / 60.0;

    long long accumulated = 0;
    int needed_quality = 2000;

    bool advance_frame = false;
    int rendered_frames = 0;
    int total_frames_needed = 300;

    flame_def.scale *= float(target_dims[1]) / float(flame_def.size[1]);

    std::vector<unsigned long long> running_xform_counts( flame_def.xforms.size(), 0 );
    std::vector<unsigned long long> current_xform_counts(flame_def.xforms.size(), 0 );

    // Main loop
    while (!glfwWindowShouldClose(gl.window))
    {

        auto shuf_path = create_shuffle_buffer(si.num_shuf_bufs);
        for (int i = 0; i < 3; i++) {
            auto new_shuf = create_shuffle_buffer(si.num_shuf_bufs);
            shuf_path.insert(shuf_path.end(), new_shuf.begin(), new_shuf.end());
        }
        std::unordered_map<std::string, float> perf_timer_results;

        float dt = float(frame_timer.time<timer::ms>()) / 1000.0f;
        float fps_avg = fps.add(dt);
        frame_timer.reset();

        xform_atomic_counters.update_all(xform_atomic_init.data());
        flame_atomic_counters.update_all(flame_stats_init.data());
        auto shuf = shuf_path.begin();

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        ImGui::Begin("Sim Configuration");
        needs_clear |= ImGui::InputInt("Warmup Passes", &si.warmup_passes, 2, 2);
        needs_clear |= ImGui::InputInt("Drawing Passes", &si.drawing_passes, 2, 2);
        needs_clear |= ImGui::Checkbox("Use random xform selection", &si.use_random_xform_selection);
        needs_clear |= ImGui::DragFloat("Temporal Sample Width", &tss_width, .01, 0, .25);
        needs_clear |= ImGui::InputInt("Quality Target", (int*) &needed_quality, 10, 100);
        do_step = ImGui::Button("Advance 1/60s");
        ImGui::End();

        ImGui::Begin("Flame Info"); {
            needs_clear |= ImGui::DragFloat("Scale", &flame_def.scale, 1, 1, 10000);
            needs_clear |= ImGui::DragFloat2("Center", flame_def.center.data(), .01, -5, 5);
            needs_clear |= ImGui::DragFloat("Rotate", &flame_def.rotate, 1.0, -360.0, 360.0);
            needs_clear |= ImGui::Checkbox("Animate", &animate);
            ImGui::InputFloat("Scale Constant", &scale_constant, 1, 5, .00001);
            ImGui::Separator();
            ImGui::DragInt("Estimator Radius", &flame_def.estimator_radius, 0.005f, 0, 20);
            ImGui::DragInt("Estimator Min", &flame_def.estimator_min, 0.005f, 0, 20);
            ImGui::DragFloat("Estimator Curve", &flame_def.estimator_curve, .001, 0, 1);

            flame_def.for_each_xform([&](int idx, flame_xform& xform) {
                std::string hash = "##xform" + std::to_string(idx) + std::to_string((unsigned int)&xform);
                if (ImGui::CollapsingHeader(("xform " + std::to_string(idx)).c_str())) {
                    needs_clear |= ImGui::DragFloat(("Weight" + hash).c_str(), &xform.weight, .001, -100, 100);
                    needs_clear |= ImGui::DragFloat(("Color" + hash).c_str(), &xform.color, .0001, 0, 1);
                    needs_clear |= ImGui::DragFloat(("Color Speed" + hash).c_str(), & xform.color_speed, .0001, 0, 1);
                    needs_clear |= ImGui::DragFloat(("Opacity" + hash).c_str(), &xform.opacity, .0001, 0, 1);
                    ImGui::Separator();
                    needs_clear |= ImGui::DragFloat3(("X Affine (a,b,c)" + hash).c_str(), xform.affine.data(), .001, -3, 3);
                    needs_clear |= ImGui::DragFloat3(("Y Affine (d,e,f)" + hash).c_str(), xform.affine.data() + 3, .001, -3, 3);

                    if (xform.post) {
                        ImGui::Separator();
                        needs_clear |= ImGui::DragFloat3(("X Post Affine (a,b,c)" + hash).c_str(), xform.post.value().data(), .001, -3, 3);
                        needs_clear |= ImGui::DragFloat3(("Y Post Affine (d,e,f)" + hash).c_str(), xform.post.value().data() + 3, .001, -3, 3);
                    }

                    for (auto& [name,v] : xform.variations) {
                        ImGui::Separator();
                        needs_clear |= ImGui::DragFloat((name + hash).c_str(), &v, .001, -100, 100);
                        for (auto& p_name : variations.get_parameters_for_variation(name)) {
                            needs_clear |= ImGui::DragFloat((p_name + hash).c_str(), &xform.var_param.at(p_name), .01, -20, 20);
                        }
                    }

                }
            });

 
            if (ImGui::CollapsingHeader("Palette")) {
                for (int i = 0; i < 256; i++) {
                    auto& c = flame_def.palette.at(i);
                    ImGui::ColorEdit4(std::to_string(i).c_str(), c.data());
                }
            }
        } ImGui::End(); //Flame Info

        float rotation = DEGREES_PER_SECOND * dt;

        //Do rotation
        if(animate || advance_frame)
            for (auto& x : flame_def.xforms) {
                if (x.animate) x.affine = rotate_affine(x.affine, rotation);
                needs_clear = true;
            }

        if (do_step) {
            for (auto& x : flame_def.xforms) {
                if (x.animate) x.affine = rotate_affine(x.affine, DEGREES_PER_SECOND/60.0);
                needs_clear = true;
            }
            do_step = false;
        }

        if (needs_clear) {
            glClearNamedBufferData(bins.name(), GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
            accumulated = 0;
            for (auto& c : running_xform_counts) c = 0;
        }

        // Rendering
        int display_w, display_h;



        timer perf_timer;
        timer perf_timer_total;
        perf_timer.reset();
        copy_flame_data_to_buffer(flame_def, buffer_map, fp);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();
        perf_timer_results["copy data"] = perf_timer.time<timer::ns>();

        glUseProgram(cs.name());

        cs.set_uniform<bool>("use_random_xform_selection", si.use_random_xform_selection);
        cs.set_uniform<int>("total_params", buffer_map["size"]);

        perf_timer.reset();
        if (needs_clear)
        {
            // inflate parameters
            glUseProgram(animate_cs.name());
            animate_cs.set_uniform<float>("temporal_sample_width", tss_width);
            glDispatchCompute(si.num_temporal_samples / 32, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();

            glUseProgram(cs.name());

            // first pass, random read from sample points and flame pass
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, samples.name());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[0].name());
            cs.set_uniform<bool>("make_xform_dist", false);
            cs.set_uniform<bool>("random_read", si.sample_random[0]);
            cs.set_uniform<bool>("random_write", si.sample_random[1]);
            cs.set_uniform<bool>("first_run", true);
            cs.set_uniform<bool>("do_draw", false);
            cs.set_uniform<float>("rand_seed", dist(generator));
            if(si.sample_random[0]) cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
            if(si.sample_random[1]) cs.set_uniform<unsigned int>("shuf_buf_idx_out", *(shuf++));
            glDispatchCompute(si.points_per_ts() / si.block_width, si.num_temporal_samples, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
            //perf_timer_results["sample"] = perf_timer.time<timer::ns>();

            cs.set_uniform<bool>("random_read", si.warmup_random[0]);
            cs.set_uniform<bool>("random_write", si.warmup_random[1]);
            cs.set_uniform<bool>("first_run", false);

            //perf_timer.reset();
            // warm up passes, random read and write
            for (int i = 0; i < si.warmup_passes; i++) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, pos[i & 1].name());
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[!(i & 1)].name());
                cs.set_uniform<float>("rand_seed", dist(generator));
                if(si.warmup_random[0]) cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
                if(si.warmup_random[1]) cs.set_uniform<unsigned int>("shuf_buf_idx_out", *(shuf++));
                glDispatchCompute(si.points_per_ts() / si.block_width, si.num_temporal_samples, 1);
                glMemoryBarrier(GL_ALL_BARRIER_BITS);
                glFinish();
            }
            needs_clear = false;
        }
        perf_timer_results["warm up"] = perf_timer.time<timer::ns>();

        fb.bind();
        frame_buffer::attach(render_targets[0].name());
        if (clear_parts) {
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            clear_parts = false;
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        //cs.set_uniform<bool>("random_read", true);
        //cs.set_uniform<bool>("random_write", false);

        auto win_min = glm::vec2(
            flame_def.center[0] - float(target_dims[0]) / flame_def.scale / 2.0,
            flame_def.center[1] - float(target_dims[1]) / flame_def.scale / 2.0);

        flame_xform::affine_t base{ 1, 0, 0, 1, 0, 0 };

        base = translate_affine(base, { target_dims[0] / 2.0f, target_dims[1] / 2.0f });
        base = scale_affine(base, flame_def.scale);
        base = rotate_affine(base, flame_def.rotate);
        base = translate_affine(base, { -flame_def.center[0], -flame_def.center[1] });

        cs.set_uniform<std::array<float, 6>>("ss_affine", base);
        cs.set_uniform<bool>("random_read", si.draw_random[0]);
        cs.set_uniform<bool>("random_write", si.draw_random[1]);
        cs.set_uniform<bool>("do_draw", true);
        cs.set_uniform<glm::uvec2>("bin_dims", glm::uvec2{ target_dims[0], target_dims[1] });

        perf_timer.reset();
        // drawing passes
        if (accumulated < needed_quality * target_dims[0] * target_dims[1]) {
            for (int i = 0; i < si.drawing_passes; i++) {
                {
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, pos[i & 1].name());
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[!(i & 1)].name());
                    cs.set_uniform<float>("rand_seed", dist(generator));
                    if (si.draw_random[0]) cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
                    if (si.draw_random[1]) cs.set_uniform<unsigned int>("shuf_buf_idx_out", *(shuf++));

                    glDispatchCompute(si.points_per_ts() / si.block_width, si.num_temporal_samples, 1);
                    glMemoryBarrier(GL_ALL_BARRIER_BITS);
                    //                glFinish();
                }
                //perf_timer_results["draw calc"] += perf_timer.time<timer::ns>();

                /*glUseProgram(particle_vf.name());

                float half_width = 800 / flame_def.scale / 2.0;
                float half_height = 592 / flame_def.scale / 2.0;

                auto proj = glm::ortho(
                        flame_def.center[0] - half_width,
                        flame_def.center[0] + half_width,
                        flame_def.center[1] + half_height,
                        flame_def.center[1] - half_height,
                        -1.0f, 1.0f);

                particle_vf.set_uniform<glm::mat4>("projection", proj);
                {
                    glBindVertexArray(vao[!(i & 1)]);
                    glDrawArrays(GL_POINTS, 0, num_points);
    //                glMemoryBarrier(GL_ALL_BARRIER_BITS);
                    glFinish();
                }
                glUseProgram(cs.name());*/
            }
            glFinish();
            float drawn_parts = float(si.drawing_passes) * si.num_points;
            pps_final = pps.add(drawn_parts / dt);
            

        }
        perf_timer_results["draw calc"] = perf_timer.time<timer::ns>();
        fb.unbind();
        glDisable(GL_BLEND);
        
        perf_timer.reset();
        /*
        {
            glUseProgram(density_cs.name());
            density_cs.set_uniform<int>("estimator_radius", flame_def.estimator_radius);
            density_cs.set_uniform<int>("estimator_min", flame_def.estimator_min);
            density_cs.set_uniform<float>("estimator_curve", flame_def.estimator_curve);
            density_cs.set_uniform<int>("in_hist", 0);
            density_cs.set_uniform<int>("out_hist", 1);
            glBindImageTexture(0, render_targets[0].name(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, render_targets[1].name(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glDispatchCompute(target_dims[0], target_dims[1], 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }*/
        perf_timer_results["density"] = perf_timer.time<timer::ns>();

        perf_timer.reset();
        {
            glUseProgram(tonemap_cs.name());
            glBindImageTexture(0, render_targets[1].name(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            tonemap_cs.set_uniform<int>("image", 0);
            tonemap_cs.set_uniform<float>("scale_constant", 1.0/pow(10.0, scale_constant));
            tonemap_cs.set_uniform<float>("gamma", flame_def.gamma);
            tonemap_cs.set_uniform<float>("brightness", flame_def.brightness);
            tonemap_cs.set_uniform<float>("vibrancy", flame_def.vibrancy);
            glDispatchCompute(target_dims[0]/8, target_dims[1]/8, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }
        perf_timer_results["tonemap"] = perf_timer.time<timer::ns>();

        glfwGetFramebufferSize(gl.window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(quad_vf.name());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, render_targets[1].name());
        quad_vf.set_uniform<int>("tex", 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(0);

        auto total_time = perf_timer_total.time<timer::ns>();
        ImGui::Begin("Timing"); {
            #define SHOW_PERF_TIMER(name) ImGui::ProgressBar(perf_timer_results[name] / float(total_time), ImVec2(0.0f, 0.0f)); ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); ImGui::Text(name)
            SHOW_PERF_TIMER("copy data");
            SHOW_PERF_TIMER("warm up");
            SHOW_PERF_TIMER("draw calc");
            ImGui::ProgressBar(perf_timer_results["warm up"] / float(si.warmup_passes) / float(total_time), ImVec2(0.0f, 0.0f)); ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); ImGui::Text("warm up per iter");
            ImGui::ProgressBar(perf_timer_results["warm up"] / float(si.drawing_passes) / float(total_time), ImVec2(0.0f, 0.0f)); ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); ImGui::Text("drawing per iter");
            SHOW_PERF_TIMER("density");
            SHOW_PERF_TIMER("tonemap");
        }ImGui::End();

        

        auto total_binned = flame_atomic_counters.get_one(0);
        accumulated += total_binned;

        GLint total_mem_kb = 0;
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);

        GLint available_mem = 0;
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available_mem);

        ImGui::Begin("Debug");
        ImGui::Text("FPS Avg: %f", 1.0 / fps_avg);
        ImGui::Text("Mem Avail: %.3fGB", available_mem / (1024.0 * 1024.0));
        ImGui::Text("Mem Used: %.3fGB", (total_mem_kb - available_mem)/(1024.0 * 1024.0));
        ImGui::Text("Parts per second: %.1fM", pps_final / 1'000'000.0f);
        ImGui::Text("Parts per frame: %.1fM", float(si.drawing_passes)* si.num_points / 1'000'000.0f);
        ImGui::Text("Binned this frame: %.1fM", total_binned / 1'000'000.0f);
        ImGui::Text("Total binned this image: %.3fB", accumulated / 1'000'000'000.0f );
        ImGui::Text("Image Quality: %.2f%%", float(accumulated) / float(needed_quality * target_dims[0] * target_dims[1]) * 100.0);
        if (ImGui::Button("Screenshot")) {
            auto pixels = render_targets[1].get_pixels();
            stbi_write_png("screenshot.png", render_targets[1].width(), render_targets[1].height(), 4, pixels.data(), 0);
        }

        float xform_sum = 0.0;
        unsigned long long total_hit = 0;
        for (auto& xform : flame_def.xforms) xform_sum += xform.weight;

        for (int i = 0; i < flame_def.xforms.size(); i++) {
            current_xform_counts[i] = xform_atomic_counters.get_one(i);
            running_xform_counts[i] += current_xform_counts[i];
            total_hit += running_xform_counts[i];
        }

        for (int i = 0; i < flame_def.xforms.size(); i++) {
            float expected = flame_def.xforms[i].weight / xform_sum;
            float actual = double(running_xform_counts[i]) / double(total_hit);

            ImGui::Text("XFORM %d: %.3f %.3f", i, expected * 100.0, actual * 100.0);
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   

        glfwSwapBuffers(gl.window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(gl.window);
    glfwTerminate();

    return 0;
}
