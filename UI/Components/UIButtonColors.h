#ifndef SPECTRUM_CPP_UI_BUTTON_COLORS_H
#define SPECTRUM_CPP_UI_BUTTON_COLORS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines all color schemes and visual properties for UIButton,
// following the Single Responsibility Principle (SRP) by separating
// visual styling from button logic.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <d2d1.h>
#include <vector>

namespace Spectrum {
    namespace UI {
        namespace ButtonColors {

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Background Gradients
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            inline std::vector<D2D1_GRADIENT_STOP> GetDefaultBackgroundStops()
            {
                return {
                    { 0.0f, D2D1::ColorF(0.25f, 0.28f, 0.34f) },
                    { 0.5f, D2D1::ColorF(0.22f, 0.25f, 0.31f) },
                    { 1.0f, D2D1::ColorF(0.18f, 0.21f, 0.27f) }
                };
            }

            inline std::vector<D2D1_GRADIENT_STOP> GetDefaultHoverBackgroundStops()
            {
                return {
                    { 0.0f, D2D1::ColorF(0.42f, 0.48f, 0.58f) },
                    { 0.5f, D2D1::ColorF(0.35f, 0.41f, 0.52f) },
                    { 1.0f, D2D1::ColorF(0.28f, 0.34f, 0.45f) }
                };
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Accent Colors
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            inline Color GetDefaultBorderColor()
            {
                return Color(0.6f, 0.7f, 0.9f, 0.15f);
            }

            inline Color GetDefaultGlowColor()
            {
                return Color(0.4f, 0.6f, 1.0f);
            }

            inline Color GetDefaultShadowColor()
            {
                return Color(0.0f, 0.0f, 0.0f, 0.3f);
            }

            inline Color GetDefaultShimmerColor()
            {
                return Color(1.0f, 1.0f, 1.0f, 0.05f);
            }

            inline Color GetDefaultTextColor()
            {
                return Color(0.95f, 0.95f, 0.98f);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Preset Color Schemes
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            namespace Presets {

                // Blue theme (default)
                inline std::vector<D2D1_GRADIENT_STOP> BlueBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.15f, 0.25f, 0.45f) },
                        { 0.5f, D2D1::ColorF(0.12f, 0.20f, 0.38f) },
                        { 1.0f, D2D1::ColorF(0.08f, 0.15f, 0.30f) }
                    };
                }

                inline std::vector<D2D1_GRADIENT_STOP> BlueHoverBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.25f, 0.40f, 0.65f) },
                        { 0.5f, D2D1::ColorF(0.20f, 0.35f, 0.58f) },
                        { 1.0f, D2D1::ColorF(0.15f, 0.28f, 0.50f) }
                    };
                }

                // Green theme
                inline std::vector<D2D1_GRADIENT_STOP> GreenBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.15f, 0.35f, 0.20f) },
                        { 0.5f, D2D1::ColorF(0.12f, 0.28f, 0.16f) },
                        { 1.0f, D2D1::ColorF(0.08f, 0.20f, 0.12f) }
                    };
                }

                inline std::vector<D2D1_GRADIENT_STOP> GreenHoverBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.25f, 0.50f, 0.30f) },
                        { 0.5f, D2D1::ColorF(0.20f, 0.42f, 0.25f) },
                        { 1.0f, D2D1::ColorF(0.15f, 0.35f, 0.20f) }
                    };
                }

                // Red/Danger theme
                inline std::vector<D2D1_GRADIENT_STOP> RedBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.45f, 0.15f, 0.15f) },
                        { 0.5f, D2D1::ColorF(0.38f, 0.12f, 0.12f) },
                        { 1.0f, D2D1::ColorF(0.30f, 0.08f, 0.08f) }
                    };
                }

                inline std::vector<D2D1_GRADIENT_STOP> RedHoverBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.65f, 0.25f, 0.25f) },
                        { 0.5f, D2D1::ColorF(0.58f, 0.20f, 0.20f) },
                        { 1.0f, D2D1::ColorF(0.50f, 0.15f, 0.15f) }
                    };
                }

                // Purple theme
                inline std::vector<D2D1_GRADIENT_STOP> PurpleBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.35f, 0.15f, 0.45f) },
                        { 0.5f, D2D1::ColorF(0.28f, 0.12f, 0.38f) },
                        { 1.0f, D2D1::ColorF(0.20f, 0.08f, 0.30f) }
                    };
                }

                inline std::vector<D2D1_GRADIENT_STOP> PurpleHoverBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.50f, 0.25f, 0.65f) },
                        { 0.5f, D2D1::ColorF(0.42f, 0.20f, 0.58f) },
                        { 1.0f, D2D1::ColorF(0.35f, 0.15f, 0.50f) }
                    };
                }

                // Dark theme
                inline std::vector<D2D1_GRADIENT_STOP> DarkBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.15f, 0.15f, 0.15f) },
                        { 0.5f, D2D1::ColorF(0.10f, 0.10f, 0.10f) },
                        { 1.0f, D2D1::ColorF(0.05f, 0.05f, 0.05f) }
                    };
                }

                inline std::vector<D2D1_GRADIENT_STOP> DarkHoverBackgroundStops()
                {
                    return {
                        { 0.0f, D2D1::ColorF(0.25f, 0.25f, 0.25f) },
                        { 0.5f, D2D1::ColorF(0.18f, 0.18f, 0.18f) },
                        { 1.0f, D2D1::ColorF(0.12f, 0.12f, 0.12f) }
                    };
                }
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Animation Colors
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            inline Color GetPressedOverlayColor()
            {
                return Color(0.0f, 0.0f, 0.0f, 0.1f);
            }

            inline Color GetHighlightColor()
            {
                return Color(1.0f, 1.0f, 1.0f, 0.1f);
            }

            inline Color GetTextShadowColor()
            {
                return Color(0.0f, 0.0f, 0.0f, 0.3f);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // State-based Color Modifiers
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            inline float GetHoverBrightnessMultiplier()
            {
                return 1.15f;
            }

            inline float GetPressedBrightnessMultiplier()
            {
                return 0.85f;
            }

            inline float GetDisabledOpacity()
            {
                return 0.4f;
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // Visual Properties
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            namespace VisualProperties {
                constexpr float DefaultCornerRadius = 6.0f;
                constexpr float DefaultBorderWidth = 1.0f;
                constexpr float GlowStrokeWidth = 2.0f;
                constexpr float GlowPadding = 3.0f;
                constexpr float ShadowOffsetX = 0.5f;
                constexpr float ShadowOffsetY = 2.0f;
                constexpr float PressedOffsetY = 2.0f;

                // Animation speeds
                constexpr float HoverAnimationSpeed = 10.0f;
                constexpr float PressAnimationSpeed = 15.0f;
                constexpr float ShimmerSpeed = 0.5f;

                // Alpha values
                constexpr float BorderAlphaNormal = 0.15f;
                constexpr float BorderAlphaHover = 0.5f;
                constexpr float GlowAlphaMultiplier = 0.6f;
                constexpr float ShadowAlphaBase = 0.3f;
                constexpr float ShadowAlphaHover = 0.2f;
            }

        } // namespace ButtonColors
    } // namespace UI
} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_BUTTON_COLORS_H