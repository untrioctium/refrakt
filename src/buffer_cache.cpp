#include "buffer_cache.hpp"

#include <xxhash.h>
#include <fstream>
#include <fmt/core.h>

void buffer_cache::buffer_group::write_buffer_impl(const char* buf, std::size_t total_size, std::size_t elem_size, std::string filename) const
{
	if (filename.size() == 0) {
		auto hash = XXH3_128bits(buf, total_size);
		filename = fmt::format("{:0>16X}{:0>16X}", hash.high64, hash.low64);
	}

	filename = path_ + filename + ".bin";

	std::ofstream cache_file{};
	cache_file.open(filename, std::ios::binary | std::ios::out);
	cache_file.write((const char*)&total_size, sizeof(std::size_t));
	cache_file.write(buf, total_size);
	cache_file.close();
}
