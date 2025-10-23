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
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Helpers/Geometry/ColorHelpers.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Structs/Paint.h"
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Math;
    using namespace Helpers::Color;

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
    )
        : m_rect(rect)
        , m_style(std::move(style))
        , m_min(min)
        , m_max(max)
        , m_step(step)
        , m_targetValueNormalized(0.0f)
        , m_currentValueNormalized(0.0f)
        , m_visualValueNormalized(0.0f)
        , m_lastCallbackValue(0.0f)
        , m_hoverAnimationProgress(0.0f)
        , m_isDragging(false)
        , m_isHovered(false)
    {
        const float normalized = Saturate(Normalize(initialValue, m_min, m_max));
        SetNormalizedValue(normalized);
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
        if (ShouldDrawGlow()) {
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
        UpdateHoverState(mousePos);
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
        const float normalized = Saturate(Normalize(value, m_min, m_max));
        SetTargetValue(normalized);
        UpdateCurrentValue(normalized);
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
        return Lerp(m_min, m_max, m_currentValueNormalized);
    }

    [[nodiscard]] float UISlider::GetVisualValue() const noexcept
    {
        return Lerp(m_min, m_max, m_visualValueNormalized);
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
    // Update Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::ProcessInput(const MouseInputState& mouseState)
    {
        UpdateHoverState(mouseState.position);

        if (!m_isDragging) {
            ProcessMouseInput(mouseState);
        }
    }

    void UISlider::UpdateAnimations(float deltaTime)
    {
        AnimateHoverProgress(deltaTime);
        AnimateValueProgress(deltaTime);
        AnimateThumbProgress(deltaTime);
    }

    void UISlider::InvokeCallbackIfChanged()
    {
        const float currentValue = GetValue();

        if (ShouldInvokeCallback(currentValue)) {
            TriggerCallback(currentValue);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Input Processing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::UpdateHoverState(const Point& mousePos)
    {
        m_isHovered = HitTest(mousePos);
    }

    void UISlider::ProcessMouseInput(const MouseInputState& mouseState)
    {
        if (m_isHovered && mouseState.isLeftButtonDown) {
            HandleMouseDown(mouseState.position);
        }
    }

    void UISlider::HandleMouseDown(const Point& mousePos)
    {
        m_isDragging = true;
        SetTargetValueFromPosition(mousePos);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::UpdateHoverAnimation(float deltaTime)
    {
        AnimateHoverProgress(deltaTime);
    }

    void UISlider::UpdateValueSmoothing(float deltaTime)
    {
        AnimateValueProgress(deltaTime);
    }

    void UISlider::UpdateThumbSmoothing(float deltaTime)
    {
        AnimateThumbProgress(deltaTime);
    }

    void UISlider::AnimateHoverProgress(float deltaTime)
    {
        const float animationStep = m_style.hoverAnimationSpeed * deltaTime;

        if (ShouldAnimateHoverIn()) {
            m_hoverAnimationProgress = std::min(1.0f, m_hoverAnimationProgress + animationStep);
        }
        else if (ShouldAnimateHoverOut()) {
            m_hoverAnimationProgress = std::max(0.0f, m_hoverAnimationProgress - animationStep);
        }
    }

    void UISlider::AnimateValueProgress(float deltaTime)
    {
        m_currentValueNormalized = ExponentialDecay(
            m_currentValueNormalized,
            m_targetValueNormalized,
            m_style.valueSmoothingSpeed,
            deltaTime
        );
    }

    void UISlider::AnimateThumbProgress(float deltaTime)
    {
        m_visualValueNormalized = ExponentialDecay(
            m_visualValueNormalized,
            m_currentValueNormalized,
            m_style.thumbSmoothingSpeed,
            deltaTime
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Value Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::SetTargetValueFromPosition(const Point& point)
    {
        const float normalizedValue = CalculateNormalizedValueFromPosition(point);
        const float snappedValue = SnapToStep(normalizedValue);
        SetTargetValue(snappedValue);
    }

    void UISlider::SetNormalizedValue(float normalizedValue)
    {
        m_targetValueNormalized = normalizedValue;
        m_currentValueNormalized = normalizedValue;
        m_visualValueNormalized = normalizedValue;
    }

    void UISlider::SetTargetValue(float normalizedValue)
    {
        m_targetValueNormalized = normalizedValue;
    }

    void UISlider::UpdateCurrentValue(float newValue)
    {
        m_currentValueNormalized = newValue;
    }

    [[nodiscard]] float UISlider::CalculateNormalizedValueFromPosition(const Point& point) const
    {
        const float normalizedX = NormalizePositionX(point);
        return Saturate(normalizedX);
    }

    [[nodiscard]] float UISlider::NormalizePositionX(const Point& point) const
    {
        return Normalize(point.x, m_rect.x, m_rect.GetRight());
    }

    [[nodiscard]] float UISlider::SnapToStep(float normalizedValue) const noexcept
    {
        if (!HasValidStep() || !HasValidRange()) {
            return normalizedValue;
        }

        return CalculateSteppedValue(normalizedValue);
    }

    [[nodiscard]] float UISlider::CalculateSteppedValue(float normalizedValue) const
    {
        const float value = Lerp(m_min, m_max, normalizedValue);
        const float numSteps = std::round((value - m_min) / m_step);
        const float snappedValue = std::clamp(m_min + numSteps * m_step, m_min, m_max);

        return Normalize(snappedValue, m_min, m_max);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UISlider::IsActive() const noexcept
    {
        return m_isHovered || m_isDragging;
    }

    [[nodiscard]] bool UISlider::ShouldAnimateHoverIn() const noexcept
    {
        return IsActive();
    }

    [[nodiscard]] bool UISlider::ShouldAnimateHoverOut() const noexcept
    {
        return !IsActive();
    }

    [[nodiscard]] bool UISlider::ShouldInvokeCallback(float currentValue) const noexcept
    {
        return std::abs(currentValue - m_lastCallbackValue) > kCallbackThreshold;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Hit Testing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UISlider::HitTest(const Point& point) const noexcept
    {
        return IsPointInHorizontalBounds(point) && IsPointInVerticalBounds(point);
    }

    [[nodiscard]] bool UISlider::IsPointInHorizontalBounds(const Point& point) const noexcept
    {
        return point.x >= m_rect.x && point.x <= m_rect.GetRight();
    }

    [[nodiscard]] bool UISlider::IsPointInVerticalBounds(const Point& point) const noexcept
    {
        return point.y >= (m_rect.y - kHitboxVerticalPadding) &&
            point.y <= (m_rect.GetBottom() + kHitboxVerticalPadding);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Calculations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] Rect UISlider::GetThumbRect() const noexcept
    {
        const float thumbX = CalculateThumbX();

        return {
            thumbX,
            m_rect.y,
            m_style.thumbWidth,
            m_rect.height
        };
    }

    [[nodiscard]] Rect UISlider::GetTrackRect() const noexcept
    {
        const float trackY = CalculateTrackY();

        return {
            m_rect.x,
            trackY,
            m_rect.width,
            m_style.trackHeight
        };
    }

    [[nodiscard]] Rect UISlider::GetFillRect() const noexcept
    {
        Rect fillRect = GetTrackRect();
        fillRect.width = CalculateFillWidth();

        return fillRect;
    }

    [[nodiscard]] Rect UISlider::GetGlowRect() const noexcept
    {
        const Rect thumbRect = GetThumbRect();

        return {
            thumbRect.x - 2.0f,
            thumbRect.y - 2.0f,
            thumbRect.width + 4.0f,
            thumbRect.height + 4.0f
        };
    }

    [[nodiscard]] float UISlider::CalculateThumbX() const noexcept
    {
        return m_rect.x + (m_visualValueNormalized * m_rect.width) - (m_style.thumbWidth * 0.5f);
    }

    [[nodiscard]] float UISlider::CalculateTrackY() const noexcept
    {
        return m_rect.y + m_rect.height * 0.5f - m_style.trackHeight * 0.5f;
    }

    [[nodiscard]] float UISlider::CalculateFillWidth() const noexcept
    {
        return m_rect.width * m_visualValueNormalized;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::DrawGlow(Canvas& canvas) const
    {
        const Rect glowRect = GetGlowRect();
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
        const Rect trackRect = GetTrackRect();
        const Rect fillRect = GetFillRect();

        DrawTrackBackground(canvas, trackRect);

        if (ShouldDrawFill(fillRect)) {
            DrawTrackFill(canvas, fillRect);
        }
    }

    void UISlider::DrawThumb(Canvas& canvas) const
    {
        const Rect thumbRect = GetThumbRect();

        DrawThumbBody(canvas, thumbRect);
        DrawThumbBorder(canvas, thumbRect);
    }

    void UISlider::DrawTrackBackground(Canvas& canvas, const Rect& trackRect) const
    {
        canvas.DrawRoundedRectangle(
            trackRect,
            m_style.trackCornerRadius,
            Paint::Fill(m_style.trackColor)
        );
    }

    void UISlider::DrawTrackFill(Canvas& canvas, const Rect& fillRect) const
    {
        canvas.DrawRoundedRectangle(
            fillRect,
            m_style.trackCornerRadius,
            Paint::Fill(m_style.fillColor)
        );
    }

    void UISlider::DrawThumbBody(Canvas& canvas, const Rect& thumbRect) const
    {
        const Color thumbColor = GetCurrentThumbColor();

        canvas.DrawRoundedRectangle(
            thumbRect,
            m_style.thumbCornerRadius,
            Paint::Fill(thumbColor)
        );
    }

    void UISlider::DrawThumbBorder(Canvas& canvas, const Rect& thumbRect) const
    {
        canvas.DrawRoundedRectangle(
            thumbRect,
            m_style.thumbCornerRadius,
            Paint::Stroke(m_style.thumbBorderColor, m_style.thumbBorderThickness)
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Color Calculations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] Color UISlider::GetCurrentThumbColor() const noexcept
    {
        return InterpolateThumbColor();
    }

    [[nodiscard]] Color UISlider::GetCurrentGlowColor() const noexcept
    {
        return CalculateGlowColor();
    }

    [[nodiscard]] Color UISlider::InterpolateThumbColor() const noexcept
    {
        return InterpolateColor(
            m_style.thumbColor,
            m_style.thumbHoverColor,
            m_hoverAnimationProgress
        );
    }

    [[nodiscard]] Color UISlider::CalculateGlowColor() const noexcept
    {
        Color glowColor = m_style.thumbGlowColor;
        glowColor.a *= m_hoverAnimationProgress * 0.5f;
        return glowColor;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Callback Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::TriggerCallback(float value)
    {
        UpdateLastCallbackValue(value);

        if (m_onValueChanged) {
            m_onValueChanged(value);
        }
    }

    void UISlider::UpdateLastCallbackValue(float value)
    {
        m_lastCallbackValue = value;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UISlider::HasValidRange() const noexcept
    {
        return (m_max - m_min) > kEpsilon;
    }

    [[nodiscard]] bool UISlider::HasValidStep() const noexcept
    {
        return m_step > kEpsilon;
    }

    [[nodiscard]] bool UISlider::ShouldDrawGlow() const noexcept
    {
        return m_hoverAnimationProgress > 0.0f;
    }

    [[nodiscard]] bool UISlider::ShouldDrawFill(const Rect& fillRect) const noexcept
    {
        return fillRect.width > 0.0f;
    }

} // namespace Spectrum