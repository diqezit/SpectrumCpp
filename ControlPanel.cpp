// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the ControlPanel with data-driven widget creation.
// 
// Key implementation details:
// - Dynamic button creation based on controller state
// - Smooth slide-in/out animation for panel visibility
// - Hierarchical input handling with toggle button priority
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControlPanel.h"
#include "AudioManager.h"
#include "ControllerCore.h"
#include "GraphicsContext.h"
#include "PanelDrawHelper.h"
#include "RendererManager.h"
#include "StringUtils.h"
#include "UIButton.h"
#include "UILayout.h"
#include "WindowManager.h"

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

    void ControlPanel::Update(const Point& mousePos, bool isMouseDown, float deltaTime)
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

    void ControlPanel::Draw(GraphicsContext& context) const
    {
        if (m_animator.IsVisible())
        {
            DrawContent(context);
        }

        PanelDrawHelper::DrawSlideToggleButton(
            context,
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

    void ControlPanel::CreateNavigationControls(RendererManager* rm, WindowManager* wm, AudioManager* am)
    {
        const std::vector<NavControlDefinition> navDefs = {
            {
                UILayout::GetNavControlY(0),
                [rm, wm] { rm->SwitchToPrevRenderer(wm->GetGraphics()); },
                [rm, wm] { rm->SwitchToNextRenderer(wm->GetGraphics()); },
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

    void ControlPanel::CreateActionButtons(WindowManager* wm, AudioManager* am)
    {
        const std::vector<ButtonDefinition> buttonDefs = {
            {
                UILayout::GetActionButtonY(0),
                L"Audio Settings",
                [this] { if (m_onShowAudioSettings) m_onShowAudioSettings(); }
            },
            {
                UILayout::GetActionButtonY(1),
                L"Toggle Overlay",
                [wm] { wm->ToggleOverlay(); }
            },
            {
                UILayout::GetActionButtonY(2),
                L"Toggle Capture",
                [am] { am->ToggleCapture(); }
            }
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
        if (m_animator.GetState() == AnimationState::Open ||
            m_animator.GetState() == AnimationState::Opening)
        {
            m_animator.Close();
        }
        else
        {
            m_animator.Open();
        }
    }

    void ControlPanel::DrawContent(GraphicsContext& context) const
    {
        context.PushTransform();
        context.TranslateBy(GetContentXOffset(), 0.0f);

        constexpr Rect panelRect = { 5.0f, 5.0f, UILayout::kControlPanelWidth, UILayout::kControlPanelHeight };

        context.DrawRoundedRectangle(panelRect, 5.0f, Color(0.1f, 0.1f, 0.1f, 0.8f), true);
        context.DrawRoundedRectangle(panelRect, 5.0f, Color(1.0f, 1.0f, 1.0f, 0.1f), false);

        for (const auto& button : m_buttons)
        {
            button->Draw(context);
        }

        DrawNavLabels(context);

        constexpr float separatorY = UILayout::GetSeparatorY();

        context.DrawLine(
            { UILayout::kPadding, separatorY },
            { UILayout::kControlPanelWidth - UILayout::kPadding + 5.0f, separatorY },
            UILayout::kSeparatorColor,
            1.0f
        );

        context.PopTransform();
    }

    void ControlPanel::DrawNavLabels(GraphicsContext& context) const
    {
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

    bool ControlPanel::IsToggleButtonHovered(const Point& mousePos) const noexcept
    {
        const Rect toggleRect = GetToggleButtonRect();

        return mousePos.x >= toggleRect.x &&
            mousePos.x <= toggleRect.GetRight() &&
            mousePos.y >= toggleRect.y &&
            mousePos.y <= toggleRect.GetBottom();
    }

    Rect ControlPanel::GetToggleButtonRect() const noexcept
    {
        const float xPos = Utils::Lerp(0.0f, UILayout::kControlPanelWidth + 5.0f, m_animator.GetProgress());

        return {
            xPos,
            UILayout::kControlPanelHeight * 0.5f - UILayout::kToggleButtonHeight * 0.5f,
            UILayout::kToggleButtonWidth,
            UILayout::kToggleButtonHeight
        };
    }

    float ControlPanel::GetContentXOffset() const noexcept
    {
        return Utils::Lerp(-(UILayout::kControlPanelWidth + 5.0f), 0.0f, m_animator.GetProgress());
    }

    Point ControlPanel::GetTransformedMousePosition(const Point& mousePos) const noexcept
    {
        return { mousePos.x - GetContentXOffset(), mousePos.y };
    }

} // namespace Spectrum