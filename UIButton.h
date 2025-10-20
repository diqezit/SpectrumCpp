// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the UIButton, a fundamental interactive component for
// the application's UI. It encapsulates its state (normal, hovered,
// pressed), appearance, and behavior, using the GraphicsContext for drawing.
//
// Defines the UIButton class and its associated ButtonStyle structure.
// The button is a fully self-contained component managing its state, animations,
// and data-driven visual style. It translates user input into a discrete
// click action.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_UI_BUTTON_H
#define SPECTRUM_CPP_UI_BUTTON_H

#include "Common.h"
#include "GraphicsContext.h"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Spectrum {

    struct ButtonStyle {
        std::vector<D2D1_GRADIENT_STOP> backgroundStops;
        std::vector<D2D1_GRADIENT_STOP> backgroundHoverStops;
        Color textColor;
        Color borderColor;
        Color glowColor;
        float cornerRadius;
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

        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);

        void Draw(GraphicsContext& context) const;

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
        // Private Implementation / Internal Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        enum class State { Normal, Hovered, Pressed };

        void ProcessInput(const Point& mousePos, bool isMouseDown);
        void UpdateAnimation(float deltaTime);

        void DrawBackground(GraphicsContext& context) const;
        void DrawText(GraphicsContext& context) const;
        void DrawBorder(GraphicsContext& context) const;
        void DrawGlow(GraphicsContext& context) const;

        [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> GetInterpolatedGradientStops() const;
        [[nodiscard]] Color GetCurrentBorderColor() const;
        [[nodiscard]] Color GetCurrentGlowColor() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static constexpr float kAnimationSpeed = 10.0f;

        Rect m_rect;
        std::wstring m_text;
        std::function<void()> m_onClick;
        ButtonStyle m_style;
        State m_state;
        float m_hoverAnimationProgress;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_BUTTON_H