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

## Disclaimer
MiniLooper is a hobby project and not a professional-grade audio tool. While I've tested it thoroughly, audio glitches or unexpected behavior may occur. If you're using it in a live performance setting with high volume or audience, please exercise caution. Unexpected loud outputs could damage equipment or hearing. Use at your own risk.

## How to use
- In Audio Settings menu, configure your audio I/O device
- In Midi Settings menu, configure your MIDI I/O (optional)
- In Source Mixer window, configure audio input(s) (from your audio device) for any of the 2 input channels or both.
- Your configuration is saved in global settings, so for the next launch it will be automatically loaded
- In Looper window, you can control and observe looper and its tracks:
  - Record new clips or overdub on top of them
  - Pause and resume looper tracks
  - Clear loops
  - Save session
  - Pick which track is controlled by foot switch (see MIDI mapping section)
- On first launch, you will be asked to pick the directory where saved sessions will be stored. You can change it later in Session Manager menu.
- In Session Manager, you can load existing sessions via file dialog.
- In Mixer and Source Mixer, you can do the following to looper track channels and input channels:
  - Adjust volume (GainDb)
  - Adjust pan
  - Apply DSP processing (to view channel's effect chain, press FX button).

### MIDI mapping
Currently, MIDI mapping is hardcoded in the following way:
- White notes C4-F4 (MIDI notes 60, 62, 64, 65) toggle recording/overdubbing for looper tracks 1-4
- Midi CC 64, which is typically sustain pedal on most midi keyboard controllers can be used as a simulated footswitch, like a hardware looper pedal:
  - Single press toggles recording/overdubbing of selected track
  - Double press clears selected track
  - Hold clears all tracks

### Interface
I tried to keep interface deliberately simple and took advantage of Dear ImGui and it's built in window docking feature.
By default all windows would be closed, you are free to open them via top bar menu and lay them out however you find comfortable.
In the screenshot attached, you can see my current preferred layout.
You can switch between 3 built-in themes (High Contrast, Neon and Warm) by clicking Theme button in the top bar menu.

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
