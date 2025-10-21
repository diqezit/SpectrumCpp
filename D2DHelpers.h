// D2DHelpers.h
#ifndef SPECTRUM_CPP_D2D_HELPERS_H
#define SPECTRUM_CPP_D2D_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines centralized Direct2D utility functions and RAII wrappers for
// managing D2D render states. This eliminates code duplication across
// multiple renderer classes and ensures proper state restoration.
//
// Key components:
// - Type conversion helpers (Color -> D2D1_COLOR_F, Point -> D2D1_POINT_2F)
// - RAII wrappers for transforms, opacity layers, and clip rectangles
// - HRESULT checking with automatic logging
// - Mathematical constants (π, 2π, degree/radian conversion)
// - Parameter sanitization utilities
// - COM object creation helpers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include "Logger.h"
#include <algorithm>
#include <string_view>

// C++20 numbers library with fallback
#if __cpp_lib_math_constants >= 201907L
#include <numbers>
#endif

namespace Spectrum {
    namespace D2DHelpers {

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Mathematical Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if __cpp_lib_math_constants >= 201907L
        inline constexpr float kPi = std::numbers::pi_v<float>;
#else
        inline constexpr float kPi = 3.14159265358979323846f;
#endif

        inline constexpr float kTwoPi = kPi * 2.0f;
        inline constexpr float kHalfPi = kPi * 0.5f;
        inline constexpr float kDegToRad = kPi / 180.0f;
        inline constexpr float kRadToDeg = 180.0f / kPi;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Type Conversion Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] inline D2D1_COLOR_F ToD2DColor(const Color& c) noexcept
        {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }

        [[nodiscard]] inline D2D1_POINT_2F ToD2DPoint(const Point& p) noexcept
        {
            return D2D1::Point2F(p.x, p.y);
        }

        [[nodiscard]] inline D2D1_RECT_F ToD2DRect(const Rect& r) noexcept
        {
            return D2D1::RectF(r.x, r.y, r.GetRight(), r.GetBottom());
        }

        [[nodiscard]] inline D2D1_SIZE_F ToD2DSize(float width, float height) noexcept
        {
            return D2D1::SizeF(width, height);
        }

        [[nodiscard]] inline D2D1_SIZE_U ToD2DSizeU(UINT32 width, UINT32 height) noexcept
        {
            return D2D1::SizeU(width, height);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Angle Conversion Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] inline float DegreesToRadians(float degrees) noexcept
        {
            return degrees * kDegToRad;
        }

        [[nodiscard]] inline float RadiansToDegrees(float radians) noexcept
        {
            return radians * kRadToDeg;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Parameter Sanitization Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace Sanitize {

            [[nodiscard]] inline float PositiveFloat(float value, float defaultValue = 0.0f) noexcept
            {
                return (value > 0.0f) ? value : defaultValue;
            }

            [[nodiscard]] inline float NonNegativeFloat(float value) noexcept
            {
                return std::max(value, 0.0f);
            }

            [[nodiscard]] inline float NormalizedFloat(float value) noexcept
            {
                return std::clamp(value, 0.0f, 1.0f);
            }

            [[nodiscard]] inline int MinValue(int value, int minValue) noexcept
            {
                return std::max(value, minValue);
            }

            [[nodiscard]] inline int ClampValue(int value, int minValue, int maxValue) noexcept
            {
                return std::clamp(value, minValue, maxValue);
            }

            [[nodiscard]] inline float Radius(float value) noexcept
            {
                return PositiveFloat(value, 1.0f);
            }

            [[nodiscard]] inline int PolygonSides(int sides) noexcept
            {
                return std::max(sides, 3);
            }

            [[nodiscard]] inline int StarPoints(int points) noexcept
            {
                return std::max(points, 2);
            }

            [[nodiscard]] inline int CircleSegments(int segments) noexcept
            {
                return std::clamp(segments, 3, 360);
            }

        } // namespace Sanitize

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // HRESULT Checking Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace HResult {

            inline void Check(HRESULT hr, std::string_view operation)
            {
                if (FAILED(hr)) {
                    LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
                }
            }

            [[nodiscard]] inline bool CheckWithReturn(HRESULT hr, std::string_view operation)
            {
                if (FAILED(hr)) {
                    LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
                    return false;
                }
                return true;
            }

            template<typename T>
            [[nodiscard]] inline bool CheckComCreation(HRESULT hr, std::string_view operation, const wrl::ComPtr<T>& object)
            {
                if (FAILED(hr) || !object) {
                    LOG_ERROR(operation << " failed with HRESULT: 0x" << std::hex << static_cast<uint32_t>(hr));
                    return false;
                }
                return true;
            }

        } // namespace HResult

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace Validate {

            [[nodiscard]] inline bool PointArray(const std::vector<Point>& points, size_t minSize = 2) noexcept
            {
                return points.size() >= minSize;
            }

            [[nodiscard]] inline bool GradientStops(const std::vector<D2D1_GRADIENT_STOP>& stops) noexcept
            {
                return stops.size() >= 2;
            }

            [[nodiscard]] inline bool PositiveRadius(float radius) noexcept
            {
                return radius > 0.0f;
            }

            [[nodiscard]] inline bool RadiusRange(float innerRadius, float outerRadius) noexcept
            {
                return innerRadius >= 0.0f && innerRadius < outerRadius;
            }

            [[nodiscard]] inline bool NonZeroAngle(float angle) noexcept
            {
                return std::abs(angle) >= 0.01f;
            }

            [[nodiscard]] inline bool RenderTargetAndBrush(ID2D1RenderTarget* rt, ID2D1Brush* brush) noexcept
            {
                return rt != nullptr && brush != nullptr;
            }

        } // namespace Validate

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // COM Creation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        template<typename TInterface, typename TFactory>
        [[nodiscard]] inline wrl::ComPtr<TInterface> CreateComObject(
            TFactory* factory,
            HRESULT(STDMETHODCALLTYPE TFactory::* createMethod)(TInterface**),
            std::string_view operationName
        )
        {
            if (!factory) return nullptr;

            wrl::ComPtr<TInterface> object;
            const HRESULT hr = (factory->*createMethod)(object.GetAddressOf());

            if (!HResult::CheckComCreation(hr, operationName, object)) {
                return nullptr;
            }

            return object;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // RAII State Management Wrappers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        class ScopedTransform final
        {
        public:
            ScopedTransform(ID2D1RenderTarget* renderTarget, const D2D1_MATRIX_3X2_F& transform)
                : m_renderTarget(renderTarget)
                , m_oldTransform(D2D1::Matrix3x2F::Identity())
                , m_transformApplied(false)
            {
                if (!m_renderTarget) return;

                m_renderTarget->GetTransform(&m_oldTransform);
                m_renderTarget->SetTransform(transform * m_oldTransform);
                m_transformApplied = true;
            }

            ~ScopedTransform()
            {
                if (m_renderTarget && m_transformApplied) {
                    m_renderTarget->SetTransform(m_oldTransform);
                }
            }

            ScopedTransform(const ScopedTransform&) = delete;
            ScopedTransform& operator=(const ScopedTransform&) = delete;

        private:
            ID2D1RenderTarget* m_renderTarget;
            D2D1_MATRIX_3X2_F m_oldTransform;
            bool m_transformApplied;
        };

        class ScopedOpacityLayer final
        {
        public:
            ScopedOpacityLayer(ID2D1RenderTarget* renderTarget, float opacity)
                : m_renderTarget(renderTarget)
                , m_layerPushed(false)
            {
                if (!m_renderTarget) return;

                wrl::ComPtr<ID2D1Layer> layer;
                if (FAILED(m_renderTarget->CreateLayer(nullptr, layer.GetAddressOf()))) return;

                D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
                params.opacity = Sanitize::NormalizedFloat(opacity);

                m_renderTarget->PushLayer(&params, layer.Get());
                m_layerPushed = true;
            }

            ~ScopedOpacityLayer()
            {
                if (m_renderTarget && m_layerPushed) {
                    m_renderTarget->PopLayer();
                }
            }

            ScopedOpacityLayer(const ScopedOpacityLayer&) = delete;
            ScopedOpacityLayer& operator=(const ScopedOpacityLayer&) = delete;

        private:
            ID2D1RenderTarget* m_renderTarget;
            bool m_layerPushed;
        };

        class ScopedClipRect final
        {
        public:
            ScopedClipRect(ID2D1RenderTarget* renderTarget, const Rect& rect)
                : m_renderTarget(renderTarget)
                , m_clipPushed(false)
            {
                if (!m_renderTarget) return;

                m_renderTarget->PushAxisAlignedClip(
                    ToD2DRect(rect),
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                );
                m_clipPushed = true;
            }

            ~ScopedClipRect()
            {
                if (m_renderTarget && m_clipPushed) {
                    m_renderTarget->PopAxisAlignedClip();
                }
            }

            ScopedClipRect(const ScopedClipRect&) = delete;
            ScopedClipRect& operator=(const ScopedClipRect&) = delete;

        private:
            ID2D1RenderTarget* m_renderTarget;
            bool m_clipPushed;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Geometry Building Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        namespace Geometry {

            inline void BeginFigure(ID2D1GeometrySink* sink, const Point& startPoint, bool filled)
            {
                if (!sink) return;

                sink->BeginFigure(
                    ToD2DPoint(startPoint),
                    filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW
                );
            }

            inline void EndFigure(ID2D1GeometrySink* sink, bool closed)
            {
                if (!sink) return;

                sink->EndFigure(
                    closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN
                );
            }

            inline void AddLine(ID2D1GeometrySink* sink, const Point& point)
            {
                if (!sink) return;
                sink->AddLine(ToD2DPoint(point));
            }

        } // namespace Geometry

    } // namespace D2DHelpers
} // namespace Spectrum

#endif