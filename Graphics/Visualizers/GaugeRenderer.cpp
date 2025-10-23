// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the GaugeRenderer for vintage VU meter visualization.
//
// Implementation details:
// - RMS loudness calculation matches perceived volume
// - Asymmetric smoothing for realistic needle ballistics
// - Peak lamp triggers at +3dB with hold time
// - Scale divisions follow VU meter standards
// - Quality settings control smoothing and visual effects
// - Uses GeometryHelpers for all geometric operations
//
// Rendering pipeline:
// 1. Background: bezel, ring, meter face with label
// 2. Scale: major/minor ticks with dB labels
// 3. Needle: triangle with shadow and metallic pivot
// 4. Peak indicator: jeweled lamp with glow effect
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/GaugeRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    using namespace Helpers::Geometry;

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

        constexpr float kVULabelHeightRatio = 0.15f;
        constexpr float kVULabelOffsetRatio = 1.5f;

        constexpr float kPeakLampRadiusOverlay = 0.04f;
        constexpr float kPeakLampRadiusNormal = 0.05f;
        constexpr float kPeakLampInnerScale = 0.8f;
        constexpr float kPeakLampGlowScale = 2.0f;
        constexpr float kPeakLampPositionOffset = 2.5f;

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
        : BaseRenderer(),
        m_currentDbValue(kDbMin),
        m_currentNeedleAngle(kAngleStart),
        m_peakHoldCounter(0),
        m_peakActive(false)
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
        m_settings = QualityPresets::Get<GaugeRenderer>(m_quality);
    }

    void GaugeRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float /*deltaTime*/
    )
    {
        const float targetDb = CalculateLoudness(spectrum);

        const float smoothing = (targetDb > m_currentDbValue)
            ? m_settings.smoothingFactorInc
            : m_settings.smoothingFactorDec;

        const float adjustedSmoothing = m_isOverlay ? smoothing * 0.5f : smoothing;

        m_currentDbValue = Helpers::Math::Lerp(m_currentDbValue, targetDb, adjustedSmoothing);

        const float targetAngle = DbToAngle(m_currentDbValue);
        m_currentNeedleAngle = Helpers::Math::Lerp(m_currentNeedleAngle, targetAngle, m_settings.riseSpeed);

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
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        const Rect gaugeRect = CalculatePaddedRect();

        if (!IsValid(gaugeRect)) return;

        DrawBackground(canvas, gaugeRect);
        DrawScale(canvas, gaugeRect);
        DrawNeedle(canvas, gaugeRect);
        DrawPeakIndicator(canvas, gaugeRect);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Drawing Components (SRP)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawBackground(
        Canvas& canvas,
        const Rect& rect
    ) const
    {
        DrawBezelLayers(canvas, rect);
        const Rect innerRect = GetInnerRect(rect);
        DrawMeterFace(canvas, innerRect);
    }

    void GaugeRenderer::DrawScale(
        Canvas& canvas,
        const Rect& rect
    ) const
    {
        const Point center = GetScaleCenter(rect);
        const float radiusX = rect.width * (m_isOverlay ? 0.4f : 0.45f);
        const float radiusY = rect.height * (m_isOverlay ? 0.45f : 0.5f);

        for (const auto& mark : kMajorMarks) {
            DrawMajorTick(canvas, center, radiusX, radiusY, mark.db, mark.label);
        }

        for (const auto& db : kMinorMarks) {
            DrawMinorTick(canvas, center, radiusX, radiusY, db);
        }
    }

    void GaugeRenderer::DrawNeedle(
        Canvas& canvas,
        const Rect& rect
    ) const
    {
        const Point center = GetNeedleCenter(rect);
        const float needleLength = std::min(rect.width, rect.height) *
            (m_isOverlay ? 0.64f : 0.7f);

        DrawNeedleBody(canvas, center, needleLength);
        DrawNeedlePivot(canvas, center, rect.width * (m_isOverlay ? 0.015f : 0.02f));
    }

    void GaugeRenderer::DrawPeakIndicator(
        Canvas& canvas,
        const Rect& rect
    ) const
    {
        const float lampRadius = std::min(rect.width, rect.height) *
            (m_isOverlay ? kPeakLampRadiusOverlay : kPeakLampRadiusNormal);

        const Point lampPos = GetPeakLampPosition(rect, lampRadius);

        DrawPeakLamp(canvas, lampPos, lampRadius);
        DrawPeakLabel(canvas, lampPos, lampRadius);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Background Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawBezelLayers(
        Canvas& canvas,
        const Rect& rect
    ) const
    {
        canvas.DrawRoundedRectangle(
            rect,
            kBezelRadius,
            Paint::Fill(Color::FromRGB(80, 80, 80))
        );

        const Rect innerRect = GetInnerRect(rect);

        canvas.DrawRoundedRectangle(
            innerRect,
            kInnerRadius,
            Paint::Fill(Color::FromRGB(105, 105, 105))
        );
    }

    void GaugeRenderer::DrawMeterFace(
        Canvas& canvas,
        const Rect& outerRect
    ) const
    {
        const Rect faceRect = GetFaceRect(outerRect);

        canvas.DrawRectangle(
            faceRect,
            Paint::Fill(Color::FromRGB(240, 240, 230))
        );

        const float textSize = outerRect.height * kVULabelHeightRatio;
        DrawVULabel(canvas, faceRect, textSize);
    }

    void GaugeRenderer::DrawVULabel(
        Canvas& canvas,
        const Rect& faceRect,
        float textSize
    ) const
    {
        const Point textPos{
            faceRect.x + faceRect.width * 0.5f,
            GetBottom(faceRect) - textSize * kVULabelOffsetRatio
        };

        const Rect textRect = CreateCentered(
            textPos,
            textSize * 2.0f,
            textSize * 1.5f
        );

        TextStyle style = TextStyle::Default()
            .WithColor(Color::Black())
            .WithSize(textSize)
            .WithAlign(TextAlign::Center);

        canvas.DrawText(L"VU", textRect, style);
    }

    Rect GaugeRenderer::GetInnerRect(const Rect& rect) const
    {
        return Deflate(rect, kBezelPadding);
    }

    Rect GaugeRenderer::GetFaceRect(const Rect& innerRect) const
    {
        return Deflate(innerRect, kInnerPadding);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Scale Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawMajorTick(
        Canvas& canvas,
        const Point& center,
        float radiusX,
        float radiusY,
        float dbValue,
        const wchar_t* label
    ) const
    {
        const float angle = DbToAngle(dbValue);
        const float tickLength = GetTickLength(dbValue, true) * radiusY;
        const auto [start, end] = GetTickPoints(
            center,
            radiusX,
            radiusY,
            angle,
            tickLength
        );

        DrawTickLine(canvas, start, end, GetTickColor(dbValue, false), 1.8f);

        if (label) {
            const float textOffset = radiusY * (m_isOverlay ? 0.1f : 0.12f);

            const Point labelPos = PointOnEllipse(
                center,
                radiusX + textOffset,
                radiusY + textOffset,
                Helpers::Math::DegreesToRadians(angle)
            );

            const float textSize = GetLabelTextSize({ 0, 0, radiusY, radiusY }, dbValue);
            const Color textColor = (dbValue >= 0.0f)
                ? Color::FromRGB(200, 0, 0)
                : Color::Black();

            DrawTickLabel(canvas, labelPos, textSize, label, textColor);
        }
    }

    void GaugeRenderer::DrawMinorTick(
        Canvas& canvas,
        const Point& center,
        float radiusX,
        float radiusY,
        float dbValue
    ) const
    {
        const float angle = DbToAngle(dbValue);
        const float tickLength = GetTickLength(dbValue, false) * radiusY;
        const auto [start, end] = GetTickPoints(
            center,
            radiusX,
            radiusY,
            angle,
            tickLength
        );

        DrawTickLine(canvas, start, end, GetTickColor(dbValue, true), 1.0f);
    }

    void GaugeRenderer::DrawTickLine(
        Canvas& canvas,
        const Point& start,
        const Point& end,
        const Color& color,
        float width
    ) const
    {
        canvas.DrawLine(start, end, Paint::Stroke(color, width));
    }

    void GaugeRenderer::DrawTickLabel(
        Canvas& canvas,
        const Point& labelPos,
        float textSize,
        const wchar_t* label,
        const Color& color
    ) const
    {
        const Rect labelRect = CreateCentered(
            labelPos,
            textSize * 3.0f,
            textSize * 1.5f
        );

        TextStyle style = TextStyle::Default()
            .WithColor(color)
            .WithSize(textSize)
            .WithAlign(TextAlign::Center);

        canvas.DrawText(label, labelRect, style);
    }

    std::pair<Point, Point> GaugeRenderer::GetTickPoints(
        const Point& center,
        float radiusX,
        float radiusY,
        float angle,
        float tickLength
    ) const
    {
        const float rad = Helpers::Math::DegreesToRadians(angle);

        const Point start = PointOnEllipse(
            center,
            radiusX - tickLength,
            radiusY - tickLength,
            rad
        );

        const Point end = PointOnEllipse(
            center,
            radiusX,
            radiusY,
            rad
        );

        return { start, end };
    }

    Color GaugeRenderer::GetTickColor(
        float dbValue,
        bool isMinor
    ) const
    {
        if (isMinor) {
            return (dbValue >= 0.0f)
                ? Color::FromRGB(180, 100, 100)
                : Color::FromRGB(100, 100, 100);
        }

        return (dbValue >= 0.0f)
            ? Color::FromRGB(220, 0, 0)
            : Color::FromRGB(80, 80, 80);
    }

    float GaugeRenderer::GetLabelTextSize(
        const Rect& rect,
        float dbValue
    ) const
    {
        float textSize = rect.height * (m_isOverlay ? 0.08f : 0.1f);

        if (dbValue == 0.0f) {
            textSize *= 1.15f;
        }

        return textSize;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Needle Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawNeedleBody(
        Canvas& canvas,
        const Point& center,
        float length
    ) const
    {
        const std::vector<Point> needlePoints = GetNeedleGeometry(length);

        auto drawCall = [&]() {
            canvas.PushTransform();
            canvas.TranslateBy(center.x, center.y);
            canvas.RotateAt({ 0.0f, 0.0f }, m_currentNeedleAngle + 90.0f);
            canvas.DrawPolygon(needlePoints, Paint::Fill(Color::Black()));
            canvas.PopTransform();
            };

        if (m_quality != RenderQuality::Low) {
            canvas.DrawWithShadow(
                drawCall,
                { kShadowOffsetX, kShadowOffsetY },
                2.0f,
                Color(0.0f, 0.0f, 0.0f, kShadowAlpha)
            );
            drawCall();
        }
        else {
            drawCall();
        }
    }

    void GaugeRenderer::DrawNeedlePivot(
        Canvas& canvas,
        const Point& center,
        float radius
    ) const
    {
        canvas.DrawCircle(
            center,
            radius,
            Paint::Fill(Color::FromRGB(60, 60, 60))
        );

        if (m_quality != RenderQuality::Low) {

            const Point offset = { -radius * 0.25f, -radius * 0.25f };
            const Point highlightPos = Add(center, offset);

            canvas.DrawCircle(
                highlightPos,
                radius * 0.4f,
                Paint::Fill(Color(1.0f, 1.0f, 1.0f, 0.6f))
            );
        }
    }

    std::vector<Point> GaugeRenderer::GetNeedleGeometry(float length) const
    {
        return {
            {0.0f, -length},
            {-kNeedleBaseWidth, 0.0f},
            {kNeedleBaseWidth, 0.0f}
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Peak Indicator Components
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void GaugeRenderer::DrawPeakLamp(
        Canvas& canvas,
        const Point& lampPos,
        float lampRadius
    ) const
    {
        if (m_peakActive && m_quality != RenderQuality::Low) {
            canvas.DrawGlow(
                lampPos,
                lampRadius * kPeakLampGlowScale,
                Color::Red(),
                0.3f
            );
        }

        canvas.DrawCircle(
            lampPos,
            lampRadius * kPeakLampInnerScale,
            Paint::Fill(GetPeakLampColor())
        );

        canvas.DrawCircle(
            lampPos,
            lampRadius,
            Paint::Stroke(Color::FromRGB(40, 40, 40), 1.2f)
        );
    }

    void GaugeRenderer::DrawPeakLabel(
        Canvas& canvas,
        const Point& lampPos,
        float lampRadius
    ) const
    {
        const Point textOffset = { 0.0f, lampRadius + lampRadius * 0.5f };
        const Point textPos = Add(lampPos, textOffset);

        const Rect textRect = CreateCentered(
            textPos,
            lampRadius * 4.0f,
            lampRadius * 1.5f
        );

        TextStyle style = TextStyle::Default()
            .WithColor(GetPeakTextColor())
            .WithSize(lampRadius)
            .WithAlign(TextAlign::Center);

        canvas.DrawText(L"PEAK", textRect, style);
    }

    Point GaugeRenderer::GetPeakLampPosition(
        const Rect& rect,
        float lampRadius
    ) const
    {
        const Point topRight = GetTopRight(rect);
        const Point offset = {
            -lampRadius * kPeakLampPositionOffset,
            lampRadius * kPeakLampPositionOffset
        };

        return Add(topRight, offset);
    }

    Color GaugeRenderer::GetPeakLampColor() const
    {
        return m_peakActive
            ? Color::Red()
            : Color::FromRGB(180, 0, 0);
    }

    Color GaugeRenderer::GetPeakTextColor() const
    {
        return m_peakActive
            ? Color::Red()
            : Color::FromRGB(180, 0, 0);
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

        return Helpers::Math::Clamp(db, kDbMin, kDbMax);
    }

    float GaugeRenderer::DbToAngle(float db) const
    {
        return Helpers::Math::Map(
            Helpers::Math::Clamp(db, kDbMin, kDbMax),
            kDbMin,
            kDbMax,
            kAngleStart,
            kAngleEnd
        );
    }

    Point GaugeRenderer::GetScaleCenter(const Rect& rect) const
    {
        const Point center = GetCenter(rect);
        const Point offset = { 0.0f, rect.height * 0.15f };

        return Add(center, offset);
    }

    Point GaugeRenderer::GetNeedleCenter(const Rect& rect) const
    {
        const Point center = GetCenter(rect);
        const float yOffset = rect.height * (m_isOverlay ? 0.35f : 0.4f);

        return Add(center, { 0.0f, yOffset });
    }

    float GaugeRenderer::GetTickLength(
        float dbValue,
        bool isMajor
    ) const
    {
        if (!isMajor) return m_isOverlay ? 0.05f : 0.06f;
        if (dbValue == 0.0f) return m_isOverlay ? 0.12f : 0.15f;
        return m_isOverlay ? 0.064f : 0.08f;
    }

} // namespace Spectrum