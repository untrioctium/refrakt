#include <string>
#include <set>
#include <sstream>

std::string replace_macro(const std::string& str, const std::string& macro, const std::string& value);

std::set<std::string> find_macros(const std::string& str);

std::string read_file(const std::string& path);

template<typename T, std::size_t Size, typename Conv>
std::array<T, Size> parse_strings(const std::string&& in, Conv&& cv) {
    std::array<T, Size> ret{};
    std::istringstream ss{ in };

    for (int i = 0; i < Size && ss; i++) {
        std::string v;
        ss >> v;
        if (!v.size()) break;
        ret[i] = cv(v);
    }

    return ret;
}

template <std::size_t Samples>
class moving_average {
public:

    template<typename T>
    float add(T v) {
        head++;
        if (head == Samples) head = 0;
        samples_[head] = static_cast<float>(v);

        float accum = 0.0;
        for (auto& v : samples_) accum += v;
        return accum / float(Samples);
    }

private:
    std::size_t head = 0;
    std::array<float, Samples> samples_ = { 0.0 };
};