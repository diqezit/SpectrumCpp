#ifndef SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H
#define SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the AudioSettingsPanel, a modal dialog for audio configuration.
// 
// This panel provides user controls for audio processing parameters like
// amplification, smoothing, and bar count. It features fade-in/scale-up
// animations and click-outside-to-close behavior.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "UI/Panels/PanelAnimator.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace Spectrum
{
    class UIButton;
    class UISlider;
    class ControllerCore;
    class Canvas;

    class AudioSettingsPanel final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit AudioSettingsPanel(ControllerCore* controller);
        ~AudioSettingsPanel() noexcept;

        AudioSettingsPanel(const AudioSettingsPanel&) = delete;
        AudioSettingsPanel& operator=(const AudioSettingsPanel&) = delete;

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
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Show();
        void Hide();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsVisible() const noexcept { return m_animator.IsVisible(); }
        [[nodiscard]] bool IsInHitbox(const Point& mousePos) const noexcept;
        [[nodiscard]] UISlider* GetSliderAt(const Point& mousePos) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetOnCloseCallback(std::function<void()>&& callback);

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Types
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct SliderWidget
        {
            std::wstring label;
            std::unique_ptr<UISlider> slider;
            std::function<std::wstring(float)> formatter;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void CreateWidgets();
        void DrawSliders(Canvas& canvas) const;
        void HandleClickOutside(const Point& mousePos, bool isMouseDown);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ControllerCore* m_controller;
        PanelAnimator m_animator;
        Rect m_panelRect;
        bool m_wasPressed;

        std::vector<SliderWidget> m_sliderWidgets;
        std::unique_ptr<UIButton> m_closeButton;
        std::function<void()> m_onCloseCallback;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H