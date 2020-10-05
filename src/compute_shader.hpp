#pragma once

#include <GL/glew.h>
#include <string>

class compute_shader {
public:
	compute_shader(const std::string& src);

	template<typename T> void set_uniform(const std::string& name, T value) {
		auto location = glGetUniformLocation(shader_, name.c_str());

		if constexpr (std::is_same_v<T, unsigned int>) glUniform1ui(location, value);
		else if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int>) glUniform1i(location, (int)value);
		else if constexpr (std::is_same_v<T, float>) glUniform1f(location, value);
	}

	GLuint name() { return shader_; }
private:
	GLuint shader_;
};