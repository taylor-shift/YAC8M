#include <random>
#include "Instructions.h"
#include "CPU.h"
#include "Keyboard.h"

namespace {
	// System and Flow Control
	static const std::string CLS_DESC = "CLS";                    // 00E0 - Clear screen
	static const std::string RET_DESC = "RET";                    // 00EE - Return from subroutine
	static const std::string JP_DESC = "JP NNN";                  // 1NNN - Jump to address
	static const std::string CALL_DESC = "CALL NNN";             // 2NNN - Call subroutine
	static const std::string JP_V0_DESC = "JP V0, NNN";         // BNNN - Jump to V0 + NNN

	// Skip Instructions (Conditionals)
	static const std::string SE_VX_NN_DESC = "SE VX, NN";       // 3XNN - Skip if VX == NN
	static const std::string SNE_VX_NN_DESC = "SNE VX, NN";     // 4XNN - Skip if VX != NN
	static const std::string SE_VX_VY_DESC = "SE VX, VY";       // 5XY0 - Skip if VX == VY
	static const std::string SNE_VX_VY_DESC = "SNE VX, VY";     // 9XY0 - Skip if VX != VY

	// Register Operations
	static const std::string LD_VX_NN_DESC = "LD VX, NN";       // 6XNN - Set VX to NN
	static const std::string ADD_VX_NN_DESC = "ADD VX, NN";     // 7XNN - Add NN to VX
	static const std::string LD_VX_VY_DESC = "LD VX, VY";       // 8XY0 - Set VX to VY
	static const std::string OR_VX_VY_DESC = "OR VX, VY";       // 8XY1 - VX = VX OR VY
	static const std::string AND_VX_VY_DESC = "AND VX, VY";     // 8XY2 - VX = VX AND VY
	static const std::string XOR_VX_VY_DESC = "XOR VX, VY";     // 8XY3 - VX = VX XOR VY
	static const std::string ADD_VX_VY_DESC = "ADD VX, VY";     // 8XY4 - Add VY to VX with carry
	static const std::string SUB_VX_VY_DESC = "SUB VX, VY";     // 8XY5 - Subtract VY from VX
	static const std::string SHR_VX_DESC = "SHR VX";            // 8XY6 - Shift VX right
	static const std::string SUBN_VX_VY_DESC = "SUBN VX, VY";   // 8XY7 - VX = VY - VX
	static const std::string SHL_VX_DESC = "SHL VX";            // 8XYE - Shift VX left

	// Memory Operations
	static const std::string LD_I_NNN_DESC = "LD I, NNN";       // ANNN - Set I to NNN
	static const std::string LD_I_VX_DESC = "LD [I], VX";       // FX55 - Store V0-VX in memory
	static const std::string LD_VX_I_DESC = "LD VX, [I]";       // FX65 - Load V0-VX from memory
	static const std::string LD_F_VX_DESC = "LD F, VX";         // FX29 - Set I to sprite location
	static const std::string LD_B_VX_DESC = "LD B, VX";         // FX33 - Store BCD of VX

	// Timer Operations
	static const std::string LD_VX_DT_DESC = "LD VX, DT";       // FX07 - Get delay timer
	static const std::string LD_DT_VX_DESC = "LD DT, VX";       // FX15 - Set delay timer
	static const std::string LD_ST_VX_DESC = "LD ST, VX";       // FX18 - Set sound timer

	// Input Operations
	static const std::string SKP_VX_DESC = "SKP VX";            // EX9E - Skip if key VX pressed
	static const std::string SKNP_VX_DESC = "SKNP VX";          // EXA1 - Skip if key VX not pressed
	static const std::string LD_VX_K_DESC = "LD VX, K";         // FX0A - Wait for key press

	// Special Operations
	static const std::string ADD_I_VX_DESC = "ADD I, VX";       // FX1E - Add VX to I
	static const std::string RND_VX_NN_DESC = "RND VX, NN";     // CXNN - Random AND
	static const std::string DRW_VX_VY_N_DESC = "DRW VX, VY, N"; // DXYN - Draw sprite
}

const std::unordered_map<uint16_t, std::pair<InstructionHandler, std::string>>
InstructionSet::instructions =
// System and Flow Control Instructions
{ {0x00E0, {
	[](CPU& cpu, const DecodedInstruction&) {
		// Clear the display - straightforward mapping to display primitive
		cpu.getDisplay().clear();
	},
	CLS_DESC
}},
{ 0x00EE, {
	[](CPU& cpu, const DecodedInstruction&) {
		// Return from subroutine
		// (Pop last stack frame and set program counter)
		uint16_t returnAddr = cpu.pop();
		if (returnAddr < Memory::MAX_SIZE) {
			cpu.setPCRegister(returnAddr);
		}
		else {
			throw std::out_of_range("Return address out of bounds");
		}
	},
	RET_DESC
}},
{ 0x1000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Jump - direct update to program counter
		if (i.nnn >= Memory::BIN_START && i.nnn < Memory::MAX_SIZE) {
			cpu.setPCRegister(i.nnn);
		}
		else {
			throw std::out_of_range("Jump address out of bounds");
		};
	},
	JP_DESC
}},
{ 0x2000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Call subroutine - save return address then jump
		if (i.nnn >= Memory::BIN_START && i.nnn < Memory::MAX_SIZE) {
			cpu.push(cpu.getPCRegister());
			cpu.setPCRegister(i.nnn);
		}
		else {
			throw std::out_of_range("Call address out of bounds");
		}
	},
	CALL_DESC
}},

// Skip Instructions (Conditionals)
{ 0x3000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Skip if equal
		if (cpu.getVRegister(i.x) == i.nn) {
			uint16_t newPC = cpu.getPCRegister() + 2;
			if (newPC < Memory::MAX_SIZE) {
				cpu.setPCRegister(newPC);
			}
		else {
			throw std::out_of_range("PC out of bounds after skip");
		}
	}},
	SE_VX_NN_DESC
}},
{ 0x4000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Skip if not equal
		if (cpu.getVRegister(i.x) != i.nn) {
			uint16_t newPC = cpu.getPCRegister() + 2;
			if (newPC < Memory::MAX_SIZE) {
				cpu.setPCRegister(newPC);
			}
		else {
			throw std::out_of_range("PC out of bounds after skip");
		}
	}},
	SNE_VX_NN_DESC
}},
{ 0x5000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Skip if VX == VY
		if (cpu.getVRegister(i.x) == cpu.getVRegister(i.y)) {
			uint16_t newPC = cpu.getPCRegister() + 2;
			if (newPC < Memory::MAX_SIZE) {
				cpu.setPCRegister(newPC);
			}
		else {
			throw std::out_of_range("PC out of bounds after skip");
		}
	}},
	SE_VX_VY_DESC
}},

// Register Operations
{ 0x6000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Set VX to NN
		cpu.setVRegister(i.x, i.nn);
	},
	LD_VX_NN_DESC
}},
{ 0x7000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Add NN to VX (no carry flag affected)
		uint8_t vx = cpu.getVRegister(i.x);
		cpu.setVRegister(i.x, vx + i.nn);
	},
	ADD_VX_NN_DESC
}},
// Register-to-Register Operations (0x8XY_)
{ 0x8000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Sets VX to VY
		cpu.setVRegister(i.x, cpu.getVRegister(i.y));
	},
	LD_VX_VY_DESC
}},
{ 0x8001, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Bitwise OR (VX = VX | VY)
		cpu.setVRegister(i.x, cpu.getVRegister(i.x) | cpu.getVRegister(i.y));
	},
	OR_VX_VY_DESC
}},
{ 0x8002, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Bitwise AND
		cpu.setVRegister(i.x, cpu.getVRegister(i.x) & cpu.getVRegister(i.y));
	},
	AND_VX_VY_DESC
}},
{ 0x8003, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Bitwise XOR
		cpu.setVRegister(i.x, cpu.getVRegister(i.x) ^ cpu.getVRegister(i.y));
	},
	XOR_VX_VY_DESC
}},
{ 0x8004, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Add with carry
		uint16_t sum = cpu.getVRegister(i.x) + cpu.getVRegister(i.y);
		cpu.setVRegister(0xF, sum > 0xFF ? 1 : 0);  // Set carry flag
		cpu.setVRegister(i.x, static_cast<uint8_t>(sum));
	},
	ADD_VX_VY_DESC
}},
{ 0x8005, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Subtract VX and VY with no borrow
		uint8_t vx = cpu.getVRegister(i.x);
		uint8_t vy = cpu.getVRegister(i.y);
		cpu.setVRegister(0xF, vx > vy ? 1 : 0);  // Set NOT borrow flag
		cpu.setVRegister(i.x, vx - vy);
	},
	SUB_VX_VY_DESC
}},
// Shift Operations
{ 0x8006, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Shift VX right, store least significant bit in VF
		uint8_t vx = cpu.getVRegister(i.x);
		cpu.setVRegister(0xF, vx & 0x1);        // LSB in VF
		cpu.setVRegister(i.x, vx >> 1);
	},
	SHR_VX_DESC
}},
{ 0x800E, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Shift VX left, store most significant bit in VF
		uint8_t vx = cpu.getVRegister(i.x);
		cpu.setVRegister(0xF, (vx & 0x80) >> 7);  // MSB in VF
		cpu.setVRegister(i.x, vx << 1);
	},
	SHL_VX_DESC
}},
{ 0x8007, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Set VX to VY - VX, set VF = 1 if VY ≥ VX
		uint8_t vx = cpu.getVRegister(i.x);
		uint8_t vy = cpu.getVRegister(i.y);
		cpu.setVRegister(0xF, vy > vx ? 1 : 0);  // NOT borrow flag
		cpu.setVRegister(i.x, vy - vx);
	},
	SUBN_VX_VY_DESC
}},
{ 0x9000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Skip if VX != VY
		if (cpu.getVRegister(i.x) != cpu.getVRegister(i.y)) {
			uint16_t newPC = cpu.getPCRegister() + 2;
			if (newPC < Memory::MAX_SIZE) {
				cpu.setPCRegister(newPC);
			}
		else {
			throw std::out_of_range("PC out of bounds after skip");
		}
	}},
	SNE_VX_VY_DESC,
}},
// Memory Operations
{ 0xA000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Set index register to immediate value
		cpu.setIRegister(i.nnn);
	},
	LD_I_NNN_DESC
}},
{ 0xF055, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Store V0 through VX in memory starting at I
		uint16_t addr = cpu.getIRegister();
		for (uint8_t reg = 0; reg <= i.x; reg++) {
			cpu.writeByte(addr + reg, cpu.getVRegister(reg));
		}
		cpu.setIRegister(addr + i.x + 1);
	},
	LD_I_VX_DESC
}},
{ 0xF065, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Load V0 through VX from memory starting at I
		uint16_t addr = cpu.getIRegister();
		for (uint8_t reg = 0; reg <= i.x; reg++) {
			cpu.setVRegister(reg, cpu.readByte(addr + reg));
		}
		cpu.setIRegister(addr + i.x + 1);
	},
	LD_VX_I_DESC
}},
{ 0xF033, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Store BCD representation of VX in memory
		uint8_t value = cpu.getVRegister(i.x);
		uint16_t addr = cpu.getIRegister();

		cpu.writeByte(addr, value / 100);           // Hundreds
		cpu.writeByte(addr + 1, (value / 10) % 10); // Tens
		cpu.writeByte(addr + 2, value % 10);        // Ones
	},
	LD_B_VX_DESC
}},

// Timer Operations
{ 0xF007, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Set Vx to the value of the delay timer
		cpu.setVRegister(i.x, cpu.getDelayTimer());
	},
	LD_VX_DT_DESC
}},
{ 0xF015, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Set delay timer to value in VX
		cpu.setDelayTimer(cpu.getVRegister(i.x));
	},
	LD_DT_VX_DESC
}},
{ 0xF018, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Set sound timer to value in VX
		cpu.setSoundTimer(cpu.getVRegister(i.x));
	},
	LD_ST_VX_DESC
}},
{ 0xD000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Draw sprite
		uint8_t x = cpu.getVRegister(i.x) % Display::WIDTH_PX;
		uint8_t y = cpu.getVRegister(i.y) % Display::HEIGHT_PX;
		uint8_t currHeight = i.n;

		// Get pointer directly to the sprite data in memory
		const uint8_t* spriteData = cpu.getMemoryPtr(cpu.getIRegister());

		bool collision = cpu.getDisplay().drawSprite(x, y, spriteData, currHeight);
		cpu.setVRegister(0xF, collision ? 1 : 0);
	},
	DRW_VX_VY_N_DESC
}},
{ 0xC000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<uint16_t> dist(0, 255);

		uint8_t randomNum = static_cast<uint8_t>(dist(gen));
		cpu.setVRegister(i.x, randomNum & i.nn);
	},
	RND_VX_NN_DESC
} },
{ 0xE09E, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Skip instruction if key code at VX is pressed
		uint8_t key = cpu.getVRegister(i.x);
		if (Keyboard::getInstance().getKeyState(key) == KeyState::Pressed) {
			uint16_t newPC = cpu.getPCRegister() + 2;
			if (newPC < Memory::MAX_SIZE) {
				cpu.setPCRegister(newPC);
			}
		else {
			throw std::out_of_range("PC out of bounds after skip");
		}
	}},
	SKP_VX_DESC,
}},
{ 0xE0A1, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Skip next instruction if key not pressed
		uint8_t key = cpu.getVRegister(i.x);
		if (Keyboard::getInstance().getKeyState(key) == KeyState::Released) {
			cpu.setPCRegister(cpu.getPCRegister() + 2);
		}
	},
	SKNP_VX_DESC
}},
{ 0xF00A, {
	[](CPU& cpu, const DecodedInstruction& i) {
		auto& keyboard = Keyboard::getInstance();
		if (auto keyPressed = keyboard.getLastKeyPressed()) {
			cpu.setVRegister(i.x, *keyPressed);
			keyboard.clearLastKeyPressed();
		}
		 else {
			// If no key is pressed, repeat this instruction
			uint16_t newPC = cpu.getPCRegister() - 2;
			if (newPC < Memory::MAX_SIZE) {
				cpu.setPCRegister(newPC);
			}
			else {
				throw std::out_of_range("PC out of bounds after key wait");
			}
	}},
	LD_VX_K_DESC
}},
{ 0xF029, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Load Font
		uint8_t digit = cpu.getVRegister(i.x);
		cpu.setIRegister(Memory::FONTS_START + (5 * digit));
	},
	LD_F_VX_DESC
}},
{ 0xF01E, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Add VX to I register
		uint16_t newIndex = cpu.getIRegister() + cpu.getVRegister(i.x);
		cpu.setVRegister(0xF, newIndex > 0xFF ? 1 : 0);
		cpu.setIRegister(newIndex);
	},
	ADD_I_VX_DESC
}},
{ 0xB000, {
	[](CPU& cpu, const DecodedInstruction& i) {
		// Jump NNN bytes from V0
		uint16_t destAddr = i.nnn + cpu.getVRegister(0);
		if (destAddr < Memory::MAX_SIZE) {
			cpu.setPCRegister(destAddr);
		}
		else {
			throw std::out_of_range("Jump address out of bounds");
		}
	},
	JP_V0_DESC
}}
};