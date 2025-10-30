#include "Graphics/Visualizers/GaugeRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Geometry;
    using namespace Helpers::Math;

    namespace {
        constexpr float kDbMax = 5.0f;
        constexpr float kDbMin = -30.0f;
        constexpr float kDbPeakThreshold = 3.0f;
        constexpr float kAngleStart = -150.0f;
        constexpr float kAngleEnd = -30.0f;
        constexpr int kPeakHoldDuration = 15;
        constexpr float kBezelPadding = 4.0f;
        constexpr float kInnerPadding = 4.0f;
        constexpr float kBezelRadius = 8.0f;
        constexpr float kInnerRadius = 6.0f;
        constexpr float kNeedleBaseWidth = 2.5f;

        struct MajorMark {
            float db;
            const wchar_t* label;
        };

        constexpr MajorMark kMajorMarks[] = {
            {-30.0f, L"-30"}, {-20.0f, L"-20"}, {-10.0f, L"-10"},
            {-7.0f, L"-7"}, {-5.0f, L"-5"}, {-3.0f, L"-3"},
            {0.0f, L"0"}, {3.0f, L"+3"}, {5.0f, L"+5"}
        };
    }

    GaugeRenderer::GaugeRenderer()
        : m_currentDbValue(kDbMin)
        , m_currentNeedleAngle(kAngleStart)
        , m_peakHoldCounter(0)
        , m_peakActive(false)
    {
        m_aspectRatio = 2.0f;
        m_padding = 0.8f;
        UpdateSettings();
    }

    void GaugeRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings::GaugeSettings>();
    }

    void GaugeRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float
    ) {
        const float targetDb = CalculateLoudness(spectrum);
        const float smoothing = (targetDb > m_currentDbValue)
            ? m_settings.smoothingFactorInc
            : m_settings.smoothingFactorDec;
        const float adjustedSmoothing = IsOverlay() ? smoothing * 0.5f : smoothing;

        m_currentDbValue = Lerp(m_currentDbValue, targetDb, adjustedSmoothing);
        m_currentNeedleAngle = Lerp(
            m_currentNeedleAngle,
            DbToAngle(m_currentDbValue),
            m_settings.riseSpeed
        );

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

    void GaugeRenderer::DoRender(Canvas& canvas, const SpectrumData&) {
        const Rect gaugeRect = CalculatePaddedRect();
        if (!IsValid(gaugeRect)) return;

        DrawBackground(canvas, gaugeRect);
        DrawScale(canvas, gaugeRect);
        DrawNeedle(canvas, gaugeRect);
        DrawPeakIndicator(canvas, gaugeRect);
    }

    void GaugeRenderer::DrawBackground(Canvas& canvas, const Rect& rect) const {
        canvas.DrawRoundedRectangle(
            rect,
            kBezelRadius,
            Paint::Fill(Color::FromRGB(80, 80, 80))
        );

        const Rect innerRect = Deflate(rect, kBezelPadding);
        canvas.DrawRoundedRectangle(
            innerRect,
            kInnerRadius,
            Paint::Fill(Color::FromRGB(105, 105, 105))
        );

        const Rect faceRect = Deflate(innerRect, kInnerPadding);
        canvas.DrawRectangle(
            faceRect,
            Paint::Fill(Color::FromRGB(240, 240, 230))
        );

        const float textSize = rect.height * 0.15f;
        const Point textPos{
            faceRect.x + faceRect.width * 0.5f,
            GetBottom(faceRect) - textSize * 1.5f
        };

        const Rect textRect = CreateCentered(textPos, textSize * 2.0f, textSize * 1.5f);
        TextStyle style = TextStyle::Default()
            .WithColor(Color::Black())
            .WithSize(textSize)
            .WithAlign(TextAlign::Center);
        canvas.DrawText(L"VU", textRect, style);
    }

    void GaugeRenderer::DrawScale(Canvas& canvas, const Rect& rect) const {
        const Point center = Add(GetCenter(rect), { 0.0f, rect.height * 0.15f });
        const float radiusX = rect.width * (IsOverlay() ? 0.4f : 0.45f);
        const float radiusY = rect.height * (IsOverlay() ? 0.45f : 0.5f);

        for (const auto& mark : kMajorMarks) {
            const float angle = DbToAngle(mark.db);
            const float rad = DegreesToRadians(angle);
            const float tickLength = (mark.db == 0.0f ? 0.15f : 0.08f) * radiusY;

            const Point start = PointOnEllipse(
                center,
                radiusX - tickLength,
                radiusY - tickLength,
                rad
            );

            const Point end = PointOnEllipse(center, radiusX, radiusY, rad);

            const Color tickColor = (mark.db >= 0.0f)
                ? Color::FromRGB(220, 0, 0)
                : Color::FromRGB(80, 80, 80);

            canvas.DrawLine(start, end, Paint::Stroke(tickColor, 1.8f));

            if (mark.label) {
                const Point labelPos = PointOnEllipse(
                    center,
                    radiusX + radiusY * 0.12f,
                    radiusY + radiusY * 0.12f,
                    rad
                );

                const float textSize = rect.height * (IsOverlay() ? 0.08f : 0.1f) *
                    (mark.db == 0.0f ? 1.15f : 1.0f);

                const Rect labelRect = CreateCentered(
                    labelPos,
                    textSize * 3.0f,
                    textSize * 1.5f
                );

                TextStyle style = TextStyle::Default()
                    .WithColor(mark.db >= 0.0f ? Color::FromRGB(200, 0, 0) : Color::Black())
                    .WithSize(textSize)
                    .WithAlign(TextAlign::Center);

                canvas.DrawText(mark.label, labelRect, style);
            }
        }
    }

    void GaugeRenderer::DrawNeedle(Canvas& canvas, const Rect& rect) const {
        const Point center = GetNeedleCenter(rect);
        const float needleLength = std::min(rect.width, rect.height) *
            (IsOverlay() ? 0.64f : 0.7f);

        const std::vector<Point> needlePoints = {
            {0.0f, -needleLength},
            {-kNeedleBaseWidth, 0.0f},
            {kNeedleBaseWidth, 0.0f}
        };

        auto drawNeedle = [&]() {
            canvas.PushTransform();
            canvas.TranslateBy(center.x, center.y);
            canvas.RotateAt({ 0.0f, 0.0f }, m_currentNeedleAngle + 90.0f);
            canvas.DrawPolygon(needlePoints, Paint::Fill(Color::Black()));
            canvas.PopTransform();
            };

        if (GetQuality() != RenderQuality::Low) {
            RenderWithShadow(canvas, drawNeedle);
        }
        else {
            drawNeedle();
        }

        const float pivotRadius = rect.width * (IsOverlay() ? 0.015f : 0.02f);
        canvas.DrawCircle(
            center,
            pivotRadius,
            Paint::Fill(Color::FromRGB(60, 60, 60))
        );

        if (GetQuality() != RenderQuality::Low) {
            const Point highlightPos = Add(
                center,
                { -pivotRadius * 0.25f, -pivotRadius * 0.25f }
            );
            canvas.DrawCircle(
                highlightPos,
                pivotRadius * 0.4f,
                Paint::Fill(Color(1.0f, 1.0f, 1.0f, 0.6f))
            );
        }
    }

    void GaugeRenderer::DrawPeakIndicator(Canvas& canvas, const Rect& rect) const {
        const float lampRadius = std::min(rect.width, rect.height) *
            (IsOverlay() ? 0.04f : 0.05f);

        const Point lampPos = Add(
            GetTopRight(rect),
            { -lampRadius * 2.5f, lampRadius * 2.5f }
        );

        if (m_peakActive && GetQuality() != RenderQuality::Low) {
            canvas.DrawGlow(lampPos, lampRadius * 2.0f, Color::Red(), 0.3f);
        }

        const Color lampColor = m_peakActive
            ? Color::Red()
            : Color::FromRGB(180, 0, 0);

        canvas.DrawCircle(
            lampPos,
            lampRadius * 0.8f,
            Paint::Fill(lampColor)
        );

        canvas.DrawCircle(
            lampPos,
            lampRadius,
            Paint::Stroke(Color::FromRGB(40, 40, 40), 1.2f)
        );

        const Point textPos = Add(lampPos, { 0.0f, lampRadius * 1.5f });
        const Rect textRect = CreateCentered(
            textPos,
            lampRadius * 4.0f,
            lampRadius * 1.5f
        );

        TextStyle style = TextStyle::Default()
            .WithColor(lampColor)
            .WithSize(lampRadius)
            .WithAlign(TextAlign::Center);

        canvas.DrawText(L"PEAK", textRect, style);
    }

    float GaugeRenderer::CalculateLoudness(const SpectrumData& spectrum) const {
        if (spectrum.empty()) return kDbMin;

        float sum = 0.0f;
        for (const float val : spectrum) {
            sum += val * val;
        }

        const float rms = std::sqrt(sum / spectrum.size());
        const float db = 20.0f * std::log10(std::max(rms, 1e-10f));
        return Clamp(db, kDbMin, kDbMax);
    }

    float GaugeRenderer::DbToAngle(float db) const {
        return MapToRange(
            Clamp(db, kDbMin, kDbMax),
            kDbMin,
            kDbMax,
            kAngleStart,
            kAngleEnd
        );
    }

    Point GaugeRenderer::GetNeedleCenter(const Rect& rect) const {
        return Add(
            GetCenter(rect),
            { 0.0f, rect.height * (IsOverlay() ? 0.35f : 0.4f) }
        );
    }

    Rect GaugeRenderer::CalculatePaddedRect() const {
        const Point center = GetViewportCenter();
        const float viewWidth = static_cast<float>(GetWidth());
        const float viewHeight = static_cast<float>(GetHeight());

        if (viewWidth <= 0.0f || viewHeight <= 0.0f) {
            return Rect{ 0.0f, 0.0f, 0.0f, 0.0f };
        }

        float width = viewWidth * m_padding;
        float height = width / m_aspectRatio;

        if (height > viewHeight * m_padding) {
            height = viewHeight * m_padding;
            width = height * m_aspectRatio;
        }

        return CreateCentered(center, width, height);
    }

} // namespace Spectrum