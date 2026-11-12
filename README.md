# DuganAutomixer

A four-microphone gain-sharing automixer built with C++17 and JUCE. It runs as a macOS Audio Unit, VST3, or standalone application.

I built this project to learn how an automixer behaves in practice and to work through the real-time details around smoothing, metering, UI communication, and plugin state. When several microphones are open, the plugin gives more gain to the active speaker and turns down quieter inputs. The result is a mono mix sent to both output channels.

![DuganAutomixer interface showing four input channels, gain sharing, output metering, and level controls](assets/dugan-automixer-ui.png)

## Features

- Four discrete microphone inputs
- Dugan-style gain sharing based on relative signal power
- Per-channel mute, editable names, and input/gain meters
- Automix bypass and -24 to +6 dB output trim
- Smoothed gain, mute, bypass, and output-level changes
- Session recall for names, mute states, automix state, and output gain

## How it works

Each input is squared and passed through an attack/release envelope follower. The gain for a channel is its share of the total measured power:

```text
P_i[n] = alpha * P_i[n-1] + (1 - alpha) * x_i[n]^2

gain_i[n] = (P_i[n] + epsilon) / (sum(P_j[n]) + 4 * epsilon)
```

With four equally powered inputs, each settles near a gain of `0.25` (`-12 dB`). When one input dominates, its gain approaches `1.0` while the others are attenuated.

The envelope uses a 10 ms attack and 100 ms release, followed by 30 ms gain smoothing. Muted channels are removed from the power calculation so a loud muted microphone cannot suppress the remaining inputs.

## Real-time design

The part I found most interesting was keeping the audio callback separate from the interface:

- DSP state uses fixed-size arrays, and `processBlock` does not explicitly allocate memory, access files, or acquire a mutex.
- The audio thread publishes meter values through atomics. The UI reads them at 30 Hz and clears stored peaks after each read.
- Input names are protected separately because the audio callback never needs to read them.
- Mute and bypass changes are faded to avoid abrupt discontinuities.
- Plugin state is serialized as XML for host-session recall.

## Building

The project uses CMake 3.22 or later and includes JUCE 9.0.2 as a Git submodule. On macOS it builds AU, VST3, and Standalone targets. On Windows it builds VST3 and Standalone targets.

For a fresh checkout, clone the repository and its JUCE submodule together:

```sh
git clone --recurse-submodules https://github.com/reiddethorne/DuganAutomixer.git
cd DuganAutomixer
```

If the repository is already cloned, initialise the submodule with:

```sh
git submodule update --init --recursive
```

### macOS

Install Xcode and CMake, then generate and build an Xcode project:

```sh
cmake -S . -B build/macos -G Xcode
cmake --build build/macos --config Release \
    --target DuganAutomixer_AU DuganAutomixer_VST3 DuganAutomixer_Standalone
```

macOS 12 is the minimum deployment target. To build a universal binary, add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` to the configure command.

### Windows

Install CMake and Visual Studio with the Desktop development with C++ workload, then run these commands from a Developer PowerShell:

```powershell
cmake -S . -B build/windows -A x64
cmake --build build/windows --config Release `
    --target DuganAutomixer_VST3 DuganAutomixer_Standalone
```

Build products are written beneath `build/macos/DuganAutomixer_artefacts/` on macOS and `build/windows/DuganAutomixer_artefacts/` on Windows. `COPY_PLUGIN_AFTER_BUILD` is disabled, so builds do not modify system plugin directories automatically.

The plugin requires a host configuration with four discrete inputs and two outputs. Both outputs carry the same mono mix.

## Current limitations

- The channel layout is fixed at four inputs and two identical outputs.
- Controls are saved with the session but are not exposed as host-automation parameters.
- Windows builds are configured but have not yet been built or tested on Windows.
- There is no output limiter, so bypassing automix may require additional headroom.
- Automated DSP tests, callback benchmarks, and systematic testing across plugin hosts are still to be added.

My next step is to add signal-based regression tests across sample rates and buffer sizes, then measure worst-case callback time rather than making assumptions about performance.

## Code guide

- [`Source/PluginProcessor.cpp`](Source/PluginProcessor.cpp): DSP, metering, bus layout, and state recall
- [`Source/PluginProcessor.h`](Source/PluginProcessor.h): processor state and audio/UI communication
- [`Source/PluginEditor.cpp`](Source/PluginEditor.cpp): controls, meters, and interface layout
