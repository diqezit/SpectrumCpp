// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the ColorPicker, a UI component that provides a
// visual way to select colors from an HSV spectrum.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/ColorPicker/ColorPicker.h"
#include "UI/Panels/ColorPicker/ColorWheelGenerator.h"
#include "UI/Panels/ColorPicker/ColorWheelRenderer.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/RenderEngine.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"

namespace Spectrum {

    ColorPicker::ColorPicker(
        const Point& position,
        float radius
    ) :
        m_bounds(position.x, position.y, radius * 2.0f, radius * 2.0f),
        m_isVisible(true),
        m_isMouseOver(false),
        m_wasPressed(false),
        m_hoverAnimationProgress(0.0f),
        m_hoverColor(Color::White())
    {
    }

    bool ColorPicker::Initialize(Canvas& canvas) {
        return CreateD2DResource(canvas);
    }

    void ColorPicker::RecreateResources(Canvas& canvas) {
        m_colorWheelBitmap.Reset();
        CreateD2DResource(canvas);
    }

    void ColorPicker::Update(
        Point mousePos,
        bool isMouseDown,
        float deltaTime
    ) {
        if (!m_isVisible) {
            m_isMouseOver = false;
            m_hoverAnimationProgress = 0.0f;
            return;
        }

        m_isMouseOver = IsInHitbox(mousePos);

        const float targetProgress = m_isMouseOver ? 1.0f : 0.0f;
        constexpr float animationSpeed = 12.0f;
        m_hoverAnimationProgress = Utils::ExponentialDecay(
            m_hoverAnimationProgress,
            targetProgress,
            animationSpeed,
            deltaTime
        );

        if (m_isMouseOver) {
            m_hoverColor = CalculateColorFromPosition(
                static_cast<int>(mousePos.x),
                static_cast<int>(mousePos.y)
            );
            if (isMouseDown && !m_wasPressed) {
                if (m_onColorSelected) {
                    m_onColorSelected(m_hoverColor);
                }
            }
        }
        m_wasPressed = isMouseDown;
    }

    void ColorPicker::Draw(Canvas& canvas) const {
        DrawWithAlpha(canvas, 1.0f);
    }

    void ColorPicker::DrawWithAlpha(
        Canvas& canvas,
        float alpha
    ) const {
        if (!m_isVisible || alpha <= 0.0f) {
            return;
        }

        if (!m_colorWheelBitmap) {
            if (!const_cast<ColorPicker*>(this)->CreateD2DResource(canvas)) {
                return;
            }
        }

        ColorWheelRenderer::DrawWheel(
            canvas,
            m_colorWheelBitmap.Get(),
            m_bounds,
            alpha
        );

        const float animatedAlpha = m_hoverAnimationProgress * alpha;
        ColorWheelRenderer::DrawBorder(
            canvas,
            m_bounds,
            m_isMouseOver,
            animatedAlpha
        );

        if (m_isMouseOver && m_hoverAnimationProgress > 0.01f) {
            ColorWheelRenderer::DrawHoverPreview(
                canvas,
                m_bounds,
                m_hoverColor,
                animatedAlpha
            );
        }
    }

    void ColorPicker::SetVisible(bool visible) {
        m_isVisible = visible;
        if (!m_isVisible) {
            m_isMouseOver = false;
            m_hoverAnimationProgress = 0.0f;
        }
    }

    void ColorPicker::SetPosition(const Point& position) {
        m_bounds.x = position.x;
        m_bounds.y = position.y;
    }

    Point ColorPicker::GetCenter() const {
        return {
            m_bounds.x + m_bounds.width * 0.5f,
            m_bounds.y + m_bounds.height * 0.5f
        };
    }

    void ColorPicker::SetOnColorSelectedCallback(ColorSelectedCallback cb) {
        m_onColorSelected = std::move(cb);
    }

    bool ColorPicker::IsInHitbox(Point mousePos) const {
        const Point center = GetCenter();
        const float radius = m_bounds.width * 0.5f;
        const float dx = mousePos.x - center.x;
        const float dy = mousePos.y - center.y;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

    Color ColorPicker::CalculateColorFromPosition(
        int x,
        int y
    ) const {
        const Point center = GetCenter();
        const float radius = m_bounds.width * 0.5f;
        const float dx = static_cast<float>(x) - center.x;
        const float dy = static_cast<float>(y) - center.y;

        const float dist = std::sqrt(dx * dx + dy * dy);
        const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
        const float sat = Utils::Saturate(dist / radius);
        return Utils::HSVtoRGB({ hue, sat, 1.0f });
    }

    bool ColorPicker::CreateD2DResource(Canvas& canvas) {
        ID2D1HwndRenderTarget* rt = canvas.GetRenderTarget();
        if (!rt) {
            return false;
        }

        const int size = static_cast<int>(m_bounds.width);
        const float radius = size * 0.5f;

        auto bitmapData = ColorWheelGenerator::GenerateBitmapData(size, radius);

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
        }
        return SUCCEEDED(hr);
    }
}