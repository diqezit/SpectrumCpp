// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the UIButton with smooth cubic easing for hover animations.
//
// This implementation uses EaseInOutCubic for more natural transitions
// compared to the previous quadratic easing.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Components/UIButton.h"
#include "UI/Components/UIButtonColors.h"
#include "Graphics/API/Helpers/Geometry/ColorHelpers.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Graphics/API/Brushes/GradientStop.h"

namespace Spectrum {

    using namespace Helpers::Math;
    using namespace Helpers::Color;
    using namespace UI::ButtonColors;
    using namespace UI::ButtonColors::VisualProperties;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UIButton::UIButton(
        const Rect& rect,
        std::wstring_view text,
        std::function<void()> onClick,
        ButtonStyle style
    )
        : m_rect(rect)
        , m_text(text)
        , m_onClick(std::move(onClick))
        , m_style(std::move(style))
        , m_state(State::Normal)
        , m_hoverAnimationProgress(0.0f)
        , m_pressAnimationProgress(0.0f)
        , m_shimmerOffset(0.0f)
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
        // Draw shadow first
        if (ShouldDrawShadow()) {
            DrawShadow(canvas);
        }

        if (ShouldDrawGlow()) {
            DrawGlow(canvas);
        }

        DrawWithPressedOffset(canvas);
        // The hover effect is now handled through gradient interpolation only
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
            /* backgroundStops */      GetDefaultBackgroundStops(),
            /* backgroundHoverStops */ GetDefaultHoverBackgroundStops(),
            /* borderColor */          GetDefaultBorderColor(),
            /* glowColor */            GetDefaultGlowColor(),
            /* shadowColor */          GetDefaultShadowColor(),
            /* shimmerColor */         GetDefaultShimmerColor(),
            /* cornerRadius */         DefaultCornerRadius,
            /* textStyle */            TextStyle::Default()
                                           .WithColor(GetDefaultTextColor())
                                           .WithAlign(TextAlign::Center)
                                           .WithParagraphAlign(ParagraphAlign::Center)
                                           .WithSize(14.0f)
                                           .WithWeight(FontWeight::Medium)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::ProcessInput(
        const Point& mousePos,
        bool isMouseDown
    )
    {
        const State previousState = m_state;
        UpdateState(mousePos, isMouseDown);
        HandleStateTransition(m_state, previousState);
    }

    void UIButton::UpdateAnimation(float deltaTime)
    {
        AnimateHoverProgress(deltaTime);
        AnimatePressProgress(deltaTime);
        // Removed shimmer animation to prevent flickering
    }

    void UIButton::UpdateState(const Point& mousePos, bool isMouseDown)
    {
        const bool isOver = IsInHitbox(mousePos);

        if (!isOver) {
            m_state = State::Normal;
            return;
        }

        m_state = isMouseDown ? State::Pressed : State::Hovered;
    }

    void UIButton::HandleStateTransition(State newState, State previousState)
    {
        if (newState != previousState) {
            TriggerClickIfReleased(previousState);
        }
    }

    void UIButton::TriggerClickIfReleased(State previousState)
    {
        if (previousState == State::Pressed && m_state == State::Hovered && m_onClick) {
            m_onClick();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::AnimateHoverProgress(float deltaTime)
    {
        if (ShouldAnimateHoverIn()) {
            IncreaseHoverProgress(deltaTime);
        }
        else if (ShouldAnimateHoverOut()) {
            DecreaseHoverProgress(deltaTime);
        }
    }

    void UIButton::AnimatePressProgress(float deltaTime)
    {
        if (m_state == State::Pressed) {
            const float step = PressAnimationSpeed * deltaTime;
            m_pressAnimationProgress = std::min(1.0f, m_pressAnimationProgress + step);
        }
        else {
            const float step = PressAnimationSpeed * deltaTime * 2.0f; // Faster release
            m_pressAnimationProgress = std::max(0.0f, m_pressAnimationProgress - step);
        }
    }

    void UIButton::AnimateShimmer(float deltaTime)
    {
        // Disabled shimmer animation to prevent flickering
        // Keep the method for potential future use
        (void)deltaTime;
        m_shimmerOffset = 0.0f;
    }

    void UIButton::IncreaseHoverProgress(float deltaTime)
    {
        const float step = CalculateAnimationStep(deltaTime);
        m_hoverAnimationProgress = std::min(1.0f, m_hoverAnimationProgress + step);
    }

    void UIButton::DecreaseHoverProgress(float deltaTime)
    {
        const float step = CalculateAnimationStep(deltaTime);
        m_hoverAnimationProgress = std::max(0.0f, m_hoverAnimationProgress - step);
    }

    [[nodiscard]] float UIButton::CalculateAnimationStep(float deltaTime) const noexcept
    {
        return HoverAnimationSpeed * deltaTime;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::DrawShadow(Canvas& canvas) const
    {
        const float shadowOffset = ShadowOffsetY + (1.0f - m_pressAnimationProgress) * 3.0f;

        Rect shadowRect = {
            m_rect.x + ShadowOffsetX,
            m_rect.y + shadowOffset,
            m_rect.width,
            m_rect.height
        };

        Color shadowColor = m_style.shadowColor;
        shadowColor.a *= (ShadowAlphaBase + m_hoverAnimationProgress * ShadowAlphaHover);

        // Draw multiple shadow layers for softer effect
        for (int i = 0; i < 3; ++i) {
            float layerAlpha = shadowColor.a * (1.0f - i * 0.3f);
            Color layerColor = shadowColor;
            layerColor.a = layerAlpha;

            canvas.DrawRoundedRectangle(
                shadowRect,
                m_style.cornerRadius,
                Paint::Fill(layerColor)
            );

            shadowRect.x += 1.0f;
            shadowRect.y += 1.0f;
        }
    }

    void UIButton::DrawGlow(Canvas& canvas) const
    {
        const Rect glowRect = CalculateGlowRect();
        const float glowRadius = CalculateGlowRadius();
        const Color glowColor = GetCurrentGlowColor();

        // Draw outer glow with smoother transition
        DrawGlowEffect(canvas, glowRect, glowRadius, glowColor);

        // Draw inner glow for pressed state
        if (m_pressAnimationProgress > 0.0f) {
            Color innerGlowColor = m_style.glowColor;
            innerGlowColor.a *= m_pressAnimationProgress * 0.2f;

            canvas.DrawRoundedRectangle(
                m_rect,
                m_style.cornerRadius,
                Paint::Stroke(innerGlowColor, DefaultBorderWidth + m_pressAnimationProgress * 0.5f)
            );
        }
    }

    void UIButton::DrawWithPressedOffset(Canvas& canvas) const
    {
        canvas.PushTransform();

        if (IsPressed()) {
            ApplyPressedTransform(canvas);
        }

        DrawBackground(canvas);
        DrawBorder(canvas);
        DrawText(canvas);

        canvas.PopTransform();
    }

    void UIButton::DrawShimmerOverlay(Canvas& canvas) const
    {
        // Disabled shimmer overlay to prevent flickering
        // The method is kept for potential future use or alternative implementations
        (void)canvas;
    }

    void UIButton::ApplyPressedTransform(Canvas& canvas) const
    {
        float pressOffset = m_pressAnimationProgress * PressedOffsetY;
        canvas.TranslateBy(0, pressOffset);
    }

    void UIButton::DrawBackground(Canvas& canvas) const
    {
        const auto& d2dStops = ShouldUseInterpolatedGradient()
            ? GetInterpolatedGradientStops()
            : m_style.backgroundStops;

        DrawBackgroundGradient(canvas, d2dStops);

        // Add subtle highlight overlay on hover
        if (m_hoverAnimationProgress > 0.0f && m_hoverAnimationProgress < 1.0f) {
            Color highlightOverlay(1.0f, 1.0f, 1.0f, 0.02f * m_hoverAnimationProgress);
            canvas.DrawRoundedRectangle(
                m_rect,
                m_style.cornerRadius,
                Paint::Fill(highlightOverlay)
            );
        }
    }

    void UIButton::DrawBorder(Canvas& canvas) const
    {
        const Color borderColor = GetCurrentBorderColor();

        canvas.DrawRoundedRectangle(
            m_rect,
            m_style.cornerRadius,
            Paint::Stroke(borderColor, DefaultBorderWidth)
        );

        // Inner highlight border - more subtle
        if (m_hoverAnimationProgress > 0.0f) {
            Rect innerRect = {
                m_rect.x + 1.0f,
                m_rect.y + 1.0f,
                m_rect.width - 2.0f,
                m_rect.height - 2.0f
            };

            Color highlightColor = GetHighlightColor();
            highlightColor.a *= m_hoverAnimationProgress * 0.5f;

            canvas.DrawRoundedRectangle(
                innerRect,
                m_style.cornerRadius - 1.0f,
                Paint::Stroke(highlightColor, 0.5f)
            );
        }
    }

    void UIButton::DrawText(Canvas& canvas) const
    {
        // Add subtle text shadow for depth
        if (m_hoverAnimationProgress > 0.3f) {
            TextStyle shadowStyle = m_style.textStyle;
            shadowStyle.color = GetTextShadowColor();
            shadowStyle.color.a *= (m_hoverAnimationProgress - 0.3f) * 0.5f;

            Rect shadowTextRect = m_rect;
            shadowTextRect.y += 1.0f;

            canvas.DrawText(m_text, shadowTextRect, shadowStyle);
        }

        // Draw main text with slight brightness adjustment on hover
        TextStyle textStyle = m_style.textStyle;
        if (m_hoverAnimationProgress > 0.0f) {
            float brightness = 1.0f + m_hoverAnimationProgress * 0.1f;
            textStyle.color = AdjustBrightness(textStyle.color, brightness);
        }

        canvas.DrawText(m_text, m_rect, textStyle);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Background Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::DrawBackgroundGradient(
        Canvas& canvas,
        const std::vector<D2D1_GRADIENT_STOP>& d2dStops
    ) const
    {
        std::vector<GradientStop> stops = ConvertGradientStops(d2dStops);

        Paint paint = Paint::LinearGradient(
            { m_rect.x, m_rect.y },
            { m_rect.x, m_rect.GetBottom() },
            stops
        );

        canvas.DrawRoundedRectangle(m_rect, m_style.cornerRadius, paint);
    }

    [[nodiscard]] std::vector<GradientStop> UIButton::ConvertGradientStops(
        const std::vector<D2D1_GRADIENT_STOP>& d2dStops
    ) const
    {
        std::vector<GradientStop> stops;
        stops.reserve(d2dStops.size());

        for (const auto& stop : d2dStops) {
            stops.push_back(ConvertSingleStop(stop));
        }

        return stops;
    }

    [[nodiscard]] GradientStop UIButton::ConvertSingleStop(
        const D2D1_GRADIENT_STOP& stop
    ) const
    {
        return {
            stop.position,
            Color(stop.color.r, stop.color.g, stop.color.b, stop.color.a)
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Glow Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIButton::DrawGlowEffect(
        Canvas& canvas,
        const Rect& /* glowRect */,  // Unused, using calculated rects in loop
        float /* glowRadius */,       // Unused, using calculated radius in loop
        const Color& glowColor
    ) const
    {
        // Multi-layer glow for softer effect
        for (int i = 2; i >= 0; --i) {
            float layerSize = GlowPadding + i * 1.5f;
            Rect layerRect = {
                m_rect.x - layerSize,
                m_rect.y - layerSize,
                m_rect.width + layerSize * 2.0f,
                m_rect.height + layerSize * 2.0f
            };

            Color layerColor = glowColor;
            layerColor.a *= (0.2f / (i + 1));

            canvas.DrawRoundedRectangle(
                layerRect,
                m_style.cornerRadius + layerSize,
                Paint::Stroke(layerColor, 1.0f)
            );
        }
    }

    [[nodiscard]] Rect UIButton::CalculateGlowRect() const
    {
        float glowExpansion = GlowPadding + m_hoverAnimationProgress * 1.5f;
        return {
            m_rect.x - glowExpansion,
            m_rect.y - glowExpansion,
            m_rect.width + glowExpansion * 2.0f,
            m_rect.height + glowExpansion * 2.0f
        };
    }

    [[nodiscard]] float UIButton::CalculateGlowRadius() const noexcept
    {
        return m_style.cornerRadius + GlowPadding;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> UIButton::GetInterpolatedGradientStops() const
    {
        if (!GradientStopsSizeMatch()) {
            return m_style.backgroundStops;
        }

        const float easedProgress = EaseInOutCubic(m_hoverAnimationProgress);
        std::vector<D2D1_GRADIENT_STOP> interpolatedStops = m_style.backgroundStops;

        for (size_t i = 0; i < interpolatedStops.size(); ++i)
        {
            interpolatedStops[i] = InterpolateGradientStop(
                m_style.backgroundStops[i],
                m_style.backgroundHoverStops[i],
                easedProgress
            );
        }

        return interpolatedStops;
    }

    [[nodiscard]] Color UIButton::GetCurrentBorderColor() const
    {
        const float easedProgress = EaseOutCubic(m_hoverAnimationProgress);
        return CalculateBorderColor(easedProgress);
    }

    [[nodiscard]] Color UIButton::GetCurrentGlowColor() const
    {
        const float easedProgress = EaseOutCubic(m_hoverAnimationProgress);
        return CalculateGlowColor(easedProgress);
    }

    [[nodiscard]] D2D1_GRADIENT_STOP UIButton::InterpolateGradientStop(
        const D2D1_GRADIENT_STOP& normalStop,
        const D2D1_GRADIENT_STOP& hoverStop,
        float progress
    ) const
    {
        D2D1_GRADIENT_STOP result = normalStop;

        result.color.r = Lerp(normalStop.color.r, hoverStop.color.r, progress);
        result.color.g = Lerp(normalStop.color.g, hoverStop.color.g, progress);
        result.color.b = Lerp(normalStop.color.b, hoverStop.color.b, progress);
        result.color.a = Lerp(normalStop.color.a, hoverStop.color.a, progress);

        return result;
    }

    [[nodiscard]] Color UIButton::CalculateBorderColor(float easedProgress) const
    {
        Color finalColor = AdjustBrightness(m_style.borderColor, 1.0f + easedProgress * 1.5f);
        finalColor.a = Lerp(BorderAlphaNormal, BorderAlphaHover * 0.7f, easedProgress);
        return finalColor;
    }

    [[nodiscard]] Color UIButton::CalculateGlowColor(float easedProgress) const
    {
        Color finalColor = m_style.glowColor;
        finalColor.a *= GlowAlphaMultiplier * easedProgress * 0.5f;
        return finalColor;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIButton::ShouldDrawShadow() const noexcept
    {
        return true; // Always draw shadow for depth
    }

    [[nodiscard]] bool UIButton::ShouldDrawGlow() const noexcept
    {
        return m_hoverAnimationProgress > 0.0f || m_pressAnimationProgress > 0.0f;
    }

    [[nodiscard]] bool UIButton::ShouldAnimateHoverIn() const noexcept
    {
        return m_state == State::Hovered || m_state == State::Pressed;
    }

    [[nodiscard]] bool UIButton::ShouldAnimateHoverOut() const noexcept
    {
        return m_state == State::Normal;
    }

    [[nodiscard]] bool UIButton::ShouldUseInterpolatedGradient() const noexcept
    {
        return m_hoverAnimationProgress > 0.0f && HasValidGradientStops();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIButton::HasValidGradientStops() const noexcept
    {
        return !m_style.backgroundStops.empty() &&
            !m_style.backgroundHoverStops.empty();
    }

    [[nodiscard]] bool UIButton::GradientStopsSizeMatch() const noexcept
    {
        return m_style.backgroundStops.size() == m_style.backgroundHoverStops.size();
    }

} // namespace Spectrum