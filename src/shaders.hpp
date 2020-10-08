#pragma once

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <iostream>
#include "util.hpp"

class shader_base {
public:
    template<typename T> void set_uniform(const std::string& name, T value) {
        auto location = glGetUniformLocation(shader_, name.c_str());

        // basic types
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int>) glUniform1i(location, (int)value);
        else if constexpr (std::is_same_v<T, unsigned int>) glUniform1ui(location, value);
        else if constexpr (std::is_same_v<T, float>) glUniform1f(location, value);

        // vecs
        else if constexpr (std::is_same_v <T, glm::vec2>) glUniform2fv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::vec3>) glUniform3fv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::vec4>) glUniform4fv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::uvec2>) glUniform2uiv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::uvec3>) glUniform3uiv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::uvec4>) glUniform4uiv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::ivec2>) glUniform2iv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::ivec3>) glUniform3iv(location, 1, glm::value_ptr(value));
        else if constexpr (std::is_same_v <T, glm::ivec4>) glUniform4iv(location, 1, glm::value_ptr(value));

        // mats
        else if constexpr (std::is_same_v<T, glm::mat4>) glUniformMatrix4fv(location, 1, 0, glm::value_ptr(value));

    }

    GLuint name() { return shader_; }
    void bind() { glUseProgram(shader_); }
    void unbind() { glUseProgram(0); }

protected:

    static std::pair<bool, GLuint> compile_source(GLuint type, const std::string& src) {
        GLuint prog = glCreateShader(type);

        std::string src_with_includes = src;
        auto macros = find_macros(src_with_includes);

        for (auto& macro : macros) {
            if (macro.find("include_") == 0) {
                std::string include_name = macro.substr(macro.find("_") + 1);
                src_with_includes = replace_macro(src_with_includes, macro, get_include_source(include_name));
            }
        }

        const char* csrc = src_with_includes.c_str();
        glShaderSource(prog, 1, &csrc, nullptr);
        glCompileShader(prog);
        int success;
        glGetShaderiv(prog, GL_COMPILE_STATUS, &success);
        return { success, prog };
    }

    template<typename... Progs>
    bool link_shader(Progs... progs) {
        shader_ = glCreateProgram();

        (glAttachShader(shader_, progs), ...);
        glLinkProgram(shader_);
        int success;
        glGetProgramiv(shader_, GL_LINK_STATUS, &success);
        return success;
    }

    static std::string get_source_error(GLuint prog) {
        static thread_local char info_log[2048];
        glGetShaderInfoLog(prog, 2048, NULL, info_log);
        return info_log;
    }


    std::string get_link_error() {
        static thread_local char info_log[2048];
        glGetProgramInfoLog(shader_, 2048, NULL, info_log);
        return info_log;
    }

    static std::string get_include_source(const std::string& include_name) {
        return read_file("shaders/include/" + include_name + ".glsl");
    }

	GLuint shader_;
};

class compute_shader : public shader_base {
public:
    compute_shader(const std::string& src) {
        auto [compiled, prog] = shader_base::compile_source(GL_COMPUTE_SHADER, src);

        if (!compiled) {
            std::cout << "COMPUTE SHADER COMPILE ERROR:\n" << get_source_error(prog) << std::endl;
        }

        if (!link_shader(prog)) {
            std::cout << "COMPUTE SHADER LINK ERROR:\n" << get_link_error() << std::endl;
        }
    }
};

class vf_shader : public shader_base {
public:
    vf_shader(const std::string& vsrc, const std::string& fsrc) {
        auto [vcompiled, vprog] = compile_source(GL_VERTEX_SHADER, vsrc);

        if (!vcompiled) {
            std::cout << "VERTEX SHADER COMPILE ERROR:\n" << get_source_error(vprog) << std::endl;
        }

        auto [fcompiled, fprog] = compile_source(GL_FRAGMENT_SHADER, fsrc);

        if (!fcompiled) {
            std::cout << "FRAGMENT SHADER COMPILE ERROR:\n" << get_source_error(fprog) << std::endl;
        }

        if (!link_shader(vprog, fprog)) {
            std::cout << "VF SHADER LINK ERROR:\n" << get_link_error() << std::endl;
        }
    }
};