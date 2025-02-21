#pragma once
#include <bitset>
#include <chrono>
#include <cstdint>

class Display {
private:
	Display() : shouldRedraw(false) {
		clear();
	}
public:
	static constexpr uint8_t WIDTH_PX = 64;
	static constexpr uint8_t HEIGHT_PX = 32;
	using ScreenBuffer = std::array<uint64_t, HEIGHT_PX>;

	ScreenBuffer pixels;
	bool shouldRedraw;

	// Singleton interface
	static Display& getInstance() {
		static Display instance;
		return instance;
	}

	// Do not copy, do not move
	Display(const Display&) = delete;
	Display& operator=(const Display&) = delete;
	Display(Display&&) = delete;
	Display& operator=(Display&&) = delete;

	void clear() {
		std::fill(pixels.begin(), pixels.end(), 0);
		shouldRedraw = true;
	}

    bool drawSprite(uint8_t x, uint8_t y, const uint8_t* spriteBytes, uint8_t height) {
        // Early exit if no sprite data provided
        if (!spriteBytes) return false;

        // Track if any pixels were flipped from set to unset (collision)
        bool collision = false;

        // Get the x position within our 64-pixel wide display
        // Using bitwise AND with 0x3F is equivalent to modulo 64
        const uint8_t xPos = x & 0x3F;

        // Process each row of the sprite
        for (uint8_t row = 0; row < height && (y + row) < HEIGHT_PX; row++) {
            // Convert our 8-bit sprite data to 64 bits so we can do 64-bit operations
            uint64_t spriteBits = static_cast<uint64_t>(spriteBytes[row]);

            // Calculate which row of the display we're modifying
            // Convert to size_t before addition to prevent overflow
            size_t rowIndex = (static_cast<size_t>(y) + static_cast<size_t>(row)) & 0x1F;
            uint64_t& screenRow = pixels[rowIndex];

            // Position our sprite bits within the 64-bit row
            uint64_t mask;
            if (xPos <= 56) {
                // Normal case - we can shift left
                // Convert to uint64_t before arithmetic to prevent overflow
                mask = spriteBits << (63 - (xPos + 7));
            }
            else {
                // Edge case - sprite wraps around right edge
                // Need to shift right instead
                mask = spriteBits >> (xPos - 56);
            }

            // Check for collisions by seeing if any 1s overlap
            if (screenRow & mask) {
                collision = true;
            }

            // XOR the sprite bits with the screen row
            // If sprite bit is 1 and screen bit is 0: turns on
            // If sprite bit is 1 and screen bit is 1: turns off
            // If sprite bit is 0: no change
            screenRow ^= mask;
        }

        // Mark the display as needing redraw
        shouldRedraw = true;
        return collision;
    }

	bool getPixel(uint8_t x, uint8_t y) const {
		const uint64_t row = pixels[y & 0x1F];
		return (row & (1ULL << (63 - (x & 0x3F)))) != 0;
	}

	bool needsRedraw() const { return shouldRedraw; }
	void setRedraw(bool state) { shouldRedraw = state; }
	const auto& getScreen() const { return pixels; }
};