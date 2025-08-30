// GraphicsContext.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// GraphicsContext.h: A wrapper around Direct2D for 2D rendering operations.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_GRAPHICS_CONTEXT_H
#define SPECTRUM_CPP_GRAPHICS_CONTEXT_H

#include "Common.h"

namespace Spectrum {

    class GraphicsContext {
    public:
        explicit GraphicsContext(HWND hwnd);
        ~GraphicsContext();

        bool Initialize();
        void BeginDraw();
        HRESULT EndDraw();
        void Resize(int width, int height);
        void Clear(const Color& color);

        // Basic shapes
        void DrawRectangle(
            const Rect& rect,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );
        void DrawRoundedRectangle(
            const Rect& rect,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );
        void DrawCircle(
            const Point& center,
            float radius,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );
        void DrawEllipse(
            const Point& center,
            float radiusX,
            float radiusY,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );
        void DrawLine(
            const Point& start,
            const Point& end,
            const Color& color,
            float strokeWidth = 1.0f
        );

        // Complex shapes
        void DrawPolyline(
            const std::vector<Point>& points,
            const Color& color,
            float strokeWidth = 1.0f
        );
        void DrawPolygon(
            const std::vector<Point>& points,
            const Color& color,
            bool filled = true,
            float strokeWidth = 1.0f
        );
        void DrawBezier(
            const Point& start,
            const Point& control1,
            const Point& control2,
            const Point& end,
            const Color& color,
            float strokeWidth = 1.0f
        );

        // Gradients
        void DrawGradientRectangle(
            const Rect& rect,
            const Color& startColor,
            const Color& endColor,
            bool horizontal = true
        );
        void DrawRadialGradient(
            const Point& center,
            float radius,
            const Color& centerColor,
            const Color& edgeColor
        );

        // Text
        void DrawText(
            const std::wstring& text,
            const Point& position,
            const Color& color,
            float fontSize = 12.0f
        );

        // Transform operations
        void PushTransform(const D2D1_MATRIX_3X2_F& transform);
        void PopTransform();
        void SetTransform(const D2D1_MATRIX_3X2_F& transform);
        void ResetTransform();

        // Getters
        ID2D1HwndRenderTarget* GetRenderTarget() const noexcept {
            return m_renderTarget.Get();
        }
        ID2D1Factory* GetFactory() const noexcept {
            return m_d2dFactory.Get();
        }
        int GetWidth() const noexcept { return m_width; }
        int GetHeight() const noexcept { return m_height; }

    private:
        bool CreateDeviceResources();
        void DiscardDeviceResources();

        ID2D1SolidColorBrush* GetSolidBrush(const Color& color);

        ID2D1LinearGradientBrush* GetLinearGradientBrush(
            const Color& startColor,
            const Color& endColor,
            const D2D1_POINT_2F& start,
            const D2D1_POINT_2F& end
        );

        ID2D1RadialGradientBrush* GetRadialGradientBrush(
            const Color& centerColor,
            const Color& edgeColor,
            const D2D1_POINT_2F& center,
            float radius
        );

    private:
        HWND m_hwnd;
        int  m_width;
        int  m_height;

        wrl::ComPtr<ID2D1Factory>          m_d2dFactory;
        wrl::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
        wrl::ComPtr<ID2D1SolidColorBrush>  m_solidBrush;
        wrl::ComPtr<IDWriteFactory>        m_writeFactory;
        wrl::ComPtr<IDWriteTextFormat>     m_textFormat;

        // Cached last-created gradient brushes
        wrl::ComPtr<ID2D1LinearGradientBrush> m_linearBrush;
        wrl::ComPtr<ID2D1RadialGradientBrush> m_radialBrush;

        std::vector<D2D1_MATRIX_3X2_F> m_transformStack;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GRAPHICS_CONTEXT_H