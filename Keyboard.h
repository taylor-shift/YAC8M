#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <SDL2/SDL_Keycode.h>

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
	// from SDL to expected CHIP-8 scan code
    static constexpr std::array<KeyMapping, 16> DEFAULT_MAPPING = { {
        {SDLK_1, 0x1}, // 1 -> 1
        {SDLK_2, 0x2}, // 2 -> 2
        {SDLK_3, 0x3}, // 3 -> 3
        {SDLK_c, 0xC}, // C -> C
        {SDLK_4, 0x4}, // 4 -> 4
        {SDLK_5, 0x5}, // 5 -> 5
        {SDLK_6, 0x6}, // 6 -> 6
        {SDLK_d, 0xD}, // D -> D
        {SDLK_7, 0x7}, // 7 -> 7
        {SDLK_8, 0x8}, // 8 -> 8
        {SDLK_9, 0x9}, // 9 -> 9
        {SDLK_e, 0xE}, // E -> E
        {SDLK_a, 0xA}, // A -> A
        {SDLK_0, 0x0}, // 0 -> 0
        {SDLK_b, 0xB}, // B -> B
        {SDLK_f, 0xF}  // F -> F
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