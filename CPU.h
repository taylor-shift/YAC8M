#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include "Memory.h"
#include "Display.h"
#include "Instructions.h"

class CPU {
private:
	CPU() :
		ram(Memory::getInstance()),
		display(Display::getInstance())
	{
		reset();
	}

	// Registers and stack
	std::array<uint8_t, 16> V{};        // General purpose registers V0-VF
	uint16_t I{ 0 };                    // Index register
	uint16_t PC{ 0 };                   // Program counter
	uint8_t SP{ 0 };                    // Stack pointer
	std::array<uint16_t, 16> stack{};

	// Timers (60Hz)
	static constexpr uint8_t TIMER_FREQUENCY = 60;
	static constexpr auto TIMER_INTERVAL = std::chrono::microseconds(1000000 / TIMER_FREQUENCY);
	uint8_t delayTimer{ 0 };
	uint8_t soundTimer{ 0 };
	std::chrono::steady_clock::time_point lastTimerUpdate;

	// System components
	Memory& ram;
	Display& display;

protected:
	// Register operations
	void setVRegister(uint8_t index, uint8_t value) { V[index] = value; }
	uint8_t getVRegister(uint8_t index) const { return V[index]; }
	void addToVRegister(uint8_t index, uint8_t value) { V[index] += value; }
	void copyVRegister(const DecodedInstruction& instruction) {
		setVRegister(instruction.x, getVRegister(instruction.y));
	}

	void setIRegister(uint16_t value) { I = value; }
	uint16_t getIRegister() const { return I; }

	void setPCRegister(uint16_t value) { PC = value; }
	uint16_t getPCRegister() const { return PC; }

	// Memory operations
	uint8_t readByte(uint16_t addr) const { return ram.read(addr); }
	void writeByte(uint16_t addr, uint8_t value) { ram.write(addr, value); }
	const uint8_t* getMemoryPtr(uint16_t addr) const { return ram.getMemoryPtr(addr); }

	// Stack operations
	void push(uint16_t value) {
		if (SP >= 16) throw std::runtime_error("Stack Overflow");
		stack[SP++] = value;
	}
	uint16_t pop() {
		if (SP == 0) throw std::runtime_error("Stack Underflow");
		return stack[--SP];
	}

	// Timers
	void setDelayTimer(uint8_t value) {
		delayTimer = value;
	}
	void setSoundTimer(uint8_t value) {
		soundTimer = value;
	}
	

	// Wipe memory + redo layout
	void reset() {
		V.fill(0);
		I = 0;
		PC = Memory::BIN_START;
		stack.fill(0);
		SP = 0;
		delayTimer = 0;
		soundTimer = 0;
	}

	// Make it so the InstructionFactory class
	// can access private members
	// (by making it a "friend")
	friend class InstructionSet;
public:

	// Public interface
	Display& getDisplay() { return display; }
	uint8_t getDelayTimer() const { return delayTimer; }
	uint8_t getSoundTimer() const { return soundTimer; }

	void updateTimers() {
		auto now = std::chrono::steady_clock::now();
		auto elapsed = now - lastTimerUpdate;

		if (elapsed >= TIMER_INTERVAL) {
			if (delayTimer > 0) delayTimer--;
			if (soundTimer > 0) soundTimer--;
			lastTimerUpdate = now;
		}
	}

	void cycle() {
		// Fetch: combine two bytes into one 16-bit opcode
		uint16_t opcode = (ram.read(PC) << 8) | ram.read(PC + 1);
		PC += 2;

		// Decode and Execute
		try {
			InstructionSet::execute(opcode, *this);
		}
		catch (const std::exception& ex) {
			std::cerr << "Error executing instruction: " << ex.what() << "\n";
		}
	}

	// Singleton interface
	static CPU& getInstance() {
		static CPU instance;
		return instance;
	}

	// Delete copy and move operations
	CPU(const CPU&) = delete;
	CPU& operator=(const CPU&) = delete;
	CPU(CPU&&) = delete;
	CPU& operator=(CPU&&) = delete;
};