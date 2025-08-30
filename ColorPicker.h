// ColorPicker.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// ColorPicker.h: A UI component for selecting a color from an HSV wheel.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_COLOR_PICKER_H
#define SPECTRUM_CPP_COLOR_PICKER_H

#include "Common.h"
#include "GraphicsContext.h"

namespace Spectrum {

    class ColorPicker {
    public:
        using ColorSelectedCallback = std::function<void(const Color&)>;

        ColorPicker(const Point& position, float radius);
        ~ColorPicker() = default;

        bool Initialize(GraphicsContext& context);
        void RecreateResources(GraphicsContext& context);

        void Draw(GraphicsContext& context);
        bool HandleMouseMove(int x, int y);
        bool HandleMouseClick(int x, int y);

        void SetVisible(bool visible) noexcept { m_isVisible = visible; }
        bool IsVisible() const noexcept { return m_isVisible; }
        bool IsMouseOver() const noexcept { return m_isMouseOver; }

        void SetOnColorSelectedCallback(ColorSelectedCallback cb) {
            m_onColorSelected = std::move(cb);
        }

    private:
        // Resources
        bool EnsureColorWheelBitmap(GraphicsContext& context);
        bool CreateColorWheelBitmap(GraphicsContext& context);
        std::vector<uint32_t> CreateBitmapData();

        // Drawing
        void DrawWheel(GraphicsContext& context, const D2D1_RECT_F& rect);
        void DrawBorder(GraphicsContext& context, const D2D1_RECT_F& rect, const Color& borderColor);
        void DrawHoverPreview(GraphicsContext& context, const D2D1_RECT_F& rect, const Color& borderColor);

        // Interaction
        void UpdateHoverColor(int x, int y);
        bool IsPointInside(int x, int y) const;

        // Helpers
        static D2D1_RECT_F MakeRect(const Point& pos, float r);
        static Point Center(const Point& pos, float r);

    private:
        Point m_position;
        float m_radius;
        bool  m_isVisible;
        bool  m_isMouseOver;
        Color m_hoverColor;

        wrl::ComPtr<ID2D1Bitmap> m_colorWheelBitmap;
        ColorSelectedCallback     m_onColorSelected;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_COLOR_PICKER_H