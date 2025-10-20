// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the UIManager, which orchestrates the lifecycle,
// drawing, and input dispatching for all high-level UI components.
// It manages which panel is currently active and receiving input.
//
// Defines the UIManager, the central orchestrator for all UI components.
// It is responsible for initializing UI panels, dispatching mouse input
// based on a clear priority model (e.g., modal-first), and managing
// component lifecycles and drawing order.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

#include "Common.h"
#include <memory>

namespace Spectrum {

    class AudioSettingsPanel;
    class ColorPicker;
    class ControlPanel;
    class ControllerCore;
    class GraphicsContext;
    class UISlider;
    class WindowManager;

    class UIManager final {
    public:
        explicit UIManager(
            ControllerCore* controller,
            WindowManager* windowManager
        );
        ~UIManager() noexcept;

        [[nodiscard]] bool Initialize();
        void RecreateResources(GraphicsContext& context, int width, int height);

        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(GraphicsContext& context) const;

    private:
        void ShowAudioSettings();
        void HideAudioSettings();

        [[nodiscard]] bool IsModalActive() const;
        [[nodiscard]] bool ShouldDrawColorPicker() const;

        ControllerCore* m_controller;
        WindowManager* m_windowManager;

        std::unique_ptr<ControlPanel> m_controlPanel;
        std::unique_ptr<AudioSettingsPanel> m_audioSettingsPanel;
        std::unique_ptr<ColorPicker> m_colorPicker;

        UISlider* m_activeSlider;
    };

}

#endif