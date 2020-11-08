#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>

#include <array>
#include <vector>
#include <iostream>
#include <optional>
#include <set>
#include <map>
#include <pugixml.hpp>
#include <fmt/core.h>
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


inline void Style()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    /// 0 = FLAT APPEARENCE
    /// 1 = MORE "3D" LOOK
    int is3D = 1;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    style.PopupRounding = 3;

    style.WindowPadding = ImVec2(4, 4);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 2);

    style.ScrollbarSize = 18;

    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = is3D;

    style.WindowRounding = 3;
    style.ChildRounding = 3;
    style.FrameRounding = 3;
    style.ScrollbarRounding = 2;
    style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK 
    style.TabBorderSize = is3D;
    style.TabRounding = 3;

    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif
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

    if (!gladLoadGL()) return { rs, 3 };

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMono-Regular.ttf", 13);
    //io.Fonts->GetTexDataAsRGBA32();

    // Setup Dear ImGui style
    Style();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(rs.window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    return { rs, 0 };
}

int main(int, char**)
{
    auto [gl, error] = init_gl(1920, 1080, false);

    if (error) {
        std::cout << "Render init exited with code: " << error << std::endl;
        return error;
    }

    std::random_device rd;
    std::mt19937 generator{rd()};
    std::uniform_real_distribution<float> dist(19232581.235235, 91212584.1241251);

    const std::size_t image_dims[2] = { 640, 360 };
    const std::size_t supersampling = 2;
    const std::size_t target_dims[2] = { image_dims[0] * supersampling, image_dims[1] * supersampling };

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    flame::set_sim_parameters(1024 * 1024, 1024, 1024);

    auto variations = flame_compiler{};
    auto flame_def = flame::load_flame("flames/electricsheep.247.11256.flam3", variations);

    auto tonemap_cs = compute_shader(read_file("shaders/tonemap.glsl"));
    auto density_vf = vf_shader(read_file("shaders/density_vert.glsl"), read_file("shaders/density_frag.glsl"));

    GLuint vao;
  
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    storage_buffer<std::array<float,4>> bins{ target_dims[0] * target_dims[1] };
    bins.zero_out();//glClearNamedBufferData(bins.name(), GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, bins.name());

    texture<float> render_targets[2] = { {target_dims[0], target_dims[1]}, {target_dims[0], target_dims[1]} };

    timer frame_timer;

    const float DEGREES_PER_SECOND = 18.0f;

    bool clear_parts = true;
    bool needs_clear = true;
    float scale_constant = 4;

    moving_average<10> fps;
    moving_average<10> pps;
    float pps_final = 0.0;

    bool animate = false;
    bool do_step = false;

    float tss_width = 1.2 / 60.0;

    long long accumulated = 0;
    int needed_quality = 2000;

    bool advance_frame = false;
    int rendered_frames = 0;
    int total_frames_needed = 300;

    //flame_def->scale *= float(target_dims[1]) / float(flame_def->size[1]);

    frame_buffer fb;

    std::vector<std::string> avail_flames;
    for (auto& f : std::filesystem::directory_iterator("flames")) {
        if (f.is_regular_file() && f.path().extension() == ".flam3") {
            avail_flames.push_back(f.path().stem().string());
        }
    }

    auto xform_atomic_counters = storage_buffer<unsigned int>(64);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, xform_atomic_counters.name());
    //auto xform_atomic_init = std::vector<unsigned int>{64, 0};
    std::vector<unsigned long long> running_xform_counts;
    running_xform_counts.resize(64);

    int warmup_passes = 16;
    int drawing_passes = 128;

    // Main loop
    while (!glfwWindowShouldClose(gl.window))
    {
        std::unordered_map<std::string, float> perf_timer_results;

        float dt = float(frame_timer.time<timer::ms>()) / 1000.0f;
        float fps_avg = fps.add(dt);
        frame_timer.reset();

        //xform_atomic_counters.update_all(xform_atomic_init.data());
        xform_atomic_counters.zero_out();
        //flame_atomic_counters.update_all(flame_stats_init.data());
        //auto shuf = shuf_path.begin();

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        ImGui::ShowDemoWindow();

        ImGui::Begin("Load Flame");
        for (auto& path : avail_flames) {
            if (ImGui::Button(path.c_str())) {
                auto new_flame = flame::load_flame("flames/" + path + ".flam3", variations);
                if (new_flame) {
                    flame_def.swap(new_flame);
                    flame_def->print_debug_info();
                    bins.zero_out();
                    running_xform_counts = std::vector<unsigned long long>(64, 0);
                    accumulated = 0;
                }
            }
        }
        ImGui::End();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) { ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Settings")) {
                    //ImGui::OpenPopup("Settings");
                }
                if (ImGui::BeginPopupModal("Settings")) {
                    if (ImGui::Button("Close")) {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
                ImGui::EndMenu(); 
            }
            if (ImGui::BeginMenu("Window")) { ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Help")) { ImGui::EndMenu(); }
            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Sim Configuration");
        needs_clear |= ImGui::InputInt("Warmup Passes", &warmup_passes, 2, 2);
        needs_clear |= ImGui::InputInt("Drawing Passes", &drawing_passes, 2, 2);
        //needs_clear |= ImGui::InputInt("Drawing ms limit", &si.abort_after_ms, 1, 1);
        //needs_clear |= ImGui::Checkbox("Use random xform selection", &si.use_random_xform_selection);
        needs_clear |= ImGui::DragFloat("Temporal Sample Width", &tss_width, .01, 0, .25);
        needs_clear |= ImGui::InputInt("Quality Target", (int*) &needed_quality, 10, 100);
        do_step = ImGui::Button("Advance 1/60s");
        ImGui::End();

        ImGui::Begin("Flame Info"); {
            needs_clear |= ImGui::DragFloat("Scale", &flame_def->scale, 1, 1, 10000);
            needs_clear |= ImGui::DragFloat2("Center", flame_def->center.data(), .01, -5, 5);
            needs_clear |= ImGui::DragFloat("Rotate", &flame_def->rotate, 1.0, -360.0, 360.0);
            needs_clear |= ImGui::Checkbox("Animate", &animate);
            ImGui::InputFloat("Scale Constant", &scale_constant, 1, 5);
            ImGui::Separator();
            ImGui::DragInt("Estimator Radius", &flame_def->estimator_radius, 0.005f, 0, 20);
            ImGui::DragInt("Estimator Min", &flame_def->estimator_min, 0.005f, 0, 20);
            ImGui::DragFloat("Estimator Curve", &flame_def->estimator_curve, .001, 0, 1);

            flame_def->for_each_xform([&](int idx, flame_xform& xform) {
                std::string hash = "##xform" + std::to_string(idx) + std::to_string((std::intptr_t)&xform);
                if (ImGui::CollapsingHeader(("xform " + std::to_string(idx)).c_str())) {
                    needs_clear |= ImGui::DragFloat(("Weight" + hash).c_str(), &xform.weight, .001, -100, 100);
                    needs_clear |= ImGui::DragFloat(("Color" + hash).c_str(), &xform.color, .0001, 0, 1);
                    needs_clear |= ImGui::DragFloat(("Color Speed" + hash).c_str(), & xform.color_speed, .0001, 0, 1);
                    needs_clear |= ImGui::DragFloat(("Opacity" + hash).c_str(), &xform.opacity, .0001, 0, 1);
                    needs_clear |= ImGui::DragFloat(("Rotation Frequency" + hash).c_str(), &xform.rotation_frequency, .01f, 0.0f, 5.0f);
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
                    auto& c = flame_def->palette.at(i);
                    ImGui::ColorEdit4(std::to_string(i).c_str(), c.data());
                }
            }
        } ImGui::End(); //Flame Info*/

        float rotation = DEGREES_PER_SECOND * dt;

        //Do rotation
        if (animate || advance_frame) {
            for (auto& x : flame_def->xforms) {
                if (x.rotation_frequency != 0.0) x.affine = flame::rotate_affine(x.affine, rotation * x.rotation_frequency);
            }
            if (flame_def->final_xform) {
                auto& fx = flame_def->final_xform.value();
                if(fx.rotation_frequency != 0.0) fx.affine = flame::rotate_affine(fx.affine, rotation * fx.rotation_frequency);
            }
            needs_clear = true;
        }

        if (needs_clear) {
            //glClearNamedBufferData(bins.name(), GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
            accumulated = 0;
            for (auto& c : running_xform_counts) c = 0;
        }

        if (flame_def->needs_warmup() || needs_clear)
        {
            flame_def->warmup(warmup_passes, tss_width);
            bins.zero_out();
            needs_clear = false;
            accumulated = 0;
        }
        auto binned = 0;
        if(accumulated < needed_quality * target_dims[0] * target_dims[1])
            binned = flame_def->draw_to_bins(bins, target_dims[0], drawing_passes);

        //cs.set_uniform<bool>("random_read", true);
        //cs.set_uniform<bool>("random_write", false);

        /*auto win_min = glm::vec2(
            flame_def.center[0] - float(target_dims[0]) / flame_def.scale / 2.0,
            flame_def.center[1] - float(target_dims[1]) / flame_def.scale / 2.0);

        flame_xform::affine_t base{ 1, 0, 0, 1, 0, 0 };

        base = translate_affine(base, { target_dims[0] / 2.0f, target_dims[1] / 2.0f });
        base = scale_affine(base, flame_def.scale);
        base = rotate_affine(base, flame_def.rotate);
        base = translate_affine(base, { -flame_def.center[0], -flame_def.center[1] });

        particle_vf.bind();

        float half_width = float(target_dims[0]) / flame_def.scale / 2.0;
        float half_height = half_width * float(target_dims[1]) / float(target_dims[0]);

        auto proj = glm::ortho(0.0f, float(target_dims[0]), float(target_dims[1]), 0.0f, -1.0f, 1.0f);

        particle_vf.set_uniform<glm::mat4>("projection", proj);

        particle_vf.set_uniform<std::array<float, 6>>("ss_affine", base);

        cs.bind();
        cs.set_uniform<bool>("random_read", si.draw_random[0]);
        cs.set_uniform<bool>("random_write", si.draw_random[1]);
        cs.set_uniform<bool>("do_draw", true);
        cs.set_uniform<glm::uvec2>("bin_dims", glm::uvec2(target_dims[0], target_dims[1]));
        cs.set_uniform<std::array<float, 6>>("ss_affine", base);

        perf_timer.reset();
        // drawing passes
        if (accumulated < needed_quality * target_dims[0] * target_dims[1]) {
            int total_passes = 0;
            for (int i = 0; i < si.drawing_passes; i++) {
                {
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, in_buf_pos, pos[i & 1].name());
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, out_buf_pos, pos[!(i & 1)].name());
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, pos[2 + (i & 1)].name());
                    cs.set_uniform<float>("rand_seed", dist(generator));

                    if (si.draw_random[0]) cs.set_uniform<unsigned int>("shuf_buf_idx_in", *(shuf++));
                    if (si.draw_random[1]) cs.set_uniform<unsigned int>("shuf_buf_idx_out", *(shuf++));

                    glDispatchCompute(si.points_per_ts() / si.block_width, si.num_temporal_samples, 1);
                    glMemoryBarrier(GL_ALL_BARRIER_BITS);
                    glFinish();
                }
                //perf_timer_results["draw calc"] += perf_timer.time<timer::ns>();

                //glUseProgram(particle_vf.name());

               
                {
                    //glBindVertexArray(vao[!(i & 1)]);
                    //glDrawArrays(GL_POINTS, 0, si.num_points);
    //                glMemoryBarrier(GL_ALL_BARRIER_BITS);
                }
                //glUseProgram(cs.name());

                total_passes++;
                if (i % 2 == 0 && perf_timer.time<timer::ms>() > si.abort_after_ms) break;
            }
            float drawn_parts = float(total_passes) * si.num_points;
            pps_final = pps.add(drawn_parts / dt);

            

        }
        perf_timer_results["draw calc"] = perf_timer.time<timer::ns>();
        
        perf_timer.reset();
        */

        fb.bind();
        frame_buffer::attach(render_targets[0].name());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glViewport(0, 0, target_dims[0], target_dims[1]);

        {
            auto proj = glm::ortho(0.0f, float(target_dims[0]), float(target_dims[1]), 0.0f, -1.0f, 1.0f);
            glUseProgram(density_vf.name());
            if (flame_def->estimator_radius > 100) flame_def->estimator_radius = 100;
            density_vf.set_uniform<int>("estimator_radius", flame_def->estimator_radius);
            density_vf.set_uniform<int>("estimator_min", flame_def->estimator_min);
            density_vf.set_uniform<float>("estimator_curve", flame_def->estimator_curve);
            density_vf.set_uniform<int>("row_width", target_dims[0]);
            density_vf.set_uniform<float>("scale_constant", 1.0 / pow(10.0, scale_constant));
            density_vf.set_uniform<float>("gamma", flame_def->gamma);
            density_vf.set_uniform<float>("brightness", flame_def->brightness);
            density_vf.set_uniform<float>("vibrancy", flame_def->vibrancy);
            density_vf.set_uniform<glm::mat4>("projection", proj);
            glDrawArrays(GL_POINTS, 0, target_dims[0] * target_dims[1]);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }
        //perf_timer_results["density"] = perf_timer.time<timer::ns>();

        fb.unbind();
        glDisable(GL_BLEND);

        //perf_timer.reset();
        {
            glUseProgram(tonemap_cs.name());
            glBindImageTexture(0, render_targets[0].name(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(1, render_targets[1].name(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            tonemap_cs.set_uniform<int>("image_in", 0);
            tonemap_cs.set_uniform<int>("image_out", 1);
            tonemap_cs.set_uniform<float>("scale_constant", 1.0/pow(10.0, scale_constant));
            tonemap_cs.set_uniform<float>("gamma", flame_def->gamma);
            tonemap_cs.set_uniform<float>("brightness", flame_def->brightness);
            tonemap_cs.set_uniform<float>("vibrancy", flame_def->vibrancy);
            glDispatchCompute(target_dims[0]/8, target_dims[1]/8, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glFinish();
        }
        //perf_timer_results["tonemap"] = perf_timer.time<timer::ns>();

        //glfwGetFramebufferSize(gl.window, &display_w, &display_h);
        //glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        

        /*glUseProgram(quad_vf.name());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, render_targets[1].name());
        quad_vf.set_uniform<int>("tex", 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(0);*/
        
        ImGui::Begin("Flame");
        ImVec2 vMin = ImGui::GetWindowContentRegionMin();
        ImVec2 vMax = ImGui::GetWindowContentRegionMax();
        float width = vMax.x - vMin.x;
        float height = width * float(image_dims[1]) / float(image_dims[0]);
        ImGui::Image((void*)render_targets[1].name(), ImVec2{ width, height });
        ImGui::End();
        /*
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

        */

        accumulated += binned;

        GLint total_mem_kb = 0;
        //glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_mem_kb);

        GLint available_mem = 0;
        //glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available_mem);
        
        ImGui::Begin("Debug");
        ImGui::Text("FPS Avg: %f", 1.0 / fps_avg);
        ImGui::Text("Mem Avail: %.3fGB", available_mem / (1024.0 * 1024.0));
        ImGui::Text("Mem Used: %.3fGB", (total_mem_kb - available_mem)/(1024.0 * 1024.0));
        ImGui::Text("Parts per second: %.1fM", pps_final / 1'000'000.0f);
        ImGui::Text("Parts per frame: %.1fM", float(128)* 1024 / 1'000'000.0f);
        ImGui::Text("Binned this frame: %.1fM", binned / 1'000'000.0f);
        ImGui::Text("Total binned this image: %.3fB", accumulated / 1'000'000'000.0f );
        ImGui::Text("Image Quality: %.2f%%", float(accumulated) / float(needed_quality * target_dims[0] * target_dims[1]) * 100.0);
        if (ImGui::Button("Screenshot")) {
            auto pixels = render_targets[1].get_pixels();
            stbi_write_png("screenshot.png", render_targets[1].width(), render_targets[1].height(), 4, pixels.data(), 0);
        }

        float xform_sum = 0.0;
        unsigned long long total_hit = 0;
        std::vector<unsigned int> current_xform_counts(64, 0);
        for (auto& xform : flame_def->xforms) xform_sum += xform.weight;

        for (int i = 0; i < flame_def->xforms.size(); i++) {
            current_xform_counts[i] = xform_atomic_counters.get_one(i);
            running_xform_counts[i] += current_xform_counts[i];
            total_hit += running_xform_counts[i];
        }

        for (int i = 0; i < flame_def->xforms.size(); i++) {
            float expected = flame_def->xforms[i].weight / xform_sum;
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
