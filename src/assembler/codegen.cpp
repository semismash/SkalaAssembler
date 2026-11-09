#include "codegen.hpp"
#include "../overload.hpp"
#include "../panic.hpp"

namespace codegen_mod {

    using instruction_mod::OpCode;
    using instruction_mod::InstType;

    inline constexpr uint8_t FMT_R = 0b000;
    inline constexpr uint8_t FMT_I = 0b001;
    inline constexpr uint8_t FMT_M = 0b010;
    inline constexpr uint8_t FMT_J = 0b011;
    inline constexpr uint8_t FMT_S = 0b100;
    inline constexpr uint8_t FMT_N = 0b101;

    CodeGen::CodeGen(const symbol_mod::SymbolTable& symbols) :
        symbols{symbols}
    { }

    uint8_t CodeGen::encode_tkn_u8(const instruction_mod::Token& token) {
        return std::visit(overload{
            [](const instruction_mod::OpCodeToken& opcode) -> uint8_t { return instruction_mod::instruction_info(opcode.value).code; },
            [](const instruction_mod::RegisterToken& reg) -> uint8_t { return static_cast<uint8_t>(reg.value); },
            [](const auto&) -> uint8_t {
                panic::panic("encode_tkn_u8: this token cannot be encoded in 8 bits");
            }
        }, token.kind);
    }

    uint16_t CodeGen::encode_tkn_u16(const instruction_mod::Token& token) {
        return std::visit(overload{
            [](const instruction_mod::AddressToken& adr) -> uint16_t { return static_cast<uint16_t>(adr.value); },
            [](const instruction_mod::ImmediateToken& imm) -> uint16_t { return static_cast<uint16_t>(imm.value); },
            [this](const instruction_mod::VariableToken& var) -> uint16_t {
                auto adr = symbols.variable_address(var.name);
                if (!adr) panic::panic("encode_tkn_u16: a variable has no address, the layout stage did not run");
                return static_cast<uint16_t>(*adr);
            },
            [this](const instruction_mod::LabelToken& label) -> uint16_t {
                auto adr = symbols.label_address(label.name);
                if (!adr) panic::panic("encode_tkn_u16: a label has no address, the layout stage did not run");
                return static_cast<uint16_t>(*adr);
            },
            [](const auto&) -> uint16_t {
                panic::panic("encode_tkn_u16: this token cannot be encoded in 16 bits");
            }
        }, token.kind);
    }

    uint32_t CodeGen::encode_R(uint8_t instr, uint8_t dest, uint8_t src1, uint8_t src2) {
        uint8_t opcode = (FMT_R << 5) | instr;
        return (opcode << 24) | (dest << 20) | (src1 << 16) | (src2 << 12);
    }

    uint32_t CodeGen::encode_I(uint8_t instr, uint8_t dest, uint8_t src, uint16_t imm) {
        uint8_t opcode = (FMT_I << 5) | instr;
        return (opcode << 24) | (dest << 20) | (src << 16) | (imm);
    }

    uint32_t CodeGen::encode_M(uint8_t instr, uint8_t reg, uint16_t adr) {
        uint8_t opcode = (FMT_M << 5) | instr;
        return (opcode << 24) | (reg << 20) | (adr);
    }

    uint32_t CodeGen::encode_J(uint8_t instr, uint16_t adr) {
        uint8_t opcode = (FMT_J << 5) | instr;
        return (opcode << 24) | (adr);
    }

    uint32_t CodeGen::encode_S(uint8_t instr, uint8_t reg) {
        uint8_t opcode = (FMT_S << 5) | instr;
        return (opcode << 24) | (reg << 20);
    }

    uint32_t CodeGen::encode_N(uint8_t instr) {
        uint8_t opcode = (FMT_N << 5) | instr;
        return (opcode << 24);
    }

    void CodeGen::write_magic(std::ofstream& output, uint32_t MAGIC_NUM) {
        output.put((MAGIC_NUM >> 24) & 0xFF);
        output.put((MAGIC_NUM >> 16) & 0xFF);
        output.put((MAGIC_NUM >> 8)  & 0xFF);
        output.put((MAGIC_NUM)       & 0xFF);
    }

    void CodeGen::write_inst(std::ofstream& output, uint32_t encoded_bytes) {
        output.put((encoded_bytes >> 24) & 0xFF);
        output.put((encoded_bytes >> 16) & 0xFF);
        output.put((encoded_bytes >> 8)  & 0xFF);
        output.put((encoded_bytes)       & 0xFF);
    }

    void CodeGen::generate(std::ofstream& output, const instruction_mod::Inst& inst) {
        uint32_t encoded_inst{0};
        uint8_t enc_opcode{0};
        uint8_t enc_reg{0};
        uint16_t enc_adr{0};
        uint16_t enc_lbl{0};
        switch (inst.inst_type) { //encode instruction based on type
            case InstType::R:
                enc_opcode = encode_tkn_u8(inst.token_arr[0].value());
                if (inst.used_size == 4) { 
                    uint8_t enc_dest = encode_tkn_u8(inst.token_arr[1].value());
                    uint8_t enc_src1 = encode_tkn_u8(inst.token_arr[2].value());
                    uint8_t enc_src2 = encode_tkn_u8(inst.token_arr[3].value());
                    encoded_inst = encode_R(enc_opcode, enc_dest, enc_src1, enc_src2);
                } else if (inst.used_size == 3) {
                    uint8_t enc_dest = encode_tkn_u8(inst.token_arr[1].value());
                    uint8_t enc_src = encode_tkn_u8(inst.token_arr[2].value());
                    encoded_inst = encode_R(enc_opcode, enc_dest, static_cast<uint8_t>(0), enc_src);
                } else {
                    panic::panic();
                }
                break;
            case InstType::I:
                enc_opcode = encode_tkn_u8(inst.token_arr[0].value());
                if (inst.used_size == 4) { 
                    uint8_t enc_dest = encode_tkn_u8(inst.token_arr[1].value());
                    uint8_t enc_src = encode_tkn_u8(inst.token_arr[2].value());
                    uint16_t enc_imm = encode_tkn_u16(inst.token_arr[3].value());
                    encoded_inst = encode_I(enc_opcode, enc_dest, enc_src, enc_imm);
                } else if (inst.used_size == 3) {
                    uint8_t enc_dest = encode_tkn_u8(inst.token_arr[1].value());
                    uint16_t enc_imm = encode_tkn_u16(inst.token_arr[2].value());
                    encoded_inst = encode_I(enc_opcode, enc_dest, static_cast<uint8_t>(0), enc_imm);
                } else {
                    panic::panic();
                }
                break;
            case InstType::M:
                enc_opcode = encode_tkn_u8(inst.token_arr[0].value());
                enc_reg = encode_tkn_u8(inst.token_arr[1].value());
                enc_adr = encode_tkn_u16(inst.token_arr[2].value());
                encoded_inst = encode_M(enc_opcode, enc_reg, enc_adr);
                break;
            case InstType::J:
                enc_opcode = encode_tkn_u8(inst.token_arr[0].value());
                enc_lbl = encode_tkn_u16(inst.token_arr[1].value());
                encoded_inst = encode_J(enc_opcode, enc_lbl);
                break;
            case InstType::S:
                enc_opcode = encode_tkn_u8(inst.token_arr[0].value());
                enc_reg = encode_tkn_u8(inst.token_arr[1].value());
                encoded_inst = encode_S(enc_opcode, enc_reg);
                break;
            case InstType::N:
                enc_opcode = encode_tkn_u8(inst.token_arr[0].value());
                encoded_inst = encode_N(enc_opcode);
                break;
            case InstType::LBL:
                return;
            default:
                panic::panic();
                break;
        }
        write_inst(output, encoded_inst);
    }

}