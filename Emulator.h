#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>
#include "CPU.h"
#include "Memory.h"
#include "Display.h"
#include "Keyboard.h"

// Configuration for our emulator window and timing
class EmulatorConfig {
public:
	// Display configuration
	static constexpr int WINDOW_SCALE = 24;  // Scale up CHIP-8's 64x32 display
	static constexpr int WINDOW_WIDTH = Display::WIDTH_PX * WINDOW_SCALE;
	static constexpr int WINDOW_HEIGHT = Display::HEIGHT_PX * WINDOW_SCALE;

	// Audio configuration
	static constexpr int AUDIO_SAMPLE_RATE = 44100;
	static constexpr int AUDIO_BUFFER_SIZE = 512;

	// Timing configuration
	static constexpr int TARGET_FPS = 120;
	static constexpr int FRAME_DELAY_MS = 1000 / TARGET_FPS;
	static constexpr int INSTRUCTIONS_PER_FRAME = 15;
};

class Emulator {
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	CPU& cpu;
	Display& display;
	Keyboard& keyboard;
	bool running;
	SDL_AudioDeviceID audioDevice;
	std::vector<uint8_t> audioBuffer;

	// Initialize SDL and create window/renderer + audio output
	bool initialize() {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
			std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
			return false;
		}

		window = SDL_CreateWindow(
			"CHIP-8 Emulator",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			EmulatorConfig::WINDOW_WIDTH,
			EmulatorConfig::WINDOW_HEIGHT,
			SDL_WINDOW_SHOWN
		);

		if (!window) {
			std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
			return false;
		}

		renderer = SDL_CreateRenderer(
			window,
			-1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
		);

		if (!renderer) {
			std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
			return false;
		}

		// Set render scale to match our window size
		SDL_RenderSetScale(renderer,
			EmulatorConfig::WINDOW_SCALE,
			EmulatorConfig::WINDOW_SCALE);

		// Init audio
		SDL_AudioSpec want, have;
		SDL_zero(want);
		want.freq = EmulatorConfig::AUDIO_SAMPLE_RATE;
		want.format = AUDIO_U8;        // Simple 8-bit unsigned audio
		want.channels = 1;             // Mono output
		want.samples = EmulatorConfig::AUDIO_BUFFER_SIZE;
		want.callback = nullptr;       // We'll use SDL_QueueAudio instead

		audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
		if (audioDevice == 0) {
			std::cerr << "Audio device initialization failed: " << SDL_GetError() << std::endl;
			return false;
		}

		// Create a simple square wave
		audioBuffer.resize(512);
		for (size_t i = 0; i < audioBuffer.size() / 2; i++) {
			audioBuffer[i] = 32;      // Half amplitude for first half
			audioBuffer[i + audioBuffer.size() / 2] = 224; // Full amplitude for second half
		}

		SDL_PauseAudioDevice(audioDevice, 0);  // Start audio device

		return true;
	}

	// Load ROM file into memory
	bool loadROM(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file) {
			std::cerr << "Failed to open ROM file: " << filename << std::endl;
			return false;
		}

		// Get file size and read it into a vector
		auto size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			std::cerr << "Failed to read ROM file" << std::endl;
			return false;
		}

		try {
			Memory::getInstance().loadProgram(buffer, Memory::BIN_START);
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to load ROM into memory: " << e.what() << std::endl;
			return false;
		}

		return true;
	}

	// Handle SDL events (keyboard input, window events, etc.)
	void handleEvents() {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				bool isPressed = event.type == SDL_KEYDOWN;
				// Convert SDL keycode to our internal representation
				if (auto chip8Key = keyboard.mapPhysicalKey(event.key.keysym.sym)) {
					keyboard.setKeyState(*chip8Key,
						isPressed ? KeyState::Pressed
						: KeyState::Released);
				}
				// Add escape key to quit
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					running = false;
				}
				break;
			}
			}
		}
	}

	// Render the CHIP-8 display to our SDL window
	void render() {
		if (!display.needsRedraw()) {
			return;
		}

		// Clear screen to black
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// Set draw color to white for pixels
		static std::vector<SDL_Point> points;
		points.clear();
		points.reserve(Display::WIDTH_PX * Display::HEIGHT_PX);

		// Draw each pixel
		for (int y = 0; y < Display::HEIGHT_PX; y++) {
			for (int x = 0; x < Display::WIDTH_PX; x++) {
				if (display.getPixel(x, y)) {
					points.push_back({ x, y });
				}
			}
		}

		if (!points.empty()) {
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderDrawPoints(renderer, points.data(), points.size());
		}

		SDL_RenderPresent(renderer);
		display.setRedraw(false);
	}

	void updateAudio() {
		if (cpu.getSoundTimer() > 0) {
			SDL_QueueAudio(
				audioDevice,
				audioBuffer.data(),
				static_cast<uint32_t>(audioBuffer.size()) // Capped at 512 bytes so safe cast
			);
		}
		else {
			SDL_ClearQueuedAudio(audioDevice);
		}
																																																																																																																		}																																																			
public:
	Emulator() :
		window(nullptr),
		renderer(nullptr),
		cpu(CPU::getInstance()),
		display(Display::getInstance()),
		keyboard(Keyboard::getInstance()),
		running(false),
		audioDevice(0)
	{
	}

	~Emulator() {
		if (renderer) SDL_DestroyRenderer(renderer);
		if (window) SDL_DestroyWindow(window);
		SDL_Quit();
	}

	bool start(const std::string& romFile) {
		if (!initialize() || !loadROM(romFile)) {
			return false;
		}

		running = true;
		run();
		return true;
	}

	void run() {
		// For tracking when to update timers
		auto lastTimerUpdate = std::chrono::steady_clock::now();

		while (running) {
			auto frameStart = SDL_GetTicks();  // Start of frame timing

			// Handle input
			handleEvents();

			// Run a batch of CPU instructions
			for (int i = 0; i < EmulatorConfig::INSTRUCTIONS_PER_FRAME; i++) {
				cpu.cycle();
			}

			// Update 60Hz timers
			auto now = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(
				now - lastTimerUpdate).count() >= 16) {
				cpu.updateTimers();
				lastTimerUpdate = now;
			}

			// Render if needed
			if (display.needsRedraw()) {
				render();
			}

			// Delay to maintain target framerate
			int frameTime = SDL_GetTicks() - frameStart;
			if (frameTime < EmulatorConfig::FRAME_DELAY_MS) {
				SDL_Delay(EmulatorConfig::FRAME_DELAY_MS - frameTime);
			}
		}
	}
};