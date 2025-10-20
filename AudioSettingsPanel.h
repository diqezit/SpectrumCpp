#ifndef SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H
#define SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H

#include "Common.h"
#include "PanelAnimator.h"
#include <vector>
#include <memory>
#include <functional>

namespace Spectrum {

    class UIButton;
    class UISlider;
    class ControllerCore;
    class GraphicsContext;

    class AudioSettingsPanel final {
    public:
        explicit AudioSettingsPanel(ControllerCore* controller);
        ~AudioSettingsPanel() noexcept;

        void Initialize();
        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(GraphicsContext& context) const;

        void Show();
        void Hide();
        [[nodiscard]] bool IsVisible() const { return m_animator.IsVisible(); }
        [[nodiscard]] bool IsInHitbox(const Point& mousePos) const;
        [[nodiscard]] UISlider* GetSliderAt(const Point& mousePos);

        void SetOnCloseCallback(std::function<void()>&& callback);

    private:
        struct SliderWidget {
            std::wstring label;
            std::unique_ptr<UISlider> slider;
            std::function<std::wstring(float)> formatter;
        };

        void CreateWidgets();
        void DrawSliders(GraphicsContext& context) const;
        void HandleClickOutside(const Point& mousePos, bool isMouseDown);

        ControllerCore* m_controller;
        PanelAnimator m_animator;
        Rect m_panelRect;
        bool m_wasPressed;

        std::vector<SliderWidget> m_sliderWidgets;
        std::unique_ptr<UIButton> m_closeButton;
        std::function<void()> m_onCloseCallback;
    };

}

#endif