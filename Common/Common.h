// Common.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Common.h: A central header file for all necessary system and standard libraries.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_COMMON_H
#define SPECTRUM_CPP_COMMON_H

#define WIN32_LEAN_AND_MEAN

// Windows headers
#include <windows.h>
#include <windowsx.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <wrl/client.h>
#include <dwmapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>

// Standard library headers
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <complex>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <random>
#include <optional>
#include <variant>

// Link required libraries
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "dwmapi.lib")

// Include project types
#include "Types.h"

namespace wrl = Microsoft::WRL;

// Logging macros
#ifdef _DEBUG
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
#else
#define LOG_DEBUG(msg)
#define LOG_WARNING(msg)
#define LOG_ERROR(msg)
#endif

#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl

#endif // SPECTRUM_CPP_COMMON_H