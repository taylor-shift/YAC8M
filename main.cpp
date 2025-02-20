#include "Emulator.h"

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <ROM file>" << "\n";
		return 1;
	}

	Emulator emulator;
	if (!emulator.start(argv[1])) {
		std::cerr << "Failed to start emulator..." << "\n";
	}

	return 0;
}