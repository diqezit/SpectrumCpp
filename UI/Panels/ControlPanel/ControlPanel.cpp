// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ControlPanel with data-driven widget creation.
//
// Key implementation details:
// - Dynamic button creation based on controller state
// - Smooth slide-in/out animation for panel visibility
// - Hierarchical input handling with toggle button priority
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/ControlPanel/ControlPanel.h"
#include "UI/Panels/PanelDrawHelper.h"
#include "UI/Components/UIButton.h"
#include "UI/Common/UILayout.h"
#include "Audio/AudioManager.h"
#include "App/ControllerCore.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/RenderEngine.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Graphics/RendererManager.h"
#include "Platform/WindowManager.h"
#include "Common/StringUtils.h"
#include "Common/MathUtils.h"

namespace Spectrum
{
    namespace
    {
        struct NavControlDefinition
        {
            float yPos;
            std::function<void()> prevAction;
            std::function<void()> nextAction;
            std::function<std::wstring()> labelSource;
        };

        struct ButtonDefinition
        {
            float yPos;
            std::wstring label;
            std::function<void()> action;
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ControlPanel::ControlPanel(ControllerCore* controller) :
        m_controller(controller),
        m_animator(UILayout::kAnimationSpeed),
        m_isToggleButtonHovered(false),
        m_wasTogglePressed(false)
    {
        m_animator.Open();
    }

    ControlPanel::~ControlPanel() noexcept = default;

    void ControlPanel::Initialize()
    {
        CreateWidgets();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControlPanel::Update(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        m_animator.Update(deltaTime);
        m_isToggleButtonHovered = IsToggleButtonHovered(mousePos);

        if (isMouseDown && m_isToggleButtonHovered && !m_wasTogglePressed)
        {
            ToggleVisibility();
        }

        m_wasTogglePressed = (isMouseDown && m_isToggleButtonHovered);

        if (m_animator.IsVisible())
        {
            const Point transformedMousePos = GetTransformedMousePosition(mousePos);
            for (auto& button : m_buttons)
            {
                button->Update(transformedMousePos, isMouseDown, deltaTime);
            }
        }
    }

    void ControlPanel::Draw(Canvas& canvas) const
    {
        if (m_animator.IsVisible())
        {
            DrawContent(canvas);
        }

        PanelDrawHelper::DrawSlideToggleButton(
            canvas,
            GetToggleButtonRect(),
            m_isToggleButtonHovered,
            !m_animator.IsVisible()
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControlPanel::SetOnShowAudioSettings(std::function<void()>&& callback)
    {
        m_onShowAudioSettings = std::move(callback);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControlPanel::CreateWidgets()
    {
        if (!m_controller) return;
        auto* rendererManager = m_controller->GetRendererManager();
        auto* windowManager = m_controller->GetWindowManager();
        auto* audioManager = m_controller->GetAudioManager();
        if (!rendererManager || !windowManager || !audioManager) return;

        m_buttons.clear();
        m_navLabels.clear();
        CreateNavigationControls(rendererManager, windowManager, audioManager);
        CreateActionButtons(windowManager, audioManager);
    }

    void ControlPanel::CreateNavigationControls(
        RendererManager* rm,
        Platform::WindowManager* /*wm*/,
        AudioManager* am
    )
    {
        const std::vector<NavControlDefinition> navDefs = {
            {
                UILayout::GetNavControlY(0),
                [rm] { rm->SwitchToPrevRenderer(); },
                [rm] { rm->SwitchToNextRenderer(); },
                [rm] { return Utils::StringToWString(std::string(rm->GetCurrentRendererName())); }
            },
            {
                UILayout::GetNavControlY(1),
                [rm] { rm->CycleQuality(-1); },
                [rm] { rm->CycleQuality(1); },
                [rm] { return Utils::StringToWString(std::string(rm->GetQualityName())); }
            },
            {
                UILayout::GetNavControlY(2),
                [am] { am->ChangeSpectrumScale(-1); },
                [am] { am->ChangeSpectrumScale(1); },
                [am] { return Utils::StringToWString(std::string(am->GetSpectrumScaleName())); }
            }
        };

        for (const auto& def : navDefs)
        {
            m_buttons.emplace_back(std::make_unique<UIButton>(
                Rect{
                    UILayout::kPadding,
                    def.yPos,
                    UILayout::kNavButtonWidth,
                    UILayout::kNavWidgetHeight
                },
                L"<",
                def.prevAction
            ));
            m_buttons.emplace_back(std::make_unique<UIButton>(
                Rect{
                    UILayout::kControlPanelWidth - UILayout::kNavButtonWidth,
                    def.yPos,
                    UILayout::kNavButtonWidth,
                    UILayout::kNavWidgetHeight
                },
                L">",
                def.nextAction
            ));
            m_navLabels.push_back({
                { UILayout::kControlPanelWidth * 0.5f + 5.0f, def.yPos + UILayout::kNavWidgetHeight * 0.5f },
                def.labelSource
                });
        }
    }

    void ControlPanel::CreateActionButtons(
        Platform::WindowManager* wm,
        AudioManager* am
    )
    {
        const std::vector<ButtonDefinition> buttonDefs = {
            { UILayout::GetActionButtonY(0), L"Audio Settings", [this] { if (m_onShowAudioSettings) m_onShowAudioSettings(); } },
            { UILayout::GetActionButtonY(1), L"Toggle Overlay", [wm] { wm->ToggleOverlay(); } },
            { UILayout::GetActionButtonY(2), L"Toggle Capture", [am] { am->ToggleCapture(); } }
        };

        for (const auto& def : buttonDefs)
        {
            m_buttons.emplace_back(std::make_unique<UIButton>(
                Rect{
                    UILayout::kPadding,
                    def.yPos,
                    UILayout::kControlPanelWidth - 2.0f * UILayout::kPadding,
                    UILayout::kStandaloneButtonHeight
                },
                def.label,
                def.action
            ));
        }
    }

    void ControlPanel::ToggleVisibility()
    {
        if (m_animator.GetState() == AnimationState::Open || m_animator.GetState() == AnimationState::Opening) {
            m_animator.Close();
        }
        else {
            m_animator.Open();
        }
    }

    void ControlPanel::DrawContent(Canvas& canvas) const
    {
        canvas.PushTransform();
        canvas.TranslateBy(GetContentXOffset(), 0.0f);

        constexpr Rect panelRect = { 5.0f, 5.0f, UILayout::kControlPanelWidth, UILayout::kControlPanelHeight };

        const Paint fillPaint = Paint::Fill(Color(0.1f, 0.1f, 0.1f, 0.8f));
        const Paint strokePaint = Paint::Stroke(Color(1.0f, 1.0f, 1.0f, 0.1f), 1.0f);

        canvas.DrawRoundedRectangle(panelRect, 5.0f, fillPaint);
        canvas.DrawRoundedRectangle(panelRect, 5.0f, strokePaint);

        for (const auto& button : m_buttons)
        {
            button->Draw(canvas);
        }

        DrawNavLabels(canvas);
        constexpr float separatorY = UILayout::GetSeparatorY();

        canvas.DrawLine(
            { UILayout::kPadding, separatorY },
            { UILayout::kControlPanelWidth - UILayout::kPadding + 5.0f, separatorY },
            Paint::Stroke(UILayout::kSeparatorColor, 1.0f)
        );

        canvas.PopTransform();
    }

    void ControlPanel::DrawNavLabels(Canvas& canvas) const
    {
        TextStyle labelStyle = TextStyle::Default()
            .WithColor(Color::White())
            .WithSize(14.0f)
            .WithAlign(TextAlign::Center)
            .WithParagraphAlign(ParagraphAlign::Center);

        for (const auto& label : m_navLabels)
        {
            const float textY = label.position.y - (UILayout::kNavWidgetHeight * 0.5f);
            const Rect textRect = {
                UILayout::kPadding + UILayout::kNavButtonWidth,
                textY,
                UILayout::kControlPanelWidth - 2.0f * (UILayout::kPadding + UILayout::kNavButtonWidth),
                UILayout::kNavWidgetHeight
            };

            canvas.DrawText(
                label.textSource(),
                textRect,
                labelStyle
            );
        }
    }

    [[nodiscard]] bool ControlPanel::IsToggleButtonHovered(const Point& mousePos) const noexcept
    {
        const Rect toggleRect = GetToggleButtonRect();
        return mousePos.x >= toggleRect.x && mousePos.x <= toggleRect.GetRight() &&
            mousePos.y >= toggleRect.y && mousePos.y <= toggleRect.GetBottom();
    }

    [[nodiscard]] Rect ControlPanel::GetToggleButtonRect() const noexcept
    {
        const float xPos = Utils::Lerp(0.0f, UILayout::kControlPanelWidth + 5.0f, m_animator.GetProgress());
        return {
            xPos,
            UILayout::kControlPanelHeight * 0.5f - UILayout::kToggleButtonHeight * 0.5f,
            UILayout::kToggleButtonWidth,
            UILayout::kToggleButtonHeight
        };
    }

    [[nodiscard]] float ControlPanel::GetContentXOffset() const noexcept
    {
        return Utils::Lerp(-(UILayout::kControlPanelWidth + 5.0f), 0.0f, m_animator.GetProgress());
    }

    [[nodiscard]] Point ControlPanel::GetTransformedMousePosition(const Point& mousePos) const noexcept
    {
        return { mousePos.x - GetContentXOffset(), mousePos.y };
    }

} // namespace Spectrum