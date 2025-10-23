#ifndef SPECTRUM_CPP_CONTROL_PANEL_H
#define SPECTRUM_CPP_CONTROL_PANEL_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the ControlPanel, the main user interface for application control.
//
// This panel provides navigation controls for renderer selection and quality
// settings, as well as action buttons for audio settings and overlay mode.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "UI/Panels/PanelAnimator.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace Spectrum
{
    class AudioManager;
    class ControllerCore;
    class Canvas;
    class RendererManager;
    class UIButton;

    namespace Platform {
        class WindowManager;
    }

    class ControlPanel final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit ControlPanel(ControllerCore* controller);
        ~ControlPanel() noexcept;

        ControlPanel(const ControlPanel&) = delete;
        ControlPanel& operator=(const ControlPanel&) = delete;
        ControlPanel(ControlPanel&&) = delete;
        ControlPanel& operator=(ControlPanel&&) = delete;

        void Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void Draw(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetOnShowAudioSettings(std::function<void()>&& callback);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Types
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct NavLabel
        {
            Point position;
            std::function<std::wstring()> textSource;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void CreateWidgets();

        void CreateNavigationControls(
            RendererManager* rm,
            Platform::WindowManager* wm,
            AudioManager* am
        );

        void CreateActionButtons(
            Platform::WindowManager* wm,
            AudioManager* am
        );

        void ToggleVisibility();
        void DrawContent(Canvas& canvas) const;
        void DrawNavLabels(Canvas& canvas) const;

        [[nodiscard]] bool IsToggleButtonHovered(const Point& mousePos) const noexcept;
        [[nodiscard]] Rect GetToggleButtonRect() const noexcept;
        [[nodiscard]] float GetContentXOffset() const noexcept;
        [[nodiscard]] Point GetTransformedMousePosition(const Point& mousePos) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ControllerCore* m_controller;
        PanelAnimator m_animator;
        bool m_isToggleButtonHovered;
        bool m_wasTogglePressed;

        std::vector<std::unique_ptr<UIButton>> m_buttons;
        std::vector<NavLabel> m_navLabels;

        std::function<void()> m_onShowAudioSettings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CONTROL_PANEL_H