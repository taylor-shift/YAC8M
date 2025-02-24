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
		if (!spriteBytes) return false;
		bool collision = false;

		// Process each row of the sprite
		for (uint8_t row = 0; row < height; row++) {

			// Get the Y position with proper wrapping
			uint8_t yPos = (y + row) % HEIGHT_PX;
			uint8_t spriteByte = spriteBytes[row];

			// Process each bit in the sprite byte
			for (uint8_t bit = 0; bit < 8; bit++) {
				// Check if current bit is set in sprite
				bool spritePixel = (spriteByte & (0x80 >> bit)) != 0;

				if (spritePixel) {
					// Calculate X position with wrapping
					uint8_t xPos = (x + bit) % WIDTH_PX;

					// Create a mask for the current pixel position
					uint64_t mask = 1ULL << (63 - xPos);

					// Check for collision
					if (pixels[yPos] & mask) {
						collision = true;
					}

					// XOR the pixel
					pixels[yPos] ^= mask;
				}
			}
		}

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