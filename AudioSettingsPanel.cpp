// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the AudioSettingsPanel. It handles the layout of
// sliders, text labels, and its own fade-in/scale-up animation, providing
// a modal dialog experience for audio configuration.
//
// Implements the AudioSettingsPanel class. This includes handling its
// open/close state transitions, animations, rendering, and the data-driven
// creation and layout management of its child UI elements.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "AudioSettingsPanel.h"
#include "AudioManager.h"
#include "ControllerCore.h"
#include "GraphicsContext.h"
#include "UIButton.h"
#include "UILayout.h"
#include "UISlider.h"
#include "WindowManager.h"
#include "PanelDrawHelper.h"
#include <sstream>
#include <iomanip>

namespace Spectrum {

    namespace {
        auto FormatFloat = [](float value) {
            std::wstringstream wss;
            wss << std::fixed << std::setprecision(2) << value;
            return wss.str();
            };

        auto FormatInt = [](float value) {
            return std::to_wstring(static_cast<int>(value));
            };
    }

    AudioSettingsPanel::AudioSettingsPanel(ControllerCore* controller)
        : m_controller(controller)
        , m_animator(UILayout::kAnimationSpeed)
        , m_wasPressed(false)
    {
    }

    AudioSettingsPanel::~AudioSettingsPanel() noexcept = default;

    void AudioSettingsPanel::Initialize() {}

    void AudioSettingsPanel::Update(const Point& mousePos, bool isMouseDown, float deltaTime) {
        if (!IsVisible()) return;

        m_animator.Update(deltaTime);

        for (auto& widget : m_sliderWidgets)
            widget.slider->UpdateHover(mousePos);

        if (m_closeButton)
            m_closeButton->Update(mousePos, isMouseDown, deltaTime);

        HandleClickOutside(mousePos, isMouseDown);
    }

    void AudioSettingsPanel::Draw(GraphicsContext& context) const {
        if (!IsVisible()) return;

        PanelDrawHelper::DrawModalBackground(context, m_panelRect, m_animator.GetProgress());

        context.PushTransform();
        const Point center = {
            m_panelRect.x + m_panelRect.width / 2.0f,
            m_panelRect.y + m_panelRect.height / 2.0f
        };
        const float scale = Utils::EaseInOut(m_animator.GetProgress());
        context.ScaleAt(center, scale, scale);

        PanelDrawHelper::DrawTitle(
            context,
            L"Audio Settings",
            { center.x, m_panelRect.y + 25.0f },
            m_animator.GetProgress()
        );

        DrawSliders(context);

        if (m_closeButton)
            m_closeButton->Draw(context);

        context.PopTransform();
    }

    void AudioSettingsPanel::Show() {
        m_animator.Open();
        CreateWidgets();
    }

    void AudioSettingsPanel::Hide() {
        m_animator.Close();
    }

    [[nodiscard]] bool AudioSettingsPanel::IsInHitbox(const Point& mousePos) const {
        if (!IsVisible()) return false;
        return mousePos.x >= m_panelRect.x &&
            mousePos.x <= m_panelRect.GetRight() &&
            mousePos.y >= m_panelRect.y &&
            mousePos.y <= m_panelRect.GetBottom();
    }

    [[nodiscard]] UISlider* AudioSettingsPanel::GetSliderAt(const Point& mousePos) {
        if (!IsVisible()) return nullptr;
        for (auto& widget : m_sliderWidgets)
            if (widget.slider->IsInHitbox(mousePos))
                return widget.slider.get();
        return nullptr;
    }

    void AudioSettingsPanel::SetOnCloseCallback(std::function<void()>&& callback) {
        m_onCloseCallback = std::move(callback);
    }

    void AudioSettingsPanel::CreateWidgets() {
        auto* am = m_controller->GetAudioManager();
        auto* wm = m_controller->GetWindowManager();
        if (!am || !wm || !wm->GetGraphics()) return;

        const int screenW = wm->GetGraphics()->GetWidth();
        const int screenH = wm->GetGraphics()->GetHeight();

        m_panelRect = {
            (screenW - UILayout::kAudioPanelWidth) / 2.0f,
            (screenH - UILayout::kAudioPanelHeight) / 2.0f,
            UILayout::kAudioPanelWidth,
            UILayout::kAudioPanelHeight
        };

        struct SliderDef {
            std::wstring label;
            float min, max, step;
            std::function<float()> getter;
            std::function<void(float)> setter;
            std::function<std::wstring(float)> formatter;
        };

        std::vector<SliderDef> sliderDefs = {
            { L"Amplification", 0.1f, 5.0f, 0.01f,
              [am]() { return am->GetAmplification(); },
              [am](float v) { am->SetAmplification(v); },
              FormatFloat },
            { L"Smoothing", 0.0f, 0.99f, 0.01f,
              [am]() { return am->GetSmoothing(); },
              [am](float v) { am->SetSmoothing(v); },
              FormatFloat },
            { L"Bar Count", 16.0f, 256.0f, 1.0f,
              [am]() { return static_cast<float>(am->GetBarCount()); },
              [am](float v) { am->SetBarCount(static_cast<size_t>(v)); },
              FormatInt }
        };

        m_sliderWidgets.clear();
        float currentY = m_panelRect.y + UILayout::kPadding + UILayout::kAudioPanelTitleHeight;
        const float sliderWidth = m_panelRect.width - 2 * UILayout::kPadding;
        const float sliderX = m_panelRect.x + UILayout::kPadding;

        for (auto& def : sliderDefs) {
            currentY += 20;
            auto slider = std::make_unique<UISlider>(
                Rect{ sliderX, currentY, sliderWidth, UILayout::kSliderHeight },
                def.min, def.max, def.getter(), def.step
            );
            slider->SetOnValueChanged(std::move(def.setter));
            m_sliderWidgets.push_back({ std::move(def.label), std::move(slider), std::move(def.formatter) });
            currentY += UILayout::kSliderHeight + UILayout::kGroupSpacing - 20;
        }

        m_closeButton = std::make_unique<UIButton>(
            Rect{ m_panelRect.GetRight() - 35.0f, m_panelRect.y + 10.0f, 25.0f, 25.0f },
            L"X",
            [this]() { if (m_onCloseCallback) m_onCloseCallback(); }
        );
    }

    void AudioSettingsPanel::DrawSliders(GraphicsContext& context) const {
        const float alpha = m_animator.GetProgress();
        const Color labelColor = { 0.8f, 0.8f, 0.8f, alpha };
        const Color valueColor = { 1.0f, 1.0f, 1.0f, alpha };

        for (const auto& widget : m_sliderWidgets) {
            widget.slider->Draw(context);

            const Rect& sliderRect = widget.slider->GetRect();
            context.DrawText(widget.label,
                { sliderRect.x, sliderRect.y + UILayout::kSliderLabelYOffset },
                labelColor, 12.0f);
            context.DrawText(widget.formatter(widget.slider->GetValue()),
                { sliderRect.GetRight(), sliderRect.y + UILayout::kSliderLabelYOffset },
                valueColor, 12.0f, DWRITE_TEXT_ALIGNMENT_TRAILING);
        }
    }

    void AudioSettingsPanel::HandleClickOutside(const Point& mousePos, bool isMouseDown) {
        if (m_wasPressed && !isMouseDown && !IsInHitbox(mousePos)) {
            Hide();
        }
        m_wasPressed = isMouseDown;
    }

}