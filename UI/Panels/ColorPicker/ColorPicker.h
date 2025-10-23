#ifndef SPECTRUM_CPP_COLOR_PICKER_H
#define SPECTRUM_CPP_COLOR_PICKER_H

#include "Common/Common.h"
#include "UI/Base/InteractiveWidget.h"
#include <functional>

namespace Spectrum
{
    class Canvas;

    class ColorPicker final : public InteractiveWidget
    {
    public:
        using ColorSelectedCallback = std::function<void(const Color&)>;

        ColorPicker(const Point& position, float radius);
        ~ColorPicker() noexcept override = default;

        ColorPicker(const ColorPicker&) = delete;
        ColorPicker& operator=(const ColorPicker&) = delete;
        ColorPicker(ColorPicker&&) = delete;
        ColorPicker& operator=(ColorPicker&&) = delete;

        bool Initialize(Canvas& canvas);
        void RecreateResources(Canvas& canvas);

        void Update(const Point& mousePos, bool isMouseDown, float deltaTime);
        void Draw(Canvas& canvas) const;

        void SetVisible(bool visible);
        void SetPosition(const Point& position);
        void OnResize(int width, int height);

        void SetOnColorSelectedCallback(ColorSelectedCallback cb);

        [[nodiscard]] bool IsVisible() const noexcept { return m_isVisible; }

    protected:
        void OnPress(const Point& mousePos) override;
        [[nodiscard]] bool HitTest(const Point& point) const noexcept override;

    private:
        bool m_isVisible;
        Color m_hoverColor;

        mutable wrl::ComPtr<ID2D1Bitmap> m_colorWheelBitmap;
        ColorSelectedCallback m_onColorSelected;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_COLOR_PICKER_H