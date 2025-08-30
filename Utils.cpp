// Utils.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Utils.cpp: Implementation of utility functions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Utils.h"

namespace Spectrum {
    namespace Utils {

        // Color utilities implementation

        Color HSVtoRGB(const HSV& hsv) {
            float h = hsv.h;
            float s = Saturate(hsv.s);
            float v = Saturate(hsv.v);

            if (s <= 0.0f) return Color(v, v, v, 1.0f);

            h = std::fmodf(h, 1.0f);
            if (h < 0.0f) h += 1.0f;
            h *= 6.0f;

            const int   i = static_cast<int>(std::floor(h));
            const float f = h - static_cast<float>(i);
            const float p = v * (1.0f - s);
            const float q = v * (1.0f - s * f);
            const float t = v * (1.0f - s * (1.0f - f));

            switch (i % 6) {
            case 0: return Color(v, t, p, 1.0f);
            case 1: return Color(q, v, p, 1.0f);
            case 2: return Color(p, v, t, 1.0f);
            case 3: return Color(p, q, v, 1.0f);
            case 4: return Color(t, p, v, 1.0f);
            case 5: return Color(v, p, q, 1.0f);
            default: return Color(v, v, v, 1.0f);
            }
        }

        HSV RGBtoHSV(const Color& rgb) {
            const float r = Saturate(rgb.r);
            const float g = Saturate(rgb.g);
            const float b = Saturate(rgb.b);

            const float maxV = std::max({ r, g, b });
            const float minV = std::min({ r, g, b });
            const float delta = maxV - minV;

            HSV out{};
            out.v = maxV;

            if (delta < 1e-6f) {
                out.s = 0.0f;
                out.h = 0.0f;
                return out;
            }

            out.s = (maxV <= 0.0f) ? 0.0f : (delta / maxV);

            if (r >= maxV)
                out.h = (g - b) / delta;
            else if (g >= maxV)
                out.h = 2.0f + (b - r) / delta;
            else
                out.h = 4.0f + (r - g) / delta;

            out.h /= 6.0f;
            if (out.h < 0.0f) out.h += 1.0f;

            return out;
        }

        uint32_t ColorToARGB(const Color& color) {
            const auto toByte = [](float c) -> uint8_t {
                return static_cast<uint8_t>(Clamp(std::round(c * 255.0f), 0.0f, 255.0f));
                };
            const uint8_t a = toByte(color.a);
            const uint8_t r = toByte(color.r);
            const uint8_t g = toByte(color.g);
            const uint8_t b = toByte(color.b);

            return (static_cast<uint32_t>(a) << 24) |
                (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) |
                static_cast<uint32_t>(b);
        }

        Color ARGBtoColor(uint32_t argb) {
            const float a = static_cast<float>((argb >> 24) & 0xFF) / 255.0f;
            const float r = static_cast<float>((argb >> 16) & 0xFF) / 255.0f;
            const float g = static_cast<float>((argb >> 8) & 0xFF) / 255.0f;
            const float b = static_cast<float>((argb) & 0xFF) / 255.0f;
            return Color(r, g, b, a);
        }

        Color InterpolateColor(const Color& c1, const Color& c2, float t) {
            t = Saturate(t);
            return Color(
                Lerp(c1.r, c2.r, t),
                Lerp(c1.g, c2.g, t),
                Lerp(c1.b, c2.b, t),
                Lerp(c1.a, c2.a, t)
            );
        }

        Color AdjustBrightness(const Color& color, float factor) {
            return Color(
                Clamp(color.r * factor, 0.0f, 1.0f),
                Clamp(color.g * factor, 0.0f, 1.0f),
                Clamp(color.b * factor, 0.0f, 1.0f),
                color.a
            );
        }

        Color AdjustSaturation(const Color& color, float factor) {
            HSV hsv = RGBtoHSV(color);
            hsv.s = Clamp(hsv.s * factor, 0.0f, 1.0f);
            Color out = HSVtoRGB(hsv);
            out.a = color.a; // preserve alpha
            return out;
        }

        // String utilities implementation

        std::wstring StringToWString(const std::string& str) {
            if (str.empty()) return std::wstring();

            const int sizeW = MultiByteToWideChar(
                CP_UTF8, 0, str.c_str(), -1, nullptr, 0
            );
            if (sizeW <= 0) return std::wstring();

            std::wstring out(static_cast<size_t>(sizeW), L'\0');
            MultiByteToWideChar(
                CP_UTF8, 0, str.c_str(), -1, out.data(), sizeW
            );
            // Drop trailing null written by WinAPI
            if (!out.empty() && out.back() == L'\0') out.pop_back();
            return out;
        }

        std::string WStringToString(const std::wstring& wstr) {
            if (wstr.empty()) return std::string();

            const int sizeA = WideCharToMultiByte(
                CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr
            );
            if (sizeA <= 0) return std::string();

            std::string out(static_cast<size_t>(sizeA), '\0');
            WideCharToMultiByte(
                CP_UTF8, 0, wstr.c_str(), -1, out.data(), sizeA, nullptr, nullptr
            );
            // Drop trailing null written by WinAPI
            if (!out.empty() && out.back() == '\0') out.pop_back();
            return out;
        }

        // Timer implementation

        Timer::Timer()
            : m_startTime(std::chrono::steady_clock::now()) {
        }

        void Timer::Reset() noexcept {
            m_startTime = std::chrono::steady_clock::now();
        }

        float Timer::GetElapsedSeconds() const noexcept {
            const auto now = std::chrono::steady_clock::now();
            return std::chrono::duration<float>(now - m_startTime).count();
        }

        float Timer::GetElapsedMilliseconds() const noexcept {
            return GetElapsedSeconds() * 1000.0f;
        }

        // Random implementation

        Random& Random::Instance() {
            static Random inst;
            return inst;
        }

        Random::Random()
            : m_generator(std::random_device{}())
            , m_unitDist(0.0f, 1.0f) {
        }

        float Random::Float(float min, float max) {
            if (max < min) std::swap(min, max);
            return min + m_unitDist(m_generator) * (max - min);
        }

        int Random::Int(int min, int max) {
            if (max < min) std::swap(min, max);
            std::uniform_int_distribution<int> dist(min, max);
            return dist(m_generator);
        }

        bool Random::Bool(float probability) {
            const float p = Saturate(probability);
            return m_unitDist(m_generator) < p;
        }

    } // namespace Utils
} // namespace Spectrum