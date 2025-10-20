// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the UISlider widget, a stateful component for selecting a value
// within a defined range. It encapsulates interaction logic, value management
// (including snapping), and exposes its state for rendering and business
// logic integration. The class is designed to operate within a state-driven
// UI loop, receiving input snapshots to update its internal state.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_UI_SLIDER_H
#define SPECTRUM_UI_SLIDER_H

#include "Common.h"
#include "GraphicsContext.h"
#include <functional>
#include <utility>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct MouseInputState {
        Point position;
        bool isLeftButtonDown;
    };

    struct SliderStyle {
        Color trackColor{ 0.1f, 0.1f, 0.1f, 0.8f };
        Color thumbColor{ 0.8f, 0.8f, 0.8f, 1.0f };
        Color thumbHoverColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        Color thumbBorderColor{ 0.1f, 0.1f, 0.1f, 0.5f };
        float trackHeight = 4.0f;
        float trackCornerRadius = 2.0f;
        float thumbWidth = 10.0f;
        float thumbCornerRadius = 3.0f;
        float thumbBorderThickness = 1.0f;
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

        // Non-copyable and non-movable to prevent accidental duplication of state
        UISlider(const UISlider&) = delete;
        UISlider& operator=(const UISlider&) = delete;
        UISlider(UISlider&&) = delete;
        UISlider& operator=(UISlider&&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Execution Loop
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(const MouseInputState& mouseState);
        void Draw(GraphicsContext& context) const;

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

        void SetOnValueChanged(
            std::function<void(float)>&& callback
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetValue() const;
        [[nodiscard]] const Rect& GetRect() const noexcept;
        [[nodiscard]] bool IsHovered() const noexcept;
        [[nodiscard]] bool IsDragging() const noexcept;
        [[nodiscard]] bool HitTest(const Point& point) const;
        [[nodiscard]] bool IsInHitbox(const Point& point) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetValueFromPosition(const Point& point);
        [[nodiscard]] float SnapToStep(float normalizedValue) const;
        [[nodiscard]] bool IsActive() const noexcept;
        [[nodiscard]] Rect GetThumbRect() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static constexpr float kEpsilon = 1e-5f;

        Rect m_rect;
        SliderStyle m_style;

        // Value properties
        float m_min;
        float m_max;
        float m_step;
        float m_currentValueNormalized;

        // State
        bool m_isDragging;
        bool m_isHovered;

        // Callback
        std::function<void(float)> m_onValueChanged;
    };

} // namespace Spectrum

#endif // SPECTRUM_UI_SLIDER_H