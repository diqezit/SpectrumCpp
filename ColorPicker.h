// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the ColorPicker, a UI component for selecting a color
// from an HSV wheel. It handles its own drawing via GraphicsContext and
// mouse interaction for color selection.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_COLOR_PICKER_H
#define SPECTRUM_CPP_COLOR_PICKER_H

#include "Common.h"
#include "GraphicsContext.h"
#include <functional>

namespace Spectrum {

    class ColorPicker {
    public:
        using ColorSelectedCallback = std::function<void(const Color&)>;

        ColorPicker(const Point& position, float radius);
        ~ColorPicker() = default;

        bool Initialize(GraphicsContext& context);
        void RecreateResources(GraphicsContext& context);

        void Update(Point mousePos, bool isMouseDown);
        void Draw(GraphicsContext& context) const;

        void SetVisible(bool visible);
        bool IsVisible() const { return m_isVisible; }

        void SetOnColorSelectedCallback(ColorSelectedCallback cb);

    private:
        bool IsInHitbox(Point mousePos) const;
        Color CalculateColorFromPosition(int x, int y) const;
        bool CreateD2DResource(GraphicsContext& context);

        Rect m_bounds;
        bool m_isVisible;
        bool m_isMouseOver;
        bool m_wasPressed;
        Color m_hoverColor;

        wrl::ComPtr<ID2D1Bitmap> m_colorWheelBitmap;
        ColorSelectedCallback m_onColorSelected;
    };

}

#endif