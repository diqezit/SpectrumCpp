#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

#include "Common/Common.h"
#include <memory>

namespace Spectrum {
    class AudioSettingsPanel;
    class ColorPicker;
    class ControlPanel;
    class ControllerCore;
    class Canvas;

    namespace Platform {
        class WindowManager;
    }

    class UIManager final
    {
    public:
        explicit UIManager(
            ControllerCore* controller,
            Platform::WindowManager* windowManager
        );
        ~UIManager() noexcept;

        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;
        UIManager(UIManager&&) = delete;
        UIManager& operator=(UIManager&&) = delete;

        [[nodiscard]] bool Initialize();

        void RecreateResources(
            Canvas& canvas,
            int width,
            int height
        );

        void Update(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void Draw(Canvas& canvas) const;

        void OnResize(int width, int height);

        [[nodiscard]] bool HandleMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

    private:
        ControllerCore* m_controller;
        Platform::WindowManager* m_windowManager;

        std::unique_ptr<ControlPanel> m_controlPanel;
        std::unique_ptr<AudioSettingsPanel> m_audioSettingsPanel;
        std::unique_ptr<ColorPicker> m_colorPicker;

        bool m_shouldShowColorPicker;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_MANAGER_H