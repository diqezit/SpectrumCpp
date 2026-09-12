#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BaseRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/PeakTracker.h"
#include "Graphics/Base/RenderUtils.h"
#include "Graphics/Visualizers/Settings/QualityPresets.h"

#include <algorithm>
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
        static constexpr float kDefaultFrameTime = 1.0f / 60.0f;
        static constexpr float kOverlayScale = 0.95f;

        BaseRenderer() = default;
        ~BaseRenderer() override = default;

        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer& operator=(const BaseRenderer&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Renderer
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetQuality(RenderQuality q) override {
            if (m_quality == q)
                return;
            m_quality = q;
            UpdateSettings();
        }

        void SetPrimaryColor(const Color& c) override { m_primaryColor = c; }

        void SetOverlayMode(bool overlay) override {
            if (m_isOverlay == overlay)
                return;
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
            UpdateAnimation(spectrum, kDefaultFrameTime);
            DoRender(ctx, spectrum);
        }

    protected:
        using SettingsType = decltype(QualityPresets::Get<Derived>(RenderQuality::Medium, false));

        virtual void UpdateSettings() {
            m_settings = QualityPresets::Get<Derived>(m_quality, m_isOverlay);
            OnSettingsUpdated();
        }

        virtual void OnSettingsUpdated() {}
        virtual void UpdateAnimation(const SpectrumData&, float) {}
        virtual void DoRender(BLContext& ctx, const SpectrumData& spectrum) = 0;

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
            float cellW = 0.0f;
            float cellH = 0.0f;
        };

        [[nodiscard]] Point GetGridCellCenter(const GridConfig& g, int col, int row) const {
            return { (col + 0.5f) * g.cellW, (row + 0.5f) * g.cellH };
        }

        [[nodiscard]] Rect GetGridCellRect(const GridConfig& g, int col, int row, float margin) const {
            return CreateCentered(GetGridCellCenter(g, col, row), g.cellW - margin, g.cellH - margin);
        }

        void SyncGrid(GridConfig& g, size_t columns, int rows) {
            const int c = int(columns);
            if (c != g.columns && HasPeakTracker())
                GetPeakTracker().Resize(columns);
            g = { rows, c, float(m_width) / c, float(m_height) / rows };
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
            const float total = float(m_width) / count;
            return { std::max(0.0f, total - spacing), spacing, total };
        }

        [[nodiscard]] Rect GetBarRect(
            const BarLayout& layout, size_t i, float h, bool fromBottom = true) const
        {
            float y = 0.0f;
            if (fromBottom)
                y = float(m_height) - h;
            return {
                i * layout.totalBarWidth + layout.spacing * 0.5f,
                y,
                layout.barWidth,
                h
            };
        }

        template<typename Fn>
        bool ForEachBar(
            const SpectrumData& spectrum,
            float spacing,
            float heightScale,
            float minHeight,
            Fn&& fn) const
        {
            const auto layout = CalculateBarLayout(spectrum.size(), spacing);
            if (layout.barWidth <= 0.0f)
                return false;

            for (size_t i = 0; i < spectrum.size(); ++i) {
                const float mag = Normalized(spectrum[i]);
                const float height = RenderUtils::MagnitudeToHeight(
                    mag, float(GetHeight()), heightScale);
                if (height < minHeight)
                    continue;
                fn(i, mag, GetBarRect(layout, i, height), layout);
            }
            return true;
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

        SettingsType  m_settings{};
        RenderQuality m_quality = RenderQuality::Medium;
        Color         m_primaryColor = DEFAULT_PRIMARY_COLOR;
        bool          m_isOverlay = false;
        int           m_width = 0;
        int           m_height = 0;
        float         m_time = 0.0f;

    private:
        std::optional<PeakTracker> m_peakTracker;
    };

} // namespace Spectrum

#endif