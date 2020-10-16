#include <yaml-cpp/yaml.h>
#include <regex>
#include <stack>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <inja/inja.hpp>
#include "util.hpp"
#include "variation_table.hpp"

using optimizer_pred_t = std::function<bool(const flame_xform&, const variation_table&)>;
using optimizer_t = std::function<std::pair<bool, std::string>(const flame_xform&, const nlohmann::json&, const variation_table&)>;

std::string make_param_str(const nlohmann::json& v, int offset = 0) { return "fp[" + std::to_string(v.get<int>() /*- offset*/) + "]"; }

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

void resolve_affine(std::string& src, const nlohmann::json& buf_map) {
    int offset = buf_map["meta"]["start"];

    // resolve affine
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 2; r++) {
            std::string name = "c" + std::to_string(c) + std::to_string(r);
            int index = c * 2 + r;
            src = replace_macro(src, name, make_param_str(buf_map["affine"][index], offset));
        }
    }
}

void resolve_post(std::string& src, const nlohmann::json& buf_map) {
    int offset = buf_map["meta"]["start"];

    // resolve affine
    for (int c = 0; c < 3; c++) {
        for (int r = 0; r < 2; r++) {
            std::string name = "p" + std::to_string(c) + std::to_string(r);
            int index = c * 2 + r;
            src = replace_macro(src, name, make_param_str(buf_map["post"][index], offset));
        }
    }
}

#define OPTIMIZER_PRED [](const flame_xform& x, const variation_table& vt) -> bool
#define OPTIMIZER_BODY [](const flame_xform& x, const nlohmann::json& xmap, const variation_table& vt) -> std::pair<bool, std::string>

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
                int offset = xmap["meta"]["start"];
                std::string src = replace_macro(base, "weight", make_param_str(xmap["variations"]["linear"], offset));
                resolve_affine(src, xmap);
                return {true, src};
            }
        },

        // default "optimizer"
        // just spits back the typical xform source
        {
            OPTIMIZER_PRED { return true; },
            OPTIMIZER_BODY {                
                std::string xform_result{};
                int offset = xmap["meta"]["start"];
                bool first_var = true;
                std::string affine = "\tv.xy = vec2(fma($c00, v.x, fma($c10, v.y, $c20)), fma($c01, v.x, fma($c11, v.y, $c21)));\n";
                // resolve variations and weights
                for (auto& [var_name, var] : x.variations) {
                    const auto& vd = vt.variation(var_name);
                    std::string var_src{};
                    var_src += "// variation: " + var_name + "\n";
                    if (!vd.source.empty()) var_src += replace_macro(vd.source, "weight", make_param_str(xmap["variations"][var_name], offset)) + "\n";

                    std::string weight_str = (vd.flags.contains("no_weight_mul")) ? "" : "$weight *";

                    if (vd.flags.contains("pre_xform")) {
                        affine += replace_macro(("v.xy += " + weight_str) + vd.result + ";", "weight", make_param_str(xmap["variations"][var_name], offset)) + "\n";
                    }
                    else {
                        var_src += replace_macro(((first_var) ? "vec2 result = " + weight_str : "result += " + weight_str) + vd.result + ";", "weight", make_param_str(xmap["variations"][var_name], offset)) + "\n";
                        xform_result += var_src;
                        first_var = false;
                    }
                }

                // resolve parameters
                for (auto& [p_name, val] : x.var_param) {
                    xform_result = replace_macro(xform_result, p_name, make_param_str(xmap["param"][p_name], offset));
                }

                // resolve precalc macros
                auto macros = find_macros(xform_result);
                std::erase_if(macros, [&vt](auto name) {return !vt.is_common(name); });

                std::map<std::string, std::set<std::string>> macro_adj;
                for (auto& m : macros) {
                    auto deps = find_macros(vt.common(m));
                    for (auto d : deps) {
                        if (vt.is_common(d) && !macro_adj.contains(d)) macro_adj[d] = find_macros(vt.common(m));
                    }
                    macro_adj[m] = find_macros(vt.common(m));
                }

                auto order = get_ordering(macro_adj);
                while (order.size()) {
                    xform_result = "float " + order.top() + " = " + vt.common(order.top()) + ";\n" + xform_result;
                    order.pop();
                }

                // append affine
                
                xform_result = affine + xform_result;
                resolve_affine(xform_result, xmap);

                // append post
                if (x.post) {
                    xform_result += "\tresult = vec2(fma($p00, result.x, fma($p10, result.y, $p20)), fma($p01, result.x, fma($p11, result.y, $p21)));\n";
                    resolve_post(xform_result, xmap);
                }

                boost::replace_all(xform_result, "\n", "\n\t");
                boost::erase_all(xform_result, "$");

                return {false, xform_result}; 
            }
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
    str = replace_macro(str, "v", "v.xy");
    str = replace_macro(str, "result", "result");
}

variation_table::variation_table(const std::string& def_path) {
    auto defs = YAML::LoadFile(def_path);
    auto variations = defs["variations"];

    for (auto it = variations.begin(); it != variations.end(); it++) {
        auto name = it->first.as<std::string>();
        auto src = it->second.as<YAML::Node>()["src"].as<std::string>("");
        default_replace_macros(src);
        auto result = it->second.as<YAML::Node>()["result"].as<std::string>("");
        default_replace_macros(result);
        auto param = it->second.as<YAML::Node>()["param"];
        auto& var = vars_[name];
        var.source = src;
        var.result = result;

        for (auto it = param.begin(); it != param.end(); it++) {
            auto pname = it->first.as<std::string>();
            var.param.push_back(pname);
            param_owners_[pname] = name;
        }

        auto flags = it->second.as<YAML::Node>()["flags"];
        for (auto it = flags.begin(); it != flags.end(); it++) 
            var.flags.insert(it->as<std::string>());
    }

    auto common = defs["common"];
    for (auto it = common.begin(); it != common.end(); it++) {
        auto name = it->first.as<std::string>();
        auto src = it->second.as<std::string>();
        default_replace_macros(src);
        common_[name] = src;
    }
}

std::string variation_table::compile_flame_xforms(const flame& f, const nlohmann::json& buf_map)
{
    std::string compiled{};

    std::string disp_func = std::string("vec4 dispatch(vec3 v, int xform){\n").append("switch(xform){\n");
    // concat sources
    for (int i = -1; i < (int) f.xforms.size(); i++) {
        if (i == -1 && !f.final_xform) continue;

        const auto& xform = (i == -1)? f.final_xform.value(): f.xforms.at(i);
        const auto& xform_map = (i == -1)? buf_map["final_xform"]: buf_map["xforms"][i];

        std::string xform_src;
        bool inlined;

        for (auto& opt : get_optimizers()) {
            if (opt.first(xform, *this))
            {
                std::tie(inlined, xform_src) = opt.second(xform, xform_map, *this);
                break;
            }
        }

        std::string dispatch_invoke = 
            fmt::format("return vec4({}, mix(((first_run)? randf(): v.z), {}, {}), {});\n", (inlined)? xform_src: "result", make_param_str(xform_map["color"]),make_param_str(xform_map["color_speed"]), make_param_str(xform_map["opacity"]));
        if (!inlined) dispatch_invoke = xform_src + dispatch_invoke;
        if(i != -1) dispatch_invoke = fmt::format("atomicAdd(xform_invoke_count[{}], 1);\n", i) + dispatch_invoke;
        // append to the dispatch function
        if (i + 1 == f.xforms.size()) {
            disp_func += "default: {\n" + dispatch_invoke + "\n}}\n";
        }
        else {
            disp_func += "case " + std::to_string(i) + ": {\n" + dispatch_invoke + "\n}\n";
        }
    }

    /*if (f.final_xform) {
        compiled += make_xform_src(f.final_xform.value(), buf_map["final_xform"], "final");
    }
    else compiled += "vec2 apply_xform_final(vec2 v) { return v; }\n";
    */

    std::string xid_func = inja::render(read_file("shaders/templates/xform_select.tpl.glsl"), buf_map);

    boost::replace_all(disp_func, "\n", "\n\t");
    disp_func += "\n}";
    return compiled + xid_func + disp_func;
}
