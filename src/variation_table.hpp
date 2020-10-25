#pragma once

#include <set>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <nlohmann/json.hpp>
#include "flame.hpp"

struct variation_definition {
    std::string source;
    std::string result;
    std::vector<std::string> param;
    std::set<std::string> flags;

};

struct flame_compiler {
public:
    flame_compiler();

    void print_variations() {
        for (auto& [k, v] : common_) {
            std::cout << k << ": " << v << std::endl;
        }
        for (auto& [k, v] : vars_) {
            std::cout << k << ":" << std::endl;

            std::cout << "\tparam:";
            for (auto& d : v.param) std::cout << " " << d;
            std::cout << std::endl;

            std::cout << v.source << std::endl;
        }
    }

    bool is_param(const std::string& name) const { return param_owners_.contains(name); }
    bool is_variation(const std::string& name) const { return vars_.contains(name); }
    bool is_common(const std::string& name) const { return common_.contains(name); }

    std::string param_owner(const std::string& param) const { return param_owners_.at(param); };
    const std::vector<std::string>& get_parameters_for_variation(const std::string& name) const { return vars_.at(name).param; }

    const variation_definition& variation(const std::string& name) const { return vars_.at(name); }
    const auto& variations() const { return vars_; }

    std::string common(const std::string& name) const { return common_.at(name); }

    std::string compile_flame_xforms(const flame& f) const;
private:

    std::string make_xform_src(const flame_xform& xform, const nlohmann::json& buf_map, const std::string& xform_name);

    std::map<std::string, std::string> common_;
    std::map<std::string, variation_definition> vars_;
    std::map<std::string, std::string> param_owners_;

};