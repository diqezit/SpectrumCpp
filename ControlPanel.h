#ifndef SPECTRUM_CPP_CONTROL_PANEL_H
#define SPECTRUM_CPP_CONTROL_PANEL_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the ControlPanel, the main user interface for application control.
// 
// This panel provides navigation controls for renderer selection and quality
// settings, as well as action buttons for audio settings and overlay mode.
// It features a slide-in/out animation controlled by a toggle button.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "PanelAnimator.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace Spectrum
{
    class AudioManager;
    class ControllerCore;
    class GraphicsContext;
    class RendererManager;
    class UIButton;
    class WindowManager;

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

        void Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(GraphicsContext& context) const;

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
        void CreateNavigationControls(RendererManager* rm, WindowManager* wm, AudioManager* am);
        void CreateActionButtons(WindowManager* wm, AudioManager* am);

        void ToggleVisibility();
        void DrawContent(GraphicsContext& context) const;
        void DrawNavLabels(GraphicsContext& context) const;

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