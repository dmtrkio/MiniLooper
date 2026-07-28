# MiniLooper

A barebones audio looper.

<img width="2559" height="1198" alt="MiniLooper" src="https://github.com/user-attachments/assets/edde783b-0070-4d44-876c-a8295cff46f5" />

## Features

- 4-track looper with Gain/Pan mixing
- Load/Save for looper sessions
- Control via keyboard shortcuts (shown in Control Help window)
- Control via MIDI bindings (hardcoded for now, I plan to make it customizable in future)
- 6 DSP effects for all tracks and input sources
  - 6-band Equalizer
  - Simple guitar amp emulator
  - Stereo chorus
  - Pitch shifter
  - Stereo reverb based on Schroeder reverberator
  - Autowah-like dynamic filter

## Dependencies

| Library | Repository |
| --- | --- |
| PortAudio | [GitHub](https://github.com/PortAudio/portaudio) |
| PortMidi | [GitHub](https://github.com/PortMidi/portmidi) |
| SDL3 | [GitHub](https://github.com/libsdl-org/SDL) |
| Dear ImGui | [GitHub](https://github.com/ocornut/imgui) |
| nlohmann::json | [GitHub](https://github.com/nlohmann/json) |
| moodycamel::readerwriterqueue | [GitHub](https://github.com/cameron314/readerwriterqueue) |
| dr_wav | [GitHub](https://github.com/mackron/dr_libs) (vendored) |
| Catch2 | [GitHub](https://github.com/catchorg/Catch2) (testing) |

All dependencies are fetched automatically via CMake `FetchContent` (except dr_wav, which is vendored).

## Prerequisites

- CMake 3.21+
- C++23 compiler

## How to Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Executable will be at:
./build/MiniLooper
```
