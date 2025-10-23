#ifndef SPECTRUM_CPP_GRADIENT_RENDERER_H
#define SPECTRUM_CPP_GRADIENT_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the GradientRenderer class, responsible for creating and managing
// gradient brushes and rendering gradient-filled shapes. It abstracts the
// complexity of Direct2D gradient resources.
//
// Key responsibilities:
// - Linear gradient rendering
// - Radial gradient rendering  
// - Gradient stop collection management
// - Gradient transformation support
//
// Design notes:
// - All render methods are const (stateless rendering)
// - Gradient resources cached via ResourceCache
// - Uses ComPtr for safe resource lifetime management
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <vector>

namespace Spectrum {

    class ResourceCache;
    struct GradientStop;

    class GradientRenderer final : public IRenderComponent
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit GradientRenderer(Spectrum::ResourceCache* resourceCache);

        GradientRenderer(const GradientRenderer&) = delete;
        GradientRenderer& operator=(const GradientRenderer&) = delete;

        void OnRenderTargetChanged(const wrl::ComPtr<ID2D1RenderTarget>& renderTarget) override;
        void OnDeviceLost() override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Gradient Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawLinearGradient(
            const Rect& rect,
            const Point& startPoint,
            const Point& endPoint,
            const std::vector<GradientStop>& stops
        ) const;

        void DrawRadialGradient(
            const Point& center,
            float radiusX,
            float radiusY,
            const std::vector<GradientStop>& stops,
            const Point& gradientOrigin = { 0, 0 }
        ) const;

        void FillRectWithLinearGradient(
            const Rect& rect,
            const std::vector<GradientStop>& stops,
            float angle = 0.0f
        ) const;

        void FillCircleWithRadialGradient(
            const Point& center,
            float radius,
            const std::vector<GradientStop>& stops
        ) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Gradient Stop Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<D2D1_GRADIENT_STOP> ConvertGradientStops(
            const std::vector<GradientStop>& stops
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1GradientStopCollection> CreateStopCollection(
            const std::vector<D2D1_GRADIENT_STOP>& d2dStops
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Gradient Point Calculation (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Point CalculateGradientStart(
            const Rect& rect,
            float angle
        ) const noexcept;

        [[nodiscard]] Point CalculateGradientEnd(
            const Rect& rect,
            float angle
        ) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Brush Creation Helpers (SRP)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1LinearGradientBrush> CreateLinearBrush(
            const Point& start,
            const Point& end,
            ID2D1GradientStopCollection* stops
        ) const;

        [[nodiscard]] wrl::ComPtr<ID2D1RadialGradientBrush> CreateRadialBrush(
            const Point& center,
            const Point& origin,
            float radiusX,
            float radiusY,
            ID2D1GradientStopCollection* stops
        ) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;
        Spectrum::ResourceCache* m_resourceCache;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GRADIENT_RENDERER_H