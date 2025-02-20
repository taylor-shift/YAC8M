cmake_minimum_required(VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("CHIP-8 Emulator" VERSION 1.0.0)

# Debug information to help understand the configuration
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_ROOT: $ENV{VCPKG_ROOT}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Set up vcpkg toolchain if not already set
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
        message(STATUS "Setting toolchain file from VCPKG_ROOT")
    else()
        message(FATAL_ERROR "VCPKG_ROOT environment variable is not set!")
    endif()
endif()

# Try to find SDL2
find_package(SDL2 CONFIG REQUIRED)

# If we get here, SDL2 was found successfully
message(STATUS "SDL2 was found!")

# Add source files to the project
add_executable(chip8_emulator 
    "main.cpp"
    "Memory.h"
    "Display.h"
    "CPU.h"
    "Instructions.h"
    "Instructions.cpp"
    "Keyboard.h"
    "Emulator.h"
)

# Link SDL2 libraries
target_link_libraries(chip8_emulator
    PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
)

# Set C++ standard
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET chip8_emulator PROPERTY CXX_STANDARD 20)
endif()

# Set the startup project in Visual Studio
if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT chip8_emulator)
endif()