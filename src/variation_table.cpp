#include <yaml-cpp/yaml.h>
#include <regex>
#include <stack>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/core.h>

#include "util.hpp"
#include "variation_table.hpp"

using optimizer_pred_t = std::function<bool(const flame_xform&)>;
using optimizer_t = std::function<std::pair<bool, std::string>(const flame_xform&, const nlohmann::json&)>;

std::string make_param_str(const nlohmann::json& v) { return "fp[" + std::to_string(v.get<int>()) + "]"; }

void resolve_affine(std::string& src, const nlohmann::json& buf_map) {
    // resolve affine
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 2; r++) {
            std::string name = "c" + std::to_string(c) + std::to_string(r);
            int index = c * 2 + r;
            src = replace_macro(src, name, make_param_str(buf_map["affine"][index]));
        }
    }
}

void resolve_post(std::string& src, const nlohmann::json& buf_map) {
    // resolve affine
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 2; r++) {
            std::string name = "p" + std::to_string(c) + std::to_string(r);
            int index = c * 2 + r;
            src = replace_macro(src, name, make_param_str(buf_map["post"][index]));
        }
    }
}

#define OPTIMIZER_PRED [](const flame_xform& x) -> bool
#define OPTIMIZER_BODY [](const flame_xform& x, const nlohmann::json& xmap) -> std::pair<bool, std::string>

auto& get_optimizers() {

    static thread_local const std::vector<std::pair<optimizer_pred_t, optimizer_t>> optimizers
    {
        // linear optimizer
        // decays an xform with no post and only a linear function to an inline affine
        {
            OPTIMIZER_PRED {
                return !x.post && x.variations.size() == 1 && (*x.variations.begin()).first == "linear"; 
            },
            OPTIMIZER_BODY {
                static const std::string base = "$weight * vec2(fma($c00, v.x, fma($c10, v.y, $c20)), fma($c01, v.x, fma($c11, v.y, $c21)))";
                std::string src = replace_macro(base, "weight", xmap["variations"]["linear"]);
                resolve_affine(src, xmap);
                return {true, src};
            }
        },

        // default "optimizer"
        // just spits back the typical xform source
        {
            OPTIMIZER_PRED { return true; },
            OPTIMIZER_BODY { return {false, "hello"}; }
        }
    };

    return optimizers;
}

#undef OPTIMIZER_PRED
#undef OPTIMIZER_BODY

void default_replace_macros(std::string& str) {
    // TODO: maybe not do this here
    str = replace_macro(str, "x", "v.x");
    str = replace_macro(str, "y", "v.y");
    str = replace_macro(str, "v", "v");
    str = replace_macro(str, "result", "result");
}

variation_table::variation_table(const std::string& def_path) {
    auto defs = YAML::LoadFile(def_path);
    auto variations = defs["variations"];

    for (auto it = variations.begin(); it != variations.end(); it++) {
        auto name = it->first.as<std::string>();
        auto src = it->second.as<YAML::Node>()["src"].as<std::string>();
        default_replace_macros(src);
        auto param = it->second.as<YAML::Node>()["param"];
        auto& var = vars_[name];
        var.source = src;

        for (auto it = param.begin(); it != param.end(); it++) {
            auto pname = it->first.as<std::string>();
            var.param.push_back(pname);
            param_owners_[pname] = name;
        }
    }

    auto common = defs["common"];
    for (auto it = common.begin(); it != common.end(); it++) {
        auto name = it->first.as<std::string>();
        auto src = it->second.as<std::string>();
        default_replace_macros(src);
        common_[name] = src;
    }
}

using adj_desc_t = std::map<std::string, std::set<std::string>>;

void order_recurse(const std::string& vertex, const adj_desc_t& adj, std::map<std::string, bool>& visited, std::stack<std::string>& stack) {
    visited[vertex] = true;

    for (auto con : adj.at(vertex)) {
        if (!visited[con])
            order_recurse(con, adj, visited, stack);
    }

    stack.push(vertex);
}

std::stack<std::string> get_ordering(const adj_desc_t& adj) {
    std::stack<std::string> ordering;
    std::map<std::string, bool> visited;
    for (auto& [k, v] : adj) visited[k] = false;

    for (auto& [v, a] : adj) {
        if (!visited[v])
            order_recurse(v, adj, visited, ordering);
    }

    return ordering;
}

std::string variation_table::make_xform_src(const flame_xform& xform, const nlohmann::json& xform_map, const std::string& xform_name) {
    std::string func_name = "apply_xform_" + xform_name;
    std::string xform_result{};

    // resolve variations and weights
    for (auto& [var_name, var] : xform.variations) {
        std::string var_src{};
        var_src += "// variation: " + var_name + "\n";
        var_src += replace_macro(vars_[var_name].source, "weight", make_param_str(xform_map["variations"][var_name])) + "\n";
        if (var_name == "pre_blur") xform_result = var_src + xform_result; else xform_result += var_src;
    }

    // resolve parameters
    for (auto& [p_name, val] : xform.var_param) {
        xform_result = replace_macro(xform_result, p_name, make_param_str(xform_map["param"][p_name]));
    }

    // resolve precalc macros
    auto macros = find_macros(xform_result);
    std::erase_if(macros, [this](auto name) {return !is_common(name); });

    std::map<std::string, std::set<std::string>> macro_adj;
    for (auto& m : macros) {
        auto deps = find_macros(common(m));
        for (auto d : deps) {
            if (is_common(d) && !macro_adj.contains(d)) macro_adj[d] = find_macros(common(m));
        }
        macro_adj[m] = find_macros(common(m));
    }

    auto order = get_ordering(macro_adj);
    while (order.size()) {
        xform_result = "float " + order.top() + " = " + common(order.top()) + ";\n" + xform_result;
        order.pop();
    }

    // append affine
    std::string affine = "\tv = vec2(fma($c00, v.x, fma($c10, v.y, $c20)), fma($c01, v.x, fma($c11, v.y, $c21)));\n";
    resolve_affine(affine, xform_map);
    xform_result = affine + "vec2 result = vec2(0.0, 0.0);\n" + xform_result;

    // append post
    if (xform.post) {
        xform_result += "\tresult = vec2(fma($p00, result.x, fma($p10, result.y, $p20)), fma($p01, result.x, fma($p11, result.y, $p21)));\n";

        // resolve post
        for (int c = 0; c < 3; c++) {
            for (int r = 0; r < 2; r++) {
                std::string name = "p" + std::to_string(c) + std::to_string(r);
                int index = c * 2 + r;
                xform_result = replace_macro(xform_result, name, make_param_str(xform_map["post"][index]));
            }
        }
    }

    //xform_result += fmt::format("if(make_xform_dist) atomicAdd(xform_invoke_count[{}], 1);\n", i);
    xform_result += "return result;\n";
    boost::replace_all(xform_result, "\n", "\n\t");
    boost::erase_all(xform_result, "$");

    std::string final_xform = "vec2 " + func_name + "(vec2 v) {\n" + xform_result + "\n}\n";

    return final_xform;
}

std::string variation_table::compile_flame_xforms(const flame& f, const nlohmann::json& buf_map)
{
    std::string compiled{};

    std::string disp_func = std::string("vec4 dispatch(vec3 v){\n")
        + "float ratio = float(gl_WorkGroupID.x) / float(gl_NumWorkGroups.x);\n"
        + "float sum = 0.0;\n";

    // concat sources
    for (int i = 0; i < f.xforms.size(); i++) {
        std::string func_name = "apply_xform_" + std::to_string(i);
        const auto& xform = f.xforms.at(i);
        const auto& xform_map = buf_map["xforms"][i];
        std::string xform_result{};

        std::string dispatch_invoke = 
            fmt::format("return vec4({}(v.xy), {} * {} + (1.0 - {}) * ((first_run)? randf(): v.z) , {});\n", func_name, make_param_str(xform_map["color"]),make_param_str(xform_map["color_speed"]), make_param_str(xform_map["color_speed"]), make_param_str(xform_map["opacity"]));
        // append to the dispatch function
        if (i + 1 == f.xforms.size()) {
            disp_func += dispatch_invoke;
        }
        else {
            disp_func += "sum += " + make_param_str(buf_map["xforms"][i]["weight"]) + ";\n";
            disp_func += "if(sum >= ratio) " + dispatch_invoke;
        }

        compiled += make_xform_src(xform, xform_map, std::to_string(i));
    }

    if (f.final_xform) {
        compiled += make_xform_src(f.final_xform.value(), buf_map["final_xform"], "final");
    }
    else compiled += "vec2 apply_xform_final(vec2 v) { return v; }\n";

    boost::replace_all(disp_func, "\n", "\n\t");
    disp_func += "\n}";

    return compiled + disp_func;
}
