// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the AudioSettingsPanel with data-driven widget creation.
// 
// Key implementation details:
// - Dynamic widget creation based on audio manager state
// - Smooth fade-in/scale-up animation
// - Click-outside-to-close behavior
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "AudioSettingsPanel.h"
#include "AudioManager.h"
#include "ControllerCore.h"
#include "GraphicsContext.h"
#include "PanelDrawHelper.h"
#include "UIButton.h"
#include "UILayout.h"
#include "UISlider.h"
#include "WindowManager.h"
#include <cmath>

namespace Spectrum
{
    namespace
    {
        constexpr int kFloatPrecision = 2;
        constexpr float kRoundingMultiplier = 100.0f;

        std::wstring FormatFloat(float value)
        {
            const float rounded = std::round(value * kRoundingMultiplier) / kRoundingMultiplier;
            std::wstring result = std::to_wstring(rounded);

            const size_t dotPos = result.find(L'.');
            if (dotPos != std::wstring::npos) {
                result = result.substr(0, dotPos + kFloatPrecision + 1);

                while (result.back() == L'0') result.pop_back();
                if (result.back() == L'.') result.pop_back();
            }

            return result;
        }

        std::wstring FormatInt(float value)
        {
            return std::to_wstring(static_cast<int>(value));
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    AudioSettingsPanel::AudioSettingsPanel(ControllerCore* controller) :
        m_controller(controller),
        m_animator(UILayout::kAnimationSpeed),
        m_panelRect{},
        m_wasPressed(false)
    {
    }

    AudioSettingsPanel::~AudioSettingsPanel() noexcept = default;

    void AudioSettingsPanel::Initialize()
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioSettingsPanel::Update(const Point& mousePos, bool isMouseDown, float deltaTime)
    {
        if (!IsVisible()) return;

        m_animator.Update(deltaTime);

        const MouseInputState sliderMouseState = { mousePos, isMouseDown };

        for (auto& widget : m_sliderWidgets)
        {
            if (widget.slider) {
                widget.slider->Update(sliderMouseState, deltaTime);
            }
        }

        if (m_closeButton)
        {
            m_closeButton->Update(mousePos, isMouseDown, deltaTime);
        }

        HandleClickOutside(mousePos, isMouseDown);
    }

    void AudioSettingsPanel::Draw(GraphicsContext& context) const
    {
        if (!IsVisible()) return;

        const float progress = m_animator.GetProgress();
        const float scale = Utils::EaseInOut(progress);

        PanelDrawHelper::DrawModalBackground(context, m_panelRect, progress);

        context.PushTransform();

        const Point center = {
            m_panelRect.x + m_panelRect.width * 0.5f,
            m_panelRect.y + m_panelRect.height * 0.5f
        };
        context.ScaleAt(center, scale, scale);

        PanelDrawHelper::DrawTitle(
            context,
            L"Audio Settings",
            { center.x, m_panelRect.y + UILayout::kAudioPanelTitleHeight * 0.5f },
            progress
        );

        DrawSliders(context);

        if (m_closeButton)
        {
            m_closeButton->Draw(context);
        }

        context.PopTransform();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioSettingsPanel::Show()
    {
        m_animator.Open();
        CreateWidgets();
    }

    void AudioSettingsPanel::Hide()
    {
        m_animator.Close();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool AudioSettingsPanel::IsInHitbox(const Point& mousePos) const noexcept
    {
        if (!IsVisible()) return false;

        return mousePos.x >= m_panelRect.x &&
            mousePos.x <= m_panelRect.GetRight() &&
            mousePos.y >= m_panelRect.y &&
            mousePos.y <= m_panelRect.GetBottom();
    }

    UISlider* AudioSettingsPanel::GetSliderAt(const Point& mousePos) const
    {
        if (!IsVisible()) return nullptr;

        for (const auto& widget : m_sliderWidgets)
        {
            if (widget.slider && widget.slider->IsInHitbox(mousePos))
            {
                return widget.slider.get();
            }
        }

        return nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioSettingsPanel::SetOnCloseCallback(std::function<void()>&& callback)
    {
        m_onCloseCallback = std::move(callback);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void AudioSettingsPanel::CreateWidgets()
    {
        if (!m_controller) return;

        auto* audioManager = m_controller->GetAudioManager();
        auto* windowManager = m_controller->GetWindowManager();
        if (!audioManager || !windowManager) return;

        auto* graphics = windowManager->GetGraphics();
        if (!graphics) return;

        const int screenWidth = graphics->GetWidth();
        const int screenHeight = graphics->GetHeight();

        m_panelRect = {
            (screenWidth - UILayout::kAudioPanelWidth) * 0.5f,
            (screenHeight - UILayout::kAudioPanelHeight) * 0.5f,
            UILayout::kAudioPanelWidth,
            UILayout::kAudioPanelHeight
        };

        struct SliderDef
        {
            std::wstring label;
            float min;
            float max;
            float step;
            std::function<float()> getter;
            std::function<void(float)> setter;
            std::function<std::wstring(float)> formatter;
        };

        std::vector<SliderDef> sliderDefs = {
            {
                L"Amplification",
                0.1f, 5.0f, 0.01f,
                [audioManager] { return audioManager->GetAmplification(); },
                [audioManager](float value) { audioManager->SetAmplification(value); },
                FormatFloat
            },
            {
                L"Smoothing",
                0.0f, 0.99f, 0.01f,
                [audioManager] { return audioManager->GetSmoothing(); },
                [audioManager](float value) { audioManager->SetSmoothing(value); },
                FormatFloat
            },
            {
                L"Bar Count",
                16.0f, 256.0f, 1.0f,
                [audioManager] { return static_cast<float>(audioManager->GetBarCount()); },
                [audioManager](float value) { audioManager->SetBarCount(static_cast<size_t>(value)); },
                FormatInt
            }
        };

        m_sliderWidgets.clear();
        m_sliderWidgets.reserve(sliderDefs.size());

        constexpr float sliderWidth = UILayout::GetSliderWidth();
        const float sliderX = m_panelRect.x + UILayout::kPadding;

        for (size_t i = 0; i < sliderDefs.size(); ++i)
        {
            auto& def = sliderDefs[i];
            const float sliderY = m_panelRect.y + UILayout::GetSliderYOffset(i);

            auto slider = std::make_unique<UISlider>(
                Rect{ sliderX, sliderY, sliderWidth, UILayout::kSliderHeight },
                def.min,
                def.max,
                def.getter(),
                def.step
            );

            slider->SetOnValueChanged(std::move(def.setter));

            m_sliderWidgets.push_back({
                std::move(def.label),
                std::move(slider),
                std::move(def.formatter)
                });
        }

        m_closeButton = std::make_unique<UIButton>(
            Rect{
                m_panelRect.GetRight() - UILayout::GetCloseButtonXOffset(),
                m_panelRect.y + UILayout::GetCloseButtonYOffset(),
                UILayout::kCloseButtonSize,
                UILayout::kCloseButtonSize
            },
            L"×",
            [this] { if (m_onCloseCallback) m_onCloseCallback(); }
        );
    }

    void AudioSettingsPanel::DrawSliders(GraphicsContext& context) const
    {
        const float alpha = m_animator.GetProgress();
        const Color labelColor = { 0.8f, 0.8f, 0.8f, alpha };
        const Color valueColor = { 1.0f, 1.0f, 1.0f, alpha };

        for (const auto& widget : m_sliderWidgets)
        {
            if (!widget.slider) continue;

            widget.slider->Draw(context);

            const Rect& sliderRect = widget.slider->GetRect();

            context.DrawText(
                widget.label,
                { sliderRect.x, sliderRect.y + UILayout::kSliderLabelYOffset },
                labelColor,
                12.0f
            );

            context.DrawText(
                widget.formatter(widget.slider->GetValue()),
                { sliderRect.GetRight(), sliderRect.y + UILayout::kSliderLabelYOffset },
                valueColor,
                12.0f,
                DWRITE_TEXT_ALIGNMENT_TRAILING
            );
        }
    }

    void AudioSettingsPanel::HandleClickOutside(const Point& mousePos, bool isMouseDown)
    {
        if (m_wasPressed && !isMouseDown && !IsInHitbox(mousePos))
        {
            Hide();
        }

        m_wasPressed = isMouseDown;
    }

} // namespace Spectrum