#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BaseRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/PeakTracker.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/API/Draw.h"
#include "Common/Common.h"

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

namespace Spectrum {

    class Renderer {
    public:
        virtual ~Renderer() = default;

        virtual void OnActivate(int width, int height) = 0;
        virtual void OnDeactivate() = 0;
        virtual void OnResize(int width, int height) = 0;

        virtual void SetQuality(RenderQuality quality) = 0;
        virtual void SetPrimaryColor(const Color& color) = 0;
        virtual void SetOverlayMode(bool overlay) = 0;

        virtual void Render(BLContext& ctx, const SpectrumData& spectrum) = 0;
        [[nodiscard]] virtual std::string_view GetName() const = 0;
    };

    template<typename Derived>
    class BaseRenderer : public Renderer {
    public:
        static constexpr float kTimeResetThreshold = 1e6f;
        static constexpr float kDefaultFrameTime = 1.0f / 60.0f;
        static constexpr float kOverlayScale = 0.95f;

        enum class RoundingMode { None, All, Top, Bottom };

        BaseRenderer() = default;
        ~BaseRenderer() override = default;

        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer& operator=(const BaseRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Renderer
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

        void OnActivate(int w, int h) override { OnResize(w, h); }
        void OnDeactivate() override {}

        void OnResize(int w, int h) override {
            m_width = w;
            m_height = h;
        }

        void Render(BLContext& ctx, const SpectrumData& spectrum) override {
            m_time += kDefaultFrameTime;
            if (m_time > kTimeResetThreshold) m_time = 0.0f;
            UpdateAnimation(spectrum, kDefaultFrameTime);
            DoRender(ctx, spectrum);
        }

    protected:
        virtual void UpdateSettings() = 0;
        virtual void UpdateAnimation(const SpectrumData&, float) {}
        virtual void DoRender(BLContext& ctx, const SpectrumData& spectrum) = 0;

        template<typename SettingsType>
        SettingsType GetQualitySettings() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Viewport
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetTime() const noexcept { return m_time; }
        [[nodiscard]] int   GetWidth() const noexcept { return m_width; }
        [[nodiscard]] int   GetHeight() const noexcept { return m_height; }
        [[nodiscard]] RenderQuality GetQuality() const noexcept { return m_quality; }
        [[nodiscard]] bool  IsOverlay() const noexcept { return m_isOverlay; }
        [[nodiscard]] Color GetPrimaryColor() const noexcept { return m_primaryColor; }

        [[nodiscard]] float GetMinDimension() const noexcept {
            return float(std::min(m_width, m_height));
        }

        [[nodiscard]] float GetMaxRadius() const noexcept {
            return GetMinDimension() * 0.45f;
        }

        [[nodiscard]] Rect GetViewportBounds() const noexcept {
            return Helpers::Geometry::CreateViewportBounds(m_width, m_height);
        }

        [[nodiscard]] Point GetViewportCenter() const noexcept {
            return Helpers::Geometry::GetViewportCenter(m_width, m_height);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Grid
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct GridConfig {
            int rows = 0;
            int columns = 0;
            float cellSize = 0.0f;
            Point gridStart{};
        };

        [[nodiscard]] GridConfig CalculateGrid(
            size_t requiredColumns,
            float cellSize,
            int maxRows = 64,
            int maxColumns = 64) const
        {
            const float scale = m_isOverlay ? kOverlayScale : 1.0f;
            const float availW = float(m_width) * scale;
            const float availH = float(m_height) * scale;

            GridConfig g;
            g.columns = std::clamp(int(std::min(requiredColumns, size_t(availW / cellSize))), 1, maxColumns);
            g.rows = std::clamp(int(availH / cellSize), 1, maxRows);
            g.cellSize = std::min(availW / g.columns, availH / g.rows);

            const Point c = GetViewportCenter();
            g.gridStart = {
                c.x - g.columns * g.cellSize * 0.5f,
                c.y - g.rows * g.cellSize * 0.5f
            };
            return g;
        }

        [[nodiscard]] Point GetGridCellCenter(const GridConfig& g, int col, int row) const {
            const float half = g.cellSize * 0.5f;
            return {
                g.gridStart.x + col * g.cellSize + half,
                g.gridStart.y + row * g.cellSize + half
            };
        }

        bool SyncGrid(GridConfig& grid, size_t columns, float cellSize, int maxRows = 64) {
            const GridConfig next = CalculateGrid(columns, cellSize, maxRows);
            if (next.columns == grid.columns && next.rows == grid.rows) return false;
            grid = next;
            if (HasPeakTracker())
                GetPeakTracker().Resize(size_t(grid.columns));
            return true;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Bars
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct BarLayout {
            float barWidth = 0.0f;
            float spacing = 0.0f;
            float totalBarWidth = 0.0f;
        };

        [[nodiscard]] BarLayout CalculateBarLayout(size_t count, float spacing) const {
            BarLayout layout;
            layout.spacing = spacing;
            layout.totalBarWidth = float(m_width) / count;
            layout.barWidth = std::max(0.0f, layout.totalBarWidth - spacing);
            return layout;
        }

        [[nodiscard]] Rect GetBarRect(
            const BarLayout& layout, size_t i, float h, bool fromBottom = true) const
        {
            return {
                i * layout.totalBarWidth + layout.spacing * 0.5f,
                fromBottom ? float(m_height) - h : 0.0f,
                layout.barWidth,
                h
            };
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Circles / Color
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::vector<Point> GetCircularPoints(
            const Point& c, float r, size_t n) const
        {
            std::vector<Point> pts;
            pts.reserve(n);
            const float step = Helpers::Constants::kTwoPi / float(n);
            for (size_t i = 0; i < n; ++i)
                pts.push_back(PointOnCircle(c, r, i * step));
            return pts;
        }

        using ColorGradient = std::vector<Color>;

        [[nodiscard]] Color SampleGradient(const ColorGradient& g, float t) const {
            const float s = std::clamp(t, 0.0f, 1.0f) * (g.size() - 1);
            const size_t i = size_t(s);
            return InterpolateColor(g[i], g[std::min(i + 1, g.size() - 1)], s - i);
        }

        [[nodiscard]] float SmoothValue(
            float cur, float target, float attack = 0.4f, float decay = 0.85f) const
        {
            return Lerp(cur, target, cur < target ? attack : (1.0f - decay));
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Draw
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawRoundedRect(
            BLContext& ctx,
            const Rect& r,
            float radius,
            const Color& color,
            RoundingMode mode = RoundingMode::All) const
        {
            if (radius <= 0.0f || mode == RoundingMode::None) {
                Draw::FillRect(ctx, r, color);
                return;
            }
            if (mode == RoundingMode::All) {
                Draw::FillRoundRect(ctx, r, radius, color);
                return;
            }
            if (r.height < radius * 2.0f) {
                Draw::FillRect(ctx, r, color);
                return;
            }

            const bool top = (mode == RoundingMode::Top);
            Draw::FillRect(ctx, {
                r.x,
                top ? r.y + radius : r.y,
                r.width,
                r.height - radius
                }, color);
            Draw::FillRoundRect(ctx, {
                r.x,
                top ? r.y : r.y + r.height - radius * 2.0f,
                r.width,
                radius * 2.0f
                }, radius, color);
        }

        void RenderWithShadow(
            BLContext& ctx,
            const std::function<void()>& draw,
            const Point& offset = { 2.0f, 2.0f },
            float alpha = 0.3f) const
        {
            ctx.save();
            ctx.translate(offset.x, offset.y);
            ctx.set_global_alpha(double(alpha));
            draw();
            ctx.restore();
            draw();
        }

        using RectBatch = std::map<Color, std::vector<Rect>>;
        using PointBatch = std::map<Color, std::vector<Point>>;

        void RenderRectBatches(
            BLContext& ctx,
            const RectBatch& batches,
            float cornerRadius = 0.0f,
            RoundingMode mode = RoundingMode::All) const
        {
            for (const auto& [color, rects] : batches)
                for (const auto& r : rects)
                    DrawRoundedRect(ctx, r, cornerRadius, color, mode);
        }

        void RenderCircleBatches(BLContext& ctx, const PointBatch& batches, float radius) const {
            for (const auto& [color, pts] : batches)
                for (const auto& p : pts)
                    Draw::FillCircle(ctx, p, radius, color);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Peak tracker
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void InitializePeakTracker(size_t size, float holdTime = 0.3f, float decayRate = 0.95f) {
            PeakTracker::Config cfg;
            cfg.holdTime = holdTime;
            cfg.decayRate = decayRate;
            m_peakTracker.emplace(size, cfg);
        }

        [[nodiscard]] bool HasPeakTracker() const { return m_peakTracker.has_value(); }
        [[nodiscard]] PeakTracker& GetPeakTracker() { return m_peakTracker.value(); }
        [[nodiscard]] const PeakTracker& GetPeakTracker() const { return m_peakTracker.value(); }

        RenderQuality m_quality = RenderQuality::Medium;
        Color         m_primaryColor = Color::FromRGB(33, 150, 243);
        bool          m_isOverlay = false;
        int           m_width = 0;
        int           m_height = 0;
        float         m_time = 0.0f;

    private:
        std::optional<PeakTracker> m_peakTracker;
    };

} // namespace Spectrum

#include "Graphics/Visualizers/Settings/QualityPresets.h"

namespace Spectrum {

    template<typename Derived>
    template<typename SettingsType>
    SettingsType BaseRenderer<Derived>::GetQualitySettings() const {
        return QualityPresets::Get<Derived>(m_quality, m_isOverlay);
    }

} // namespace Spectrum

#endif