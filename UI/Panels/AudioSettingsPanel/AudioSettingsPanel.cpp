// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Audio settings panel using Template Method hooks
//
// DrawHeader adds title rendering without duplicating transform logic
// DrawBody adds slider labels before calling base implementation
// All animation and transform handled by ModalPanelBase
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/AudioSettingsPanel/AudioSettingsPanel.h"
#include "UI/Panels/PanelDrawHelper.h"
#include "UI/Common/UILayout.h"
#include "UI/Common/UIFormatters.h"
#include "UI/Factories/WidgetFactory.h"
#include "Audio/AudioManager.h"
#include "App/ControllerCore.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/RenderEngine.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Platform/WindowManager.h"

namespace Spectrum
{
    using namespace UIFormatters;

    AudioSettingsPanel::AudioSettingsPanel(ControllerCore* controller)
        : ModalPanelBase()
        , m_controller(controller)
    {
    }

    AudioSettingsPanel::~AudioSettingsPanel() noexcept = default;

    void AudioSettingsPanel::Initialize()
    {
        if (!m_controller)
            return;

        auto* windowManager = m_controller->GetWindowManager();
        if (!windowManager || !windowManager->GetRenderEngine())
            return;

        const int width = windowManager->GetRenderEngine()->GetWidth();
        const int height = windowManager->GetRenderEngine()->GetHeight();

        OnResizeImpl(width, height);
    }

    void AudioSettingsPanel::SetOnCloseCallback(std::function<void()>&& callback)
    {
        m_onCloseCallback = std::move(callback);
    }

    std::vector<WidgetDefinition> AudioSettingsPanel::GetWidgetDefinitions() const
    {
        if (!m_controller)
            return {};

        auto* audioManager = m_controller->GetAudioManager();
        if (!audioManager)
            return {};

        PanelDependencies deps;
        deps.audioManager = audioManager;
        deps.onClosePanel = [this]()
            {
                if (m_onCloseCallback)
                    m_onCloseCallback();
            };

        return WidgetFactory::CreateAudioSettingsPanelWidgets(deps, m_bounds);
    }

    void AudioSettingsPanel::OnResizeImpl(int width, int height)
    {
        const float windowWidth = static_cast<float>(width);
        const float windowHeight = static_cast<float>(height);
        const float panelWidth = UILayout::AudioSettingsPanel::kWidth;
        constexpr float panelHeight = UILayout::AudioSettingsPanel::GetPanelHeight();

        SetBounds(UILayout::CalculateCenteredRect(
            windowWidth,
            windowHeight,
            panelWidth,
            panelHeight
        ));
    }

    void AudioSettingsPanel::DrawHeader(Canvas& canvas, float alpha) const
    {
        const Point center = {
            m_bounds.x + m_bounds.width * 0.5f,
            m_bounds.y + m_bounds.height * 0.5f
        };

        PanelDrawHelper::DrawTitle(
            canvas,
            L"Audio Settings",
            { center.x, m_bounds.y + UILayout::AudioSettingsPanel::kTitleHeight * 0.5f },
            alpha
        );
    }

    void AudioSettingsPanel::DrawBody(Canvas& canvas, float alpha) const
    {
        DrawSliderLabels(canvas, alpha);
        ModalPanelBase::DrawBody(canvas, alpha);
    }

    void AudioSettingsPanel::DrawSliderLabels(Canvas& canvas, float alpha) const
    {
        struct SliderLabelInfo
        {
            std::wstring id;
            std::wstring label;
            std::function<std::wstring(float)> formatter;
        };

        const SliderLabelInfo labels[] = {
            { L"amplification", L"Amplification", FormatFloat },
            { L"smoothing",     L"Smoothing",     FormatFloat },
            { L"barcount",      L"Bar Count",     FormatInt   }
        };

        for (const auto& info : labels)
        {
            auto* slider = GetSlider(info.id);
            if (!slider)
                continue;

            PanelDrawHelper::DrawSliderLabel(
                canvas,
                info.label,
                slider->GetValue(),
                info.formatter,
                slider->GetRect(),
                UILayout::AudioSettingsPanel::kLabelYOffset,
                alpha
            );
        }
    }

} // namespace Spectrum