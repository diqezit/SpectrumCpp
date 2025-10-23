#ifndef SPECTRUM_CPP_GRAPHICS_API_HELPERS_HRESULT_HELPERS_H
#define SPECTRUM_CPP_GRAPHICS_API_HELPERS_HRESULT_HELPERS_H

#include "Common/Common.h"
#include <string_view>
#include <wrl/client.h>

namespace Spectrum::Helpers::HResult {

    inline void Check(HRESULT hr, std::string_view operation)
    {
        UNREFERENCED_PARAMETER(operation);
        if (FAILED(hr)) {
            LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
        }
    }

    [[nodiscard]] inline bool CheckWithReturn(HRESULT hr, std::string_view operation)
    {
        UNREFERENCED_PARAMETER(operation);
        if (FAILED(hr)) {
            LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
            return false;
        }
        return true;
    }

    template<typename T>
    [[nodiscard]] inline bool CheckComCreation(HRESULT hr, std::string_view operation, const Microsoft::WRL::ComPtr<T>& object)
    {
        UNREFERENCED_PARAMETER(operation);
        if (FAILED(hr) || !object) {
            LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
            return false;
        }
        return true;
    }

    template<typename T>
    [[nodiscard]] inline bool CheckComCreation(HRESULT hr, std::string_view operation, const T* object)
    {
        UNREFERENCED_PARAMETER(operation);
        if (FAILED(hr) || !object) {
            LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
            return false;
        }
        return true;
    }

} // namespace Spectrum::Helpers::HResult

#endif // SPECTRUM_CPP_GRAPHICS_API_HELPERS_HRESULT_HELPERS_H