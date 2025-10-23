#ifndef SPECTRUM_CPP_UI_FORMATTERS_H
#define SPECTRUM_CPP_UI_FORMATTERS_H

#include <string>
#include <sstream>
#include <iomanip>

namespace Spectrum::UIFormatters {

    inline std::wstring FormatFloat(float value, int precision = 2)
    {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(precision) << value;
        return oss.str();
    }

    inline std::wstring FormatInt(float value)
    {
        return std::to_wstring(static_cast<int>(value));
    }

    inline std::wstring FormatPercentage(float value)
    {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(0) << (value * 100.0f) << L"%";
        return oss.str();
    }

    inline std::wstring FormatDecibels(float value)
    {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << value << L" dB";
        return oss.str();
    }

} // namespace Spectrum::UIFormatters

#endif // SPECTRUM_CPP_UI_FORMATTERS_H