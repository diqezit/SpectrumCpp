// ColorPicker.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// ColorPicker.cpp: Implementation of the ColorPicker class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "ColorPicker.h"
#include "Utils.h"

namespace Spectrum {

    namespace {
        inline uint32_t MakeWheelPixel(float dx, float dy, float radius) {
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius) return 0u;

            const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
            const float sat = dist / radius;
            const Color rgb = Utils::HSVtoRGB({ hue, sat, 1.0f });
            return Utils::ColorToARGB(rgb);
        }

        inline D2D1_COLOR_F ToD2D(const Color& c) {
            return D2D1::ColorF(c.r, c.g, c.b, c.a);
        }
    } // anonymous

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

    bool ColorPicker::CreateColorWheelBitmap(GraphicsContext& context) {
        ID2D1HwndRenderTarget* rt = context.GetRenderTarget();
        if (!rt) return false;

        const int size = static_cast<int>(m_radius * 2.0f);
        auto bitmapData = CreateBitmapData();

        const D2D1_SIZE_U bmpSize = D2D1::SizeU(size, size);
        const D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        HRESULT hr = rt->CreateBitmap(
            bmpSize,
            bitmapData.data(),
            size * sizeof(uint32_t),
            &bmpProps,
            m_colorWheelBitmap.GetAddressOf()
        );
        if (FAILED(hr)) {
            m_colorWheelBitmap.Reset();
            return false;
        }
        return true;
    }

    std::vector<uint32_t> ColorPicker::CreateBitmapData() {
        const int size = static_cast<int>(m_radius * 2.0f);
        std::vector<uint32_t> pixels(static_cast<size_t>(size) * size, 0u);

        for (int y = 0; y < size; ++y) {
            const float fy = static_cast<float>(y) - m_radius;
            for (int x = 0; x < size; ++x) {
                const float fx = static_cast<float>(x) - m_radius;
                pixels[static_cast<size_t>(y) * size + x] =
                    MakeWheelPixel(fx, fy, m_radius);
            }
        }
        return pixels;
    }

    void ColorPicker::Draw(GraphicsContext& context) {
        if (!m_isVisible) return;
        if (!EnsureColorWheelBitmap(context)) return;

        const D2D1_RECT_F rect = MakeRect(m_position, m_radius);
        DrawWheel(context, rect);

        const Color border = m_isMouseOver
            ? Color(0.5f, 0.5f, 0.5f, 1.0f)
            : Color(0.3f, 0.3f, 0.3f, 1.0f);

        DrawBorder(context, rect, border);

        if (m_isMouseOver) DrawHoverPreview(context, rect, border);
    }

    void ColorPicker::DrawWheel(GraphicsContext& context, const D2D1_RECT_F& rect) {
        if (!m_colorWheelBitmap) return;
        context.GetRenderTarget()->DrawBitmap(m_colorWheelBitmap.Get(), rect);
    }

    void ColorPicker::DrawBorder(GraphicsContext& context,
        const D2D1_RECT_F& rect,
        const Color& borderColor) {
        const Point c = Center(m_position, m_radius);
        context.DrawCircle(c, m_radius + 2.0f, borderColor, false);
    }

    void ColorPicker::DrawHoverPreview(GraphicsContext& context,
        const D2D1_RECT_F& rect,
        const Color& borderColor) {
        constexpr float previewSize = 24.0f;
        const float x = rect.left + m_radius - previewSize * 0.5f;
        const float y = rect.top - previewSize - 4.0f;

        const Rect r(x, y, previewSize, previewSize);
        context.DrawRectangle(r, m_hoverColor, true);

        const Rect border(x - 1.0f, y - 1.0f, previewSize + 2.0f, previewSize + 2.0f);
        context.DrawRectangle(border, borderColor, false);
    }

    bool ColorPicker::IsPointInside(int x, int y) const {
        const Point c = Center(m_position, m_radius);
        const float dx = static_cast<float>(x) - c.x;
        const float dy = static_cast<float>(y) - c.y;
        return (dx * dx + dy * dy) <= (m_radius * m_radius);
    }

    void ColorPicker::UpdateHoverColor(int x, int y) {
        const Point c = Center(m_position, m_radius);
        const float dx = static_cast<float>(x) - c.x;
        const float dy = static_cast<float>(y) - c.y;

        const float dist = std::sqrt(dx * dx + dy * dy);
        const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
        const float sat = Utils::Clamp(dist / m_radius, 0.0f, 1.0f);

        m_hoverColor = Utils::HSVtoRGB({ hue, sat, 1.0f });
    }

    bool ColorPicker::HandleMouseMove(int x, int y) {
        if (!m_isVisible) return false;
        m_isMouseOver = IsPointInside(x, y);
        if (m_isMouseOver) UpdateHoverColor(x, y);
        return m_isMouseOver;
    }

    bool ColorPicker::HandleMouseClick(int x, int y) {
        if (!m_isVisible || !m_isMouseOver) return false;
        if (m_onColorSelected) m_onColorSelected(m_hoverColor);
        return true;
    }

    D2D1_RECT_F ColorPicker::MakeRect(const Point& pos, float r) {
        return D2D1::RectF(pos.x, pos.y, pos.x + r * 2.0f, pos.y + r * 2.0f);
    }

    Point ColorPicker::Center(const Point& pos, float r) {
        return { pos.x + r, pos.y + r };
    }

} // namespace Spectrum