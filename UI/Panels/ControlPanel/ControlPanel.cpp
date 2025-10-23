#include "UI/Panels/ControlPanel/ControlPanel.h"
#include "UI/Panels/PanelDrawHelper.h"
#include "UI/Common/UILayout.h"
#include "UI/Factories/WidgetFactory.h"
#include "Audio/AudioManager.h"
#include "App/ControllerCore.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/RendererManager.h"
#include "Platform/WindowManager.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"

namespace Spectrum
{
    using namespace Helpers::Math;

    ControlPanel::ControlPanel(ControllerCore* controller)
        : m_controller(controller)
        , m_animator(UILayout::Animation::kSpeed)
        , m_currentXOffset(0.0f)
        , m_isToggleButtonHovered(false)
        , m_wasTogglePressed(false)
    {
        m_animator.Open();

        m_panelBounds = {
            UILayout::kPadding,
            UILayout::kPadding,
            UILayout::ControlPanel::kWidth,
            UILayout::ControlPanel::GetPanelHeight()
        };
    }

    ControlPanel::~ControlPanel() noexcept = default;

    void ControlPanel::Initialize()
    {
        RecreateWidgets();
    }

    void ControlPanel::Update(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        UpdateLayout(deltaTime);

        m_isToggleButtonHovered = mousePos.x >= m_toggleButtonBounds.x &&
            mousePos.x <= m_toggleButtonBounds.GetRight() &&
            mousePos.y >= m_toggleButtonBounds.y &&
            mousePos.y <= m_toggleButtonBounds.GetBottom();

        if (isMouseDown && m_isToggleButtonHovered && !m_wasTogglePressed)
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

        m_wasTogglePressed = (isMouseDown && m_isToggleButtonHovered);

        if (m_animator.IsVisible())
        {
            const Point transformedMousePos = {
                mousePos.x - m_currentXOffset,
                mousePos.y
            };
            m_widgetManager.Update(transformedMousePos, isMouseDown, deltaTime);
        }
    }

    void ControlPanel::Draw(Canvas& canvas) const
    {
        if (m_animator.IsVisible())
        {
            canvas.PushTransform();
            canvas.TranslateBy(m_currentXOffset, 0.0f);

            PanelDrawHelper::DrawPanelBackground(
                canvas,
                m_panelBounds,
                UILayout::kPanelBackgroundColor,
                UILayout::kPanelBorderColor,
                UILayout::VisualProperties::kPanelCornerRadius
            );

            constexpr float lineY = UILayout::ControlPanel::GetSeparatorY();
            canvas.DrawLine(
                { UILayout::kPadding, lineY },
                { UILayout::kPadding + UILayout::ControlPanel::kWidth - UILayout::kPadding, lineY },
                Paint::Stroke(UILayout::kSeparatorColor, UILayout::VisualProperties::kDefaultBorderWidth)
            );

            m_widgetManager.Draw(canvas);

            canvas.PopTransform();
        }

        PanelDrawHelper::DrawSlideToggleButton(
            canvas,
            m_toggleButtonBounds,
            m_isToggleButtonHovered,
            m_animator.GetState() == AnimationState::Closed ||
            m_animator.GetState() == AnimationState::Closing
        );
    }

    void ControlPanel::SetOnShowAudioSettings(std::function<void()>&& callback)
    {
        m_onShowAudioSettings = std::move(callback);
    }

    void ControlPanel::UpdateLayout(float deltaTime)
    {
        m_animator.Update(deltaTime);

        const float progress = m_animator.GetProgress();
        m_currentXOffset = Lerp(
            -(UILayout::ControlPanel::kWidth + UILayout::kPadding),
            0.0f,
            progress
        );

        m_toggleButtonBounds = {
            UILayout::kPadding + UILayout::ControlPanel::kWidth + m_currentXOffset,
            UILayout::kPadding + (UILayout::ControlPanel::GetPanelHeight() - UILayout::kToggleButtonHeight) * 0.5f,
            UILayout::kToggleButtonWidth,
            UILayout::kToggleButtonHeight
        };
    }

    void ControlPanel::RecreateWidgets()
    {
        if (!m_controller)
            return;

        PanelDependencies deps;
        deps.rendererManager = m_controller->GetRendererManager();
        deps.audioManager = m_controller->GetAudioManager();
        deps.windowManager = m_controller->GetWindowManager();
        deps.onShowAudioSettings = [this]()
            {
                if (m_onShowAudioSettings)
                    m_onShowAudioSettings();
            };

        m_widgetManager.CreateWidgets(WidgetFactory::CreateControlPanelWidgets(deps));
    }

} // namespace Spectrum