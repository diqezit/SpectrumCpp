// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the drawing state machine and its transitions
//
// Implementation details:
// - Delegates all rendering commands to the DeviceResourceManager
// - Guarantees BeginDraw/EndDraw are always called as a pair
// - Protects state-changing operations with a mutex for thread safety
// - Validates current state before allowing any operation to proceed
//
// Error handling strategy:
// - Treats D2DERR_RECREATE_TARGET as a signal to trigger device recovery
// - Logs specific Win32 error codes to aid in platform-specific debugging
// - Handles overlay errors by requesting a full resource recreation
// - Propagates error HRESULTs to the caller for high-level response
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Core/DrawStateController.h"
#include "Graphics/API/Core/DeviceResourceManager.h"
#include "Graphics/API/D2DHelpers.h"

namespace Spectrum {

    using namespace Helpers::HResult;
    using namespace Helpers::TypeConversion;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    DrawStateController::DrawStateController(DeviceResourceManager* resourceManager)
        : m_resourceManager(resourceManager)
        , m_state(DrawingState::Idle)
        , m_currentWidth(1)
        , m_currentHeight(1)
        , m_isOverlay(false)
        , m_deviceLostHandled(false)
    {
        if (!m_resourceManager) {
            LOG_ERROR("DrawStateController: Null resource manager provided");
            m_state = DrawingState::Error;
        }
    }

    DrawStateController::~DrawStateController()
    {
        EnsureDrawingComplete();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool DrawStateController::BeginDraw()
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (m_state == DrawingState::Drawing) {
            LOG_ERROR("BeginDraw() called while already drawing");
            return false;
        }

        if (m_state == DrawingState::Error) {
            LOG_ERROR("BeginDraw() called in error state - call ResetErrorState() first");
            return false;
        }

        if (!m_resourceManager || !m_resourceManager->GetRenderTarget()) {
            LOG_ERROR("BeginDraw() failed: no render target available");
            m_state = DrawingState::Error;
            return false;
        }

        try {
            StartDrawingOperation();
            m_state = DrawingState::Drawing;
            return true;
        }
        catch (const std::exception& e) {
            (void)e; // Suppress unreferenced variable
            LOG_ERROR("BeginDraw() exception: " << e.what());
            m_state = DrawingState::Error;
            return false;
        }
    }

    HRESULT DrawStateController::EndDraw()
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (m_state != DrawingState::Drawing) {
            LOG_WARNING("EndDraw() called without BeginDraw()");
            return S_OK;
        }

        const HRESULT hr = FinishDrawingOperation();
        m_state = DrawingState::Idle;

        HandleDrawResult(hr);
        return hr;
    }

    void DrawStateController::Clear(const Color& color)
    {
        if (m_state != DrawingState::Drawing) {
            LOG_ERROR("Clear() called outside BeginDraw/EndDraw");
            return;
        }

        ID2D1RenderTarget* target = m_resourceManager->GetRenderTarget();
        if (target) {
            target->Clear(ToD2DColor(color));
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool DrawStateController::IsDrawing() const noexcept
    {
        return m_state.load() == DrawingState::Drawing;
    }

    bool DrawStateController::CanResize() const noexcept
    {
        return m_state.load() == DrawingState::Idle;
    }

    DrawStateController::DrawingState DrawStateController::GetState() const noexcept
    {
        return m_state.load();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Error Recovery
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void DrawStateController::HandleDeviceLost()
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (m_deviceLostHandled) {
            return; // Avoid recursive handling
        }

        m_deviceLostHandled = true;
        m_state = DrawingState::Error;

        LOG_WARNING("Device lost detected, requesting resource recreation");

        if (m_resourceManager) {
            m_resourceManager->NotifyComponentsDeviceLost();
            m_resourceManager->DiscardDeviceResources();
        }

        m_deviceLostHandled = false;
    }

    void DrawStateController::ResetErrorState()
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (m_state == DrawingState::Error) {
            m_state = DrawingState::Idle;
            LOG_INFO("Error state reset, ready for drawing");
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Size Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void DrawStateController::UpdateSize(int width, int height)
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (m_state == DrawingState::Drawing) {
            LOG_ERROR("Cannot update size while drawing");
            return;
        }

        m_currentWidth = width;
        m_currentHeight = height;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Internal State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void DrawStateController::SetState(DrawingState state)
    {
        m_state.store(state);
    }

    bool DrawStateController::ValidateDrawingState(DrawingState required) const
    {
        return m_state.load() == required;
    }

    void DrawStateController::EnsureDrawingComplete() noexcept
    {
        if (m_state.load() == DrawingState::Drawing) {
            LOG_ERROR("DrawStateController destroyed while drawing in progress");
            m_state.store(DrawingState::Idle);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Internal Drawing Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void DrawStateController::StartDrawingOperation()
    {
        // FIX: Removed incorrect 'using namespace DeviceResourceManager;'

        std::visit([this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            // FIX: Qualified nested types with the class name 'DeviceResourceManager::'
            if constexpr (std::is_same_v<T, DeviceResourceManager::OverlayTargetResources>) {
                BeginOverlayDraw();
                m_isOverlay = true;
            }
            else if constexpr (std::is_same_v<T, DeviceResourceManager::HwndTargetResources>) {
                BeginHwndDraw();
                m_isOverlay = false;
            }
            else if constexpr (std::is_same_v<T, std::monostate>) {
                LOG_ERROR("BeginDraw() called with no valid render target");
                throw std::runtime_error("No render target available");
            }
            }, m_resourceManager->GetResources());
    }

    void DrawStateController::BeginOverlayDraw()
    {
        // FIX: Qualified the type used in std::get
        auto& resources = std::get<DeviceResourceManager::OverlayTargetResources>(
            m_resourceManager->GetResources()
        );

        RECT clientRect = { 0, 0, m_currentWidth, m_currentHeight };

        HRESULT hr = resources.renderTarget->BindDC(resources.hdc.get(), &clientRect);
        if (FAILED(hr)) {
            LOG_ERROR("BindDC failed: 0x" << std::hex << hr);
            throw std::runtime_error("Failed to bind DC for overlay drawing");
        }

        resources.renderTarget->BeginDraw();
    }

    void DrawStateController::BeginHwndDraw()
    {
        // FIX: Qualified the type used in std::get
        auto& resources = std::get<DeviceResourceManager::HwndTargetResources>(
            m_resourceManager->GetResources()
        );

        resources.renderTarget->BeginDraw();
    }

    HRESULT DrawStateController::FinishDrawingOperation()
    {
        HRESULT hr = S_OK;

        std::visit([this, &hr](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            // FIX: Qualified nested types with the class name 'DeviceResourceManager::'
            if constexpr (std::is_same_v<T, DeviceResourceManager::OverlayTargetResources>) {
                hr = EndOverlayDraw();
            }
            else if constexpr (std::is_same_v<T, DeviceResourceManager::HwndTargetResources>) {
                hr = EndHwndDraw();
            }
            else if constexpr (std::is_same_v<T, std::monostate>) {
                hr = E_FAIL;
                LOG_ERROR("EndDraw() called with no valid render target");
            }
            }, m_resourceManager->GetResources());

        return hr;
    }

    HRESULT DrawStateController::EndOverlayDraw()
    {
        // FIX: Qualified the type used in std::get
        auto& resources = std::get<DeviceResourceManager::OverlayTargetResources>(
            m_resourceManager->GetResources()
        );

        HRESULT hr = resources.renderTarget->EndDraw();

        if (SUCCEEDED(hr)) {
            hr = m_resourceManager->UpdateLayeredWindow(
                resources.hdc.get(),
                m_currentWidth,
                m_currentHeight
            );
        }

        return hr;
    }

    HRESULT DrawStateController::EndHwndDraw()
    {
        // FIX: Qualified the type used in std::get
        auto& resources = std::get<DeviceResourceManager::HwndTargetResources>(
            m_resourceManager->GetResources()
        );

        return resources.renderTarget->EndDraw();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Error Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void DrawStateController::HandleDrawResult(HRESULT hr)
    {
        if (hr == D2DERR_RECREATE_TARGET) {
            HandleDeviceLost();
        }
        else if (FAILED(hr)) {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
                HandleWin32Error(hr);
            }
            else if (m_isOverlay) {
                HandleOverlayError(hr);
            }
            else {
                HandleGenericError(hr);
            }
        }
    }

    void DrawStateController::HandleWin32Error(HRESULT hr)
    {
        const DWORD errorCode = HRESULT_CODE(hr);
        LOG_ERROR("Win32 error in rendering: " << errorCode << " (" << std::hex << hr << ")");

        // Set error state for critical errors
        if (errorCode == ERROR_INVALID_WINDOW_HANDLE ||
            errorCode == ERROR_DC_NOT_FOUND) {
            m_state = DrawingState::Error;
        }
    }

    void DrawStateController::HandleOverlayError(HRESULT hr)
    {
        LOG_WARNING("Overlay rendering error: 0x" << std::hex << hr);

        // For overlay errors, we may need to recreate resources
        if (hr != S_OK && hr != D2DERR_RECREATE_TARGET) {
            LOG_INFO("Requesting resource recreation for overlay");
            HandleDeviceLost();
        }
    }

    void DrawStateController::HandleGenericError(HRESULT hr)
    {
        (void)hr; // Suppress unreferenced parameter warning in release builds
        LOG_ERROR("EndDraw failed with HRESULT: 0x" << std::hex << hr);

        // For persistent errors, set error state
        static int errorCount = 0;
        if (++errorCount > 3) {
            m_state = DrawingState::Error;
            errorCount = 0;
        }
    }

} // namespace Spectrum