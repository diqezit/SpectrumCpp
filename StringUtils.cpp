#include "StringUtils.h"

namespace Spectrum {
    namespace Utils {

        std::wstring StringToWString(const std::string& str) {
            if (str.empty()) {
                return std::wstring();
            }

            const int sizeW = MultiByteToWideChar(
                CP_UTF8, 0, str.c_str(), -1, nullptr, 0
            );
            if (sizeW <= 0) {
                return std::wstring();
            }

            std::wstring out(static_cast<size_t>(sizeW), L'\0');
            MultiByteToWideChar(
                CP_UTF8, 0, str.c_str(), -1, out.data(), sizeW
            );

            if (!out.empty() && out.back() == L'\0') {
                out.pop_back();
            }
            return out;
        }

        std::string WStringToString(const std::wstring& wstr) {
            if (wstr.empty()) {
                return std::string();
            }

            const int sizeA = WideCharToMultiByte(
                CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr
            );
            if (sizeA <= 0) {
                return std::string();
            }

            std::string out(static_cast<size_t>(sizeA), '\0');
            WideCharToMultiByte(
                CP_UTF8, 0, wstr.c_str(), -1, out.data(), sizeA, nullptr, nullptr
            );

            if (!out.empty() && out.back() == '\0') {
                out.pop_back();
            }
            return out;
        }

    } // namespace Utils
} // namespace Spectrum