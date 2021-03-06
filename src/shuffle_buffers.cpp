#include <vector>
#include <random>
#include <thread>
#include <algorithm>
#include <string>

#include "buffer_cache.hpp"

std::vector<std::uint32_t> create_shuffle_buffer(std::uint32_t size) {
    static thread_local std::mt19937_64 generator{ (std::uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count()};

    std::vector<std::uint32_t> vec{};
    vec.reserve(size);

    for (std::uint32_t i = 0; i < size; i++) {
        vec.push_back(i);
    }

    std::shuffle(vec.begin(), vec.end(), generator);
    return vec;
}

void make_shuffle_buffers(std::uint32_t size, std::size_t count) {
    std::vector<std::vector<std::uint32_t>> buffers{ count };
    std::vector<std::thread> threads;

    std::vector<std::uint32_t> base_vec;
    base_vec.resize(size);

    for (std::uint32_t i = 0; i < size; i++) {
        base_vec[i] = i;
    }

    const int batch_size = 8;
    int total_written = 0;

    for (int batched = 0; batched < (count / batch_size); batched++) {
        for (std::size_t i = 0; i < batch_size && total_written < count; i++) {
            threads.emplace_back(std::thread([size, &base_vec]() {
                static thread_local std::mt19937_64 generator{ (std::uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count() };
                std::vector<std::uint32_t> vec = std::vector<std::uint32_t>{ base_vec };
                std::shuffle(vec.begin(), vec.end(), generator);

                buffer_cache::buffer_group("shuffle", std::to_string(size)).write_buffer(vec);

                }));
            total_written++;
        }
        for (auto& t : threads) t.join();
        threads.clear();
    }
}
