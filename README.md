[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.txt)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![Build](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)
[![Latest Release](https://img.shields.io/github/v/release/diqezit/SpectrumCpp)](https://github.com/diqezit/SpectrumCpp/releases/latest)

<p align="center">
  <img src="https://github.com/user-attachments/assets/e9fd7878-be77-425b-beff-33864c91e155" width="700" alt="Main window"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/c21553d4-45cd-4549-8331-12bffa6b0358" width="340" alt="Screenshot 1"/>
  &nbsp;&nbsp;
  <img src="https://github.com/user-attachments/assets/8c31c940-f0cb-40d2-8455-31edeec50d63" width="340" alt="Screenshot 2"/>
</p>

A lightweight audio visualizer for Windows. Captures desktop audio (or a microphone/other input device) via WASAPI and renders it in real time. Works as a normal window or as a click-through overlay.

## Features

*   **Audio source picker:** Default Output (WASAPI loopback) or any capture device, selectable from the settings panel.
*   **13 visualizer styles:** Bars, Wave, Circular Wave, Cubes, Fire, Gauge, Kenwood Bars, LED Panel, Matrix LED, Particles, Sunburst (Polyline Wave), Sphere, Waterfall.
*   **Overlay mode:** Transparent, borderless, click-through window that stays on top.
*   **Settings panel:** ImGui window for renderer, FFT window, bar count, amplification, smoothing, primary color, and overlay toggle.
*   **Rendering:** Blend2D (software, on D3D11 swapchain) for the visualization; ImGui/D3D11 for the settings panel.

## Hotkeys

Active only while the main window or settings panel has focus (overlay toggle works globally).

| Key              | Action                                 |
| ---------------- | ---------------------------------------|
| **Space**        | Start / Stop audio capture             |
| **O**            | Toggle Overlay Mode                    |
| **R**            | Switch to the next visualizer style    |
| **Q**            | Cycle through render qualities         |
| **Up / Down**    | Increase / Decrease amplification      |
| **Left / Right** | Change FFT window                      |
| **- / +**        | Decrease / Increase bar count          |
| **ESC**          | Exit the app (or exit overlay mode)    |

## Notes

*   **Audio Source:** Defaults to WASAPI loopback of the default playback device. To visualize a microphone or another input, open the settings panel (click the "Show Settings" button in the top-right corner) and pick a device under Audio → Audio Source.
*   **Overlay Performance:** In overlay mode, frame rate can drop when a fullscreen application has focus — this is a Windows compositor limitation, not a bug.

## How to Build

**Requirements:**
*   Windows 10/11 (x64)
*   Visual Studio 2019/2022 with "Desktop development with C++"
*   CMake 3.20+
*   Git (dependencies are fetched into `third_party/` on first configure)

**Steps:**
1.  Clone the repository.
2.  Configure with CMake (e.g. open the folder in Visual Studio, or run `cmake -B build -S .`).
3.  Build the `SpectrumCpp` target in Release x64.

The executable is placed under `build/bin/<Config>/SpectrumCpp.exe`.

## License

MIT License. See `LICENSE.txt`.
