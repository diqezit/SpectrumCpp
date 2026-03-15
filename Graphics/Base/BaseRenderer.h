#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CRTP base for all visualizers.
// Provides viewport, color, animation, layout, and batch-rendering utils.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/IRenderer.h"
#include "Graphics/Base/PeakTracker.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Common/Common.h"
#include <optional>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>

namespace Spectrum {

    class Canvas;

    namespace Settings {
        template<typename T> struct QualityTraits;
    }

    template<typename Derived>
    class BaseRenderer : public IRenderer {
    public:
        static constexpr float kTimeResetThreshold = 1e6f;
        static constexpr float kDefaultFrameTime = 1.0f / 60.0f;

        enum class RoundingMode { None, All, Top, Bottom };

        BaseRenderer()
            : m_quality(RenderQuality::Medium)
            , m_primaryColor(Color::FromRGB(33, 150, 243))
            , m_isOverlay(false)
            , m_width(0), m_height(0)
            , m_aspectRatio(0.0f)
            , m_padding(1.0f)
            , m_time(0.0f)
        {
        }

        ~BaseRenderer() override = default;

        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer& operator=(const BaseRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // IRenderer overrides
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void SetQuality(RenderQuality q) override {
            if (m_quality == q) return;
            m_quality = q;
            UpdateSettings();
        }

        void SetPrimaryColor(const Color& c) override { m_primaryColor = c; }

        void SetOverlayMode(bool overlay) override {
            if (m_isOverlay == overlay) return;
            m_isOverlay = overlay;
            UpdateSettings();
        }

        void OnActivate(int w, int h) override {
            m_width = std::max(w, 0);
            m_height = std::max(h, 0);
        }

        void Render(Canvas& canvas, const SpectrumData& spectrum) override {
            if (spectrum.empty() || m_width <= 0 || m_height <= 0) return;
            m_time += kDefaultFrameTime;
            if (m_time > kTimeResetThreshold) m_time = 0.0f;
            UpdateAnimation(spectrum, kDefaultFrameTime);
            DoRender(canvas, spectrum);
        }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Hooks for derived
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        virtual void UpdateSettings() = 0;
        virtual void UpdateAnimation(const SpectrumData&, float) {}
        virtual void DoRender(Canvas& canvas, const SpectrumData& spectrum) = 0;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Quality settings helper
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        template<typename SettingsType>
        SettingsType GetQualitySettings() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Viewport
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] float GetTime()         const noexcept { return m_time; }
        [[nodiscard]] int   GetWidth()        const noexcept { return m_width; }
        [[nodiscard]] int   GetHeight()       const noexcept { return m_height; }
        [[nodiscard]] float GetMinDimension() const noexcept { return static_cast<float>(std::min(m_width, m_height)); }
        [[nodiscard]] float GetMaxDimension() const noexcept { return static_cast<float>(std::max(m_width, m_height)); }
        [[nodiscard]] float GetMaxRadius()    const noexcept { return GetMinDimension() * 0.45f; }

        [[nodiscard]] RenderQuality GetQuality()     const noexcept { return m_quality; }
        [[nodiscard]] bool          IsOverlay()      const noexcept { return m_isOverlay; }
        [[nodiscard]] Color         GetPrimaryColor() const noexcept { return m_primaryColor; }

        [[nodiscard]] Rect  GetViewportBounds() const noexcept {
            return { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height) };
        }

        [[nodiscard]] Point GetViewportCenter() const noexcept {
            return { m_width * 0.5f, m_height * 0.5f };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Grid layout
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct GridConfig {
            int   rows = 0;
            int   columns = 0;
            float cellSize = 0.0f;
            Point gridStart{};
        };

        [[nodiscard]] GridConfig CalculateGrid(
            size_t requiredColumns, float cellSize,
            int maxRows = 64, int maxColumns = 64) const
        {
            const float availW = m_isOverlay ? m_width * 0.95f : static_cast<float>(m_width);
            const float availH = m_isOverlay ? m_height * 0.95f : static_cast<float>(m_height);

            GridConfig g;
            g.columns = std::clamp(static_cast<int>(std::min(requiredColumns, static_cast<size_t>(availW / cellSize))), 1, maxColumns);
            g.rows = std::clamp(static_cast<int>(availH / cellSize), 1, maxRows);
            g.cellSize = std::min(availW / g.columns, availH / g.rows);

            const Point center = GetViewportCenter();
            g.gridStart = {
                center.x - g.columns * g.cellSize * 0.5f,
                center.y - g.rows * g.cellSize * 0.5f
            };
            return g;
        }

        [[nodiscard]] Point GetGridCellCenter(const GridConfig& g, int col, int row) const {
            const float half = g.cellSize * 0.5f;
            return { g.gridStart.x + col * g.cellSize + half,
                     g.gridStart.y + row * g.cellSize + half };
        }

        [[nodiscard]] size_t GetGridIndex(const GridConfig& g, int col, int row) const noexcept {
            return static_cast<size_t>(row) * g.columns + col;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Bar layout
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct BarLayout {
            float barWidth = 0.0f;
            float spacing = 0.0f;
            float totalBarWidth = 0.0f;
        };

        [[nodiscard]] BarLayout CalculateBarLayout(size_t count, float spacing) const {
            if (count == 0) return {};
            BarLayout l;
            l.spacing = spacing;
            l.totalBarWidth = static_cast<float>(m_width) / count;
            l.barWidth = std::max(0.0f, l.totalBarWidth - spacing);
            return l;
        }

        [[nodiscard]] Rect GetBarRect(const BarLayout& l, size_t i, float h, bool fromBottom = true) const {
            const float x = i * l.totalBarWidth + l.spacing * 0.5f;
            const float y = fromBottom ? static_cast<float>(m_height) - h : 0.0f;
            return { x, y, l.barWidth, h };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Geometry helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] Point GetPointOnCircle(const Point& c, float r, float angle) const {
            return { c.x + r * std::cos(angle), c.y + r * std::sin(angle) };
        }

        [[nodiscard]] std::vector<Point> GetCircularPoints(const Point& c, float r, size_t n) const {
            if (n == 0) return {};
            std::vector<Point> pts;
            pts.reserve(n);
            const float step = TWO_PI / n;
            for (size_t i = 0; i < n; ++i)
                pts.push_back(GetPointOnCircle(c, r, i * step));
            return pts;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Color helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] Color AdjustBrightness(const Color& c, float f) const {
            return { std::clamp(c.r * f, 0.0f, 1.0f),
                     std::clamp(c.g * f, 0.0f, 1.0f),
                     std::clamp(c.b * f, 0.0f, 1.0f), c.a };
        }

        [[nodiscard]] Color AdjustSaturation(const Color& c, float f) const {
            const float gray = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
            return { Helpers::Math::Lerp(gray, c.r, f),
                     Helpers::Math::Lerp(gray, c.g, f),
                     Helpers::Math::Lerp(gray, c.b, f), c.a };
        }

        [[nodiscard]] Color AdjustAlpha(const Color& c, float a) const {
            return { c.r, c.g, c.b, std::clamp(a, 0.0f, 1.0f) };
        }

        [[nodiscard]] Color InterpolateColors(const Color& a, const Color& b, float t) const {
            return { Helpers::Math::Lerp(a.r, b.r, t),
                     Helpers::Math::Lerp(a.g, b.g, t),
                     Helpers::Math::Lerp(a.b, b.b, t),
                     Helpers::Math::Lerp(a.a, b.a, t) };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Gradient
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        using ColorGradient = std::vector<Color>;

        [[nodiscard]] Color SampleGradient(const ColorGradient& g, float t) const {
            if (g.empty()) return {};
            if (g.size() == 1) return g[0];
            const float s = std::clamp(t, 0.0f, 1.0f) * (g.size() - 1);
            const size_t i = static_cast<size_t>(s);
            const size_t j = std::min(i + 1, g.size() - 1);
            return InterpolateColors(g[i], g[j], s - i);
        }

        [[nodiscard]] ColorGradient CreateGradient(const Color& a, const Color& b, size_t steps) const {
            if (steps == 0) return {};
            ColorGradient g;
            g.reserve(steps);
            for (size_t i = 0; i < steps; ++i) {
                const float t = steps > 1 ? static_cast<float>(i) / (steps - 1) : 0.0f;
                g.push_back(InterpolateColors(a, b, t));
            }
            return g;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Smoothing / easing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] float SmoothValue(float cur, float target, float attack = 0.4f, float decay = 0.85f) const {
            const float rate = (cur < target) ? attack : (1.0f - decay);
            return Helpers::Math::Lerp(cur, target, rate);
        }

        [[nodiscard]] std::vector<float> SmoothValues(
            const std::vector<float>& cur, const SpectrumData& target,
            float attack = 0.4f, float decay = 0.85f) const
        {
            std::vector<float> out = cur;
            const size_t n = std::min(cur.size(), target.size());
            for (size_t i = 0; i < n; ++i)
                out[i] = SmoothValue(cur[i], target[i], attack, decay);
            return out;
        }

        [[nodiscard]] float SmoothStep(float e0, float e1, float x) const {
            const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        [[nodiscard]] float EaseInOut(float t) const { return t * t * (3.0f - 2.0f * t); }
        [[nodiscard]] float EaseIn(float t)    const { return t * t; }
        [[nodiscard]] float EaseOut(float t)   const { return t * (2.0f - t); }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Drawing helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void DrawRoundedRect(
            Canvas& canvas, const Rect& rect, float radius,
            const Paint& paint, RoundingMode mode = RoundingMode::All) const
        {
            if (radius <= 0.0f || mode == RoundingMode::None) {
                canvas.DrawRectangle(rect, paint);
                return;
            }

            if (mode == RoundingMode::All) {
                canvas.DrawRoundedRectangle(rect, radius, paint);
                return;
            }

            if (rect.height < radius * 2.0f) {
                canvas.DrawRectangle(rect, paint);
                return;
            }

            if (mode == RoundingMode::Top) {
                canvas.DrawRectangle({ rect.x, rect.y + radius, rect.width, rect.height - radius }, paint);
                canvas.DrawRoundedRectangle({ rect.x, rect.y, rect.width, radius * 2.0f }, radius, paint);
            }
            else { // Bottom
                canvas.DrawRectangle({ rect.x, rect.y, rect.width, rect.height - radius }, paint);
                canvas.DrawRoundedRectangle({ rect.x, rect.y + rect.height - radius * 2.0f, rect.width, radius * 2.0f }, radius, paint);
            }
        }

        void RenderWithShadow(
            Canvas& canvas, const std::function<void()>& draw,
            const Point& offset = { 2.0f, 2.0f }, float alpha = 0.3f) const
        {
            canvas.DrawWithShadow(draw, offset, Color(0, 0, 0, alpha));
        }

        void RenderWithGlow(
            Canvas& canvas, const std::function<void()>& draw,
            const Point& center, float radius,
            const Color& color, float intensity = 0.8f) const
        {
            canvas.DrawGlow(center, radius, color, intensity);
            draw();
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Batch rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        using RectBatch = std::map<Color, std::vector<Rect>>;
        using PointBatch = std::map<Color, std::vector<Point>>;

        void RenderRectBatches(
            Canvas& canvas, const RectBatch& batches,
            float cornerRadius = 0.0f, RoundingMode mode = RoundingMode::All) const
        {
            for (const auto& [color, rects] : batches) {
                if (rects.empty()) continue;
                const Paint paint = Paint::Fill(color);
                if (cornerRadius > 0.0f || mode != RoundingMode::None) {
                    for (const auto& r : rects)
                        DrawRoundedRect(canvas, r, cornerRadius, paint, mode);
                }
                else {
                    canvas.DrawRectangleBatch(rects, paint);
                }
            }
        }

        void RenderCircleBatches(Canvas& canvas, const PointBatch& batches, float radius) const {
            for (const auto& [color, pts] : batches)
                if (!pts.empty())
                    canvas.DrawCircleBatch(pts, radius, Paint::Fill(color));
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Peak tracker
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void InitializePeakTracker(size_t size, float holdTime = 0.3f, float decayRate = 0.95f) {
            PeakTracker::Config cfg;
            cfg.holdTime = holdTime;
            cfg.decayRate = decayRate;
            m_peakTracker.emplace(size, cfg);
        }

        [[nodiscard]] bool               HasPeakTracker()  const { return m_peakTracker.has_value(); }
        [[nodiscard]] PeakTracker& GetPeakTracker() { return m_peakTracker.value(); }
        [[nodiscard]] const PeakTracker& GetPeakTracker()  const { return m_peakTracker.value(); }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Utilities
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] float MapToRange(float v, float inMin, float inMax, float outMin, float outMax) const {
            return Helpers::Math::Map(v, inMin, inMax, outMin, outMax);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // State
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        RenderQuality m_quality;
        Color         m_primaryColor;
        bool          m_isOverlay;
        int           m_width;
        int           m_height;
        float         m_aspectRatio;
        float         m_padding;
        mutable float m_time;

    private:
        std::optional<PeakTracker> m_peakTracker;
    };

} // namespace Spectrum

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GetQualitySettings — needs QualityPresets (included after full definition)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Visualizers/Settings/QualityPresets.h"

namespace Spectrum {

    template<typename Derived>
    template<typename SettingsType>
    SettingsType BaseRenderer<Derived>::GetQualitySettings() const {
        return QualityPresets::Get<Derived>(m_quality, m_isOverlay);
    }

} // namespace Spectrum

#endif