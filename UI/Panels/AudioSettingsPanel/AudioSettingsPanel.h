#ifndef SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H
#define SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Audio settings modal using Template Method pattern
//
// DrawHeader hook adds title without duplicating transform logic
// DrawBody hook adds slider labels before calling base widget rendering
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/ModalPanelBase.h"
#include <functional>

namespace Spectrum
{
    class ControllerCore;

    class AudioSettingsPanel final : public ModalPanelBase
    {
    public:
        explicit AudioSettingsPanel(ControllerCore* controller);
        ~AudioSettingsPanel() noexcept override;

        AudioSettingsPanel(const AudioSettingsPanel&) = delete;
        AudioSettingsPanel& operator=(const AudioSettingsPanel&) = delete;

        void Initialize();
        void SetOnCloseCallback(std::function<void()>&& callback);

    protected:
        std::vector<WidgetDefinition> GetWidgetDefinitions() const override;
        void OnResizeImpl(int width, int height) override;
        void DrawHeader(Canvas& canvas, float alpha) const override;
        void DrawBody(Canvas& canvas, float alpha) const override;

    private:
        void DrawSliderLabels(Canvas& canvas, float alpha) const;

        ControllerCore* m_controller;
        std::function<void()> m_onCloseCallback;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_SETTINGS_PANEL_H