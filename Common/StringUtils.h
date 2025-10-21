#ifndef SPECTRUM_CPP_STRING_UTILS_H
#define SPECTRUM_CPP_STRING_UTILS_H

#include "Common.h"
#include <string>
#include <string_view>
#include <memory>

namespace Spectrum {
    namespace Utils {

        inline std::string_view ToString(FFTWindowType type) {
            switch (type) {
            case FFTWindowType::Hann: return "Hann";
            case FFTWindowType::Hamming: return "Hamming";
            case FFTWindowType::Blackman: return "Blackman";
            case FFTWindowType::Rectangular: return "Rectangular";
            default: return "Unknown";
            }
        }

        inline std::string_view ToString(SpectrumScale type) {
            switch (type) {
            case SpectrumScale::Linear: return "Linear";
            case SpectrumScale::Logarithmic: return "Logarithmic";
            case SpectrumScale::Mel: return "Mel";
            default: return "Unknown";
            }
        }

        std::wstring StringToWString(const std::string& str);
        std::string  WStringToString(const std::wstring& wstr);

        template<typename... Args>
        [[nodiscard]] std::string Format(
            const std::string& fmt, Args... args
        ) {
            int size = std::snprintf(nullptr, 0, fmt.c_str(), args...) + 1;
            if (size <= 0) {
                return std::string();
            }

            std::unique_ptr<char[]> buf(new char[static_cast<size_t>(size)]);
            std::snprintf(
                buf.get(), static_cast<size_t>(size), fmt.c_str(), args...
            );
            return std::string(
                buf.get(), buf.get() + (static_cast<size_t>(size) - 1)
            );
        }

    } // namespace Utils
} // namespace Spectrum

#endif