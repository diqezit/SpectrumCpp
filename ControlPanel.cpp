// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the ControlPanel class. It lays out the primary
// UI controls and connects their actions to the core logic, managing the
// slide-in/slide-out animation for the main control surface.
//
// Implements the ControlPanel by defining its data-driven widget creation,
// state-based animation logic, and input handling. It translates user
// interactions on the UI into commands for the core application systems.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControlPanel.h"
#include "AudioManager.h"
#include "ControllerCore.h"
#include "GraphicsContext.h"
#include "RendererManager.h"
#include "StringUtils.h"
#include "UIButton.h"
#include "UILayout.h"
#include "WindowManager.h"
#include "PanelDrawHelper.h"

namespace Spectrum {

    namespace {
        struct NavControlDefinition {
            float yPos;
            std::function<void()> prevAction;
            std::function<void()> nextAction;
            std::function<std::wstring()> labelSource;
        };

        struct ButtonDefinition {
            float yPos;
            std::wstring label;
            std::function<void()> action;
        };
    }

    ControlPanel::ControlPanel(ControllerCore* controller) :
        m_controller(controller),
        m_animator(UILayout::kAnimationSpeed),
        m_isToggleButtonHovered(false),
        m_wasTogglePressed(false)
    {
        m_animator.Open();
    }

    ControlPanel::~ControlPanel() noexcept = default;

    void ControlPanel::Initialize() {
        CreateWidgets();
    }

    void ControlPanel::Update(const Point& mousePos, bool isMouseDown, float deltaTime) {
        m_animator.Update(deltaTime);

        m_isToggleButtonHovered = IsToggleButtonHovered(mousePos);
        if (isMouseDown && m_isToggleButtonHovered && !m_wasTogglePressed) {
            ToggleVisibility();
        }
        m_wasTogglePressed = (isMouseDown && m_isToggleButtonHovered);

        if (m_animator.IsVisible()) {
            const Point transformedMousePos = GetTransformedMousePosition(mousePos);
            for (auto& button : m_buttons) {
                button->Update(transformedMousePos, isMouseDown, deltaTime);
            }
        }
    }

    void ControlPanel::Draw(GraphicsContext& context) const {
        if (m_animator.IsVisible()) {
            DrawContent(context);
        }

        PanelDrawHelper::DrawSlideToggleButton(
            context,
            GetToggleButtonRect(),
            m_isToggleButtonHovered,
            !m_animator.IsVisible()
        );
    }

    void ControlPanel::SetOnShowAudioSettings(std::function<void()>&& callback) {
        m_onShowAudioSettings = std::move(callback);
    }

    void ControlPanel::CreateWidgets() {
        if (!m_controller) return;
        auto* rm = m_controller->GetRendererManager();
        auto* wm = m_controller->GetWindowManager();
        auto* am = m_controller->GetAudioManager();
        if (!rm || !wm || !am) return;

        m_buttons.clear();
        m_navLabels.clear();

        CreateNavigationControls(rm, wm);
        CreateActionButtons(wm, am);
    }

    void ControlPanel::CreateNavigationControls(RendererManager* rm, WindowManager* wm) {
        const std::vector<NavControlDefinition> navDefs = {
            {
                UILayout::kPadding,
                [rm, wm]() { rm->SwitchToPrevRenderer(wm->GetGraphics()); },
                [rm, wm]() { rm->SwitchToNextRenderer(wm->GetGraphics()); },
                [rm]() { return Utils::StringToWString(std::string(rm->GetCurrentRendererName())); }
            },
            {
                UILayout::kPadding + UILayout::kNavWidgetHeight + UILayout::kWidgetSpacing,
                [rm]() { rm->CycleQuality(-1); },
                [rm]() { rm->CycleQuality(1); },
                [rm]() { return Utils::StringToWString(std::string(rm->GetQualityName())); }
            }
        };

        for (const auto& def : navDefs)
        {
            m_buttons.emplace_back(std::make_unique<UIButton>(
                Rect{ UILayout::kPadding, def.yPos, UILayout::kNavButtonWidth, UILayout::kNavWidgetHeight },
                L"<",
                def.prevAction
            ));
            m_buttons.emplace_back(std::make_unique<UIButton>(
                Rect{ kPanelWidth - UILayout::kNavButtonWidth, def.yPos, UILayout::kNavButtonWidth, UILayout::kNavWidgetHeight },
                L">",
                def.nextAction
            ));
            m_navLabels.push_back({
                { kPanelWidth / 2.0f + 5.0f, def.yPos + UILayout::kNavWidgetHeight / 2.0f },
                def.labelSource
                });
        }
    }

    void ControlPanel::CreateActionButtons(WindowManager* wm, AudioManager* am) {
        float currentY = UILayout::kPadding + 2 * (UILayout::kNavWidgetHeight + UILayout::kWidgetSpacing) + UILayout::kGroupSpacing;
        const std::vector<ButtonDefinition> buttonDefs = {
            {
                currentY,
                L"Audio Settings",
                [this]() { if (m_onShowAudioSettings) m_onShowAudioSettings(); }
            },
            {
                currentY += UILayout::kStandaloneButtonHeight + UILayout::kWidgetSpacing,
                L"Toggle Overlay",
                [wm]() { wm->ToggleOverlay(); }
            },
            {
                currentY += UILayout::kStandaloneButtonHeight + UILayout::kWidgetSpacing,
                L"Toggle Capture",
                [am]() { am->ToggleCapture(); }
            }
        };

        for (const auto& def : buttonDefs)
        {
            m_buttons.emplace_back(std::make_unique<UIButton>(
                Rect{ UILayout::kPadding, def.yPos, kPanelWidth - 2 * UILayout::kPadding, UILayout::kStandaloneButtonHeight },
                def.label,
                def.action
            ));
        }
    }

    void ControlPanel::ToggleVisibility() {
        if (m_animator.GetState() == AnimationState::Open || m_animator.GetState() == AnimationState::Opening) {
            m_animator.Close();
        }
        else {
            m_animator.Open();
        }
    }

    void ControlPanel::DrawContent(GraphicsContext& context) const {
        context.PushTransform();
        context.TranslateBy(GetContentXOffset(), 0.0f);

        const Rect panelRect = { 5.0f, 5.0f, kPanelWidth, kPanelHeight };
        context.DrawRoundedRectangle(panelRect, 5.0f, Color(0.1f, 0.1f, 0.1f, 0.8f), true);
        context.DrawRoundedRectangle(panelRect, 5.0f, Color(1.0f, 1.0f, 1.0f, 0.1f), false);

        for (const auto& button : m_buttons)
            button->Draw(context);

        DrawNavLabels(context);

        const float separatorY = UILayout::kPadding +
            2 * (UILayout::kNavWidgetHeight + UILayout::kWidgetSpacing) +
            UILayout::kGroupSpacing / 2.0f;

        context.DrawLine(
            { UILayout::kPadding, separatorY },
            { kPanelWidth - UILayout::kPadding + 5.0f, separatorY },
            UILayout::kSeparatorColor, 1.0f
        );

        context.PopTransform();
    }

    void ControlPanel::DrawNavLabels(GraphicsContext& context) const {
        for (const auto& label : m_navLabels)
        {
            context.DrawText(
                label.textSource(),
                label.position,
                Color::White(),
                14.0f,
                DWRITE_TEXT_ALIGNMENT_CENTER
            );
        }
    }

    [[nodiscard]] bool ControlPanel::IsToggleButtonHovered(const Point& mousePos) const {
        const Rect toggleRect = GetToggleButtonRect();
        return mousePos.x >= toggleRect.x &&
            mousePos.x <= toggleRect.GetRight() &&
            mousePos.y >= toggleRect.y &&
            mousePos.y <= toggleRect.GetBottom();
    }

    [[nodiscard]] Rect ControlPanel::GetToggleButtonRect() const {
        const float xPos = Utils::Lerp(0.0f, kPanelWidth + 5.0f, m_animator.GetProgress());
        return {
            xPos, kPanelHeight / 2.0f - 20.0f,
            kToggleButtonWidth, 40.0f
        };
    }

    [[nodiscard]] float ControlPanel::GetContentXOffset() const {
        return Utils::Lerp(-(kPanelWidth + 5.0f), 0.0f, m_animator.GetProgress());
    }

    [[nodiscard]] Point ControlPanel::GetTransformedMousePosition(const Point& mousePos) const {
        return { mousePos.x - GetContentXOffset(), mousePos.y };
    }
}