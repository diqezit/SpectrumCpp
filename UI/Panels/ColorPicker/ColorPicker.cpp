// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the ColorPicker, a UI component that provides a
// visual way to select colors from an HSV spectrum.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Panels/ColorPicker/ColorPicker.h"
#include "UI/Panels/ColorPicker/ColorWheelGenerator.h"
#include "UI/Panels/ColorPicker/ColorWheelRenderer.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/RenderEngine.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Helpers/Geometry/ColorHelpers.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Anonymous namespace for internal constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr float kHoverAnimationSpeed = 12.0f;
        constexpr float kMinVisibleAlpha = 0.01f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ColorPicker::Update(
        Point mousePos,
        bool isMouseDown,
        float deltaTime
    ) {
        UpdateVisibility();

        if (!m_isVisible) {
            return;
        }

        UpdateHoverState(mousePos);
        UpdateHoverAnimation(deltaTime);

        if (m_isMouseOver) {
            UpdateHoverColor(mousePos);
        }

        HandleMouseClick(isMouseDown);
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

        EnsureResourcesCreated(canvas);

        DrawColorWheel(canvas, alpha);
        DrawBorderIfNeeded(canvas, alpha);
        DrawHoverPreviewIfNeeded(canvas, alpha);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ColorPicker::UpdateVisibility() {
        if (!m_isVisible) {
            m_isMouseOver = false;
            m_hoverAnimationProgress = 0.0f;
        }
    }

    void ColorPicker::UpdateHoverState(Point mousePos) {
        m_isMouseOver = IsInHitbox(mousePos);
    }

    void ColorPicker::UpdateHoverAnimation(float deltaTime) {
        const float targetProgress = CalculateTargetHoverProgress();

        m_hoverAnimationProgress = Helpers::Math::ExponentialDecay(
            m_hoverAnimationProgress,
            targetProgress,
            kHoverAnimationSpeed,
            deltaTime
        );
    }

    void ColorPicker::UpdateHoverColor(Point mousePos) {
        m_hoverColor = CalculateColorFromPosition(
            static_cast<int>(mousePos.x),
            static_cast<int>(mousePos.y)
        );
    }

    void ColorPicker::HandleMouseClick(bool isMouseDown) {
        if (m_isMouseOver && isMouseDown && !m_wasPressed) {
            if (m_onColorSelected) {
                m_onColorSelected(m_hoverColor);
            }
        }
        m_wasPressed = isMouseDown;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Drawing Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ColorPicker::EnsureResourcesCreated(Canvas& canvas) const {
        if (!m_colorWheelBitmap) {
            const_cast<ColorPicker*>(this)->CreateD2DResource(canvas);
        }
    }

    void ColorPicker::DrawColorWheel(Canvas& canvas, float alpha) const {
        if (!m_colorWheelBitmap) {
            return;
        }

        ColorWheelRenderer::DrawWheel(
            canvas,
            m_colorWheelBitmap.Get(),
            m_bounds,
            alpha
        );
    }

    void ColorPicker::DrawBorderIfNeeded(Canvas& canvas, float alpha) const {
        const float animatedAlpha = CalculateAnimatedAlpha(alpha);

        ColorWheelRenderer::DrawBorder(
            canvas,
            m_bounds,
            m_isMouseOver,
            animatedAlpha
        );
    }

    void ColorPicker::DrawHoverPreviewIfNeeded(Canvas& canvas, float alpha) const {
        if (!m_isMouseOver || m_hoverAnimationProgress <= kMinVisibleAlpha) {
            return;
        }

        const float animatedAlpha = CalculateAnimatedAlpha(alpha);

        ColorWheelRenderer::DrawHoverPreview(
            canvas,
            m_bounds,
            m_hoverColor,
            animatedAlpha
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool ColorPicker::IsInHitbox(Point mousePos) const {
        const Point center = GetCenter();
        const float radius = m_bounds.width * 0.5f;
        const float dx = mousePos.x - center.x;
        const float dy = mousePos.y - center.y;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

    [[nodiscard]] Color ColorPicker::CalculateColorFromPosition(
        int x,
        int y
    ) const {
        const Point center = GetCenter();
        const float radius = m_bounds.width * 0.5f;
        const float dx = static_cast<float>(x) - center.x;
        const float dy = static_cast<float>(y) - center.y;

        const float dist = std::sqrt(dx * dx + dy * dy);
        const float hue = (std::atan2(dy, dx) / PI + 1.0f) * 0.5f;
        const float sat = Helpers::Math::Saturate(dist / radius);
        return Helpers::Color::HSVtoRGB({ hue, sat, 1.0f });
    }

    [[nodiscard]] float ColorPicker::CalculateTargetHoverProgress() const {
        return m_isMouseOver ? 1.0f : 0.0f;
    }

    [[nodiscard]] float ColorPicker::CalculateAnimatedAlpha(float baseAlpha) const {
        return m_hoverAnimationProgress * baseAlpha;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool ColorPicker::CreateD2DResource(Canvas& canvas) {
        ID2D1RenderTarget* rt = canvas.GetRenderTarget();
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

} // namespace Spectrum