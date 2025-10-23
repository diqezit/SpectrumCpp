#ifndef SPECTRUM_CPP_DRAW_STATE_CONTROLLER_H
#define SPECTRUM_CPP_DRAW_STATE_CONTROLLER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the controller for the drawing state machine
//
// Responsibilities:
// - Manages the BeginDraw/EndDraw lifecycle for each frame
// - Prevents race conditions between drawing and other operations
// - Interprets HRESULT codes to initiate error recovery
// - Coordinates with DeviceResourceManager for rendering commands
//
// Implementation details:
// - Uses a state machine to enforce valid operation order
// - Delegates all rendering commands to the resource manager
// - Propagates device loss signals for resource recreation
// - Abstracts away mode-specific drawing logic (HWND vs Overlay)
//
// Thread safety:
// - All public methods are designed to be thread-safe
// - State transitions are protected by a mutex to prevent corruption
// - Read-only state queries use atomics for lock-free performance
// - Guarantees that drawing cannot happen concurrently with resizing
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <mutex>
#include <atomic>

namespace Spectrum {

    class DeviceResourceManager;

    class DrawStateController final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing State Enumeration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        enum class DrawingState {
            Idle,       // Not drawing, ready to begin
            Drawing,    // Currently in drawing operation
            Error       // Error state, needs recovery
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit DrawStateController(DeviceResourceManager* resourceManager);
        ~DrawStateController();

        DrawStateController(const DrawStateController&) = delete;
        DrawStateController& operator=(const DrawStateController&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Drawing Operations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool BeginDraw();
        [[nodiscard]] HRESULT EndDraw();
        void Clear(const Color& color);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsDrawing() const noexcept;
        [[nodiscard]] bool CanResize() const noexcept;
        [[nodiscard]] DrawingState GetState() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Error Recovery
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void HandleDeviceLost();
        void ResetErrorState();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Size Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSize(int width, int height);
        [[nodiscard]] int GetWidth() const noexcept { return m_currentWidth; }
        [[nodiscard]] int GetHeight() const noexcept { return m_currentHeight; }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Internal State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetState(DrawingState state);
        [[nodiscard]] bool ValidateDrawingState(DrawingState required) const;
        void EnsureDrawingComplete() noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Internal Drawing Operations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void StartDrawingOperation();
        [[nodiscard]] HRESULT FinishDrawingOperation();

        void BeginOverlayDraw();
        void BeginHwndDraw();

        [[nodiscard]] HRESULT EndOverlayDraw();
        [[nodiscard]] HRESULT EndHwndDraw();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Error Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void HandleDrawResult(HRESULT hr);
        void HandleWin32Error(HRESULT hr);
        void HandleOverlayError(HRESULT hr);
        void HandleGenericError(HRESULT hr);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        DeviceResourceManager* m_resourceManager;  // Non-owning pointer

        mutable std::mutex m_stateMutex;
        std::atomic<DrawingState> m_state;

        int m_currentWidth;
        int m_currentHeight;

        bool m_isOverlay;
        bool m_deviceLostHandled;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_DRAW_STATE_CONTROLLER_H