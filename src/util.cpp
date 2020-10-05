#include <regex>
#include <fstream>
#include <streambuf>
#include "util.hpp"

std::string replace_macro(const std::string& str, const std::string& name, const std::string& value)
{
  return std::regex_replace(str, std::regex("\\$" + name + "([^a-zA-Z0-9_])"), value + "$1");
}

std::set<std::string> find_macros(const std::string& str)
{
    std::set<std::string> result{};

    static const std::regex macro_re(R"re(\$([a-z0-9_]+))re");
    for (auto r = std::sregex_iterator(str.begin(), str.end(), macro_re);
        r != std::sregex_iterator{}; r++)
    {
        result.insert((*r)[1]);
    }

    return result;
}

std::string read_file(const std::string& path)
{
    std::ifstream f(path);
    return std::string( std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() );
}
