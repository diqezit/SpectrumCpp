// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the UIButton with smooth cubic easing for hover animations.
//
// This implementation uses EaseInOutCubic for более natural transitions
// compared to the previous quadratic easing.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Components/UIButton.h"
#include "Common/ColorUtils.h"
#include "Common/MathUtils.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Graphics/API/Brushes/GradientStop.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UIButton::UIButton(
        const Rect& rect,
        std::wstring_view text,
        std::function<void()> onClick,
        ButtonStyle style
    ) :
        m_rect(rect),
        m_text(text),
        m_onClick(std::move(onClick)),
        m_style(std::move(style)),
        m_state(State::Normal),
        m_hoverAnimationProgress(0.0f)
    {
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Interface
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::Update(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        ProcessInput(mousePos, isMouseDown);
        UpdateAnimation(deltaTime);
    }

    void UIButton::Draw(Canvas& canvas) const
    {
        if (m_hoverAnimationProgress > 0.0f) {
            DrawGlow(canvas);
        }

        canvas.PushTransform();

        if (IsPressed()) {
            canvas.TranslateBy(1.0f, 1.0f);
        }

        DrawBackground(canvas);
        DrawBorder(canvas);
        DrawText(canvas);

        canvas.PopTransform();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIButton::IsHovered() const noexcept
    {
        return m_state == State::Hovered;
    }

    [[nodiscard]] bool UIButton::IsPressed() const noexcept
    {
        return m_state == State::Pressed;
    }

    [[nodiscard]] bool UIButton::IsInHitbox(const Point& mousePos) const noexcept
    {
        return mousePos.x >= m_rect.x && mousePos.x <= m_rect.GetRight() &&
            mousePos.y >= m_rect.y && mousePos.y <= m_rect.GetBottom();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::SetStyle(const ButtonStyle& style)
    {
        m_style = style;
    }

    void UIButton::SetRect(const Rect& rect) noexcept
    {
        m_rect = rect;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] ButtonStyle UIButton::GetDefaultStyle()
    {
        return {
            /* backgroundStops */      { { 0.0f, D2D1::ColorF(0.2f, 0.22f, 0.25f) },
                                         { 1.0f, D2D1::ColorF(0.15f, 0.17f, 0.2f) } },
            /* backgroundHoverStops */ { { 0.0f, D2D1::ColorF(0.35f, 0.38f, 0.42f) },
                                         { 1.0f, D2D1::ColorF(0.25f, 0.27f, 0.3f) } },
            /* borderColor */          Color(1.0f, 1.0f, 1.0f, 0.1f),
            /* glowColor */            Color(0.5f, 0.7f, 1.0f),
            /* cornerRadius */         4.0f,
            /* textStyle */            TextStyle::Default()
                                           .WithColor(Color::White())
                                           .WithAlign(TextAlign::Center)
                                           .WithParagraphAlign(ParagraphAlign::Center)
                                           .WithSize(14.0f)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::ProcessInput(
        const Point& mousePos,
        bool isMouseDown
    )
    {
        const bool isOver = IsInHitbox(mousePos);
        const State previousState = m_state;

        if (!isOver) {
            m_state = State::Normal;
            return;
        }

        if (isMouseDown) {
            m_state = State::Pressed;
        }
        else {
            m_state = State::Hovered;
            if (previousState == State::Pressed && m_onClick) {
                m_onClick();
            }
        }
    }

    void UIButton::UpdateAnimation(float deltaTime)
    {
        const float animationStep = kAnimationSpeed * deltaTime;

        if (m_state == State::Hovered || m_state == State::Pressed) {
            m_hoverAnimationProgress = std::min(1.0f, m_hoverAnimationProgress + animationStep);
        }
        else {
            m_hoverAnimationProgress = std::max(0.0f, m_hoverAnimationProgress - animationStep);
        }
    }

    void UIButton::DrawBackground(Canvas& canvas) const
    {
        const auto& d2dStops = (m_hoverAnimationProgress <= 0.0f)
            ? m_style.backgroundStops
            : GetInterpolatedGradientStops();

        std::vector<GradientStop> stops;
        stops.reserve(d2dStops.size());
        for (const auto& stop : d2dStops) {
            stops.push_back({
                stop.position,
                Color(stop.color.r, stop.color.g, stop.color.b, stop.color.a)
                });
        }

        Paint paint = Paint::LinearGradient(
            { m_rect.x, m_rect.y },
            { m_rect.x, m_rect.GetBottom() },
            stops
        );

        canvas.DrawRoundedRectangle(m_rect, m_style.cornerRadius, paint);
    }

    void UIButton::DrawBorder(Canvas& canvas) const
    {
        const Color borderColor = GetCurrentBorderColor();
        canvas.DrawRoundedRectangle(
            m_rect,
            m_style.cornerRadius,
            Paint::Stroke(borderColor, 1.0f)
        );
    }

    void UIButton::DrawGlow(Canvas& canvas) const
    {
        Rect glowRect = m_rect;
        glowRect.x -= 2.0f;
        glowRect.y -= 2.0f;
        glowRect.width += 4.0f;
        glowRect.height += 4.0f;

        const Color glowColor = GetCurrentGlowColor();
        const float glowRadius = m_style.cornerRadius + 2.0f;

        canvas.DrawRoundedRectangle(
            glowRect,
            glowRadius,
            Paint::Stroke(glowColor, 2.0f)
        );
    }

    void UIButton::DrawText(Canvas& canvas) const
    {
        canvas.DrawText(
            m_text,
            m_rect,
            m_style.textStyle
        );
    }

    [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> UIButton::GetInterpolatedGradientStops() const
    {
        if (m_style.backgroundStops.size() != m_style.backgroundHoverStops.size()) {
            return m_style.backgroundStops;
        }

        auto interpolatedStops = m_style.backgroundStops;
        const float easedProgress = Utils::EaseInOutCubic(m_hoverAnimationProgress);

        for (size_t i = 0; i < interpolatedStops.size(); ++i)
        {
            const auto& normalColor = m_style.backgroundStops[i].color;
            const auto& hoverColor = m_style.backgroundHoverStops[i].color;
            auto& targetColor = interpolatedStops[i].color;

            targetColor.r = Utils::Lerp(normalColor.r, hoverColor.r, easedProgress);
            targetColor.g = Utils::Lerp(normalColor.g, hoverColor.g, easedProgress);
            targetColor.b = Utils::Lerp(normalColor.b, hoverColor.b, easedProgress);
            targetColor.a = Utils::Lerp(normalColor.a, hoverColor.a, easedProgress);
        }

        return interpolatedStops;
    }

    [[nodiscard]] Color UIButton::GetCurrentBorderColor() const
    {
        const float easedProgress = Utils::EaseOutCubic(m_hoverAnimationProgress);
        Color finalColor = Utils::AdjustBrightness(m_style.borderColor, 1.0f + easedProgress * 1.5f);
        finalColor.a = Utils::Lerp(0.1f, 0.4f, easedProgress);
        return finalColor;
    }

    [[nodiscard]] Color UIButton::GetCurrentGlowColor() const
    {
        const float easedProgress = Utils::EaseOutCubic(m_hoverAnimationProgress);
        Color finalColor = m_style.glowColor;
        finalColor.a *= 0.5f * easedProgress;
        return finalColor;
    }

} // namespace Spectrum