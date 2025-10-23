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
#include "Graphics/API/Helpers/Utils/StringUtils.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"

namespace Spectrum
{
    using namespace Helpers::Utils;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Anonymous namespace for internal helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace
    {
        [[nodiscard]] TextStyle CreateNavLabelStyle()
        {
            return TextStyle::Default()
                .WithColor(Color::White())
                .WithSize(14.0f)
                .WithAlign(TextAlign::Center)
                .WithParagraphAlign(ParagraphAlign::Center);
        }
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
        UpdateAnimation(deltaTime);
        UpdateToggleButton(mousePos, isMouseDown);

        if (m_animator.IsVisible())
        {
            UpdateButtons(mousePos, isMouseDown, deltaTime);
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
        const auto navDefs = CreateNavControlDefinitions(rm, am);

        for (const auto& def : navDefs)
        {
            AddNavigationButtons(def);
            AddNavigationLabel(def);
        }
    }

    void ControlPanel::CreateActionButtons(
        Platform::WindowManager* wm,
        AudioManager* am
    )
    {
        const auto buttonDefs = CreateActionButtonDefinitions(wm, am);

        for (const auto& def : buttonDefs)
        {
            AddActionButton(def);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Widget Creation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::vector<ControlPanel::NavControlDefinition>
        ControlPanel::CreateNavControlDefinitions(
            RendererManager* rm,
            AudioManager* am
        ) const
    {
        return {
            {
                UILayout::GetNavControlY(0),
                [rm] { rm->SwitchToPrevRenderer(); },
                [rm] { rm->SwitchToNextRenderer(); },
                [rm] { return StringToWString(std::string(rm->GetCurrentRendererName())); }
            },
            {
                UILayout::GetNavControlY(1),
                [rm] { rm->CycleQuality(-1); },
                [rm] { rm->CycleQuality(1); },
                [rm] { return StringToWString(std::string(rm->GetQualityName())); }
            },
            {
                UILayout::GetNavControlY(2),
                [am] { am->ChangeSpectrumScale(-1); },
                [am] { am->ChangeSpectrumScale(1); },
                [am] { return StringToWString(std::string(am->GetSpectrumScaleName())); }
            }
        };
    }

    [[nodiscard]] std::vector<ControlPanel::ButtonDefinition>
        ControlPanel::CreateActionButtonDefinitions(
            Platform::WindowManager* wm,
            AudioManager* am
        ) const
    {
        return {
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
    }

    void ControlPanel::AddNavigationButtons(const NavControlDefinition& def)
    {
        // Previous button
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

        // Next button
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
    }

    void ControlPanel::AddNavigationLabel(const NavControlDefinition& def)
    {
        m_navLabels.push_back({
            {
                UILayout::kControlPanelWidth * 0.5f + 5.0f,
                def.yPos + UILayout::kNavWidgetHeight * 0.5f
            },
            def.labelSource
            });
    }

    void ControlPanel::AddActionButton(const ButtonDefinition& def)
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ControlPanel::UpdateAnimation(float deltaTime)
    {
        m_animator.Update(deltaTime);
    }

    void ControlPanel::UpdateToggleButton(const Point& mousePos, bool isMouseDown)
    {
        m_isToggleButtonHovered = IsToggleButtonHovered(mousePos);

        if (isMouseDown && m_isToggleButtonHovered && !m_wasTogglePressed)
        {
            ToggleVisibility();
        }

        m_wasTogglePressed = (isMouseDown && m_isToggleButtonHovered);
    }

    void ControlPanel::UpdateButtons(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        const Point transformedMousePos = GetTransformedMousePosition(mousePos);

        for (auto& button : m_buttons)
        {
            button->Update(transformedMousePos, isMouseDown, deltaTime);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

    void ControlPanel::DrawContent(Canvas& canvas) const
    {
        canvas.PushTransform();
        canvas.TranslateBy(GetContentXOffset(), 0.0f);

        DrawPanelBackground(canvas);
        DrawButtons(canvas);
        DrawNavLabels(canvas);
        DrawSeparator(canvas);

        canvas.PopTransform();
    }

    void ControlPanel::DrawPanelBackground(Canvas& canvas) const
    {
        constexpr Rect panelRect = {
            5.0f,
            5.0f,
            UILayout::kControlPanelWidth,
            UILayout::kControlPanelHeight
        };

        const Paint fillPaint = Paint::Fill(Color(0.1f, 0.1f, 0.1f, 0.8f));
        const Paint strokePaint = Paint::Stroke(Color(1.0f, 1.0f, 1.0f, 0.1f), 1.0f);

        canvas.DrawRoundedRectangle(panelRect, 5.0f, fillPaint);
        canvas.DrawRoundedRectangle(panelRect, 5.0f, strokePaint);
    }

    void ControlPanel::DrawButtons(Canvas& canvas) const
    {
        for (const auto& button : m_buttons)
        {
            button->Draw(canvas);
        }
    }

    void ControlPanel::DrawNavLabels(Canvas& canvas) const
    {
        const TextStyle labelStyle = CreateNavLabelStyle();

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

    void ControlPanel::DrawSeparator(Canvas& canvas) const
    {
        constexpr float separatorY = UILayout::GetSeparatorY();

        canvas.DrawLine(
            { UILayout::kPadding, separatorY },
            { UILayout::kControlPanelWidth - UILayout::kPadding + 5.0f, separatorY },
            Paint::Stroke(UILayout::kSeparatorColor, 1.0f)
        );
    }

    [[nodiscard]] bool ControlPanel::IsToggleButtonHovered(const Point& mousePos) const noexcept
    {
        const Rect toggleRect = GetToggleButtonRect();
        return mousePos.x >= toggleRect.x && mousePos.x <= toggleRect.GetRight() &&
            mousePos.y >= toggleRect.y && mousePos.y <= toggleRect.GetBottom();
    }

    [[nodiscard]] Rect ControlPanel::GetToggleButtonRect() const noexcept
    {
        const float xPos = Helpers::Math::Lerp(
            0.0f,
            UILayout::kControlPanelWidth + 5.0f,
            m_animator.GetProgress()
        );

        return {
            xPos,
            UILayout::kControlPanelHeight * 0.5f - UILayout::kToggleButtonHeight * 0.5f,
            UILayout::kToggleButtonWidth,
            UILayout::kToggleButtonHeight
        };
    }

    [[nodiscard]] float ControlPanel::GetContentXOffset() const noexcept
    {
        return Helpers::Math::Lerp(
            -(UILayout::kControlPanelWidth + 5.0f),
            0.0f,
            m_animator.GetProgress()
        );
    }

    [[nodiscard]] Point ControlPanel::GetTransformedMousePosition(const Point& mousePos) const noexcept
    {
        return { mousePos.x - GetContentXOffset(), mousePos.y };
    }

} // namespace Spectrum