#ifndef SPECTRUM_CPP_WASAPI_HELPER_H
#define SPECTRUM_CPP_WASAPI_HELPER_H

#include "Common/Common.h"

namespace Spectrum {
    namespace WASAPI {

        class ScopedCOMInitializer {
        public:
            ScopedCOMInitializer();
            ~ScopedCOMInitializer();

            ScopedCOMInitializer(const ScopedCOMInitializer&) = delete;
            ScopedCOMInitializer& operator=(const ScopedCOMInitializer&) = delete;

            bool IsInitialized() const { return m_initialized; }

        private:
            bool m_initialized;
        };

        inline bool CheckResult(HRESULT hr, const std::string& errorMessage) {
            UNREFERENCED_PARAMETER(errorMessage);
            if (FAILED(hr)) {
                LOG_ERROR(errorMessage << " - HRESULT: 0x" << std::hex << hr);
                return false;
            }
            return true;
        }

    } // namespace WASAPI
} // namespace Spectrum

#endif // SPECTRUM_CPP_WASAPI_HELPER_H