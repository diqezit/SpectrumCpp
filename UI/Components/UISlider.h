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
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ProcessInput(const MouseInputState& mouseState);
        void UpdateAnimations(float deltaTime);
        void UpdateHoverAnimation(float deltaTime);
        void UpdateValueSmoothing(float deltaTime);
        void UpdateThumbSmoothing(float deltaTime);

        void SetTargetValueFromPosition(const Point& point);
        [[nodiscard]] float SnapToStep(float normalizedValue) const noexcept;
        [[nodiscard]] bool IsActive() const noexcept;
        [[nodiscard]] bool HitTest(const Point& point) const noexcept;
        [[nodiscard]] Rect GetThumbRect() const noexcept;

        void DrawGlow(Canvas& canvas) const;
        void DrawTrack(Canvas& canvas) const;
        void DrawThumb(Canvas& canvas) const;

        [[nodiscard]] Color GetCurrentThumbColor() const noexcept;
        [[nodiscard]] Color GetCurrentGlowColor() const noexcept;

        void InvokeCallbackIfChanged();

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