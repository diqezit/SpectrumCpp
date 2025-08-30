// Types.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Types.h: Core data types, constants, and enumerations for the project.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_TYPES_H
#define SPECTRUM_CPP_TYPES_H

#include <cstdint>
#include <vector>
#include <array>

namespace Spectrum {

    // Mathematical constants
    inline constexpr float PI = 3.14159265358979323846f;
    inline constexpr float TWO_PI = 2.0f * PI;
    inline constexpr float HALF_PI = PI / 2.0f;
    inline constexpr float DEG_TO_RAD = PI / 180.0f;
    inline constexpr float RAD_TO_DEG = 180.0f / PI;

    // Default configuration values
    inline constexpr size_t DEFAULT_FFT_SIZE = 2048;
    inline constexpr size_t DEFAULT_BAR_COUNT = 64;
    inline constexpr float DEFAULT_SMOOTHING = 0.8f;
    inline constexpr float DEFAULT_AMPLIFICATION = 1.0f;
    inline constexpr int DEFAULT_SAMPLE_RATE = 44100;
    inline constexpr float DEFAULT_FPS = 60.0f;
    inline constexpr float FRAME_TIME = 1.0f / DEFAULT_FPS;

    // RGBA color structure
    struct Color {
        float r, g, b, a;

        constexpr Color(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) noexcept
            : r(r), g(g), b(b), a(a) {
        }

        static constexpr Color FromRGB(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) noexcept {
            return Color(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
        }

        static constexpr Color Black() noexcept { return Color(0, 0, 0, 1); }
        static constexpr Color White() noexcept { return Color(1, 1, 1, 1); }
        static constexpr Color Red() noexcept { return Color(1, 0, 0, 1); }
        static constexpr Color Green() noexcept { return Color(0, 1, 0, 1); }
        static constexpr Color Blue() noexcept { return Color(0, 0, 1, 1); }
        static constexpr Color Transparent() noexcept { return Color(0, 0, 0, 0); }

        Color operator*(float scalar) const noexcept {
            return Color(r * scalar, g * scalar, b * scalar, a);
        }

        Color operator+(const Color& other) const noexcept {
            return Color(r + other.r, g + other.g, b + other.b, a);
        }

        Color Lerp(const Color& target, float t) const noexcept {
            return Color(
                r + (target.r - r) * t,
                g + (target.g - g) * t,
                b + (target.b - b) * t,
                a + (target.a - a) * t
            );
        }
    };

    // Rectangle structure
    struct Rect {
        float x, y, width, height;

        constexpr Rect(float x = 0, float y = 0, float width = 0, float height = 0) noexcept
            : x(x), y(y), width(width), height(height) {
        }

        float GetRight() const noexcept { return x + width; }
        float GetBottom() const noexcept { return y + height; }
        float GetCenterX() const noexcept { return x + width * 0.5f; }
        float GetCenterY() const noexcept { return y + height * 0.5f; }

        bool Contains(float px, float py) const noexcept {
            return px >= x && px <= GetRight() && py >= y && py <= GetBottom();
        }
    };

    // Point structure
    struct Point {
        float x, y;

        constexpr Point(float x = 0, float y = 0) noexcept : x(x), y(y) {}

        float Distance(const Point& other) const noexcept {
            float dx = x - other.x;
            float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        Point operator+(const Point& other) const noexcept {
            return Point(x + other.x, y + other.y);
        }

        Point operator-(const Point& other) const noexcept {
            return Point(x - other.x, y - other.y);
        }

        Point operator*(float scalar) const noexcept {
            return Point(x * scalar, y * scalar);
        }
    };

    // Enumeration for rendering styles
    enum class RenderStyle : uint8_t {
        Bars = 0,
        Wave,
        CircularWave,
        Cubes,
        Fire,
        LedPanel,
        Count // Always last
    };

    // Enumeration for rendering quality
    enum class RenderQuality : uint8_t {
        Low = 0,
        Medium,
        High,
        Count // Always last
    };

    // FFT Windowing functions
    enum class FFTWindowType : uint8_t {
        Hann = 0,
        Hamming,
        Blackman,
        Rectangular,
        Count // Always last
    };

    // Spectrum scaling modes
    enum class SpectrumScale : uint8_t {
        Linear = 0,
        Logarithmic,
        Mel,
        Count // Always last
    };

    // Application state
    struct ApplicationState {
        bool isCapturing = false;
        bool isAnimating = false;
        bool isOverlayActive = false;
        RenderStyle currentRenderer = RenderStyle::Bars;
        RenderQuality quality = RenderQuality::Medium;
    };

    // Audio configuration
    struct AudioConfig {
        size_t fftSize = DEFAULT_FFT_SIZE;
        size_t barCount = DEFAULT_BAR_COUNT;
        float amplification = DEFAULT_AMPLIFICATION;
        float smoothing = DEFAULT_SMOOTHING;
        FFTWindowType windowType = FFTWindowType::Hann;
        SpectrumScale scaleType = SpectrumScale::Logarithmic;
    };

    // Type aliases
    using SpectrumData = std::vector<float>;
    using AudioBuffer = std::vector<float>;
    using ColorPalette = std::array<Color, 8>;

} // namespace Spectrum

#endif // SPECTRUM_CPP_TYPES_H