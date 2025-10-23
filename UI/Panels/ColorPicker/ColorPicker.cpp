#include "UI/Panels/ColorPicker/ColorPicker.h"
#include "UI/Common/UILayout.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Helpers/Geometry/ColorHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include <cmath>
#include <vector>

namespace Spectrum
{
    using namespace Helpers::Math;
    using namespace Helpers::Color;

    namespace
    {
        inline Point GetCenter(const Rect& bounds)
        {
            return { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
        }

        inline float GetRadius(const Rect& bounds)
        {
            return bounds.width * 0.5f;
        }

        inline Color CalculateHSVColor(const Point& center, float radius, const Point& point)
        {
            const float dx = point.x - center.x;
            const float dy = point.y - center.y;
            const float dist = std::sqrt(dx * dx + dy * dy);
            const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
            const float sat = Saturate(dist / radius);
            return HSVtoRGB({ hue, sat, 1.0f });
        }
    }

    ColorPicker::ColorPicker(const Point& position, float radius)
        : InteractiveWidget(Rect{ position.x, position.y, radius * 2.0f, radius * 2.0f })
        , m_isVisible(true)
        , m_hoverColor(Color::White())
    {
        m_hoverAnimation.m_speed = 12.0f;
    }

    bool ColorPicker::Initialize(Canvas& canvas)
    {
        ID2D1RenderTarget* rt = canvas.GetRenderTarget();
        if (!rt)
            return false;

        const int size = static_cast<int>(m_bounds.width);
        const float radius = size * 0.5f;

        std::vector<uint32_t> pixels(static_cast<size_t>(size) * size, 0u);

        for (int y = 0; y < size; ++y)
        {
            const float fy = static_cast<float>(y) - radius;
            for (int x = 0; x < size; ++x)
            {
                const float fx = static_cast<float>(x) - radius;
                const float dist = std::sqrt(fx * fx + fy * fy);

                if (dist <= radius)
                {
                    const float hue = (std::atan2(fy, fx) / PI + 1.0f) * 0.5f;
                    const float sat = dist / radius;
                    const Color rgb = HSVtoRGB({ hue, sat, 1.0f });
                    pixels[static_cast<size_t>(y) * size + x] = ColorToARGB(rgb);
                }
            }
        }

        const D2D1_SIZE_U bmpSize = D2D1::SizeU(size, size);
        const D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        HRESULT hr = rt->CreateBitmap(
            bmpSize,
            pixels.data(),
            size * sizeof(uint32_t),
            &bmpProps,
            m_colorWheelBitmap.GetAddressOf()
        );

        if (FAILED(hr))
        {
            m_colorWheelBitmap.Reset();
            return false;
        }

        return true;
    }

    void ColorPicker::RecreateResources(Canvas& canvas)
    {
        m_colorWheelBitmap.Reset();
        Initialize(canvas);
    }

    void ColorPicker::Update(const Point& mousePos, bool isMouseDown, float deltaTime)
    {
        if (!m_isVisible)
        {
            m_isHovered = false;
            m_hoverAnimation.m_currentValue = 0.0f;
            m_hoverAnimation.m_targetValue = 0.0f;
            return;
        }

        UpdateInteraction(mousePos, isMouseDown);
        UpdateVisualStates(deltaTime);

        if (m_isHovered)
        {
            m_hoverColor = CalculateHSVColor(GetCenter(m_bounds), GetRadius(m_bounds), mousePos);
        }
    }

    void ColorPicker::Draw(Canvas& canvas) const
    {
        if (!m_isVisible)
            return;

        if (m_colorWheelBitmap)
        {
            ID2D1RenderTarget* rt = canvas.GetRenderTarget();
            if (rt)
            {
                const D2D1_RECT_F d2dRect = D2D1::RectF(
                    m_bounds.x, m_bounds.y, m_bounds.GetRight(), m_bounds.GetBottom()
                );
                rt->DrawBitmap(m_colorWheelBitmap.Get(), d2dRect, 1.0f);
            }
        }

        const Point center = GetCenter(m_bounds);
        const float radius = GetRadius(m_bounds);
        const float hoverProgress = m_hoverAnimation.m_currentValue;

        const float baseAlpha = m_isHovered ? 1.0f : 0.6f;
        const float borderAlpha = Lerp(0.3f, baseAlpha, hoverProgress);
        const float borderThickness = Lerp(1.0f, 2.0f, hoverProgress);

        canvas.DrawCircle(center, radius + 2.0f, Paint::Stroke(Color(0.5f, 0.5f, 0.5f, borderAlpha), borderThickness));

        if (m_isHovered && hoverProgress > 0.01f)
        {
            const float scale = EaseOutBack(hoverProgress);
            const float actualSize = 24.0f * scale;
            const Rect previewRect(
                m_bounds.x + radius - actualSize * 0.5f,
                m_bounds.y - actualSize - 4.0f,
                actualSize,
                actualSize
            );

            Color previewColor = m_hoverColor;
            previewColor.a *= hoverProgress;

            canvas.DrawRectangle(previewRect, Paint::Fill(previewColor));
            canvas.DrawRectangle(
                Rect(previewRect.x - 1.0f, previewRect.y - 1.0f, actualSize + 2.0f, actualSize + 2.0f),
                Paint::Stroke(Color(0.5f, 0.5f, 0.5f, hoverProgress), 1.0f)
            );
        }
    }

    void ColorPicker::SetVisible(bool visible)
    {
        m_isVisible = visible;
        if (!m_isVisible)
        {
            m_isHovered = false;
            m_hoverAnimation.m_currentValue = 0.0f;
            m_hoverAnimation.m_targetValue = 0.0f;
        }
    }

    void ColorPicker::SetPosition(const Point& position)
    {
        m_bounds.x = position.x;
        m_bounds.y = position.y;
    }

    void ColorPicker::OnResize(int width, int /*height*/)
    {
        SetPosition(UILayout::CalculateTopRightPosition(
            static_cast<float>(width),
            m_bounds.width,
            m_bounds.height
        ));
    }

    void ColorPicker::SetOnColorSelectedCallback(ColorSelectedCallback cb)
    {
        m_onColorSelected = std::move(cb);
    }

    void ColorPicker::OnPress(const Point& mousePos)
    {
        if (m_onColorSelected)
        {
            m_hoverColor = CalculateHSVColor(GetCenter(m_bounds), GetRadius(m_bounds), mousePos);
            m_onColorSelected(m_hoverColor);
        }
    }

    bool ColorPicker::HitTest(const Point& point) const noexcept
    {
        const Point center = GetCenter(m_bounds);
        const float radius = GetRadius(m_bounds);
        const float dx = point.x - center.x;
        const float dy = point.y - center.y;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

} // namespace Spectrum