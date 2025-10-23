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

#include "Graphics/Visualizers/GaugeRenderer.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/TextStyle.h"
#include "Common/MathUtils.h"
#include "Common/ColorUtils.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/API/Canvas.h"
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
        Canvas& canvas,
        const SpectrumData& /*spectrum*/
    )
    {
        const Rect gaugeRect = CalculatePaddedRect();

        if (gaugeRect.width <= 0.0f || gaugeRect.height <= 0.0f) return;

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
            faceRect.GetBottom() - textSize * kVULabelOffsetRatio
        };

        const Rect textRect = CreateCenteredTextRect(
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
        return {
            rect.x + kBezelPadding,
            rect.y + kBezelPadding,
            rect.width - kBezelPadding * 2.0f,
            rect.height - kBezelPadding * 2.0f
        };
    }

    Rect GaugeRenderer::GetFaceRect(const Rect& innerRect) const
    {
        return {
            innerRect.x + kInnerPadding,
            innerRect.y + kInnerPadding,
            innerRect.width - kInnerPadding * 2.0f,
            innerRect.height - kInnerPadding * 2.0f
        };
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
            const float rad = Utils::DegToRad(angle);

            const Point labelPos{
                center.x + (radiusX + textOffset) * std::cos(rad),
                center.y + (radiusY + textOffset) * std::sin(rad)
            };

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
        const Rect labelRect = CreateCenteredTextRect(
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
        const float rad = Utils::DegToRad(angle);

        const Point start{
            center.x + (radiusX - tickLength) * std::cos(rad),
            center.y + (radiusY - tickLength) * std::sin(rad)
        };

        const Point end{
            center.x + radiusX * std::cos(rad),
            center.y + radiusY * std::sin(rad)
        };

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
            const Point highlightPos{
                center.x - radius * 0.25f,
                center.y - radius * 0.25f
            };

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
        const Point textPos{
            lampPos.x,
            lampPos.y + lampRadius + lampRadius * 0.5f
        };

        const Rect textRect = CreateCenteredTextRect(
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
        return {
            rect.GetRight() - lampRadius * kPeakLampPositionOffset,
            rect.y + lampRadius * kPeakLampPositionOffset
        };
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

    float GaugeRenderer::GetTickLength(
        float dbValue,
        bool isMajor
    ) const
    {
        if (!isMajor) return m_isOverlay ? 0.05f : 0.06f;
        if (dbValue == 0.0f) return m_isOverlay ? 0.12f : 0.15f;
        return m_isOverlay ? 0.064f : 0.08f;
    }

    Rect GaugeRenderer::CreateCenteredTextRect(
        const Point& center,
        float width,
        float height
    ) const
    {
        return {
            center.x - width * 0.5f,
            center.y - height * 0.5f,
            width,
            height
        };
    }

} // namespace Spectrum