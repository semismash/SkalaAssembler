#include "symbol_table.hpp"
#include "../panic.hpp"

namespace symbol_mod {

    bool SymbolTable::declare_label(const std::string& name, size_t inst_index) {
        auto [_, inserted] = labels.insert({name, LabelSymbol{inst_index, std::nullopt}});
        return inserted;
    }

    bool SymbolTable::has_label(const std::string& name) const {
        return labels.contains(name);
    }

    std::optional<size_t> SymbolTable::label_address(const std::string& name) const {
        auto it = labels.find(name);
        if (it == labels.end()) return std::nullopt;
        return it->second.address;
    }

    void SymbolTable::set_label_address(const std::string& name, size_t address) {
        auto it = labels.find(name);
        if (it == labels.end()) panic::panic("set_label_address: unknown label");
        it->second.address = address;
    }

    std::vector<std::string> SymbolTable::label_names() const {
        std::vector<std::string> names;
        names.reserve(labels.size());
        for (const auto& [name, symbol] : labels) names.push_back(name);
        return names;
    }

    size_t SymbolTable::label_inst_index(const std::string& name) const {
        auto it = labels.find(name);
        if (it == labels.end()) panic::panic("label_inst_index: unknown label");
        return it->second.inst_index;
    }

    std::optional<size_t> SymbolTable::variable_address(const std::string& name) const {
        auto it = variables.find(name);
        if (it == variables.end()) return std::nullopt;
        return it->second;
    }

    void SymbolTable::set_variable_address(const std::string& name, size_t address) {
        variables[name] = address;
    }

}
