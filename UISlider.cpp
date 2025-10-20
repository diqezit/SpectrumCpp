// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the UISlider class. This file contains the logic for processing
// user input to update the slider's value, handling state transitions
// (hover, drag), and providing geometric information for rendering, based
// on a configurable style and value range.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UISlider.h"
#include "MathUtils.h"
#include <algorithm>
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
        m_currentValueNormalized(0.0f),
        m_isDragging(false),
        m_isHovered(false),
        m_onValueChanged(nullptr)
    {
        SetValue(initialValue);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::Update(const MouseInputState& mouseState)
    {
        m_isHovered = HitTest(mouseState.position);

        if (m_isDragging) {
            if (mouseState.isLeftButtonDown) {
                SetValueFromPosition(mouseState.position);
            }
            else {
                m_isDragging = false;
            }
        }
        else if (m_isHovered && mouseState.isLeftButtonDown) {
            m_isDragging = true;
            SetValueFromPosition(mouseState.position);
        }
    }

    void UISlider::Draw(GraphicsContext& context) const
    {
        const Rect trackRect = {
            m_rect.x,
            m_rect.y + m_rect.height / 2.0f - m_style.trackHeight / 2.0f,
            m_rect.width,
            m_style.trackHeight
        };
        context.DrawRoundedRectangle(
            trackRect,
            m_style.trackCornerRadius,
            m_style.trackColor,
            true
        );

        const Rect thumbRect = GetThumbRect();
        const Color thumbColor = IsActive() ? m_style.thumbHoverColor : m_style.thumbColor;

        context.DrawRoundedRectangle(
            thumbRect,
            m_style.thumbCornerRadius,
            thumbColor,
            true
        );
        context.DrawRoundedRectangle(
            thumbRect,
            m_style.thumbCornerRadius,
            m_style.thumbBorderColor,
            false,
            m_style.thumbBorderThickness
        );
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
        SetValueFromPosition(mousePos);
    }

    void UISlider::EndDrag()
    {
        m_isDragging = false;
    }

    void UISlider::Drag(const Point& mousePos)
    {
        if (m_isDragging) SetValueFromPosition(mousePos);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries & Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::SetValue(float value)
    {
        const float normalizedValue = Utils::Saturate(Utils::Normalize(value, m_min, m_max));

        if (std::abs(m_currentValueNormalized - normalizedValue) > kEpsilon) {
            m_currentValueNormalized = normalizedValue;
            if (m_onValueChanged) m_onValueChanged(GetValue());
        }
        else {
            m_currentValueNormalized = normalizedValue;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::SetOnValueChanged(
        std::function<void(float)>&& callback
    )
    {
        m_onValueChanged = std::move(callback);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float UISlider::GetValue() const
    {
        return Utils::Lerp(m_min, m_max, m_currentValueNormalized);
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

    [[nodiscard]] bool UISlider::HitTest(const Point& point) const
    {
        // Use a slightly larger hitbox for easier interaction, especially vertically
        const float verticalPadding = 4.0f;
        return point.x >= m_rect.x &&
            point.x <= m_rect.GetRight() &&
            point.y >= (m_rect.y - verticalPadding) &&
            point.y <= (m_rect.GetBottom() + verticalPadding);
    }

    [[nodiscard]] bool UISlider::IsInHitbox(const Point& point) const
    {
        return HitTest(point);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UISlider::SetValueFromPosition(const Point& point)
    {
        const float normalizedX = Utils::Normalize(point.x, m_rect.x, m_rect.GetRight());
        const float rawNormalizedValue = Utils::Saturate(normalizedX);
        const float newNormalizedValue = SnapToStep(rawNormalizedValue);

        if (std::abs(newNormalizedValue - m_currentValueNormalized) < kEpsilon) return;

        m_currentValueNormalized = newNormalizedValue;

        if (m_onValueChanged) m_onValueChanged(GetValue());
    }

    [[nodiscard]] float UISlider::SnapToStep(float normalizedValue) const
    {
        if (m_step <= 0.0f) return normalizedValue;

        const float totalRange = m_max - m_min;
        if (totalRange <= 0.0f) return normalizedValue;

        // Convert normalized value to the actual scale
        const float value = Utils::Lerp(m_min, m_max, normalizedValue);

        // Calculate the nearest step multiple
        const float numSteps = std::round((value - m_min) / m_step);
        const float snappedValue = m_min + numSteps * m_step;

        // Clamp the snapped value to the valid range and convert back to normalized
        const float clampedValue = std::clamp(snappedValue, m_min, m_max);
        return Utils::Normalize(clampedValue, m_min, m_max);
    }

    [[nodiscard]] bool UISlider::IsActive() const noexcept
    {
        return m_isHovered || m_isDragging;
    }

    [[nodiscard]] Rect UISlider::GetThumbRect() const
    {
        const float thumbX = m_rect.x + (m_currentValueNormalized * m_rect.width) - (m_style.thumbWidth / 2.0f);
        return {
            thumbX,
            m_rect.y,
            m_style.thumbWidth,
            m_rect.height
        };
    }

} // namespace Spectrum