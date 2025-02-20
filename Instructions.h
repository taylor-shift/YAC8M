#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <stdexcept>

// Forward declare CPU class
// to avoid circular deps
class CPU;

struct DecodedInstruction {
	// Different parts of the instruction mean different things
	uint16_t raw;      // The complete opcode (e.g., 0x6A23)
	uint8_t type;      // First nibble (e.g., 0x6 for "Set Register")
	uint8_t x;         // Second nibble - often a register index
	uint8_t y;         // Third nibble - often another register index
	uint8_t n;         // Last nibble - often a 4-bit number
	uint8_t nn;        // Last byte - often an 8-bit immediate value
	uint16_t nnn;      // Last 12 bits - often a memory address

	DecodedInstruction(uint16_t opcode) : raw(opcode) {
		type = (opcode & 0xF000) >> 12;
		x = (opcode & 0x0F00) >> 8;
		y = (opcode & 0x00F0) >> 4;
		n = opcode & 0x000F;
		nn = opcode & 0x00FF;
		nnn = opcode & 0x0FFF;
	}

	uint16_t getPatternKey() const {
		// For 00E0 and 00EE, use exact opcode
		if (raw == 0x00E0 || raw == 0x00EE) {
			return raw;
		}
		// For 8XY_ + 9XY_ instructions, preserve type and n
		if (type == 0x8 || (type == 0x9 && n == 0)) {
			return (type << 12) | n;
		}
		// For F___ and E___ instructions, preserve type and nn
		if (type == 0xF || type == 0xE) {
			return (type << 12) | nn;
		}
		// For all others, just use type
		return type << 12;
	}
};

// Instruction handlers type is pointer to CPU member function
using InstructionHandler = std::function<void(CPU&, const DecodedInstruction&)>;

class InstructionSet {
private:
	static const std::unordered_map<uint16_t, std::pair<InstructionHandler, std::string>> instructions;

public:
	static void execute(uint16_t opcode, CPU& cpu) {
		DecodedInstruction decoded(opcode);
		auto it = instructions.find(decoded.getPatternKey());
		if (it != instructions.end()) {
			it->second.first(cpu, decoded);
		}
		else {
			throw std::runtime_error("Unknown instruction: " + std::to_string(opcode));
		}
	}
};