#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace buffer_cache {

	class buffer_type;

	class buffer_group {
	public:
		buffer_group(const std::string& type, const std::string& group) : type_(type), group_(group) {
			path_ = "cache/" + type + "/" + group + "/";
			std::filesystem::create_directories(path_);
		}

	//	std::size_t number_available() const;

		template<typename T>
		void write_buffer(const std::vector<T>& buf, std::string filename = {}) const {
			write_buffer_impl((const char*)buf.data(), buf.size() * sizeof(T), sizeof(T), filename);
		}

		template<typename T>
		std::vector<T> read_buffer(const std::string& buf_name) {
			std::vector<T> result;

			std::ifstream cache_file{};
			cache_file.open(path_ + buf_name + ".bin", std::ios::binary | std::ios::in);
			std::size_t total_size;
			cache_file.read((char*) &total_size, sizeof(std::size_t));

			result.resize(total_size / sizeof(T));
			cache_file.read((char*)result.data(), total_size);
			cache_file.close();

			return result;
		}

		std::vector<std::string> cached_buffers() const {
			std::vector<std::string> result;
			for (auto& f : std::filesystem::directory_iterator(path_)) {
				if (f.is_regular_file() && f.path().extension() == ".bin") {
					result.push_back(f.path().stem().string());
				}
			}

			return result;
		}

	private:
		
		std::string path_;
		std::string type_;
		std::string group_;

		void write_buffer_impl(const char* buf, std::size_t total_size, std::size_t elem_size, std::string filename = {}) const;
	};

	class buffer_type {
		buffer_group group(const std::string& group) {

		}

		bool has_group(const std::string& group) {

		}


	};

	buffer_type type(const std::string& t);
}