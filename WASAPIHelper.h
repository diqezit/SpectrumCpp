// WASAPIHelper.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// WASAPIHelper.h: Helper classes and functions for WASAPI operations.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_WASAPI_HELPER_H
#define SPECTRUM_CPP_WASAPI_HELPER_H

#include "Common.h"

namespace Spectrum {
    namespace WASAPI {

        // RAII wrapper for COM initialization
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

        // Helper to check HRESULT and log errors
        inline bool CheckResult(HRESULT hr, const std::string& errorMessage) {
            if (FAILED(hr)) {
                LOG_ERROR(errorMessage << " - HRESULT: 0x" << std::hex << hr);
                return false;
            }
            return true;
        }

    } // namespace WASAPI
} // namespace Spectrum

#endif // SPECTRUM_CPP_WASAPI_HELPER_H