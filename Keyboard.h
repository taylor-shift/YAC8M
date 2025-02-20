#pragma once
#include <array>
#include <cstdint>
#include <optional>

enum class KeyState {
    Released,
    Pressed
};

struct KeyMapping {
    uint8_t physicalKey; // Keyboard scan code/key value
    uint8_t chip8Key;    // CHIP-8 hex value (0-F)
};

class Keyboard {
private:
    // Current state of all 16 CHIP-8 keys
    std::array<KeyState, 16> keyStates;

    // Optional: stores the last key that was pressed
    // Used for the LD VX, K instruction (FX0A)
    std::optional<uint8_t> lastKeyPressed;

    // Default key mappings (can be customized)
    static constexpr std::array<KeyMapping, 16> DEFAULT_MAPPING = { {
        {0x31, 0x1}, // 1 -> 1
        {0x32, 0x2}, // 2 -> 2
        {0x33, 0x3}, // 3 -> 3
        {0x43, 0xC}, // C -> C
        {0x34, 0x4}, // 4 -> 4
        {0x35, 0x5}, // 5 -> 5
        {0x36, 0x6}, // 6 -> 6
        {0x44, 0xD}, // D -> D
        {0x37, 0x7}, // 7 -> 7
        {0x38, 0x8}, // 8 -> 8
        {0x39, 0x9}, // 9 -> 9
        {0x45, 0xE}, // E -> E
        {0x41, 0xA}, // A -> A
        {0x30, 0x0}, // 0 -> 0
        {0x42, 0xB}, // B -> B
        {0x46, 0xF}  // F -> F
    } };

    Keyboard() {
        reset();
    }
public:
    static Keyboard& getInstance() {
        static Keyboard instance;
        return instance;
    }

    // Key state ops
    KeyState getKeyState(uint8_t chip8Key) const {
        if (chip8Key > 0xF) return KeyState::Released;
        return keyStates[chip8Key];
    }
    void setKeyState(uint8_t chip8Key, KeyState state) {
        if (chip8Key > 0xF) return;
        keyStates[chip8Key] = state;
        if (state == KeyState::Pressed) {
            lastKeyPressed = chip8Key;
        }
    }
    std::optional<uint8_t> mapPhysicalKey(uint8_t physicalKey) const {
        for (const auto& mapping : DEFAULT_MAPPING) {
            if (mapping.physicalKey == physicalKey) {
                return mapping.chip8Key;
            }
        }
        return std::nullopt;
    }

    // Last key pressed tracking
    std::optional<uint8_t> getLastKeyPressed() const {
        return lastKeyPressed;
    }
    void clearLastKeyPressed() { lastKeyPressed.reset(); }

    void reset() {
        keyStates.fill(KeyState::Released);
        lastKeyPressed.reset();
    }

    // In your main loop or input handling function
    void handleKeyEvent(uint8_t physicalKey, bool isPressed) {
        auto& keyboard = Keyboard::getInstance();
        if (auto chip8Key = keyboard.mapPhysicalKey(physicalKey)) {
            keyboard.setKeyState(*chip8Key,
                isPressed ? KeyState::Pressed : KeyState::Released);
        }
    }

    // Dont copy and dont move
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;
    Keyboard(Keyboard&&) = delete;
    Keyboard& operator=(Keyboard&&) = delete;

};