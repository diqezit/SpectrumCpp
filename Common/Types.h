// Types.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Types.h: Core data types, constants, and enumerations for the project.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_TYPES_H
#define SPECTRUM_CPP_TYPES_H

#include <cstdint>
#include <vector>
#include <array>
#include <cmath>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Mathematical constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    inline constexpr float PI = 3.14159265358979323846f;
    inline constexpr float TWO_PI = 2.0f * PI;
    inline constexpr float HALF_PI = PI / 2.0f;
    inline constexpr float DEG_TO_RAD = PI / 180.0f;
    inline constexpr float RAD_TO_DEG = 180.0f / PI;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Default configuration values
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    inline constexpr size_t DEFAULT_FFT_SIZE = 2048;
    inline constexpr size_t DEFAULT_BAR_COUNT = 64;
    inline constexpr float DEFAULT_SMOOTHING = 0.8f;
    inline constexpr float DEFAULT_AMPLIFICATION = 1.0f;
    inline constexpr int DEFAULT_SAMPLE_RATE = 44100;
    inline constexpr float DEFAULT_FPS = 60.0f;
    inline constexpr float FRAME_TIME = 1.0f / DEFAULT_FPS;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Core data structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    struct Color {
        float r, g, b, a;
        constexpr Color(
            float r_ = 0.0f, float g_ = 0.0f, float b_ = 0.0f, float a_ = 1.0f
        ) noexcept : r(r_), g(g_), b(b_), a(a_) {
        }

        static constexpr Color FromRGB(
            uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255
        ) noexcept {
            return Color(
                red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f
            );
        }

        static constexpr Color Black() noexcept { return Color(0.0f, 0.0f, 0.0f); }
        static constexpr Color White() noexcept { return Color(1.0f, 1.0f, 1.0f); }
        static constexpr Color Red() noexcept { return Color(1.0f, 0.0f, 0.0f); }
        static constexpr Color Green() noexcept { return Color(0.0f, 1.0f, 0.0f); }
        static constexpr Color Blue() noexcept { return Color(0.0f, 0.0f, 1.0f); }
        static constexpr Color Transparent() noexcept { return Color(0, 0, 0, 0); }
    };

    struct Rect {
        float x, y, width, height;
        constexpr Rect(
            float x_ = 0.f, float y_ = 0.f, float w_ = 0.f, float h_ = 0.f
        ) noexcept : x(x_), y(y_), width(w_), height(h_) {
        }

        float GetRight() const noexcept { return x + width; }
        float GetBottom() const noexcept { return y + height; }
    };

    struct Point {
        float x, y;
        constexpr Point(float x_ = 0.f, float y_ = 0.f) noexcept : x(x_), y(y_) {}

        Point operator+(const Point& other) const noexcept {
            return Point(x + other.x, y + other.y);
        }
        Point operator*(float scalar) const noexcept {
            return Point(x * scalar, y * scalar);
        }
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Drawing Style Structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    // Describes the drawing style (brush, pen) for Canvas operations
    // Analogous to SKPaint from Skia or Pen/Brush from GDI+
    struct Paint {
        Color color = Color::White();
        float strokeWidth = 1.0f;
        bool isFilled = true;
        // In the future add more parameters
    };


    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Enumerations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    enum class RenderStyle : uint8_t {
        Bars = 0, Wave, CircularWave, Cubes, Fire, LedPanel, Gauge, KenwoodBars, Count
    };

    enum class RenderQuality : uint8_t {
        Low = 0, Medium, High, Count
    };

    enum class FFTWindowType : uint8_t {
        Hann = 0, Hamming, Blackman, Rectangular, Count
    };

    enum class SpectrumScale : uint8_t {
        Linear = 0, Logarithmic, Mel, Count
    };

    enum class InputAction {
        ToggleCapture,
        ToggleAnimation,
        ToggleOverlay,
        SwitchRenderer,
        CycleQuality,
        CycleSpectrumScale,
        IncreaseAmplification,
        DecreaseAmplification,
        NextFFTWindow,
        PrevFFTWindow,
        IncreaseBarCount,
        DecreaseBarCount,
        Exit
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Configuration structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    struct AudioConfig {
        size_t fftSize = DEFAULT_FFT_SIZE;
        size_t barCount = DEFAULT_BAR_COUNT;
        float amplification = DEFAULT_AMPLIFICATION;
        float smoothing = DEFAULT_SMOOTHING;
        FFTWindowType windowType = FFTWindowType::Hann;
        SpectrumScale scaleType = SpectrumScale::Logarithmic;
    };


    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Type aliases
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    using SpectrumData = std::vector<float>;
    using AudioBuffer = std::vector<float>;
    using ColorPalette = std::array<Color, 8>;

} // namespace Spectrum

#endif // SPECTRUM_CPP_TYPES_H