#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace symbol_mod {

    // A label is declared by the classifier (it only knows which instruction follows it),
    // and gets its address from the layout stage.
    struct LabelSymbol {
        size_t inst_index;                  // number of instructions before the label
        std::optional<size_t> address;      // byte address, set by the layout stage
    };

    class SymbolTable {

        private:
            std::unordered_map<std::string, LabelSymbol> labels;
            std::unordered_map<std::string, size_t> variables;      // name -> byte address, set by the layout stage

        public:
            bool declare_label(const std::string& name, size_t inst_index);   // false if the label already exists
            bool has_label(const std::string& name) const;
            std::optional<size_t> label_address(const std::string& name) const;
            void set_label_address(const std::string& name, size_t address);
            std::vector<std::string> label_names() const;
            size_t label_inst_index(const std::string& name) const;

            std::optional<size_t> variable_address(const std::string& name) const;
            void set_variable_address(const std::string& name, size_t address);

    };

}
