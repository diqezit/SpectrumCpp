// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the ColorPicker, a UI component for selecting a color
// from an HSV wheel. It handles its own drawing via Canvas and
// mouse interaction for color selection.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_COLOR_PICKER_H
#define SPECTRUM_CPP_COLOR_PICKER_H

#include "Common/Common.h"
#include <functional>

namespace Spectrum {

    class Canvas;

    class ColorPicker {
    public:
        using ColorSelectedCallback = std::function<void(const Color&)>;

        ColorPicker(
            const Point& position,
            float radius
        );
        ~ColorPicker() = default;

        bool Initialize(Canvas& canvas);
        void RecreateResources(Canvas& canvas);

        void Update(
            Point mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void Draw(Canvas& canvas) const;

        void DrawWithAlpha(
            Canvas& canvas,
            float alpha
        ) const;

        void SetVisible(bool visible);
        void SetPosition(const Point& position);

        bool IsVisible() const { return m_isVisible; }
        Point GetCenter() const;

        void SetOnColorSelectedCallback(ColorSelectedCallback cb);

    private:
        bool IsInHitbox(Point mousePos) const;
        Color CalculateColorFromPosition(int x, int y) const;
        bool CreateD2DResource(Canvas& canvas);

        Rect m_bounds;
        bool m_isVisible;
        bool m_isMouseOver;
        bool m_wasPressed;
        float m_hoverAnimationProgress;
        Color m_hoverColor;

        wrl::ComPtr<ID2D1Bitmap> m_colorWheelBitmap;
        ColorSelectedCallback m_onColorSelected;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_COLOR_PICKER_H