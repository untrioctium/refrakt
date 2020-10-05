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