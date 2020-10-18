#pragma once
#include <array>
#include "buffer_objects.hpp"
#include "shaders.hpp"

class histogram_plotter {
public:
	using sample_buffer_t = storage_buffer<std::array<float, 4>>;
	using palette_buffer_t = storage_buffer < std::array<std::array<float, 4>, 256>>;
	using histogram_buffer_t = texture<float>;

	virtual ~histogram_plotter() = 0;

	virtual std::size_t plot_to_image(const sample_buffer_t& samples, const palette_buffer_t& palette, histogram_buffer_t& image) = 0;

};

class rasterizing_plotter: public histogram_plotter {
public:
	rasterizing_plotter(): particle_shader_(read_file("shaders/particle_vert.glsl"), read_file("shaders/particle_frag.glsl")) {}

private:

	vf_shader particle_shader_;

};

class unsafe_accumulating_plotter : public histogram_plotter {

};