#ifndef SPECTRUM_CPP_UI_BUTTON_H
#define SPECTRUM_CPP_UI_BUTTON_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the UIButton, a fundamental interactive component for
// the application's UI. It encapsulates its state (normal, hovered,
// pressed), appearance, and behavior, using the Canvas for drawing.
//
// Defines the UIButton class and its associated ButtonStyle structure.
// The button is a fully self-contained component managing its state, animations,
// and data-driven visual style. It translates user input into a discrete
// click action.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Graphics/API/Brushes/GradientStop.h"
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <d2d1.h>

namespace Spectrum {

    class Canvas;

    struct ButtonStyle {
        std::vector<D2D1_GRADIENT_STOP> backgroundStops;
        std::vector<D2D1_GRADIENT_STOP> backgroundHoverStops;
        Color borderColor;
        Color glowColor;
        Color shadowColor;
        Color shimmerColor;
        float cornerRadius;
        TextStyle textStyle;
    };

    class UIButton final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit UIButton(
            const Rect& rect,
            std::wstring_view text,
            std::function<void()> onClick,
            ButtonStyle style = GetDefaultStyle()
        );

        ~UIButton() noexcept = default;

        // Prevent copy and move operations
        UIButton(const UIButton&) = delete;
        UIButton& operator=(const UIButton&) = delete;
        UIButton(UIButton&&) = delete;
        UIButton& operator=(UIButton&&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void Draw(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsHovered() const noexcept;
        [[nodiscard]] bool IsPressed() const noexcept;
        [[nodiscard]] bool IsInHitbox(const Point& mousePos) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetStyle(const ButtonStyle& style);
        void SetRect(const Rect& rect) noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] static ButtonStyle GetDefaultStyle();

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        enum class State { Normal, Hovered, Pressed };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Update Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ProcessInput(const Point& mousePos, bool isMouseDown);
        void UpdateAnimation(float deltaTime);

        void UpdateState(const Point& mousePos, bool isMouseDown);
        void HandleStateTransition(State newState, State previousState);
        void TriggerClickIfReleased(State previousState);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void AnimateHoverProgress(float deltaTime);
        void AnimatePressProgress(float deltaTime);
        void AnimateShimmer(float deltaTime);
        void IncreaseHoverProgress(float deltaTime);
        void DecreaseHoverProgress(float deltaTime);
        [[nodiscard]] float CalculateAnimationStep(float deltaTime) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawShadow(Canvas& canvas) const;
        void DrawGlow(Canvas& canvas) const;
        void DrawBackground(Canvas& canvas) const;
        void DrawBorder(Canvas& canvas) const;
        void DrawText(Canvas& canvas) const;
        void DrawShimmerOverlay(Canvas& canvas) const;

        void DrawWithPressedOffset(Canvas& canvas) const;
        void ApplyPressedTransform(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Background Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawBackgroundGradient(Canvas& canvas, const std::vector<D2D1_GRADIENT_STOP>& d2dStops) const;
        [[nodiscard]] std::vector<GradientStop> ConvertGradientStops(const std::vector<D2D1_GRADIENT_STOP>& d2dStops) const;
        [[nodiscard]] GradientStop ConvertSingleStop(const D2D1_GRADIENT_STOP& stop) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Glow Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawGlowEffect(Canvas& canvas, const Rect& glowRect, float glowRadius, const Color& glowColor) const;
        [[nodiscard]] Rect CalculateGlowRect() const;
        [[nodiscard]] float CalculateGlowRadius() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Color Calculations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> GetInterpolatedGradientStops() const;
        [[nodiscard]] Color GetCurrentBorderColor() const;
        [[nodiscard]] Color GetCurrentGlowColor() const;

        [[nodiscard]] D2D1_GRADIENT_STOP InterpolateGradientStop(
            const D2D1_GRADIENT_STOP& normalStop,
            const D2D1_GRADIENT_STOP& hoverStop,
            float progress
        ) const;

        [[nodiscard]] Color CalculateBorderColor(float easedProgress) const;
        [[nodiscard]] Color CalculateGlowColor(float easedProgress) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ShouldDrawShadow() const noexcept;
        [[nodiscard]] bool ShouldDrawGlow() const noexcept;
        [[nodiscard]] bool ShouldAnimateHoverIn() const noexcept;
        [[nodiscard]] bool ShouldAnimateHoverOut() const noexcept;
        [[nodiscard]] bool ShouldUseInterpolatedGradient() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HasValidGradientStops() const noexcept;
        [[nodiscard]] bool GradientStopsSizeMatch() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        Rect m_rect;
        std::wstring m_text;
        std::function<void()> m_onClick;
        ButtonStyle m_style;
        State m_state;
        float m_hoverAnimationProgress;
        float m_pressAnimationProgress;
        float m_shimmerOffset;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_BUTTON_H