#include "compute_shader.hpp"
#include <iostream>

compute_shader::compute_shader(const std::string& src)
{
    GLuint flame_prog = glCreateShader(GL_COMPUTE_SHADER);
    const char* csrc = src.c_str();
    glShaderSource(flame_prog, 1, &csrc, nullptr);
    glCompileShader(flame_prog);
    // print compile errors if any
    int success;
    char infoLog[2048];
    glGetShaderiv(flame_prog, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(flame_prog, 2048, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROG::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint flame_shader = glCreateProgram();
    glAttachShader(flame_shader, flame_prog);
    glLinkProgram(flame_shader);

    glGetProgramiv(flame_shader, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(flame_shader, 2048, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    shader_ = flame_shader;
}
