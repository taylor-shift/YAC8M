#pragma once
#include <bitset>
#include <chrono>
#include <cstdint>

class Display {
public:
	static constexpr uint8_t WIDTH_PX = 64;
	static constexpr uint8_t HEIGHT_PX = 32;

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
		pixels.reset();
		shouldRedraw = true;
	}

	// Draws a sprite to the screen using XOR operation:
	// - If sprite pixel is 1 and screen pixel is 0: pixel turns on
	// - If sprite pixel is 1 and screen pixel is 1: pixel turns off (collision)
	// - If sprite pixel is 0: screen pixel unchanged
	// Returns true if any pixels were erased (collision occurred)
	bool drawSprite(uint8_t x, uint8_t y, const uint8_t* spriteBytes, uint8_t height) {
		if (!spriteBytes) return false;

		bool collision = false;
		for (uint8_t row = 0; row < height; row++) {

			uint8_t currByte = spriteBytes[row];
			for (uint8_t col = 0; col < 8; col++) {

				if (currByte & (0x80 >> col)) {
					uint8_t pixelX = x + col;
					uint8_t pixelY = y + row;

					bool pixelSet = getPixel(pixelX, pixelY);
					if (pixelSet) {
						collision = true;
					}

					setPixel(pixelX, pixelY, !pixelSet);
				}
			}
		}
		// Prep for next frame
		shouldRedraw = true;
		return collision;
	}

	bool getPixel(uint8_t x, uint8_t y) const {
		return pixels[getIndex(x, y)];
	}

	bool needsRedraw() const { return shouldRedraw; }
	void setRedraw(bool state) { shouldRedraw = state; }
	const auto& getScreen() const { return pixels; }

private:
	std::bitset<WIDTH_PX* HEIGHT_PX> pixels;
	bool shouldRedraw;

	size_t getIndex(uint8_t x, uint8_t y) const noexcept {
		return (static_cast<size_t>(y & 0x1F)) * WIDTH_PX + (x & 0x3F);
	}

	void setPixel(uint8_t x, uint8_t y, bool state) noexcept {
		pixels[getIndex(x, y)] = state;
	}

	Display() : shouldRedraw(false) {
		clear();
	}
};