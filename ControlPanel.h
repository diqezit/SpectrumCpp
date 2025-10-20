// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the ControlPanel, a container for the primary UI
// controls. It's responsible for initializing buttons for high-level
// actions and managing its own visibility with a slide-in/slide-out animation.
//
// Defines the ControlPanel, which acts as the main user-facing surface for
// application control. It manages the layout and interaction of core command
// buttons and orchestrates its own slide-in/out animation, decoupling UI
// presentation from the core application logic.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_CONTROL_PANEL_H
#define SPECTRUM_CPP_CONTROL_PANEL_H

#include "Common.h"
#include "PanelAnimator.h"
#include <vector>
#include <memory>
#include <functional>

namespace Spectrum {

    class AudioManager;
    class ControllerCore;
    class GraphicsContext;
    class RendererManager;
    class UIButton;
    class WindowManager;

    class ControlPanel final {
    public:
        explicit ControlPanel(ControllerCore* controller);
        ~ControlPanel() noexcept;

        void Initialize();
        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(GraphicsContext& context) const;
        void SetOnShowAudioSettings(std::function<void()>&& callback);

    private:
        struct NavLabel {
            Point position;
            std::function<std::wstring()> textSource;
        };

        void CreateWidgets();
        void CreateNavigationControls(RendererManager* rm, WindowManager* wm);
        void CreateActionButtons(WindowManager* wm, AudioManager* am);

        void ToggleVisibility();
        void DrawContent(GraphicsContext& context) const;
        void DrawNavLabels(GraphicsContext& context) const;

        [[nodiscard]] bool IsToggleButtonHovered(const Point& mousePos) const;
        [[nodiscard]] Rect GetToggleButtonRect() const;
        [[nodiscard]] float GetContentXOffset() const;
        [[nodiscard]] Point GetTransformedMousePosition(const Point& mousePos) const;

        static constexpr float kPanelWidth = 220.0f;
        static constexpr float kPanelHeight = 220.0f;
        static constexpr float kToggleButtonWidth = 20.0f;

        ControllerCore* m_controller;
        PanelAnimator m_animator;
        bool m_isToggleButtonHovered;
        bool m_wasTogglePressed;

        std::vector<std::unique_ptr<UIButton>> m_buttons;
        std::vector<NavLabel> m_navLabels;

        std::function<void()> m_onShowAudioSettings;
    };

}

#endif