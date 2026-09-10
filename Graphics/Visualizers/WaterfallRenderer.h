#ifndef SPECTRUM_CPP_WATERFALL_RENDERER_H
#define SPECTRUM_CPP_WATERFALL_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// WaterfallRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

#include <array>
#include <cmath>
#include <vector>

namespace Spectrum {

    class WaterfallRenderer final : public BaseRenderer<WaterfallRenderer> {
    public:
        WaterfallRenderer() { UpdateSettings(); }

        [[nodiscard]] std::string_view GetName() const override { return "Waterfall"; }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::WaterfallSettings>();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float dt) override {
            m_swayPhase = std::fmod(
                m_swayPhase + kSwaySpeed * dt,
                Helpers::Constants::kTwoPi);
            if (spectrum.empty()) return;

            m_sliceTimer += dt;
            if (m_sliceTimer < kSliceInterval) return;
            m_sliceTimer -= kSliceInterval;

            RenderUtils::ResampleSpectrum(
                spectrum,
                m_buffer.data() + m_head * kSampleCount,
                kSampleCount,
                kAmplitudeScale);

            m_head = (m_head + 1) % kHistory;
            if (m_count < kHistory)
                ++m_count;
        }

        void DoRender(BLContext& ctx, const SpectrumData& spectrum) override {
            if (m_count == 0 && spectrum.empty()) return;

            const Camera cam = MakeCamera();
            const float frac = Saturate(m_sliceTimer / kSliceInterval);

            size_t oldest = 0;
            if (m_count == kHistory)
                oldest = m_head;

            for (size_t n = 0; n < m_count; ++n) {
                DrawSlice(
                    ctx,
                    cam,
                    m_buffer.data() + ((oldest + n) % kHistory) * kSampleCount,
                    static_cast<float>(m_count - 1 - n) + frac);
            }

            if (spectrum.empty()) return;

            std::array<float, kSampleCount> live{};
            RenderUtils::ResampleSpectrum(spectrum, live.data(), kSampleCount, kAmplitudeScale);
            DrawSlice(ctx, cam, live.data(), 0.0f);
        }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        static constexpr float kXSpan = 0.72f;
        static constexpr float kZSpan = 0.72f;
        static constexpr float kFixedRotX = 0.62f;
        static constexpr float kBaseRotY = 0.55f;
        static constexpr float kSwayAmplitude = 0.30f;
        static constexpr float kSwaySpeed = 0.25f;
        static constexpr float kSliceInterval = 0.07f;
        static constexpr float kAmplitudeScale = 0.45f;
        static constexpr float kVerticalOffset = 0.06f;
        static constexpr float kPerspNumerator = 1.7f;
        static constexpr float kPerspDenominator = 2.4f;
        static constexpr Color kFillColor = Color::FromRGB(13, 13, 26);
        static constexpr float kFillAlpha = 0.16f;
        static constexpr float kFadeSlices = 12.0f;
        static constexpr size_t kHistory = 63;
        static constexpr size_t kSampleCount = 96;

        struct Camera {
            float cy = 1.0f;
            float sy = 0.0f;
            float cx = 1.0f;
            float sx = 0.0f;
            float scale = 1.0f;
            float originX = 0.0f;
            float originY = 0.0f;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Camera
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] Camera MakeCamera() const {
            const float yaw = kBaseRotY + kSwayAmplitude * std::sin(m_swayPhase);
            const Point center = GetViewportCenter();
            return {
                std::cos(yaw),
                std::sin(yaw),
                std::cos(kFixedRotX),
                std::sin(kFixedRotX),
                GetMinDimension() * m_settings.perspectiveDepth,
                center.x,
                center.y + static_cast<float>(GetHeight()) * kVerticalOffset
            };
        }

        [[nodiscard]] static Point Project(const Camera& cam, float x, float y, float z) {
            const float yawX = x * cam.cy - z * cam.sy;
            const float yawZ = x * cam.sy + z * cam.cy;
            const float pitchY = y * cam.cx - yawZ * cam.sx;
            const float depth = y * cam.sx + yawZ * cam.cx;
            const float s = cam.scale * kPerspNumerator / (kPerspDenominator + depth);
            return { cam.originX + yawX * s, cam.originY + pitchY * s };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawSlice(
            BLContext& ctx,
            const Camera& cam,
            const float* samples,
            float fromFront)
        {
            const float history = static_cast<float>(kHistory);
            const float fade = Saturate((history - fromFront) / kFadeSlices);
            if (fade <= 0.0f) return;

            const float depth = Saturate(1.0f - fromFront / history);
            const float z = kZSpan * (1.0f - 2.0f * depth);
            const float step = (kXSpan * 2.0f) / static_cast<float>(kSampleCount - 1);

            m_fill.clear();
            m_line.clear();
            m_fill.push_back(Project(cam, -kXSpan, 0.0f, z));
            for (size_t i = 0; i < kSampleCount; ++i) {
                const Point p = Project(
                    cam,
                    -kXSpan + static_cast<float>(i) * step,
                    -samples[i],
                    z);
                m_fill.push_back(p);
                m_line.push_back(p);
            }
            m_fill.push_back(Project(cam, kXSpan, 0.0f, z));

            Draw::FillPolygon(ctx, m_fill, AdjustAlpha(kFillColor, kFillAlpha * fade));
            Draw::StrokePolyline(
                ctx,
                m_line,
                AdjustAlpha(GetPrimaryColor(), fade),
                m_settings.lineWidth);
        }

        Settings::WaterfallSettings m_settings{};
        std::array<float, kHistory* kSampleCount> m_buffer{};
        std::vector<Point> m_fill;
        std::vector<Point> m_line;
        size_t m_head = 0;
        size_t m_count = 0;
        float m_sliceTimer = 0.0f;
        float m_swayPhase = 0.0f;
    };

} // namespace Spectrum

#endif
