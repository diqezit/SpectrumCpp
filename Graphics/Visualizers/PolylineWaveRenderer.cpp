#include "Graphics/Visualizers/PolylineWaveRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

    namespace {
        constexpr float kRadiusFactor = 0.7f;
        constexpr float kCoreRadiusMin = 0.08f;
        constexpr float kCoreRadiusMax = 0.15f;
        constexpr float kCoreRadiusSmoothing = 0.1f;
        constexpr float kBarLengthScale = 0.6f;
        constexpr float kMinBarWidth = 2.0f;
        constexpr float kMaxBarWidth = 15.0f;
        constexpr float kMinMagnitude = 0.02f;
    }

    PolylineWaveRenderer::PolylineWaveRenderer()
        : m_currentCoreRadius(0.0f)
        , m_targetCoreRadius(0.0f)
    {
        UpdateSettings();
    }

    void PolylineWaveRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
        m_currentCoreRadius = kCoreRadiusMin;
        m_targetCoreRadius = kCoreRadiusMin;
    }

    void PolylineWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        if (m_settings.useFill && !spectrum.empty()) {
            float sum = 0.0f;
            for (float magnitude : spectrum) {
                sum += magnitude;
            }

            const float avgIntensity = Saturate(sum / spectrum.size());
            m_targetCoreRadius = Lerp(kCoreRadiusMin, kCoreRadiusMax, avgIntensity);

            const float smoothingFactor = Clamp(
                kCoreRadiusSmoothing * deltaTime * 60.0f,
                0.0f,
                1.0f
            );

            m_currentCoreRadius = Lerp(
                m_currentCoreRadius,
                m_targetCoreRadius,
                smoothingFactor
            );
        }
    }

    void PolylineWaveRenderer::DoRender(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        if (spectrum.empty()) return;

        EnsureBarDirections(spectrum.size());

        const Point center = GetViewportCenter();
        const float baseRadius = GetMinDimension() * 0.5f * kRadiusFactor;
        const float circumference = TWO_PI * baseRadius;
        const float barWidth = Clamp(
            circumference / spectrum.size() * 0.75f,
            kMinBarWidth,
            kMaxBarWidth
        );

        if (m_settings.useFill) {
            const float coreRadius = baseRadius * m_currentCoreRadius;
            canvas.DrawCircle(
                center,
                coreRadius,
                Paint::Fill(AdjustAlpha(GetPrimaryColor(), 0.3f))
            );
        }

        RenderBars(canvas, spectrum, center, baseRadius, barWidth);
    }

    void PolylineWaveRenderer::EnsureBarDirections(size_t barCount) {
        if (m_barDirections.size() != barCount) {
            m_barDirections = GetCircularPoints(
                { 0.0f, 0.0f },
                1.0f,
                barCount
            );
        }
    }

    void PolylineWaveRenderer::RenderBars(
        Canvas& canvas,
        const SpectrumData& spectrum,
        const Point& center,
        float baseRadius,
        float barWidth
    ) const {
        const Paint paint = Paint::Stroke(GetPrimaryColor(), barWidth)
            .WithStrokeCap(StrokeCap::Round);

        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (spectrum[i] < kMinMagnitude) continue;

            const float barLength = CalculateBarLength(spectrum[i], baseRadius);
            const Point start = Add(
                center,
                Multiply(m_barDirections[i], baseRadius)
            );
            const Point end = Add(
                center,
                Multiply(m_barDirections[i], baseRadius + barLength)
            );

            canvas.DrawLine(start, end, paint);
        }
    }

    float PolylineWaveRenderer::CalculateBarLength(
        float magnitude,
        float radius
    ) const {
        const float normalizedLength = magnitude * radius * kBarLengthScale;
        const float minLength = radius * 0.05f;
        return std::max(normalizedLength, minLength);
    }

} // namespace Spectrum