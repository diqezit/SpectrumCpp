// WASAPIHelper.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// WASAPIHelper.cpp: Implementation of WASAPI helper functions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "WASAPIHelper.h"

namespace Spectrum {
    namespace WASAPI {

        ScopedCOMInitializer::ScopedCOMInitializer() : m_initialized(false) {
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

            if (SUCCEEDED(hr)) {
                m_initialized = true;
            }
            else if (hr == RPC_E_CHANGED_MODE) {
                // Already initialized in a different mode, which is acceptable.
                m_initialized = true;
            }
            else {
                LOG_ERROR("Failed to initialize COM: " << hr);
            }
        }

        ScopedCOMInitializer::~ScopedCOMInitializer() {
            if (m_initialized) {
                CoUninitialize();
            }
        }

    } // namespace WASAPI
} // namespace Spectrum