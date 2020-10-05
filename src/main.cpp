#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <GL/glew.h>    
#include <GLFW/glfw3.h>

#include <array>
#include <chrono>
#include <vector>
#include <iostream>
#include <optional>
#include <set>
#include <map>
#include <pugixml.hpp>
#include <nlohmann/json.hpp>
#include "buffer_objects.hpp"
#include "variation_table.hpp"
#include "util.hpp"
#include "shaders.hpp"

#include "flame.hpp"

void glDebugOutput(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar* message,const void* userParam);
std::vector<float> make_sample_points(std::uint32_t count);

std::vector<std::uint32_t> create_shuffle_buffer(std::uint32_t size);
auto make_shuffle_buffers(std::uint32_t size, std::size_t count)->std::vector<std::uint32_t>;

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
        xform_map["weight"] = counter++;
        xform_map["affine"] = std::vector{ counter++, counter++, counter++, counter++, counter++, counter++ };
        if (xform.post) xform_map["post"] = std::vector{ counter++, counter++, counter++, counter++, counter++, counter++ };
        xform_map["variations"] = nlohmann::json::object();
        for (auto& [k, v] : xform.variations) xform_map["variations"][k] = counter++;
        xform_map["param"] = nlohmann::json::object();
        for (auto& [k, v] : xform.var_param) xform_map["param"][k] = counter++;
        xform_map["color"] = counter++;
        xform_map["color_speed"] = counter++;
        return xform_map;
    };

    for (const auto& xform : flame.xforms) {
        buffer_map["xforms"].push_back(make_xform_map(xform));
    }

    if (flame.final_xform) buffer_map["final_xform"] = make_xform_map(flame.final_xform.value());

    return buffer_map;
}

using fp_buffer_t = std::array<float, 256>;
void copy_flame_data_to_buffer(const flame& f, const buffer_map_t& buf_map, storage_buffer<float>& pbuf) {

    fp_buffer_t buf;

    float normal_weight = 0.0;
    for (auto& x : f.xforms) normal_weight += x.weight;

    for (int i = 0; i < f.xforms.size(); i++) {
        const auto& xform = f.xforms.at(i);
        const auto& xmap = buf_map["xforms"][i];

        auto write_buf = [&](auto idx, auto v) {
            buf[idx] = v;
           // std::cout << idx << ": " << v << std::endl;
        };

        write_buf(xmap["weight"], xform.weight/ normal_weight);
        for (int a = 0; a < 6; a++) write_buf(xmap["affine"][a], xform.affine[a]);
        for (auto& [n, w] : xform.variations) write_buf(xmap["variations"][n],w);
        for (auto& [n, v] : xform.var_param) write_buf(xmap["param"][n], v);
        if(xform.post) for (int a = 0; a < 6; a++) write_buf(xmap["post"][a], xform.post.value()[a]);
        write_buf(xmap["color"], xform.color);
        write_buf(xmap["color_speed"], xform.color_speed);
    }

    pbuf.update_all(buf.data());
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 4.6 + GLSL 460
    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1920, 1200, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = glewInit() != GLEW_OK;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    std::size_t num_points = 512 * 512;
    storage_buffer<float> samples{ make_sample_points(num_points) };

    auto variations = variation_table{ "variations.yaml" };
    auto flame_def = load_flame("flames/electricsheep.247.25757.flam3", variations);
    auto buffer_map = make_shader_buffer_map(flame_def);
    std::cout << buffer_map.dump(1) << std::endl;
    auto shader_src = replace_macro(read_file("shaders/flame.glsl"), "varsource", variations.compile_flame_xforms(flame_def, buffer_map));

    std::cout << shader_src << std::endl;

    auto cs = compute_shader(shader_src);
    auto particle_vf = vf_shader(read_file("shaders/particle_vert.glsl"), read_file("shaders/particle_frag.glsl"));
    auto tonemap_cs = compute_shader(read_file("shaders/tonemap.glsl"));
    auto quad_vf = vf_shader(read_file("shaders/quad_vert.glsl"), read_file("shaders/quad_frag.glsl"));
    auto density_cs = compute_shader(read_file("shaders/density.glsl"));

    storage_buffer<std::array<float, 4>> pos[2] = { {num_points},{num_points} };
    GLuint vao[2];
  
    glGenVertexArrays(2, vao);
    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, pos[1].name());
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(vao[1]);
    glBindBuffer(GL_ARRAY_BUFFER, pos[0].name());
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    storage_buffer<float> fp{ 256 };
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, fp.name());

    storage_buffer<std::array<float, 4>> palette{ 256, flame_def.palette.data() };
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, palette.name());

    auto atomic_init = std::vector<unsigned int>(flame_def.xforms.size(), 0);
    storage_buffer<unsigned int> atomic_counters{ atomic_init };
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, atomic_counters.name());

    int num_shuf_bufs = 512;
    auto shuf = make_shuffle_buffers(num_points, num_shuf_bufs);
    storage_buffer<std::uint32_t> shuf_buf{ shuf.size(), shuf.data() };
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, shuf_buf.name());
    shuf.clear();

    const int in_buf_pos = 0;
    const int out_buf_pos = 1;
    const int in_buf_col = 2;
    const int out_buf_col = 3;

    const std::size_t target_dims[2] = { 1280, 720 };

    texture<float> render_targets[2] = { {1280, 720}, {1280, 720} };
    frame_buffer fb;

    timer frame_timer;

    const float DEGREES_PER_SECOND = 45.0f;

    int warmup_passes = 16;
    int drawing_passes = 64;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        auto shuf_path = create_shuffle_buffer(num_shuf_bufs);
        std::unordered_map<std::string, float> perf_timer_results;

        float dt = float(frame_timer.time<timer::ms>()) / 1000.0f;
        frame_timer.reset();

        atomic_counters.update_all(atomic_init.data());
        auto shuf = shuf_path.begin();

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        ImGui::Begin("Passes");
        ImGui::InputInt("Warmup Passes", &warmup_passes, 2, 2);
        ImGui::InputInt("Drawing Passes", &drawing_passes, 2, 2);
        ImGui::End();

        ImGui::Begin("Flame Info"); {
            ImGui::DragFloat("Scale", &flame_def.scale, 1, 1, 1000);
            ImGui::DragFloat2("Center", flame_def.center.data(), .01, -5, 5);
            ImGui::Separator();
            ImGui::DragInt("Estimator Radius", &flame_def.estimator_radius, 0.005f, 0, 20);
            ImGui::DragInt("Estimator Min", &flame_def.estimator_min, 0.005f, 0, 20);
            ImGui::DragFloat("Estimator Curve", &flame_def.estimator_curve, .001, 0, 1);

            flame_def.for_each_xform([&](int idx, flame_xform& xform) {
                std::string hash = "##xform" + std::to_string(idx) + std::to_string((unsigned int)&xform);
                if (ImGui::CollapsingHeader(("xform " + std::to_string(idx)).c_str())) {
                    ImGui::DragFloat(("Weight" + hash).c_str(), &xform.weight, .001, -100, 100);
                    ImGui::DragFloat(("Color" + hash).c_str(), &xform.color, .0001, 0, 1);
                    ImGui::DragFloat(("Color Speed" + hash).c_str(), & xform.color_speed, .0001, 0, 1);
                    ImGui::Separator();
                    ImGui::DragFloat3(("X Affine (a,b,c)" + hash).c_str(), xform.affine.data(), .001, -3, 3);
                    ImGui::DragFloat3(("Y Affine (d,e,f)" + hash).c_str(), xform.affine.data() + 3, .001, -3, 3);

                    if (xform.post) {
                        ImGui::Separator();
                        ImGui::DragFloat3(("X Post Affine (a,b,c)" + hash).c_str(), xform.post.value().data(), .001, -3, 3);
                        ImGui::DragFloat3(("Y Post Affine (d,e,f)" + hash).c_str(), xform.post.value().data() + 3, .001, -3, 3);
                    }

                    for (auto& [name,v] : xform.variations) {
                        ImGui::Separator();
                        ImGui::DragFloat((name + hash).c_str(), &v, .001, -100, 100);
                        for (auto& p_name : variations.get_parameters_for_xform(name)) {
                            ImGui::DragFloat((p_name + hash).c_str(), &xform.var_param.at(p_name), .01, -20, 20);
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

        // Rendering
        int display_w, display_h;

        float rotation = DEGREES_PER_SECOND * dt;

        // Do rotation
        //for (auto& x : flame_def.xforms) {
        //    if (x.animate) x.affine = rotate_affine(x.affine, rotation);
        //}

        timer perf_timer;
        timer perf_timer_total;
        perf_timer.reset();
        copy_flame_data_to_buffer(flame_def, buffer_map, fp);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();
        perf_timer_results["copy data"] = perf_timer.time<timer::ns>();

        glUseProgram(cs.name());

        perf_timer.reset();
        // first pass, random read from sample points and flame pass
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, samples.name());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[0].name());
        cs.set_uniform<bool>("make_xform_dist", false);
        cs.set_uniform<bool>("random_read", true);
        cs.set_uniform<bool>("random_write", false);
        cs.set_uniform<bool>("first_run", true);
        cs.set_uniform<float>("rand_seed", 1356462.0f);
        cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
        glDispatchCompute(num_points / 128, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        glFinish();
        perf_timer_results["sample"] = perf_timer.time<timer::ns>();

        cs.set_uniform<bool>("random_read", true);
        cs.set_uniform<bool>("random_write", true);
        cs.set_uniform<bool>("first_run", false);

        perf_timer.reset();
        // warm up passes, random read and write
        for (int i = 0; i < warmup_passes; i++) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, pos[i & 1].name());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[!(i & 1)].name());
            cs.set_uniform<float>("rand_seed", 12512512.0f + i);
            cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
            cs.set_uniform<unsigned int>("shuf_buf_idx_out", *(shuf++));
            glDispatchCompute(num_points / 128, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }
        perf_timer_results["warm up"] = perf_timer.time<timer::ns>();

        fb.bind();
        frame_buffer::attach(render_targets[0].name());
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        //cs.set_uniform<bool>("random_read", true);
        //cs.set_uniform<bool>("random_write", false);

        perf_timer.reset();
        // drawing passes
        for (int i = 0; i < drawing_passes; i++) {
            {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, pos[i & 1].name());
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[!(i & 1)].name());
                cs.set_uniform<float>("rand_seed", 1312412.0f + i);
                cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
                glDispatchCompute(num_points / 128, 1, 1);
                glMemoryBarrier(GL_ALL_BARRIER_BITS);
//                glFinish();
            }
            perf_timer_results["draw calc"] += perf_timer.time<timer::ns>();

            glUseProgram(particle_vf.name());

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
            glUseProgram(cs.name());
        }
        glFinish();
        perf_timer_results["draw calc"] = perf_timer.time<timer::ns>();
        fb.unbind();
        glDisable(GL_BLEND);
        
        perf_timer.reset();
        {
            glUseProgram(density_cs.name());
            density_cs.set_uniform<int>("estimator_radius", flame_def.estimator_radius);
            density_cs.set_uniform<int>("estimator_min", flame_def.estimator_min);
            density_cs.set_uniform<float>("estimator_curve", flame_def.estimator_curve);
            density_cs.set_uniform<int>("in_hist", 0);
            density_cs.set_uniform<int>("out_hist", 1);
            glBindImageTexture(0, render_targets[0].name(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, render_targets[1].name(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glDispatchCompute(1280, 720, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }
        perf_timer_results["density"] = perf_timer.time<timer::ns>();

        perf_timer.reset();
        {
            glUseProgram(tonemap_cs.name());
            glBindImageTexture(0, render_targets[1].name(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            tonemap_cs.set_uniform<int>("image", 0);
            glDispatchCompute(1280, 720, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }
        perf_timer_results["tonemap"] = perf_timer.time<timer::ns>();

        glfwGetFramebufferSize(window, &display_w, &display_h);
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
            SHOW_PERF_TIMER("density");
            SHOW_PERF_TIMER("tonemap");
        }ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
   

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
