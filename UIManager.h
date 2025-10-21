#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the UIManager, the central orchestrator for all UI components
// with comprehensive delta-time based animation support.
// 
// This class manages the lifecycle, drawing order, and input dispatching
// for high-level UI panels. It implements:
// - Modal-first priority input system
// - Smooth fade animations for modal overlays
// - Mouse capture coordination for drag operations
// - Frame-rate independent transitions
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <memory>
#include <functional>

namespace Spectrum
{
    class AudioSettingsPanel;
    class ColorPicker;
    class ControlPanel;
    class ControllerCore;
    class GraphicsContext;
    class UISlider;
    class WindowManager;

    class UIManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit UIManager(ControllerCore* controller, WindowManager* windowManager);
        ~UIManager() noexcept;

        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;

        [[nodiscard]] bool Initialize();
        void RecreateResources(GraphicsContext& context, int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(GraphicsContext& context) const;

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
        WindowManager* m_windowManager;

        std::unique_ptr<ControlPanel> m_controlPanel;
        std::unique_ptr<AudioSettingsPanel> m_audioSettingsPanel;
        std::unique_ptr<ColorPicker> m_colorPicker;

        UISlider* m_activeSlider;

        float m_modalOverlayAlpha;
        float m_colorPickerAlpha;
        bool m_shouldShowColorPicker;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_MANAGER_H