#ifndef SPECTRUM_CPP_GAUGE_RENDERER_H
#define SPECTRUM_CPP_GAUGE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GaugeRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class GaugeRenderer final : public BaseRenderer<GaugeRenderer> {
    public:
        GaugeRenderer() {
            m_db = kDbMin;
            m_angle = kAngleStart;
            UpdateSettings();
        }

        [[nodiscard]] std::string_view GetName() const override { return "Gauge"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::GaugeSettings>();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float) override {
            const float target = Loudness(spectrum);
            const float smoothing = (target > m_db)
                ? m_settings.smoothingFactorInc
                : m_settings.smoothingFactorDec;

            m_db = Lerp(m_db, target, IsOverlay() ? smoothing * 0.5f : smoothing);
            m_angle = Lerp(m_angle, DbToAngle(m_db), m_settings.riseSpeed);

            if (target >= kDbPeakThreshold) {
                m_peak = true;
                m_peakHold = kPeakHold;
            }
            else if (m_peakHold > 0) {
                --m_peakHold;
            }
            else {
                m_peak = false;
            }
        }

        void DoRender(BLContext& ctx, const SpectrumData&) override {
            const Rect rect = PaddedRect();
            if (!Helpers::Geometry::IsValid(rect)) return;
            DrawBackground(ctx, rect);
            DrawScale(ctx, rect);
            DrawNeedle(ctx, rect);
            DrawPeak(ctx, rect);
        }

    private:
        static constexpr float kDbMax = 5.0f;
        static constexpr float kDbMin = -30.0f;
        static constexpr float kDbPeakThreshold = 3.0f;
        static constexpr float kAngleStart = -150.0f;
        static constexpr float kAngleEnd = -30.0f;
        static constexpr int   kPeakHold = 15;

        struct Mark { float db; const char* label; };
        static constexpr Mark kMarks[] = {
            { -30.0f, "-30" }, { -20.0f, "-20" }, { -10.0f, "-10" },
            {  -7.0f,  "-7" }, {  -5.0f,  "-5" }, {  -3.0f,  "-3" },
            {   0.0f,   "0" }, {   3.0f,  "+3" }, {   5.0f,  "+5" }
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Draw
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawBackground(BLContext& ctx, const Rect& rect) const {
            using namespace Helpers::Geometry;
            Draw::FillRoundRect(ctx, rect, 8.0f, Color::FromRGB(80, 80, 80));

            const Rect inner = Deflate(rect, 4.0f);
            Draw::FillRoundRect(ctx, inner, 6.0f, Color::FromRGB(105, 105, 105));

            const Rect face = Deflate(inner, 4.0f);
            Draw::FillRect(ctx, face, Color::FromRGB(240, 240, 230));

            const float size = rect.height * 0.15f;
            Draw::FillTextCentered(
                ctx, "VU",
                CreateCentered(
                    { face.x + face.width * 0.5f, GetBottom(face) - size * 1.5f },
                    size * 2.0f, size * 1.5f),
                Color::Black(), size);
        }

        void DrawScale(BLContext& ctx, const Rect& rect) const {
            using namespace Helpers::Geometry;
            const Point center = Add(GetCenter(rect), { 0.0f, rect.height * 0.15f });
            const float rx = rect.width * (IsOverlay() ? 0.4f : 0.45f);
            const float ry = rect.height * (IsOverlay() ? 0.45f : 0.5f);

            for (const auto& mark : kMarks) {
                const float rad = DegreesToRadians(DbToAngle(mark.db));
                const float tick = (mark.db == 0.0f ? 0.15f : 0.08f) * ry;
                const Color color = mark.db >= 0.0f
                    ? Color::FromRGB(220, 0, 0)
                    : Color::FromRGB(80, 80, 80);

                Draw::StrokeLine(ctx,
                    PointOnEllipse(center, rx - tick, ry - tick, rad),
                    PointOnEllipse(center, rx, ry, rad),
                    color, 1.8f);

                const float textSize = rect.height
                    * (IsOverlay() ? 0.08f : 0.1f)
                    * (mark.db == 0.0f ? 1.15f : 1.0f);
                Draw::FillTextCentered(
                    ctx, mark.label,
                    CreateCentered(
                        PointOnEllipse(center, rx + ry * 0.12f, ry + ry * 0.12f, rad),
                        textSize * 3.0f, textSize * 1.5f),
                    mark.db >= 0.0f ? Color::FromRGB(200, 0, 0) : Color::Black(),
                    textSize);
            }
        }

        void DrawNeedle(BLContext& ctx, const Rect& rect) const {
            using namespace Helpers::Geometry;
            const Point center = Add(GetCenter(rect), {
                0.0f, rect.height * (IsOverlay() ? 0.35f : 0.4f)
                });
            const float length = std::min(rect.width, rect.height)
                * (IsOverlay() ? 0.64f : 0.7f);
            const std::vector<Point> needle = {
                { 0.0f, -length }, { -2.5f, 0.0f }, { 2.5f, 0.0f }
            };

            auto draw = [&]() {
                ctx.save();
                ctx.translate(center.x, center.y);
                ctx.rotate(DegreesToRadians(m_angle + 90.0f));
                Draw::FillPolygon(ctx, needle, Color::Black());
                ctx.restore();
                };

            if (GetQuality() != RenderQuality::Low) RenderWithShadow(ctx, draw);
            else draw();

            const float pivot = rect.width * (IsOverlay() ? 0.015f : 0.02f);
            Draw::FillCircle(ctx, center, pivot, Color::FromRGB(60, 60, 60));
            if (GetQuality() != RenderQuality::Low) {
                Draw::FillCircle(ctx,
                    Add(center, { -pivot * 0.25f, -pivot * 0.25f }),
                    pivot * 0.4f, Color(1.0f, 1.0f, 1.0f, 0.6f));
            }
        }

        void DrawPeak(BLContext& ctx, const Rect& rect) const {
            using namespace Helpers::Geometry;
            const float r = std::min(rect.width, rect.height) * (IsOverlay() ? 0.04f : 0.05f);
            const Point pos = Add(GetTopRight(rect), { -r * 2.5f, r * 2.5f });
            const Color lamp = m_peak ? Color::Red() : Color::FromRGB(180, 0, 0);

            if (m_peak && GetQuality() != RenderQuality::Low)
                Draw::Glow(ctx, pos, r * 2.0f, Color::Red(), 0.3f);

            Draw::FillCircle(ctx, pos, r * 0.8f, lamp);
            Draw::StrokeCircle(ctx, pos, r, Color::FromRGB(40, 40, 40), 1.2f);
            Draw::FillTextCentered(
                ctx, "PEAK",
                CreateCentered(Add(pos, { 0.0f, r * 1.5f }), r * 4.0f, r * 1.5f),
                lamp, r);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float Loudness(const SpectrumData& spectrum) const {
            if (spectrum.empty()) return kDbMin;
            float sum = 0.0f;
            for (float v : spectrum) sum += v * v;
            const float rms = std::sqrt(sum / static_cast<float>(spectrum.size()));
            return Clamp(20.0f * std::log10(std::max(rms, 1e-10f)), kDbMin, kDbMax);
        }

        [[nodiscard]] float DbToAngle(float db) const {
            return Map(Clamp(db, kDbMin, kDbMax), kDbMin, kDbMax, kAngleStart, kAngleEnd);
        }

        [[nodiscard]] Rect PaddedRect() const {
            using namespace Helpers::Geometry;
            const float vw = static_cast<float>(GetWidth());
            const float vh = static_cast<float>(GetHeight());
            if (vw <= 0.0f || vh <= 0.0f) return {};

            float w = vw * m_padding;
            float h = w / m_aspectRatio;
            if (h > vh * m_padding) {
                h = vh * m_padding;
                w = h * m_aspectRatio;
            }
            return CreateCentered(GetViewportCenter(), w, h);
        }

        Settings::GaugeSettings m_settings{};
        float m_aspectRatio = 2.0f;
        float m_padding = 0.8f;
        float m_db = 0.0f;
        float m_angle = 0.0f;
        int   m_peakHold = 0;
        bool  m_peak = false;
    };

} // namespace Spectrum

#endif