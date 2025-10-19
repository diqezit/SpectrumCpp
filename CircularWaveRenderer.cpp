// Implements the CircularWaveRenderer using GraphicsContext helpers

#include "CircularWaveRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include "RenderUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation & Appearance Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr float CENTER_RADIUS = 30.0f;
        constexpr float MAX_RADIUS_FACTOR = 0.45f;
        constexpr float MIN_STROKE = 1.5f;
        constexpr float STROKE_CLAMP_FACTOR = 6.0f;
        constexpr float WAVE_INFLUENCE = 1.0f;
        constexpr float WAVE_PHASE_OFFSET = 0.1f;
        constexpr float ROTATION_INTENSITY_FACTOR = 0.3f;
        constexpr float MIN_MAGNITUDE_THRESHOLD = 0.01f;
        constexpr float GLOW_THRESHOLD = 0.5f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor & Settings
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    CircularWaveRenderer::CircularWaveRenderer() :
        m_angle(0.0f),
        m_waveTime(0.0f)
    {
        m_primaryColor = Color::FromRGB(0, 150, 255);
        UpdateSettings();
    }

    void CircularWaveRenderer::UpdateSettings() {
        if (m_isOverlay) {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = { false, 4.f, 12, 0.4f, 1.5f };
                break;
            case RenderQuality::High:
                m_settings = { true, 6.f, 20, 0.4f, 1.5f };
                break;
            default: // Medium
                m_settings = { true, 5.f, 16, 0.4f, 1.5f };
                break;
            }
        }
        else {
            switch (m_quality) {
            case RenderQuality::Low:
                m_settings = { false, 6.f, 16, 0.5f, 2.0f };
                break;
            case RenderQuality::High:
                m_settings = { true, 8.f, 32, 0.5f, 2.0f };
                break;
            default: // Medium
                m_settings = { true, 7.f, 24, 0.5f, 2.0f };
                break;
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Core Animation & Render Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float deltaTime
    ) {
        float avgIntensity = RenderUtils::GetAverageMagnitude(spectrum);

        // make rotation speed react to music intensity
        m_angle += m_settings.rotationSpeed
            * (1.0f + avgIntensity * ROTATION_INTENSITY_FACTOR)
            * deltaTime;
        if (m_angle > TWO_PI) {
            m_angle -= TWO_PI; // prevent angle from growing indefinitely
        }

        m_waveTime += m_settings.waveSpeed * deltaTime;
    }

    void CircularWaveRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& spectrum
    ) {
        const Point center = { m_width * 0.5f, m_height * 0.5f };
        const float maxRadius = std::min(m_width, m_height) * MAX_RADIUS_FACTOR;

        int ringCount = std::min(
            static_cast<int>(spectrum.size()),
            m_settings.maxRings
        );
        if (ringCount == 0) return;

        float ringStep = (maxRadius - CENTER_RADIUS) / ringCount;

        // render rings from back to front for correct alpha blending
        for (int i = ringCount - 1; i >= 0; i--) {
            float magnitude = GetRingMagnitude(spectrum, i, ringCount);
            // skip rendering invisible or barely visible rings for performance
            if (magnitude < MIN_MAGNITUDE_THRESHOLD) continue;

            float radius = CalculateRingRadius(i, ringStep, magnitude);
            // keep rings within the visible and calculated bounds
            if (radius <= 0 || radius > maxRadius) continue;

            float distanceFactor = 1.0f - radius / maxRadius;

            RenderCalculatedRing(context, center, radius, magnitude, distanceFactor);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Single Ring Rendering Steps (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void CircularWaveRenderer::RenderCalculatedRing(
        GraphicsContext& context,
        const Point& center,
        float radius,
        float magnitude,
        float distanceFactor
    ) {
        float strokeWidth = CalculateStrokeWidth(magnitude);
        Color ringColor = CalculateRingColor(magnitude, distanceFactor);

        // draw glow first so the main ring appears on top
        RenderGlowForRing(context, center, radius, strokeWidth, magnitude, ringColor);
        RenderMainRing(context, center, radius, strokeWidth, ringColor);
    }

    void CircularWaveRenderer::RenderGlowForRing(
        GraphicsContext& context,
        const Point& center,
        float radius,
        float strokeWidth,
        float magnitude,
        const Color& baseColor
    ) {
        // use glow only on high-energy rings to avoid visual clutter
        if (!m_settings.useGlow || magnitude <= GLOW_THRESHOLD) return;

        Color glowColor = baseColor;
        glowColor.a *= 0.5f; // glow should be less intense than the ring itself

        // glow radius must be larger than ring to be visible behind it
        float glowRadius = radius + strokeWidth;
        context.DrawGlow(center, glowRadius, glowColor, magnitude * 0.8f);
    }

    void CircularWaveRenderer::RenderMainRing(
        GraphicsContext& context,
        const Point& center,
        float radius,
        float strokeWidth,
        const Color& color
    ) {
        float innerRadius = radius - strokeWidth / 2.0f;
        float outerRadius = radius + strokeWidth / 2.0f;

        // ensure radii are valid to prevent rendering artifacts
        if (innerRadius < outerRadius) {
            context.DrawRing(center, innerRadius, outerRadius, color);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    Color CircularWaveRenderer::CalculateRingColor(
        float magnitude,
        float distanceFactor
    ) const {
        // fade out rings as they move away from center
        float alpha = Utils::Saturate(magnitude * 1.5f * distanceFactor);
        Color ringColor = m_primaryColor;
        ringColor.a = alpha;
        return ringColor;
    }

    float CircularWaveRenderer::CalculateRingRadius(
        int index,
        float ringStep,
        float magnitude
    ) const {
        float baseRadius = CENTER_RADIUS + index * ringStep;
        // add sine wave offset for a fluid, dynamic motion
        float waveOffset = std::sin(
            m_waveTime + index * WAVE_PHASE_OFFSET + m_angle
        ) * magnitude * ringStep * WAVE_INFLUENCE;

        return baseRadius + waveOffset;
    }

    float CircularWaveRenderer::CalculateStrokeWidth(float magnitude) const {
        return Utils::Clamp(
            MIN_STROKE + magnitude * STROKE_CLAMP_FACTOR,
            MIN_STROKE,
            m_settings.maxStroke
        );
    }

    float CircularWaveRenderer::GetRingMagnitude(
        const SpectrumData& spectrum,
        int ringIndex,
        int ringCount
    ) {
        if (spectrum.empty() || ringCount == 0) return 0.0f;

        size_t start = static_cast<size_t>(ringIndex) * spectrum.size() / ringCount;
        size_t end = std::min(
            (static_cast<size_t>(ringIndex) + 1) * spectrum.size() / ringCount,
            spectrum.size()
        );

        if (start >= end) return 0.0f;

        float sum = 0.0f;
        for (size_t i = start; i < end; ++i) {
            sum += spectrum[i];
        }
        return sum / static_cast<float>(end - start);
    }
}