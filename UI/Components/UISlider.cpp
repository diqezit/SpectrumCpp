// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the UISlider class with smooth, delta-time based animations
// for professional-quality UI feedback.
//
// Key implementation details:
// - Exponential smoothing for value changes (no instant jumps)
// - Separate visual smoothing for thumb position (eliminates jitter)
// - Hover animation system matching UIButton behavior
// - Optimized callback invocation with threshold checking
// - Frame-rate independent animations via deltaTime
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Components/UISlider.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include <cmath>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UISlider::UISlider(
        const Rect& rect,
        float min,
        float max,
        float initialValue,
        float step,
        SliderStyle style
    ) :
        m_rect(rect),
        m_style(std::move(style)),
        m_min(min),
        m_max(max),
        m_step(step),
        m_targetValueNormalized(0.0f),
        m_currentValueNormalized(0.0f),
        m_visualValueNormalized(0.0f),
        m_lastCallbackValue(0.0f),
        m_hoverAnimationProgress(0.0f),
        m_isDragging(false),
        m_isHovered(false)
    {
        const float normalized = Utils::Saturate(Utils::Normalize(initialValue, m_min, m_max));
        m_targetValueNormalized = normalized;
        m_currentValueNormalized = normalized;
        m_visualValueNormalized = normalized;
        m_lastCallbackValue = initialValue;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::Update(
        const MouseInputState& mouseState,
        float deltaTime
    )
    {
        ProcessInput(mouseState);
        UpdateAnimations(deltaTime);
        InvokeCallbackIfChanged();
    }

    void UISlider::Draw(Canvas& canvas) const
    {
        if (m_hoverAnimationProgress > 0.0f) {
            DrawGlow(canvas);
        }

        DrawTrack(canvas);
        DrawThumb(canvas);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event Handling (Compatibility Layer)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::UpdateHover(const Point& mousePos)
    {
        m_isHovered = HitTest(mousePos);
    }

    void UISlider::BeginDrag(const Point& mousePos)
    {
        m_isDragging = true;
        SetTargetValueFromPosition(mousePos);
    }

    void UISlider::EndDrag()
    {
        m_isDragging = false;
    }

    void UISlider::Drag(const Point& mousePos)
    {
        if (m_isDragging)
        {
            SetTargetValueFromPosition(mousePos);
            m_currentValueNormalized = m_targetValueNormalized;
            m_visualValueNormalized = m_targetValueNormalized;
            InvokeCallbackIfChanged();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries & Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::SetValue(float value)
    {
        const float normalized = Utils::Saturate(Utils::Normalize(value, m_min, m_max));
        m_targetValueNormalized = normalized;
        m_currentValueNormalized = normalized;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::SetOnValueChanged(std::function<void(float)>&& callback)
    {
        m_onValueChanged = std::move(callback);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float UISlider::GetValue() const noexcept
    {
        return Utils::Lerp(m_min, m_max, m_currentValueNormalized);
    }

    [[nodiscard]] float UISlider::GetVisualValue() const noexcept
    {
        return Utils::Lerp(m_min, m_max, m_visualValueNormalized);
    }

    [[nodiscard]] const Rect& UISlider::GetRect() const noexcept
    {
        return m_rect;
    }

    [[nodiscard]] bool UISlider::IsHovered() const noexcept
    {
        return m_isHovered;
    }

    [[nodiscard]] bool UISlider::IsDragging() const noexcept
    {
        return m_isDragging;
    }

    [[nodiscard]] bool UISlider::IsInHitbox(const Point& point) const noexcept
    {
        return HitTest(point);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::ProcessInput(const MouseInputState& mouseState)
    {
        m_isHovered = HitTest(mouseState.position);

        if (m_isDragging) {
            return;
        }

        if (m_isHovered && mouseState.isLeftButtonDown) {
            m_isDragging = true;
            SetTargetValueFromPosition(mouseState.position);
        }
    }

    void UISlider::UpdateAnimations(float deltaTime)
    {
        UpdateHoverAnimation(deltaTime);
        UpdateValueSmoothing(deltaTime);
        UpdateThumbSmoothing(deltaTime);
    }

    void UISlider::UpdateHoverAnimation(float deltaTime)
    {
        const float animationStep = m_style.hoverAnimationSpeed * deltaTime;

        if (IsActive()) {
            m_hoverAnimationProgress = std::min(1.0f, m_hoverAnimationProgress + animationStep);
        }
        else {
            m_hoverAnimationProgress = std::max(0.0f, m_hoverAnimationProgress - animationStep);
        }
    }

    void UISlider::UpdateValueSmoothing(float deltaTime)
    {
        m_currentValueNormalized = Utils::ExponentialDecay(
            m_currentValueNormalized,
            m_targetValueNormalized,
            m_style.valueSmoothingSpeed,
            deltaTime
        );
    }

    void UISlider::UpdateThumbSmoothing(float deltaTime)
    {
        m_visualValueNormalized = Utils::ExponentialDecay(
            m_visualValueNormalized,
            m_currentValueNormalized,
            m_style.thumbSmoothingSpeed,
            deltaTime
        );
    }

    void UISlider::SetTargetValueFromPosition(const Point& point)
    {
        const float normalizedX = Utils::Normalize(point.x, m_rect.x, m_rect.GetRight());
        const float rawNormalizedValue = Utils::Saturate(normalizedX);
        m_targetValueNormalized = SnapToStep(rawNormalizedValue);
    }

    [[nodiscard]] float UISlider::SnapToStep(float normalizedValue) const noexcept
    {
        if (m_step <= 0.0f) {
            return normalizedValue;
        }

        const float totalRange = m_max - m_min;
        if (totalRange <= 0.0f) {
            return normalizedValue;
        }

        const float value = Utils::Lerp(m_min, m_max, normalizedValue);
        const float numSteps = std::round((value - m_min) / m_step);
        const float snappedValue = std::clamp(m_min + numSteps * m_step, m_min, m_max);

        return Utils::Normalize(snappedValue, m_min, m_max);
    }

    [[nodiscard]] bool UISlider::IsActive() const noexcept
    {
        return m_isHovered || m_isDragging;
    }

    [[nodiscard]] bool UISlider::HitTest(const Point& point) const noexcept
    {
        return point.x >= m_rect.x &&
            point.x <= m_rect.GetRight() &&
            point.y >= (m_rect.y - kHitboxVerticalPadding) &&
            point.y <= (m_rect.GetBottom() + kHitboxVerticalPadding);
    }

    [[nodiscard]] Rect UISlider::GetThumbRect() const noexcept
    {
        const float thumbX = m_rect.x +
            (m_visualValueNormalized * m_rect.width) -
            (m_style.thumbWidth * 0.5f);

        return {
            thumbX,
            m_rect.y,
            m_style.thumbWidth,
            m_rect.height
        };
    }

    void UISlider::DrawGlow(Canvas& canvas) const
    {
        const Rect thumbRect = GetThumbRect();
        Rect glowRect = thumbRect;
        glowRect.x -= 2.0f;
        glowRect.y -= 2.0f;
        glowRect.width += 4.0f;
        glowRect.height += 4.0f;

        const Color glowColor = GetCurrentGlowColor();
        const float glowRadius = m_style.thumbCornerRadius + 2.0f;

        canvas.DrawRoundedRectangle(
            glowRect,
            glowRadius,
            Paint::Stroke(glowColor, 2.0f)
        );
    }

    void UISlider::DrawTrack(Canvas& canvas) const
    {
        const Rect trackRect = {
            m_rect.x,
            m_rect.y + m_rect.height * 0.5f - m_style.trackHeight * 0.5f,
            m_rect.width,
            m_style.trackHeight
        };

        Rect fillRect = trackRect;
        fillRect.width *= m_visualValueNormalized;

        canvas.DrawRoundedRectangle(
            trackRect,
            m_style.trackCornerRadius,
            Paint::Fill(m_style.trackColor)
        );

        if (fillRect.width > 0.0f) {
            canvas.DrawRoundedRectangle(
                fillRect,
                m_style.trackCornerRadius,
                Paint::Fill(m_style.fillColor)
            );
        }
    }

    void UISlider::DrawThumb(Canvas& canvas) const
    {
        const Rect thumbRect = GetThumbRect();
        const Color thumbColor = GetCurrentThumbColor();

        canvas.DrawRoundedRectangle(
            thumbRect,
            m_style.thumbCornerRadius,
            Paint::Fill(thumbColor)
        );

        canvas.DrawRoundedRectangle(
            thumbRect,
            m_style.thumbCornerRadius,
            Paint::Stroke(m_style.thumbBorderColor, m_style.thumbBorderThickness)
        );
    }

    [[nodiscard]] Color UISlider::GetCurrentThumbColor() const noexcept
    {
        return Utils::InterpolateColor(
            m_style.thumbColor,
            m_style.thumbHoverColor,
            m_hoverAnimationProgress
        );
    }

    [[nodiscard]] Color UISlider::GetCurrentGlowColor() const noexcept
    {
        Color glowColor = m_style.thumbGlowColor;
        glowColor.a *= m_hoverAnimationProgress * 0.5f;
        return glowColor;
    }

    void UISlider::InvokeCallbackIfChanged()
    {
        const float currentValue = GetValue();

        if (std::abs(currentValue - m_lastCallbackValue) > kCallbackThreshold) {
            m_lastCallbackValue = currentValue;
            if (m_onValueChanged) {
                m_onValueChanged(currentValue);
            }
        }
    }

} // namespace Spectrum