#include <vector>
#include <random>
#include <thread>
#include <algorithm>

std::vector<std::uint32_t> create_shuffle_buffer(std::uint32_t size) {
    static thread_local std::mt19937 generator{};

    std::vector<std::uint32_t> vec{};
    vec.reserve(size);

    for (std::uint32_t i = 0; i < size; i++) {
        vec.push_back(i);
    }

    std::shuffle(vec.begin(), vec.end(), generator);
    return vec;
}

auto make_shuffle_buffers(std::uint32_t size, std::size_t count) -> std::vector<std::uint32_t> {
    std::vector<std::vector<std::uint32_t>> buffers{ count };
    std::vector<std::thread> threads;

    std::vector<std::uint32_t> base_vec;
    base_vec.resize(size);

    for (std::uint32_t i = 0; i < size; i++) {
        base_vec[i] = i;
    }

    for (std::size_t i = 0; i < count; i++) {
        threads.emplace_back(std::thread([size, &base_vec](std::vector<std::uint32_t>& vec) {
            static thread_local std::mt19937_64 generator{ (std::uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count() };
            vec = std::vector<std::uint32_t>{ base_vec };

            std::shuffle(vec.begin(), vec.end(), generator);
            }, std::ref(buffers[i])));
    }

    for (auto& t : threads) t.join();
    
    std::vector<std::uint32_t> result{};
    for (auto& b : buffers) {
        result.insert(result.end(), b.begin(), b.end());
    }
    return result;

}
