#ifndef SPECTRUM_CPP_CONTROL_PANEL_H
#define SPECTRUM_CPP_CONTROL_PANEL_H

#include "Common/Common.h"
#include "UI/Panels/PanelAnimator.h"
#include "UI/Core/WidgetManager.h"
#include <functional>
#include <memory>

namespace Spectrum
{
    class ControllerCore;
    class Canvas;

    class ControlPanel final
    {
    public:
        explicit ControlPanel(ControllerCore* controller);
        ~ControlPanel() noexcept;

        ControlPanel(const ControlPanel&) = delete;
        ControlPanel& operator=(const ControlPanel&) = delete;
        ControlPanel(ControlPanel&&) = delete;
        ControlPanel& operator=(ControlPanel&&) = delete;

        void Initialize();
        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(Canvas& canvas) const;
        void SetOnShowAudioSettings(std::function<void()>&& callback);

    private:
        void UpdateLayout(float deltaTime);
        void RecreateWidgets();

        ControllerCore* m_controller;
        PanelAnimator m_animator;

        WidgetManager m_widgetManager;

        Rect m_panelBounds;
        Rect m_toggleButtonBounds;
        float m_currentXOffset;

        bool m_isToggleButtonHovered;
        bool m_wasTogglePressed;

        std::function<void()> m_onShowAudioSettings;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_CONTROL_PANEL_H