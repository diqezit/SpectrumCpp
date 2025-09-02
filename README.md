
# SpectrumC++: Real-Time Audio Visualizer

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.txt)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![Build](https://img.shields.io/badge/Build-Visual_Studio-5C2D91?logo=visualstudio)
[![Latest Release](https://img.shields.io/github/v/release/diqezit/SpectrumCpp)](https://github.com/diqezit/SpectrumCpp/releases/latest)

<img width="802" height="632" alt="image" src="https://github.com/user-attachments/assets/74336351-12e9-4207-b49d-caf398699d82" />

A simple and lightweight audio visualizer for Windows. It captures any sound playing on your PC (like music from Spotify, YouTube, or games) and turns it into beautiful, real-time animations. Works both as a standalone window and as a seamless, click-through overlay on top of your screen.

## 🚀 Core Features

*   **Capture Any Desktop Audio:** Uses WASAPI Loopback to visualize sound from any application—no configuration needed.
*   **Multiple Visualization Styles:** Choose from several built-in renderers: Bars, Wave, Circular Wave, Cubes, Fire, and LED Panel.
*   **Seamless Overlay Mode:** A transparent, borderless, and click-through window that you can place over your games, desktop, or other applications.
*   **Real-Time Customization:** Use hotkeys to change colors, styles, sensitivity, and other parameters instantly.
*   **High Performance:** Written in C++ with Direct2D for smooth, 60 FPS hardware-accelerated rendering.
*   **Zero Dependencies:** No need to install any third-party libraries. Just build and run.

## ⌨️ Hotkeys

| Key             | Action                              |
| --------------- | ----------------------------------- |
| **Space**       | Start / Stop audio capture          |
| **O**           | Toggle Overlay Mode                 |
| **R**           | Switch to the next visualizer style |
| **Q**           | Cycle through render qualities      |
| **Up / Down**   | Increase / Decrease sensitivity     |
| **Left / Right**| Change FFT window (adjusts visuals) |
| **- / +**       | Decrease / Increase bar count       |
| **S**           | Switch frequency scale (Linear/Log) |
| **ESC**         | Exit the app (or exit overlay mode) |

## 💡 Tips & Notes

*   **Audio Source:** The visualizer captures sound from your **default playback device**. If you don't see any activity, make sure the correct device is set as default in Windows Sound settings.
*   **Overlay Performance:** For the smoothest 60 FPS animation in overlay mode, you may need to click on your desktop or an empty area to make it the "active" window. When a fullscreen game or another application is active in the foreground, Windows may limit the visualizer's frame rate to ~30 FPS.

## 🛠️ How to Build & Run

**Requirements:**
*   Windows 10 or 11 (x64)
*   Visual Studio 2019/2022 with the **"Desktop development with C++"** workload.

**Steps:**
1.  Clone this repository to your computer.
2.  Open the `SpectrumC++.sln` file in Visual Studio.
3.  Set the configuration to **Release** and the platform to **x64**.
4.  Build the solution (press **F5** or go to `Build` > `Build Solution`).

The executable (`SpectrumC++.exe`) will be located in the `x64/Release` folder.

## 📄 License

This project is licensed under the MIT License. See the `LICENSE.txt` file for details.
