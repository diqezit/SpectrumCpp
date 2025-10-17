// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the UIManager, the central orchestrator for all UI components
// with comprehensive delta-time based animation support.
//
// This class manages the lifecycle, drawing order, and input dispatching
// for high-level UI panels. It implements:
// - Modal-first priority input system
// - Smooth fade animations for modal overlays
// - Mouse capture coordination for drag operations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

#include "Common/Common.h"
#include <memory>
#include <functional>

namespace Spectrum {
    class AudioSettingsPanel;
    class ColorPicker;
    class ControlPanel;
    class ControllerCore;
    class Canvas;
    class UISlider;

    namespace Platform {
        class WindowManager; // Forward declaration for corrected namespace
    }

    class UIManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit UIManager(ControllerCore* controller, Platform::WindowManager* windowManager);
        ~UIManager() noexcept;

        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;
        UIManager(UIManager&&) = delete;
        UIManager& operator=(UIManager&&) = delete;

        [[nodiscard]] bool Initialize();
        void RecreateResources(Canvas& canvas, int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Window Message Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ShowAudioSettings();
        void HideAudioSettings();
        void ReleaseMouseCapture();

        void UpdateModalOverlayAnimation(float deltaTime);
        void UpdateColorPickerVisibility(float deltaTime);

        void RepositionColorPicker(int width, int height);

        [[nodiscard]] bool IsModalActive() const noexcept;
        [[nodiscard]] bool ShouldDrawColorPicker() const noexcept;
        [[nodiscard]] float GetModalOverlayAlpha() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static constexpr float kModalFadeSpeed = 12.0f;
        static constexpr float kColorPickerFadeSpeed = 8.0f;
        static constexpr float kMaxModalOverlayAlpha = 0.6f;

        ControllerCore* m_controller;
        Platform::WindowManager* m_windowManager; // Corrected type

        std::unique_ptr<ControlPanel> m_controlPanel;
        std::unique_ptr<AudioSettingsPanel> m_audioSettingsPanel;
        std::unique_ptr<ColorPicker> m_colorPicker;

        UISlider* m_activeSlider; // Non-owning pointer

        float m_modalOverlayAlpha;
        float m_colorPickerAlpha;
        bool m_shouldShowColorPicker;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_MANAGER_H