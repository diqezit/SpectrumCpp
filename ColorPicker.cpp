// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Implements the ColorPicker, a UI component that provides a visual
// way to select colors from an HSV spectrum.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "ColorPicker.h"
#include "MathUtils.h"
#include "ColorUtils.h"

namespace Spectrum {

    namespace {
        uint32_t MakeWheelPixel(float dx, float dy, float radius) {
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) return 0u;

            const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
            const float sat = dist / radius;
            const Color rgb = Utils::HSVtoRGB({ hue, sat, 1.0f });
            return Utils::ColorToARGB(rgb);
        }

        D2D1_RECT_F MakeRect(const Point& pos, float r) {
            return D2D1::RectF(pos.x, pos.y, pos.x + r * 2.0f, pos.y + r * 2.0f);
        }

        Point Center(const Point& pos, float r) {
            return { pos.x + r, pos.y + r };
        }
    }

    ColorPicker::ColorPicker(const Point& position, float radius)
        : m_position(position)
        , m_radius(radius)
        , m_isVisible(true)
        , m_isMouseOver(false)
        , m_hoverColor(Color::White()) {
    }

    bool ColorPicker::Initialize(GraphicsContext& context) {
        return CreateColorWheelBitmap(context);
    }

    void ColorPicker::RecreateResources(GraphicsContext& context) {
        m_colorWheelBitmap.Reset();
        CreateColorWheelBitmap(context);
    }

    bool ColorPicker::EnsureColorWheelBitmap(GraphicsContext& context) {
        if (m_colorWheelBitmap) return true;
        return CreateColorWheelBitmap(context);
    }

    bool ColorPicker::CreateD2D1BitmapFromData(GraphicsContext& context, const std::vector<uint32_t>& data) {
        ID2D1HwndRenderTarget* rt = context.GetRenderTarget();
        if (!rt) return false;

        const int size = static_cast<int>(m_radius * 2.0f);
        const D2D1_SIZE_U bmpSize = D2D1::SizeU(size, size);
        const D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        HRESULT hr = rt->CreateBitmap(
            bmpSize, data.data(), size * sizeof(uint32_t), &bmpProps, m_colorWheelBitmap.GetAddressOf()
        );

        if (FAILED(hr)) m_colorWheelBitmap.Reset();
        return SUCCEEDED(hr);
    }

    bool ColorPicker::CreateColorWheelBitmap(GraphicsContext& context) {
        auto bitmapData = CreateBitmapData();
        return CreateD2D1BitmapFromData(context, bitmapData);
    }

    std::vector<uint32_t> ColorPicker::CreateBitmapData() {
        const int size = static_cast<int>(m_radius * 2.0f);
        std::vector<uint32_t> pixels(static_cast<size_t>(size) * size, 0u);

        for (int y = 0; y < size; ++y) {
            const float fy = static_cast<float>(y) - m_radius;
            for (int x = 0; x < size; ++x) {
                const float fx = static_cast<float>(x) - m_radius;
                pixels[static_cast<size_t>(y) * size + x] = MakeWheelPixel(fx, fy, m_radius);
            }
        }
        return pixels;
    }

    Color ColorPicker::GetBorderColor() const {
        return m_isMouseOver
            ? Color(0.5f, 0.5f, 0.5f, 1.0f)
            : Color(0.3f, 0.3f, 0.3f, 1.0f);
    }

    void ColorPicker::Draw(GraphicsContext& context) {
        if (!m_isVisible) return;
        if (!EnsureColorWheelBitmap(context)) return;

        const D2D1_RECT_F rect = MakeRect(m_position, m_radius);
        DrawWheel(context, rect);
        DrawBorder(context, rect);
        if (m_isMouseOver) DrawHoverPreview(context, rect);
    }

    void ColorPicker::DrawWheel(GraphicsContext& context, const D2D1_RECT_F& rect) {
        if (!m_colorWheelBitmap) return;
        context.GetRenderTarget()->DrawBitmap(m_colorWheelBitmap.Get(), rect);
    }

    void ColorPicker::DrawBorder(GraphicsContext& context, const D2D1_RECT_F& rect) {
        const Point c = Center(m_position, m_radius);
        context.DrawCircle(c, m_radius + 2.0f, GetBorderColor(), false);
    }

    void ColorPicker::DrawHoverPreview(GraphicsContext& context, const D2D1_RECT_F& rect) {
        constexpr float previewSize = 24.0f;
        const float x = rect.left + m_radius - previewSize * 0.5f;
        const float y = rect.top - previewSize - 4.0f;

        const Rect r(x, y, previewSize, previewSize);
        context.DrawRectangle(r, m_hoverColor, true);

        const Rect border(x - 1.0f, y - 1.0f, previewSize + 2.0f, previewSize + 2.0f);
        context.DrawRectangle(border, GetBorderColor(), false);
    }

    bool ColorPicker::IsPointInside(int x, int y) const {
        const Point c = Center(m_position, m_radius);
        const float dx = static_cast<float>(x) - c.x;
        const float dy = static_cast<float>(y) - c.y;
        return (dx * dx + dy * dy) <= (m_radius * m_radius);
    }

    Color ColorPicker::CalculateColorFromPosition(int x, int y) const {
        const Point c = Center(m_position, m_radius);
        const float dx = static_cast<float>(x) - c.x;
        const float dy = static_cast<float>(y) - c.y;

        const float dist = std::sqrt(dx * dx + dy * dy);
        const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
        const float sat = Utils::Saturate(dist / m_radius);

        return Utils::HSVtoRGB({ hue, sat, 1.0f });
    }

    void ColorPicker::UpdateHoverColor(int x, int y) {
        m_hoverColor = CalculateColorFromPosition(x, y);
    }

    bool ColorPicker::HandleMouseMove(int x, int y) {
        if (!m_isVisible) return false;
        m_isMouseOver = IsPointInside(x, y);
        if (m_isMouseOver) UpdateHoverColor(x, y);
        return m_isMouseOver;
    }

    void ColorPicker::InvokeColorSelectionCallback() {
        if (m_onColorSelected) m_onColorSelected(m_hoverColor);
    }

    bool ColorPicker::HandleMouseClick(int x, int y) {
        if (!m_isVisible || !IsPointInside(x, y)) return false;
        InvokeColorSelectionCallback();
        return true;
    }

} // namespace Spectrum