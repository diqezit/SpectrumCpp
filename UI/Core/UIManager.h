// UIManager.h
#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

#include "Common/Common.h"
#include <memory>
#include <string>

namespace Spectrum {

    class ImGuiContext;
    class ControllerCore;
    class AudioManager;
    class RendererManager;

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
        void Shutdown();

        void BeginFrame();
        void Render();
        void EndFrame();

        void OnResize(int width, int height);

        [[nodiscard]] bool HandleMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

    private:
        AudioManager* GetAudioManager() const;
        RendererManager* GetRendererManager() const;

        RenderStyle StringToRenderStyle(const std::string& name) const;
        const char* RenderStyleToString(RenderStyle style) const;

        void RenderControlPanel();
        void RenderAudioSettings();
        void RenderColorPicker();

        ControllerCore* m_controller;
        Platform::WindowManager* m_windowManager;

        std::unique_ptr<ImGuiContext> m_imguiContext;

        bool m_showControlPanel = true;
        bool m_showAudioSettings = false;
        bool m_showColorPicker = false;
        Color m_selectedColor = Color::White();
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_MANAGER_H