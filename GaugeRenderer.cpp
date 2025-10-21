// GaugeRenderer.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the GaugeRenderer for vintage VU meter visualization.
//
// Implementation details:
// - RMS loudness calculation matches perceived volume
// - Asymmetric smoothing for realistic needle ballistics
// - Peak lamp triggers at +3dB with hold time
// - Scale divisions follow VU meter standards
// - Quality settings control smoothing and visual effects
//
// Rendering pipeline:
// 1. Background: bezel, ring, meter face with label
// 2. Scale: major/minor ticks with dB labels
// 3. Needle: triangle with shadow and metallic pivot
// 4. Peak indicator: jeweled lamp with glow effect
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GaugeRenderer.h"
#include "D2DHelpers.h"
#include "MathUtils.h"
#include "RenderUtils.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace D2DHelpers;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr float kDbMax = 5.0f;
        constexpr float kDbMin = -30.0f;
        constexpr float kDbPeakThreshold = 3.0f;

        constexpr float kAngleStart = -150.0f;
        constexpr float kAngleEnd = -30.0f;
        constexpr float kAngleRange = kAngleEnd - kAngleStart;

        constexpr int kPeakHoldDuration = 15;

        constexpr float kBezelPadding = 4.0f;
        constexpr float kInnerPadding = 4.0f;
        constexpr float kBezelRadius = 8.0f;
        constexpr float kInnerRadius = 6.0f;

        constexpr float kShadowOffsetX = 2.0f;
        constexpr float kShadowOffsetY = 2.0f;
        constexpr float kShadowAlpha = 0.3f;

        constexpr float kNeedleBaseWidth = 2.5f;

        struct MajorMark { float db; const wchar_t* label; };
        constexpr MajorMark kMajorMarks[] = {
            {-30.0f, L"-30"}, {-20.0f, L"-20"}, {-10.0f, L"-10"},
            {-7.0f, L"-7"}, {-5.0f, L"-5"}, {-3.0f, L"-3"},
            {0.0f, L"0"}, {3.0f, L"+3"}, {5.0f, L"+5"}
        };

        constexpr float kMinorMarks[] = {
            -25.0f, -15.0f, -12.5f, -9.0f, -8.0f,
            -6.0f, -4.0f, -2.0f, -1.0f, 1.0f, 2.0f, 4.0f
        };

    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    GaugeRenderer::GaugeRenderer()
        : m_currentDbValue(kDbMin)
        , m_currentNeedleAngle(kAngleStart)
        , m_peakHoldCounter(0)
        , m_peakActive(false)
        , m_smoothingFactorInc(0.2f)
        , m_smoothingFactorDec(0.05f)
        , m_riseSpeed(0.15f)
    {
        m_aspectRatio = 2.0f;
        m_padding = 0.8f;
        UpdateSettings();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // BaseRenderer Overrides
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::UpdateSettings()
    {
        switch (m_quality) {
        case RenderQuality::Low:
            m_smoothingFactorInc = 0.25f;
            m_smoothingFactorDec = 0.06f;
            m_riseSpeed = 0.12f;
            break;
        case RenderQuality::High:
            m_smoothingFactorInc = 0.15f;
            m_smoothingFactorDec = 0.04f;
            m_riseSpeed = 0.20f;
            break;
        case RenderQuality::Medium:
        default:
            m_smoothingFactorInc = 0.20f;
            m_smoothingFactorDec = 0.05f;
            m_riseSpeed = 0.15f;
            break;
        }
    }

    void GaugeRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float /*deltaTime*/
    )
    {
        const float targetDb = CalculateLoudness(spectrum);

        const float smoothing = (targetDb > m_currentDbValue)
            ? m_smoothingFactorInc
            : m_smoothingFactorDec;

        const float adjustedSmoothing = m_isOverlay ? smoothing * 0.5f : smoothing;

        m_currentDbValue = Utils::Lerp(m_currentDbValue, targetDb, adjustedSmoothing);

        const float targetAngle = DbToAngle(m_currentDbValue);
        m_currentNeedleAngle = Utils::Lerp(m_currentNeedleAngle, targetAngle, m_riseSpeed);

        if (targetDb >= kDbPeakThreshold) {
            m_peakActive = true;
            m_peakHoldCounter = kPeakHoldDuration;
        }
        else if (m_peakHoldCounter > 0) {
            --m_peakHoldCounter;
        }
        else {
            m_peakActive = false;
        }
    }

    void GaugeRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& /*spectrum*/
    )
    {
        const Rect gaugeRect = CalculatePaddedRect();

        if (gaugeRect.width <= 0.0f || gaugeRect.height <= 0.0f) return;

        DrawBackground(context, gaugeRect);
        DrawScale(context, gaugeRect);
        DrawPeakIndicator(context, gaugeRect);
        DrawNeedle(context, gaugeRect);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Drawing Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawBackground(
        GraphicsContext& context,
        const Rect& rect
    ) const
    {
        context.DrawRoundedRectangle(
            rect,
            kBezelRadius,
            Color::FromRGB(80, 80, 80),
            true
        );

        const Rect innerRect{
            rect.x + kBezelPadding,
            rect.y + kBezelPadding,
            rect.width - kBezelPadding * 2.0f,
            rect.height - kBezelPadding * 2.0f
        };

        context.DrawRoundedRectangle(
            innerRect,
            kInnerRadius,
            Color::FromRGB(105, 105, 105),
            true
        );

        const Rect bgRect{
            innerRect.x + kInnerPadding,
            innerRect.y + kInnerPadding,
            innerRect.width - kInnerPadding * 2.0f,
            innerRect.height - kInnerPadding * 2.0f
        };

        context.DrawRectangle(bgRect, Color::FromRGB(240, 240, 230), true);

        const float vuTextSize = rect.height * 0.15f;
        const Point textPos{
            bgRect.x + bgRect.width * 0.5f,
            bgRect.GetBottom() - vuTextSize * 1.5f
        };

        context.DrawText(
            L"VU",
            textPos,
            Color::Black(),
            vuTextSize,
            DWRITE_TEXT_ALIGNMENT_CENTER
        );
    }

    void GaugeRenderer::DrawScale(
        GraphicsContext& context,
        const Rect& rect
    ) const
    {
        const Point center = GetScaleCenter(rect);
        const float radiusX = rect.width * (m_isOverlay ? 0.4f : 0.45f);
        const float radiusY = rect.height * (m_isOverlay ? 0.45f : 0.5f);

        for (const auto& mark : kMajorMarks) {
            DrawMajorTick(context, center, radiusX, radiusY, mark.db, mark.label);
        }

        for (const auto& db : kMinorMarks) {
            DrawMinorTick(context, center, radiusX, radiusY, db);
        }
    }

    void GaugeRenderer::DrawNeedle(
        GraphicsContext& context,
        const Rect& rect
    ) const
    {
        const Point center = GetNeedleCenter(rect);
        const float needleLength = std::min(rect.width, rect.height) *
            (m_isOverlay ? 0.64f : 0.7f);

        DrawNeedleBody(context, center, needleLength);
        DrawNeedlePivot(context, center, rect.width * (m_isOverlay ? 0.015f : 0.02f));
    }

    void GaugeRenderer::DrawPeakIndicator(
        GraphicsContext& context,
        const Rect& rect
    ) const
    {
        const float lampRadius = std::min(rect.width, rect.height) *
            (m_isOverlay ? 0.04f : 0.05f);

        const Point lampPos{
            rect.GetRight() - lampRadius * 2.5f,
            rect.y + lampRadius * 2.5f
        };

        if (m_peakActive && m_quality != RenderQuality::Low) {
            context.DrawGlow(lampPos, lampRadius * 2.0f, Color::Red(), 0.3f);
        }

        const Color lampColor = m_peakActive
            ? Color::Red()
            : Color::FromRGB(180, 0, 0);

        context.DrawCircle(lampPos, lampRadius * 0.8f, lampColor, true);

        context.DrawCircle(
            lampPos,
            lampRadius,
            Color::FromRGB(40, 40, 40),
            false,
            1.2f
        );

        const float textSize = lampRadius;
        const Point textPos{
            lampPos.x,
            lampPos.y + lampRadius + textSize * 0.5f
        };

        const Color textColor = m_peakActive
            ? Color::Red()
            : Color::FromRGB(180, 0, 0);

        context.DrawText(
            L"PEAK",
            textPos,
            textColor,
            textSize,
            DWRITE_TEXT_ALIGNMENT_CENTER
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Scale Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawMajorTick(
        GraphicsContext& context,
        const Point& center,
        float radiusX,
        float radiusY,
        float dbValue,
        const wchar_t* label
    ) const
    {
        const float angle = DbToAngle(dbValue);
        const float rad = Utils::DegToRad(angle);
        const float tickLength = GetTickLength(dbValue, true) * radiusY;

        const Point start{
            center.x + (radiusX - tickLength) * std::cos(rad),
            center.y + (radiusY - tickLength) * std::sin(rad)
        };

        const Point end{
            center.x + radiusX * std::cos(rad),
            center.y + radiusY * std::sin(rad)
        };

        const Color tickColor = (dbValue >= 0.0f)
            ? Color::FromRGB(220, 0, 0)
            : Color::FromRGB(80, 80, 80);

        context.DrawLine(start, end, tickColor, 1.8f);

        if (!label) return;

        const float textOffset = radiusY * (m_isOverlay ? 0.1f : 0.12f);
        float textSize = radiusY * (m_isOverlay ? 0.08f : 0.1f);

        if (dbValue == 0.0f) textSize *= 1.15f;

        const Point labelPos{
            center.x + (radiusX + textOffset) * std::cos(rad),
            center.y + (radiusY + textOffset) * std::sin(rad)
        };

        const Color textColor = (dbValue >= 0.0f)
            ? Color::FromRGB(200, 0, 0)
            : Color::Black();

        context.DrawText(label, labelPos, textColor, textSize, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    void GaugeRenderer::DrawMinorTick(
        GraphicsContext& context,
        const Point& center,
        float radiusX,
        float radiusY,
        float dbValue
    ) const
    {
        const float angle = DbToAngle(dbValue);
        const float rad = Utils::DegToRad(angle);
        const float tickLength = GetTickLength(dbValue, false) * radiusY;

        const Point start{
            center.x + (radiusX - tickLength) * std::cos(rad),
            center.y + (radiusY - tickLength) * std::sin(rad)
        };

        const Point end{
            center.x + radiusX * std::cos(rad),
            center.y + radiusY * std::sin(rad)
        };

        const Color minorColor = (dbValue >= 0.0f)
            ? Color::FromRGB(180, 100, 100)
            : Color::FromRGB(100, 100, 100);

        context.DrawLine(start, end, minorColor, 1.0f);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Needle Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawNeedleBody(
        GraphicsContext& context,
        const Point& center,
        float length
    ) const
    {
        const std::vector<Point> needlePoints = {
            {0.0f, -length},
            {-kNeedleBaseWidth, 0.0f},
            {kNeedleBaseWidth, 0.0f}
        };

        if (m_quality != RenderQuality::Low) {
            context.DrawWithShadow(
                [&]() {
                    context.PushTransform();
                    context.TranslateBy(center.x, center.y);
                    context.RotateAt({ 0.0f, 0.0f }, m_currentNeedleAngle + 90.0f);
                    context.DrawPolygon(needlePoints, Color::Black(), true);
                    context.PopTransform();
                },
                { kShadowOffsetX, kShadowOffsetY },
                2.0f,
                Color(0.0f, 0.0f, 0.0f, kShadowAlpha)
            );
        }

        context.PushTransform();
        context.TranslateBy(center.x, center.y);
        context.RotateAt({ 0.0f, 0.0f }, m_currentNeedleAngle + 90.0f);
        context.DrawPolygon(needlePoints, Color::Black(), true);
        context.PopTransform();
    }

    void GaugeRenderer::DrawNeedlePivot(
        GraphicsContext& context,
        const Point& center,
        float radius
    ) const
    {
        if (m_quality != RenderQuality::Low) {
            const Point highlightPos{
                center.x - radius * 0.25f,
                center.y - radius * 0.25f
            };

            context.DrawCircle(
                center,
                radius,
                Color::FromRGB(60, 60, 60),
                true
            );

            context.DrawCircle(
                highlightPos,
                radius * 0.4f,
                Color(1.0f, 1.0f, 1.0f, 0.6f),
                true
            );
        }
        else {
            context.DrawCircle(
                center,
                radius,
                Color::FromRGB(60, 60, 60),
                true
            );
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float GaugeRenderer::CalculateLoudness(const SpectrumData& spectrum) const
    {
        if (spectrum.empty()) return kDbMin;

        float sum = 0.0f;
        for (const float val : spectrum) {
            sum += val * val;
        }

        const float rms = std::sqrt(sum / spectrum.size());
        const float db = 20.0f * std::log10(std::max(rms, 1e-10f));

        return Utils::Clamp(db, kDbMin, kDbMax);
    }

    float GaugeRenderer::DbToAngle(float db) const
    {
        const float normalized = (Utils::Clamp(db, kDbMin, kDbMax) - kDbMin) /
            (kDbMax - kDbMin);
        return kAngleStart + normalized * kAngleRange;
    }

    Point GaugeRenderer::GetScaleCenter(const Rect& rect) const
    {
        return {
            rect.x + rect.width * 0.5f,
            rect.y + rect.height * 0.5f + rect.height * 0.15f
        };
    }

    Point GaugeRenderer::GetNeedleCenter(const Rect& rect) const
    {
        return {
            rect.x + rect.width * 0.5f,
            rect.y + rect.height * 0.5f +
                rect.height * (m_isOverlay ? 0.35f : 0.4f)
        };
    }

    float GaugeRenderer::GetTickLength(float dbValue, bool isMajor) const
    {
        if (!isMajor) return m_isOverlay ? 0.05f : 0.06f;
        if (dbValue == 0.0f) return m_isOverlay ? 0.12f : 0.15f;
        return m_isOverlay ? 0.064f : 0.08f;
    }

} // namespace Spectrum