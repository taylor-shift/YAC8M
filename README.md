# CHIP-8 Emulator

A modern C++ implementation of a CHIP-8 emulator with full audio-visual support. This emulator provides accurate emulation of the original CHIP-8 system with configurable display scaling and sound output.

## Features

- Full CHIP-8 CPU emulation
- High-quality display output with 24x scaling (64x32 to 1536x768)
- Audio support with configurable sample rate (44.1kHz)
- Precise memory management with protected memory regions
- Built using modern C++ with SDL2 for cross-platform compatibility
- CMake build system

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.10 or higher
- SDL2 development libraries

#### Windows
- vcpkg package manager
- SDL2 (installed via vcpkg)

#### Linux
Debian/Ubuntu:
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev
```

Fedora:
```bash
sudo dnf install gcc-c++ cmake SDL2-devel
```

Arch Linux:
```bash
sudo pacman -S base-devel cmake sdl2
```

OpenSUSE:
```bash
sudo zypper install gcc-c++ cmake libSDL2-devel
```

### Build Instructions

#### Windows
1. Clone the repository:
```bash
git clone [repository-url]
cd chip8-emulator
```

2. Configure with CMake using vcpkg:
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
```

3. Build:
```bash
cmake --build build
```

#### Linux
1. Clone the repository:
```bash
git clone [repository-url]
cd chip8-emulator
```

2. Configure with CMake:
```bash
cmake -B build -S .
```

3. Build (with parallel compilation):
```bash
cmake --build build -j$(nproc)
```

## Usage

Run the emulator with a ROM file:
```bash
./chip8-emulator [path-to-rom]
```

### Controls

The emulator uses a direct mapping of keyboard keys to CHIP-8 keys:

- `1` through `9`: Map directly to CHIP-8 keys 1-9
- `A` through `F`: Map directly to CHIP-8 keys A-F
- `0`: Maps to CHIP-8 key 0
- `ESC`: Quit emulator

## Implementation Details

### Memory Layout

The emulator implements a strict memory protection system with dedicated regions:

- `0x000-0x1FF`: Reserved for interpreter (protected)
- `0x200-0xE9F`: Program/ROM space
- `0xEA0-0xEFF`: Stack space (16 levels)
- `0xF00-0xFFF`: Display refresh
- `0x050-0x09F`: Built-in font data (protected)

### Instruction Set

The emulator supports all standard CHIP-8 instructions, organized into categories:

#### System and Flow Control
- `00E0`: Clear display
- `00EE`: Return from subroutine
- `1NNN`: Jump to address NNN
- `2NNN`: Call subroutine at NNN
- `BNNN`: Jump to address NNN + V0

#### Skip Instructions (Conditionals)
- `3XNN`: Skip if VX == NN
- `4XNN`: Skip if VX != NN
- `5XY0`: Skip if VX == VY
- `9XY0`: Skip if VX != VY

#### Register Operations
- `6XNN`: Set VX to NN
- `7XNN`: Add NN to VX
- `8XY0`: Set VX to VY
- `8XY1`: VX = VX OR VY
- `8XY2`: VX = VX AND VY
- `8XY3`: VX = VX XOR VY
- `8XY4`: Add VY to VX with carry
- `8XY5`: Subtract VY from VX
- `8XY6`: Shift VX right
- `8XY7`: VX = VY - VX
- `8XYE`: Shift VX left

#### Memory Operations
- `ANNN`: Set index register I to NNN
- `FX55`: Store V0-VX in memory starting at I
- `FX65`: Load V0-VX from memory starting at I
- `FX29`: Set I to sprite location for character in VX
- `FX33`: Store BCD representation of VX

#### Timer Operations
- `FX07`: Get delay timer value
- `FX15`: Set delay timer
- `FX18`: Set sound timer

#### Input Operations
- `EX9E`: Skip if key VX pressed
- `EXA1`: Skip if key VX not pressed
- `FX0A`: Wait for key press

#### Graphics
- `DXYN`: Draw sprite at (VX,VY) with N bytes of sprite data

#### Special
- `CXNN`: Random AND operation
- `FX1E`: Add VX to index register I

### CPU Execution Model

The CPU implements a fetch-decode-execute cycle:

1. **Fetch**: Reads two bytes from memory at PC to form a 16-bit opcode
2. **Decode**: Breaks down the opcode into components:
   - First nibble (type)
   - Second nibble (X register)
   - Third nibble (Y register)
   - Last nibble (N)
   - Last byte (NN)
   - Last 12 bits (NNN)
3. **Execute**: Processes the instruction with bounds checking and memory protection

#### Timing
- CPU frequency: 100 kHz (configurable)
- Timers: 60 Hz update rate
- Sound: Square wave generated when sound timer > 0

### Display System

#### Display Architecture
- Native resolution: 64x32 pixels monochrome
- Memory-mapped display buffer (0xF00-0xFFF)
- Efficient 64-bit row representation
- Hardware-accelerated scaling via SDL2

#### Display Buffer Organization
- Uses 32 rows of 64-bit integers
- Each bit represents one pixel (1 = white, 0 = black)
- Total display memory: 256 bytes
- Direct memory mapping to CHIP-8's display region

#### Sprite Drawing System
The sprite drawing system implements CHIP-8's XOR-based graphics:

1. **Sprite Format**
   - 8 pixels wide, 1-15 pixels tall
   - Each byte represents one row of 8 pixels
   - Sprite data loaded from memory at address I
   - Built-in 4x5 pixel font sprites (0-F)

2. **Drawing Process**
   - XOR operation for pixel collisions
   - Automatic screen wrapping
   - Collision detection sets VF register
   - Efficient 64-bit row operations

## Test ROMs

The emulator includes several test ROMs in the `test_roms/` directory:

- `1-chip8-logo.ch8`: Displays the CHIP-8 logo
- `3-corax.ch8`: Corax test ROM
- `chiptest.ch8`: Basic functionality test
- `flags.ch8`: Tests flag operations
- `keypadtest.ch8`: Tests keyboard input
- `test_opcode.ch8`: Tests various opcodes
- `octojam9title.ch8`: Octojam 9 title screen demo
