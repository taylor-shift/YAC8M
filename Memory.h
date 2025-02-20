#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <stdexcept>

class Memory {
public:
	static constexpr uint16_t MAX_SIZE = 4096;
	static constexpr uint16_t BIN_START = 0x200;
	static constexpr uint16_t STACK_START = 0xEA0;
	static constexpr uint16_t DISPLAY_START = 0xF00;
	static constexpr uint16_t FONTS_START = 0x50;
	static constexpr uint16_t FONTS_SIZE = 80;
private:
	std::array<uint8_t, MAX_SIZE> RAM;
	// CHIP-8 font sprites - each character is 5 bytes
	// Each byte represents a row of 8 pixels, but we only use the first 4 bits
	// Example: 0xF0 = 1111 0000 = ████░░░░
	static constexpr std::array<uint8_t, FONTS_SIZE> FONTS = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	enum class MemoryRegion {
		Interpreter,    // 0x000-0x1FF - Reserved for interpreter
		Program,        // 0x200-0xE9F - Program/ROM space
		Stack,          // 0xEA0-0xEFF - Stack space
		Display         // 0xF00-0xFFF - Display refresh
	};

public:
	uint8_t read(uint16_t addr) const {
		// Make sure our read is in-bounds
		// (CHIP-8 has 4Kb of memory
		// from 0x000 to 0xFFF)
		if (addr >= MAX_SIZE) {
			throw std::out_of_range(
				"Memory read access violation."
			);
		}
		return RAM[addr];
	};

	void write(uint16_t addr, uint8_t val) {
		if (addr >= MAX_SIZE) {
			throw std::out_of_range(
				"Memory write access violation: Out of bounds"
			);
		}

		auto memoryRegion = getRegion(addr);
		switch (memoryRegion) {
		case MemoryRegion::Interpreter:
			throw std::runtime_error(
				"Memory write violation: Tried to write to interpreter memory"
			);
		case MemoryRegion::Stack:
			throw std::runtime_error(
				"Memory write violation: Stack memory cannot be directly modified"
			);
		case MemoryRegion::Program:
			if (addr >= FONTS_START && addr < FONTS_START + FONTS_SIZE) {
				throw std::runtime_error(
					"Memory write violation: Tried to write to font memory"
				);
			}
			RAM[addr] = val;
			break;
		case MemoryRegion::Display:
			RAM[addr] = val;
			break;
		}
	}

	void loadProgram(const std::vector<uint8_t>& romBytes, uint16_t startAddr) {
		if (startAddr > MAX_SIZE || (static_cast<size_t>(MAX_SIZE - startAddr)) < romBytes.size()) {
			throw std::runtime_error(
				"ROM is too large to load into memory, would exceed bounds."
			);
		}
		if (startAddr < BIN_START) {
			throw std::runtime_error(
				"Cannot load ROM into reserved memory."
			);
		}
		if (startAddr + romBytes.size() > STACK_START) {
			throw std::runtime_error(
				"Loading ROM would overwrite reserved memory."
			);
		}

		std::copy(romBytes.begin(), romBytes.end(), RAM.begin() + startAddr);
	}

	void reset() {
		RAM.fill(0);
		std::copy(FONTS.begin(), FONTS.end(), RAM.data() + FONTS_START);
	}

	const uint8_t* getMemoryPtr(uint16_t addr) const {
		if (addr >= MAX_SIZE) {
			throw std::out_of_range("Memory access violation");
		}
		return &RAM[addr];
	}

	Memory(const Memory&) = delete;
	Memory& operator=(const Memory&) = delete;
	Memory(Memory&&) = delete;
	Memory& operator=(Memory&&) = delete;

	static Memory& getInstance() {
		static Memory instance;
		return instance;
	}

private:
	Memory() {
		reset();
	}

	MemoryRegion getRegion(uint16_t addr) const noexcept {
		if (addr < BIN_START) return MemoryRegion::Interpreter;
		if (addr < 0xEA0) return MemoryRegion::Program;
		if (addr < 0xF00) return MemoryRegion::Stack;
		return MemoryRegion::Display;
	}

};
