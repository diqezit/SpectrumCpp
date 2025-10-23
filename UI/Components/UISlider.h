#ifndef SPECTRUM_UI_SLIDER_H
#define SPECTRUM_UI_SLIDER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the UISlider widget, a stateful component for selecting a value
// within a defined range with smooth, frame-rate independent animations.
//
// This implementation features:
// - Smooth value interpolation with configurable damping
// - Hover animations identical to UIButton
// - Visual thumb smoothing for fluid dragging experience
// - Delta-time based animations for consistent 60fps+ feel
// - Optimized callback invocation with change threshold
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <functional>

namespace Spectrum {

    class Canvas; // Forward declaration

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct MouseInputState {
        Point position;
        bool isLeftButtonDown;
    };

    struct SliderStyle {
        Color trackColor{ 0.1f, 0.1f, 0.1f, 0.8f };
        Color fillColor{ 0.2f, 0.4f, 0.8f, 1.0f };
        Color thumbColor{ 0.8f, 0.8f, 0.8f, 1.0f };
        Color thumbHoverColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        Color thumbBorderColor{ 0.1f, 0.1f, 0.1f, 0.5f };
        Color thumbGlowColor{ 0.5f, 0.7f, 1.0f, 0.8f };

        float trackHeight = 4.0f;
        float trackCornerRadius = 2.0f;
        float thumbWidth = 10.0f;
        float thumbCornerRadius = 3.0f;
        float thumbBorderThickness = 1.0f;

        float hoverAnimationSpeed = 12.0f;
        float valueSmoothingSpeed = 25.0f;
        float thumbSmoothingSpeed = 30.0f;
    };

    class UISlider final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        UISlider(
            const Rect& rect,
            float min,
            float max,
            float initialValue,
            float step = 0.0f,
            SliderStyle style = {}
        );
        ~UISlider() noexcept = default;

        UISlider(const UISlider&) = delete;
        UISlider& operator=(const UISlider&) = delete;
        UISlider(UISlider&&) = delete;
        UISlider& operator=(UISlider&&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const MouseInputState& mouseState, float deltaTime);
        void Draw(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Event Handling (Compatibility Layer)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateHover(const Point& mousePos);
        void BeginDrag(const Point& mousePos);
        void EndDrag();
        void Drag(const Point& mousePos);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries & Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetValue(float value);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetOnValueChanged(std::function<void(float)>&& callback);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetValue() const noexcept;
        [[nodiscard]] float GetVisualValue() const noexcept;
        [[nodiscard]] const Rect& GetRect() const noexcept;
        [[nodiscard]] bool IsHovered() const noexcept;
        [[nodiscard]] bool IsDragging() const noexcept;
        [[nodiscard]] bool IsInHitbox(const Point& point) const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Update Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ProcessInput(const MouseInputState& mouseState);
        void UpdateAnimations(float deltaTime);
        void InvokeCallbackIfChanged();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Input Processing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateHoverState(const Point& mousePos);
        void ProcessMouseInput(const MouseInputState& mouseState);
        void HandleMouseDown(const Point& mousePos);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateHoverAnimation(float deltaTime);
        void UpdateValueSmoothing(float deltaTime);
        void UpdateThumbSmoothing(float deltaTime);

        void AnimateHoverProgress(float deltaTime);
        void AnimateValueProgress(float deltaTime);
        void AnimateThumbProgress(float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Value Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetTargetValueFromPosition(const Point& point);
        void SetNormalizedValue(float normalizedValue);
        void SetTargetValue(float normalizedValue);
        void UpdateCurrentValue(float newValue);

        [[nodiscard]] float CalculateNormalizedValueFromPosition(const Point& point) const;
        [[nodiscard]] float NormalizePositionX(const Point& point) const;
        [[nodiscard]] float SnapToStep(float normalizedValue) const noexcept;
        [[nodiscard]] float CalculateSteppedValue(float normalizedValue) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsActive() const noexcept;
        [[nodiscard]] bool ShouldAnimateHoverIn() const noexcept;
        [[nodiscard]] bool ShouldAnimateHoverOut() const noexcept;
        [[nodiscard]] bool ShouldInvokeCallback(float currentValue) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Hit Testing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HitTest(const Point& point) const noexcept;
        [[nodiscard]] bool IsPointInHorizontalBounds(const Point& point) const noexcept;
        [[nodiscard]] bool IsPointInVerticalBounds(const Point& point) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Calculations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Rect GetThumbRect() const noexcept;
        [[nodiscard]] Rect GetTrackRect() const noexcept;
        [[nodiscard]] Rect GetFillRect() const noexcept;
        [[nodiscard]] Rect GetGlowRect() const noexcept;

        [[nodiscard]] float CalculateThumbX() const noexcept;
        [[nodiscard]] float CalculateTrackY() const noexcept;
        [[nodiscard]] float CalculateFillWidth() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawGlow(Canvas& canvas) const;
        void DrawTrack(Canvas& canvas) const;
        void DrawThumb(Canvas& canvas) const;

        void DrawTrackBackground(Canvas& canvas, const Rect& trackRect) const;
        void DrawTrackFill(Canvas& canvas, const Rect& fillRect) const;
        void DrawThumbBody(Canvas& canvas, const Rect& thumbRect) const;
        void DrawThumbBorder(Canvas& canvas, const Rect& thumbRect) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Color GetCurrentThumbColor() const noexcept;
        [[nodiscard]] Color GetCurrentGlowColor() const noexcept;
        [[nodiscard]] Color InterpolateThumbColor() const noexcept;
        [[nodiscard]] Color CalculateGlowColor() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Callback Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void TriggerCallback(float value);
        void UpdateLastCallbackValue(float value);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HasValidRange() const noexcept;
        [[nodiscard]] bool HasValidStep() const noexcept;
        [[nodiscard]] bool ShouldDrawGlow() const noexcept;
        [[nodiscard]] bool ShouldDrawFill(const Rect& fillRect) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static constexpr float kEpsilon = 1e-5f;
        static constexpr float kCallbackThreshold = 1e-4f;
        static constexpr float kHitboxVerticalPadding = 4.0f;

        Rect m_rect;
        SliderStyle m_style;

        float m_min;
        float m_max;
        float m_step;

        float m_targetValueNormalized;
        float m_currentValueNormalized;
        float m_visualValueNormalized;
        float m_lastCallbackValue;

        float m_hoverAnimationProgress;

        bool m_isDragging;
        bool m_isHovered;

        std::function<void(float)> m_onValueChanged;
    };

} // namespace Spectrum

#endif // SPECTRUM_UI_SLIDER_H