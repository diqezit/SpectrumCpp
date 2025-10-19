#include "GaugeRenderer.h"
#include "MathUtils.h"
#include "ColorUtils.h"
#include <algorithm>
#include <cmath>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    namespace {
        // VU meter standard range matches analog equipment
        constexpr float DB_MAX = 5.0f;
        constexpr float DB_MIN = -30.0f;
        constexpr float DB_PEAK_THRESHOLD = 3.0f;

        // needle sweep matches physical meter response time
        constexpr float ANGLE_START = -150.0f;
        constexpr float ANGLE_END = -30.0f;
        constexpr float ANGLE_RANGE = ANGLE_END - ANGLE_START;

        // peak lamp holds like analog VU meter
        constexpr int PEAK_HOLD_DURATION = 15;

        // VU meter standard scale divisions
        const std::vector<std::pair<float, const wchar_t*>> MAJOR_MARKS = {
            {-30.0f, L"-30"}, {-20.0f, L"-20"}, {-10.0f, L"-10"},
            {-7.0f, L"-7"}, {-5.0f, L"-5"}, {-3.0f, L"-3"},
            {0.0f, L"0"}, {3.0f, L"+3"}, {5.0f, L"+5"}
        };

        // minor marks follow VU meter subdivision pattern
        const std::vector<float> MINOR_MARKS = {
            -25.0f,             // single mark between wide -30 to -20 range
            -15.0f, -12.5f,     // two marks between -20 to -10 range
            -9.0f, -8.0f,       // fill between -10 to -7
            -6.0f,              // single between -7 to -5
            -4.0f,              // single between -5 to -3
            -2.0f, -1.0f,       // fill between -3 to 0
            1.0f, 2.0f,         // fill between 0 to +3
            4.0f                // single between +3 to +5
        };

        // vintage VU meter face colors
        const std::vector<D2D1_GRADIENT_STOP> GAUGE_BG_GRADIENT = {
            {0.0f, D2D1::ColorF(0.98f, 0.98f, 0.94f)},
            {1.0f, D2D1::ColorF(0.90f, 0.90f, 0.84f)}
        };

        const std::vector<D2D1_GRADIENT_STOP> NEEDLE_CENTER_GRADIENT = {
            {0.0f, D2D1::ColorF(D2D1::ColorF::White)},
            {0.3f, D2D1::ColorF(0.7f, 0.7f, 0.7f)},
            {1.0f, D2D1::ColorF(0.23f, 0.23f, 0.23f)}
        };

        const std::vector<D2D1_GRADIENT_STOP> PEAK_ACTIVE_GRADIENT = {
            {0.0f, D2D1::ColorF(D2D1::ColorF::White)},
            {0.3f, D2D1::ColorF(1.0f, 0.7f, 0.7f)},
            {1.0f, D2D1::ColorF(D2D1::ColorF::Red)}
        };

        const std::vector<D2D1_GRADIENT_STOP> PEAK_INACTIVE_GRADIENT = {
            {0.0f, D2D1::ColorF(0.86f, 0.86f, 0.86f)},
            {0.3f, D2D1::ColorF(0.7f, 0.0f, 0.0f)},
            {1.0f, D2D1::ColorF(0.31f, 0.0f, 0.0f)}
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constructor
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    GaugeRenderer::GaugeRenderer()
        : m_currentDbValue(DB_MIN)
        , m_currentNeedleAngle(ANGLE_START)
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Settings Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // balance visual smoothness with responsiveness
    void GaugeRenderer::UpdateSettings() {
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // needle moves faster up than down like real VU meter
    void GaugeRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float /*deltaTime*/
    ) {
        float targetDb = CalculateLoudness(spectrum);

        float smoothing = (targetDb > m_currentDbValue)
            ? m_smoothingFactorInc
            : m_smoothingFactorDec;

        // overlay mode needs slower response to avoid distraction
        if (m_isOverlay)
            smoothing *= 0.5f;

        m_currentDbValue = Utils::Lerp(
            m_currentDbValue,
            targetDb,
            smoothing
        );

        float targetAngle = DbToAngle(m_currentDbValue);
        m_currentNeedleAngle = Utils::Lerp(
            m_currentNeedleAngle,
            targetAngle,
            m_riseSpeed
        );

        // peak lamp triggers at +3dB like hardware meters
        if (targetDb >= DB_PEAK_THRESHOLD) {
            m_peakActive = true;
            m_peakHoldCounter = PEAK_HOLD_DURATION;
        }
        else if (m_peakHoldCounter > 0) {
            m_peakHoldCounter--;
        }
        else {
            m_peakActive = false;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Render
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    void GaugeRenderer::DoRender(
        GraphicsContext& context,
        const SpectrumData& /*spectrum*/
    ) {
        Rect gaugeRect = CalculatePaddedRect();
        if (gaugeRect.width <= 0 || gaugeRect.height <= 0)
            return;

        DrawBackground(context, gaugeRect);
        DrawScale(context, gaugeRect);
        DrawPeakIndicator(context, gaugeRect);
        DrawNeedle(context, gaugeRect);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Background Drawing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // recreate vintage VU meter appearance
    void GaugeRenderer::DrawBackground(
        GraphicsContext& context,
        const Rect& rect
    ) const {
        // black bezel frame
        context.DrawRoundedRectangle(
            rect,
            8.0f,
            Color::FromRGB(80, 80, 80),
            true
        );

        // chrome inner ring
        Rect innerRect(
            rect.x + 4,
            rect.y + 4,
            rect.width - 8,
            rect.height - 8
        );
        context.DrawRoundedRectangle(
            innerRect,
            6.0f,
            Color::FromRGB(105, 105, 105),
            true
        );

        // cream colored meter face
        Rect bgRect(
            innerRect.x + 4,
            innerRect.y + 4,
            innerRect.width - 8,
            innerRect.height - 8
        );
        context.DrawGradientRectangle(
            bgRect,
            GAUGE_BG_GRADIENT,
            false
        );

        // VU label centered at bottom
        float vuTextSize = rect.height * 0.15f;
        Point textPos = {
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Scale Drawing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    void GaugeRenderer::DrawScale(
        GraphicsContext& context,
        const Rect& rect
    ) const {
        Point center = GetScaleCenter(rect);
        float radiusX = rect.width * (m_isOverlay ? 0.4f : 0.45f);
        float radiusY = rect.height * (m_isOverlay ? 0.45f : 0.5f);

        // draw major marks with labels
        for (const auto& [value, label] : MAJOR_MARKS)
            DrawMajorTick(context, center, radiusX, radiusY, value, label);

        // draw minor marks
        for (float value : MINOR_MARKS)
            DrawMinorTick(context, center, radiusX, radiusY, value);
    }

    // major ticks with labels for primary scale divisions
    void GaugeRenderer::DrawMajorTick(
        GraphicsContext& context,
        const Point& center,
        float radiusX,
        float radiusY,
        float value,
        const wchar_t* label
    ) const {
        float angle = DbToAngle(value);
        float rad = Utils::DegToRad(angle);
        float tickLength = GetTickLength(value, true) * radiusY;

        Point start = {
            center.x + (radiusX - tickLength) * std::cos(rad),
            center.y + (radiusY - tickLength) * std::sin(rad)
        };
        Point end = {
            center.x + radiusX * std::cos(rad),
            center.y + radiusY * std::sin(rad)
        };

        // red zone starts at 0dB
        Color tickColor = (value >= 0)
            ? Color::FromRGB(220, 0, 0)
            : Color::FromRGB(80, 80, 80);
        context.DrawLine(start, end, tickColor, 1.8f);

        if (!label)
            return;

        float textOffset = radiusY * (m_isOverlay ? 0.1f : 0.12f);
        float textSize = radiusY * (m_isOverlay ? 0.08f : 0.1f);
        // zero mark larger for emphasis
        if (value == 0.0f)
            textSize *= 1.15f;

        Point labelPos = {
            center.x + (radiusX + textOffset) * std::cos(rad),
            center.y + (radiusY + textOffset) * std::sin(rad)
        };

        // align text based on position around arc
        DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_CENTER;
        if (angle < -120.0f)
            align = DWRITE_TEXT_ALIGNMENT_TRAILING;
        else if (angle > -60.0f)
            align = DWRITE_TEXT_ALIGNMENT_LEADING;

        Color textColor = (value >= 0)
            ? Color::FromRGB(200, 0, 0)
            : Color::Black();
        context.DrawText(
            label,
            labelPos,
            textColor,
            textSize,
            align
        );
    }

    // minor ticks for fine reading between majors
    void GaugeRenderer::DrawMinorTick(
        GraphicsContext& context,
        const Point& center,
        float radiusX,
        float radiusY,
        float value
    ) const {
        float angle = DbToAngle(value);
        float rad = Utils::DegToRad(angle);
        float tickLength = GetTickLength(value, false) * radiusY;

        Point start = {
            center.x + (radiusX - tickLength) * std::cos(rad),
            center.y + (radiusY - tickLength) * std::sin(rad)
        };
        Point end = {
            center.x + radiusX * std::cos(rad),
            center.y + radiusY * std::sin(rad)
        };

        // lighter color for minor marks
        Color minorColor = (value >= 0)
            ? Color::FromRGB(180, 100, 100)
            : Color::FromRGB(100, 100, 100);
        context.DrawLine(start, end, minorColor, 1.0f);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Needle Drawing
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    void GaugeRenderer::DrawNeedle(
        GraphicsContext& context,
        const Rect& rect
    ) const {
        Point center = GetNeedleCenter(rect);
        float needleLength = std::min(rect.width, rect.height) *
            (m_isOverlay ? 0.64f : 0.7f);

        DrawNeedleBody(context, center, needleLength);
        DrawNeedlePivot(context, center, rect.width * (m_isOverlay ? 0.015f : 0.02f));
    }

    // simple triangle needle like classic meters
    void GaugeRenderer::DrawNeedleBody(
        GraphicsContext& context,
        const Point& center,
        float length
    ) const {
        std::vector<Point> needlePoints = {
            {0.0f, -length},
            {-2.5f, 0.0f},
            {2.5f, 0.0f}
        };

        // shadow for depth
        if (m_quality != RenderQuality::Low) {
            context.DrawWithShadow(
                [&]() {
                    context.PushTransform();
                    context.TranslateBy(center.x, center.y);
                    context.RotateAt({ 0, 0 }, m_currentNeedleAngle + 90.0f);
                    context.DrawPolygon(needlePoints, Color::Black(), true);
                    context.PopTransform();
                },
                { 2.0f, 2.0f },
                2.0f,
                Color(0, 0, 0, 0.3f)
            );
        }

        // main needle
        context.PushTransform();
        context.TranslateBy(center.x, center.y);
        context.RotateAt({ 0, 0 }, m_currentNeedleAngle + 90.0f);
        context.DrawPolygon(needlePoints, Color::Black(), true);
        context.PopTransform();
    }

    // metallic pivot cap at needle base
    void GaugeRenderer::DrawNeedlePivot(
        GraphicsContext& context,
        const Point& center,
        float radius
    ) const {
        if (m_quality != RenderQuality::Low) {
            context.DrawRadialGradient(
                center,
                radius,
                NEEDLE_CENTER_GRADIENT
            );

            // specular highlight simulates metal cap
            Point highlightPos = {
                center.x - radius * 0.25f,
                center.y - radius * 0.25f
            };
            context.DrawCircle(
                highlightPos,
                radius * 0.4f,
                Color(1, 1, 1, 0.6f),
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Peak Indicator
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // peak lamp matches vintage meter style
    void GaugeRenderer::DrawPeakIndicator(
        GraphicsContext& context,
        const Rect& rect
    ) const {
        float lampRadius = std::min(rect.width, rect.height) *
            (m_isOverlay ? 0.04f : 0.05f);

        // positioned in top right corner, clear of scale
        float lampX = rect.GetRight() - lampRadius * 2.5f;
        float lampY = rect.y + lampRadius * 2.5f;
        Point lampPos = { lampX, lampY };

        // glow simulates incandescent bulb
        if (m_peakActive && m_quality != RenderQuality::Low)
            context.DrawGlow(lampPos, lampRadius * 2.0f, Color::Red(), 0.3f);

        // jeweled lamp appearance
        if (m_quality != RenderQuality::Low) {
            context.DrawRadialGradient(
                lampPos,
                lampRadius * 0.8f,
                m_peakActive ? PEAK_ACTIVE_GRADIENT : PEAK_INACTIVE_GRADIENT
            );
        }
        else {
            Color lampColor = m_peakActive
                ? Color::Red()
                : Color::FromRGB(180, 0, 0);
            context.DrawCircle(lampPos, lampRadius * 0.8f, lampColor, true);
        }

        // chrome bezel around lamp
        context.DrawCircle(
            lampPos,
            lampRadius,
            Color::FromRGB(40, 40, 40),
            false,
            1.2f
        );

        // label aligned below lamp
        float textSize = lampRadius;
        Point textPos = {
            lampPos.x,
            lampPos.y + lampRadius + textSize * 0.5f
        };
        Color textColor = m_peakActive
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RMS gives perceived loudness better than peak
    float GaugeRenderer::CalculateLoudness(const SpectrumData& spectrum) const {
        if (spectrum.empty())
            return DB_MIN;

        float sum = 0.0f;
        for (float val : spectrum)
            sum += val * val;

        float rms = std::sqrt(sum / spectrum.size());
        // prevent log of zero
        float db = 20.0f * std::log10(std::max(rms, 1e-10f));

        return Utils::Clamp(db, DB_MIN, DB_MAX);
    }

    // map db range to needle sweep angle
    float GaugeRenderer::DbToAngle(float db) const {
        float normalized = (Utils::Clamp(db, DB_MIN, DB_MAX) - DB_MIN) /
            (DB_MAX - DB_MIN);
        return ANGLE_START + normalized * ANGLE_RANGE;
    }

    // scale arc positioned in upper portion
    Point GaugeRenderer::GetScaleCenter(const Rect& rect) const {
        return {
            rect.x + rect.width * 0.5f,
            rect.y + rect.height * 0.5f + rect.height * 0.15f
        };
    }

    // needle pivot below scale for proper arc intersection
    Point GaugeRenderer::GetNeedleCenter(const Rect& rect) const {
        return {
            rect.x + rect.width * 0.5f,
            rect.y + rect.height * 0.5f +
                rect.height * (m_isOverlay ? 0.35f : 0.4f)
        };
    }

    // zero mark longer for visual reference
    float GaugeRenderer::GetTickLength(float value, bool isMajor) const {
        // use relative length to radiusY
        if (!isMajor)
            return (m_isOverlay ? 0.05f : 0.06f);

        if (value == 0.0f)
            return (m_isOverlay ? 0.12f : 0.15f);

        return (m_isOverlay ? 0.064f : 0.08f);
    }

} // namespace Spectrum