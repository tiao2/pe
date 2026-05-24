#include <string>
#include <sstream>
#include <map>
#include <cstring>
#include "phy_engine/phy_engine.h"
#include "phy_engine/circuits/circuit.h"
#include "phy_engine/verilog/digital/digital.h"
#include "phy_engine/verilog/digital/pe_synth.h"
#include "phy_engine/netlist/operation.h"
#include "phy_engine/model/models/digital/logical/input.h"
#include "phy_engine/model/models/digital/logical/output.h"

using namespace phy_engine;
using namespace phy_engine::verilog::digital;
using namespace phy_engine::model;

static std::map<std::string, std::string> parse_inputs(const char* json_str) {
    std::map<std::string, std::string> inputs;
    if (!json_str || *json_str == '\0') return inputs;
    std::string s(json_str);
    size_t pos = s.find('{');
    if (pos == std::string::npos) return inputs;
    size_t end = s.rfind('}');
    if (end == std::string::npos) return inputs;
    s = s.substr(pos+1, end-pos-1);
    size_t start = 0;
    while (start < s.size()) {
        size_t colon = s.find(':', start);
        if (colon == std::string::npos) break;
        size_t key_start = s.find('"', start);
        if (key_start == std::string::npos) break;
        size_t key_end = s.find('"', key_start+1);
        if (key_end == std::string::npos) break;
        std::string key = s.substr(key_start+1, key_end-key_start-1);
        size_t val_start = s.find('"', colon+1);
        if (val_start == std::string::npos) break;
        size_t val_end = s.find('"', val_start+1);
        if (val_end == std::string::npos) break;
        std::string value = s.substr(val_start+1, val_end-val_start-1);
        inputs[key] = value;
        start = val_end + 1;
    }
    return inputs;
}

extern "C" const char* simulate_verilog(const char* verilog_code, const char* inputs_json) {
    static std::string result;
    try {
        auto src = ::fast_io::u8string_view{(const char8_t*)verilog_code, std::strlen(verilog_code)};
        auto cr = compile(src);
        if (!cr.errors.empty()) {
            result = "{\"error\":\"Verilog compile failed\"}";
            return result.c_str();
        }
        if (cr.modules.empty()) {
            result = "{\"error\":\"No module found\"}";
            return result.c_str();
        }
        auto design = build_design(std::move(cr));
        auto* top_mod = &design.modules.back();
        auto top_inst = elaborate(design, *top_mod);
        if (!top_inst.mod) {
            result = "{\"error\":\"Elaboration failed\"}";
            return result.c_str();
        }
        circult c;
        c.set_analyze_type(analyze_type::OP);
        auto& nl = c.get_netlist();
        std::vector<model::node_t*> port_nodes;
        std::map<std::string, model::model_base*> input_models;
        std::map<std::string, model::model_base*> output_models;
        for (size_t i = 0; i < top_inst.mod->ports.size(); ++i) {
            auto& p = top_inst.mod->ports[i];
            auto& node = netlist::create_node(nl);
            port_nodes.push_back(&node);
            std::string name((const char*)p.name.data(), p.name.size());
            if (p.dir == port_dir::input) {
                auto [m, pos] = netlist::add_model(nl, INPUT{});
                m->name = ::fast_io::u8string(reinterpret_cast<const char8_t*>(name.c_str()), name.size());
                netlist::add_to_node(nl, *m, 0, node);
                input_models[name] = m;
            } else if (p.dir == port_dir::output) {
                auto [m, pos] = netlist::add_model(nl, OUTPUT{});
                m->name = ::fast_io::u8string(reinterpret_cast<const char8_t*>(name.c_str()), name.size());
                netlist::add_to_node(nl, *m, 0, node);
                output_models[name] = m;
            }
        }
        pe_synth_options opt;
        opt.opt_level = 3;
        opt.assume_binary_inputs = true;
        pe_synth_error err;
        if (!synthesize_to_pe_netlist(nl, top_inst, port_nodes, &err, opt)) {
            std::string err_msg((const char*)err.message.data(), err.message.size());
            result = "{\"error\":\"Synthesis failed: " + err_msg + "\"}";
            return result.c_str();
        }
        c.set_analyze_type(analyze_type::OP);
        if (!c.analyze()) {
            result = "{\"error\":\"Initial analysis failed\"}";
            return result.c_str();
        }
        auto inputs = parse_inputs(inputs_json);
        for (auto& kv : input_models) {
            std::string state_str = "H";
            auto it = inputs.find(kv.first);
            if (it != inputs.end()) state_str = it->second;
            model::digital_node_statement_t state = model::digital_node_statement_t::H;
            if (state_str == "L") state = model::digital_node_statement_t::L;
            else if (state_str == "X") state = model::digital_node_statement_t::X;
            else if (state_str == "Z") state = model::digital_node_statement_t::Z;
            model::variant v;
            v.digital = state;
            v.type = model::variant_type::digital;
            kv.second->ptr->set_attribute(0, v);
        }
        c.digital_clk();
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (auto& kv : output_models) {
            auto pin_view = kv.second->ptr->generate_pin_view();
            if (pin_view.size && pin_view.pins[0].nodes && pin_view.pins[0].nodes->num_of_analog_node == 0) {
                auto state = pin_view.pins[0].nodes->node_information.dn.state;
                const char* s = "X";
                if (state == model::digital_node_statement_t::L) s = "L";
                else if (state == model::digital_node_statement_t::H) s = "H";
                else if (state == model::digital_node_statement_t::Z) s = "Z";
                if (!first) ss << ",";
                ss << "\"" << kv.first << "\":\"" << s << "\"";
                first = false;
            }
        }
        ss << "}";
        result = ss.str();
    } catch (const std::exception& e) {
        result = std::string("{\"error\":\"Exception: ") + e.what() + "\"}";
    }
    return result.c_str();
}