#ifndef SPECTRUM_CPP_BASE_RENDERER_H
#define SPECTRUM_CPP_BASE_RENDERER_H

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
    class BaseRenderer : public IRenderer
    {
    public:
        static inline constexpr float kTimeResetThreshold = 1e6f;
        static inline constexpr float kDefaultFrameTime = 1.0f / 60.0f;

        enum class RoundingMode {
            None,
            All,
            Top,
            Bottom
        };

        BaseRenderer();
        virtual ~BaseRenderer() = default;

        BaseRenderer(const BaseRenderer&) = delete;
        BaseRenderer& operator=(const BaseRenderer&) = delete;

        void SetQuality(RenderQuality quality) override;
        void SetPrimaryColor(const Color& color) override;
        void SetOverlayMode(bool isOverlay) override;
        void OnActivate(int width, int height) override;
        void Render(Canvas& canvas, const SpectrumData& spectrum) override;

    protected:
        virtual void UpdateSettings() = 0;
        virtual void UpdateAnimation(const SpectrumData&, float) {}
        virtual void DoRender(Canvas& canvas, const SpectrumData& spectrum) = 0;

        template<typename SettingsType>
        SettingsType GetQualitySettings() const;

        [[nodiscard]] bool IsRenderable(
            const SpectrumData& spectrum
        ) const noexcept;

        [[nodiscard]] float GetTime() const noexcept {
            return m_time;
        }

        [[nodiscard]] int GetWidth() const noexcept {
            return m_width;
        }

        [[nodiscard]] int GetHeight() const noexcept {
            return m_height;
        }

        [[nodiscard]] float GetMinDimension() const noexcept;
        [[nodiscard]] float GetMaxDimension() const noexcept;

        [[nodiscard]] RenderQuality GetQuality() const noexcept {
            return m_quality;
        }

        [[nodiscard]] bool IsOverlay() const noexcept {
            return m_isOverlay;
        }

        [[nodiscard]] Color GetPrimaryColor() const noexcept {
            return m_primaryColor;
        }

        [[nodiscard]] Rect GetViewportBounds() const noexcept;
        [[nodiscard]] Point GetViewportCenter() const noexcept;
        [[nodiscard]] float GetMaxRadius() const noexcept;

        struct GridConfig {
            int rows = 0;
            int columns = 0;
            float cellSize = 0.0f;
            Point gridStart = { 0.0f, 0.0f };
        };

        [[nodiscard]] GridConfig CalculateGrid(
            size_t requiredColumns,
            float cellSize,
            int maxRows = 64,
            int maxColumns = 64
        ) const;

        [[nodiscard]] Point GetGridCellCenter(
            const GridConfig& grid,
            int col,
            int row
        ) const;

        [[nodiscard]] size_t GetGridIndex(
            const GridConfig& grid,
            int col,
            int row
        ) const noexcept;

        struct BarLayout {
            float barWidth = 0.0f;
            float spacing = 0.0f;
            float totalBarWidth = 0.0f;
        };

        [[nodiscard]] BarLayout CalculateBarLayout(
            size_t barCount,
            float spacing
        ) const;

        [[nodiscard]] Rect GetBarRect(
            const BarLayout& layout,
            size_t index,
            float height,
            bool fromBottom = true
        ) const;

        [[nodiscard]] Point GetPointOnCircle(
            const Point& center,
            float radius,
            float angleRadians
        ) const;

        [[nodiscard]] std::vector<Point> GetCircularPoints(
            const Point& center,
            float radius,
            size_t count
        ) const;

        [[nodiscard]] Color AdjustBrightness(
            const Color& color,
            float factor
        ) const;

        [[nodiscard]] Color AdjustSaturation(
            const Color& color,
            float factor
        ) const;

        [[nodiscard]] Color AdjustAlpha(
            const Color& color,
            float alpha
        ) const;

        [[nodiscard]] Color InterpolateColors(
            const Color& a,
            const Color& b,
            float t
        ) const;

        using ColorGradient = std::vector<Color>;

        [[nodiscard]] Color SampleGradient(
            const ColorGradient& gradient,
            float t
        ) const;

        [[nodiscard]] ColorGradient CreateGradient(
            const Color& start,
            const Color& end,
            size_t steps
        ) const;

        [[nodiscard]] float SmoothValue(
            float current,
            float target,
            float attackRate = 0.4f,
            float decayRate = 0.85f
        ) const;

        [[nodiscard]] std::vector<float> SmoothValues(
            const std::vector<float>& current,
            const SpectrumData& target,
            float attackRate = 0.4f,
            float decayRate = 0.85f
        ) const;

        [[nodiscard]] float SmoothStep(
            float edge0,
            float edge1,
            float x
        ) const;

        [[nodiscard]] float EaseInOut(float t) const;
        [[nodiscard]] float EaseIn(float t) const;
        [[nodiscard]] float EaseOut(float t) const;

        void DrawRoundedRect(
            Canvas& canvas,
            const Rect& rect,
            float radius,
            const Paint& paint,
            RoundingMode mode = RoundingMode::All
        ) const;

        void RenderWithShadow(
            Canvas& canvas,
            const std::function<void()>& drawCall,
            const Point& offset = { 2.0f, 2.0f },
            float alpha = 0.3f
        ) const;

        void RenderWithGlow(
            Canvas& canvas,
            const std::function<void()>& drawCall,
            const Point& center,
            float radius,
            const Color& glowColor,
            float intensity = 0.8f
        ) const;

        using RectBatch = std::map<Color, std::vector<Rect>>;
        using PointBatch = std::map<Color, std::vector<Point>>;

        void RenderRectBatches(
            Canvas& canvas,
            const RectBatch& batches,
            float cornerRadius = 0.0f,
            RoundingMode mode = RoundingMode::All
        ) const;

        void RenderCircleBatches(
            Canvas& canvas,
            const PointBatch& batches,
            float radius
        ) const;

        void InitializePeakTracker(
            size_t size,
            float holdTime = 0.3f,
            float decayRate = 0.95f
        );

        [[nodiscard]] bool HasPeakTracker() const {
            return m_peakTracker.has_value();
        }

        [[nodiscard]] PeakTracker& GetPeakTracker() {
            return m_peakTracker.value();
        }

        [[nodiscard]] const PeakTracker& GetPeakTracker() const {
            return m_peakTracker.value();
        }

        [[nodiscard]] static PeakTracker::Config CreatePeakConfig(
            float holdTime,
            float decayRate = 0.95f,
            float minVisible = 0.01f
        );

        [[nodiscard]] float MapToRange(
            float value,
            float inMin,
            float inMax,
            float outMin,
            float outMax
        ) const;

        RenderQuality m_quality;
        Color m_primaryColor;
        bool m_isOverlay;
        int m_width;
        int m_height;
        float m_aspectRatio;
        float m_padding;
        mutable float m_time;

    private:
        void UpdateTime(float deltaTime) const;
        void SetViewport(int width, int height) noexcept;

        std::optional<PeakTracker> m_peakTracker;
    };

    template<typename Derived>
    BaseRenderer<Derived>::BaseRenderer()
        : m_quality(RenderQuality::Medium)
        , m_primaryColor(Color::FromRGB(33, 150, 243))
        , m_isOverlay(false)
        , m_width(0)
        , m_height(0)
        , m_aspectRatio(0.0f)
        , m_padding(1.0f)
        , m_time(0.0f)
    {
    }

    template<typename Derived>
    void BaseRenderer<Derived>::SetQuality(RenderQuality quality) {
        if (m_quality == quality) return;
        m_quality = quality;
        UpdateSettings();
    }

    template<typename Derived>
    void BaseRenderer<Derived>::SetOverlayMode(bool isOverlay) {
        if (m_isOverlay == isOverlay) return;
        m_isOverlay = isOverlay;
        UpdateSettings();
    }

    template<typename Derived>
    void BaseRenderer<Derived>::SetPrimaryColor(const Color& color) {
        m_primaryColor = color;
    }

    template<typename Derived>
    void BaseRenderer<Derived>::OnActivate(int width, int height) {
        SetViewport(width, height);
    }

    template<typename Derived>
    void BaseRenderer<Derived>::Render(
        Canvas& canvas,
        const SpectrumData& spectrum
    ) {
        if (!IsRenderable(spectrum)) return;
        UpdateTime(kDefaultFrameTime);
        UpdateAnimation(spectrum, kDefaultFrameTime);
        DoRender(canvas, spectrum);
    }

    template<typename Derived>
    bool BaseRenderer<Derived>::IsRenderable(
        const SpectrumData& spectrum
    ) const noexcept {
        return !spectrum.empty() && m_width > 0 && m_height > 0;
    }

    template<typename Derived>
    void BaseRenderer<Derived>::UpdateTime(float deltaTime) const {
        m_time += deltaTime;
        if (m_time > kTimeResetThreshold) {
            m_time = 0.0f;
        }
    }

    template<typename Derived>
    void BaseRenderer<Derived>::SetViewport(
        int width,
        int height
    ) noexcept {
        m_width = std::max(width, 0);
        m_height = std::max(height, 0);
    }

    template<typename Derived>
    float BaseRenderer<Derived>::GetMinDimension() const noexcept {
        return static_cast<float>(std::min(m_width, m_height));
    }

    template<typename Derived>
    float BaseRenderer<Derived>::GetMaxDimension() const noexcept {
        return static_cast<float>(std::max(m_width, m_height));
    }

    template<typename Derived>
    Rect BaseRenderer<Derived>::GetViewportBounds() const noexcept {
        return Rect{
            0.0f,
            0.0f,
            static_cast<float>(m_width),
            static_cast<float>(m_height)
        };
    }

    template<typename Derived>
    Point BaseRenderer<Derived>::GetViewportCenter() const noexcept {
        return Point{ m_width * 0.5f, m_height * 0.5f };
    }

    template<typename Derived>
    float BaseRenderer<Derived>::GetMaxRadius() const noexcept {
        return GetMinDimension() * 0.45f;
    }

    template<typename Derived>
    typename BaseRenderer<Derived>::GridConfig
        BaseRenderer<Derived>::CalculateGrid(
            size_t requiredColumns,
            float cellSize,
            int maxRows,
            int maxColumns
        ) const {
        const float availableWidth = m_isOverlay
            ? m_width * 0.95f
            : static_cast<float>(m_width);

        const float availableHeight = m_isOverlay
            ? m_height * 0.95f
            : static_cast<float>(m_height);

        GridConfig grid;
        grid.columns = std::clamp(
            static_cast<int>(std::min(
                requiredColumns,
                static_cast<size_t>(availableWidth / cellSize)
            )),
            1,
            maxColumns
        );

        grid.rows = std::clamp(
            static_cast<int>(availableHeight / cellSize),
            1,
            maxRows
        );

        grid.cellSize = std::min(
            availableWidth / grid.columns,
            availableHeight / grid.rows
        );

        const float gridWidth = grid.columns * grid.cellSize;
        const float gridHeight = grid.rows * grid.cellSize;
        const Point center = GetViewportCenter();

        grid.gridStart = {
            center.x - gridWidth * 0.5f,
            center.y - gridHeight * 0.5f
        };

        return grid;
    }

    template<typename Derived>
    Point BaseRenderer<Derived>::GetGridCellCenter(
        const GridConfig& grid,
        int col,
        int row
    ) const {
        const float halfCell = grid.cellSize * 0.5f;
        return {
            grid.gridStart.x + col * grid.cellSize + halfCell,
            grid.gridStart.y + row * grid.cellSize + halfCell
        };
    }

    template<typename Derived>
    size_t BaseRenderer<Derived>::GetGridIndex(
        const GridConfig& grid,
        int col,
        int row
    ) const noexcept {
        return static_cast<size_t>(row) * grid.columns + col;
    }

    template<typename Derived>
    typename BaseRenderer<Derived>::BarLayout
        BaseRenderer<Derived>::CalculateBarLayout(
            size_t barCount,
            float spacing
        ) const {
        if (barCount == 0) return {};

        BarLayout layout;
        layout.spacing = spacing;
        layout.totalBarWidth = static_cast<float>(m_width) / barCount;
        layout.barWidth = std::max(0.0f, layout.totalBarWidth - spacing);

        return layout;
    }

    template<typename Derived>
    Rect BaseRenderer<Derived>::GetBarRect(
        const BarLayout& layout,
        size_t index,
        float height,
        bool fromBottom
    ) const {
        const float x = index * layout.totalBarWidth + layout.spacing * 0.5f;
        const float y = fromBottom
            ? static_cast<float>(m_height) - height
            : 0.0f;

        return { x, y, layout.barWidth, height };
    }

    template<typename Derived>
    Point BaseRenderer<Derived>::GetPointOnCircle(
        const Point& center,
        float radius,
        float angleRadians
    ) const {
        return {
            center.x + radius * std::cos(angleRadians),
            center.y + radius * std::sin(angleRadians)
        };
    }

    template<typename Derived>
    std::vector<Point> BaseRenderer<Derived>::GetCircularPoints(
        const Point& center,
        float radius,
        size_t count
    ) const {
        if (count == 0) return {};

        std::vector<Point> points;
        points.reserve(count);

        const float angleStep = TWO_PI / count;
        for (size_t i = 0; i < count; ++i) {
            points.push_back(
                GetPointOnCircle(center, radius, i * angleStep)
            );
        }

        return points;
    }

    template<typename Derived>
    Color BaseRenderer<Derived>::AdjustBrightness(
        const Color& color,
        float factor
    ) const {
        return Color{
            std::clamp(color.r * factor, 0.0f, 1.0f),
            std::clamp(color.g * factor, 0.0f, 1.0f),
            std::clamp(color.b * factor, 0.0f, 1.0f),
            color.a
        };
    }

    template<typename Derived>
    Color BaseRenderer<Derived>::AdjustSaturation(
        const Color& color,
        float factor
    ) const {
        const float gray = color.r * 0.299f +
            color.g * 0.587f +
            color.b * 0.114f;

        return Color{
            Helpers::Math::Lerp(gray, color.r, factor),
            Helpers::Math::Lerp(gray, color.g, factor),
            Helpers::Math::Lerp(gray, color.b, factor),
            color.a
        };
    }

    template<typename Derived>
    Color BaseRenderer<Derived>::AdjustAlpha(
        const Color& color,
        float alpha
    ) const {
        return Color{
            color.r,
            color.g,
            color.b,
            std::clamp(alpha, 0.0f, 1.0f)
        };
    }

    template<typename Derived>
    Color BaseRenderer<Derived>::InterpolateColors(
        const Color& a,
        const Color& b,
        float t
    ) const {
        return Color{
            Helpers::Math::Lerp(a.r, b.r, t),
            Helpers::Math::Lerp(a.g, b.g, t),
            Helpers::Math::Lerp(a.b, b.b, t),
            Helpers::Math::Lerp(a.a, b.a, t)
        };
    }

    template<typename Derived>
    Color BaseRenderer<Derived>::SampleGradient(
        const ColorGradient& gradient,
        float t
    ) const {
        if (gradient.empty()) return Color{};
        if (gradient.size() == 1) return gradient[0];

        const float scaledT = std::clamp(t, 0.0f, 1.0f) *
            (gradient.size() - 1);
        const size_t i1 = static_cast<size_t>(scaledT);
        const size_t i2 = std::min(i1 + 1, gradient.size() - 1);
        const float fraction = scaledT - i1;

        return InterpolateColors(gradient[i1], gradient[i2], fraction);
    }

    template<typename Derived>
    typename BaseRenderer<Derived>::ColorGradient
        BaseRenderer<Derived>::CreateGradient(
            const Color& start,
            const Color& end,
            size_t steps
        ) const {
        if (steps == 0) return {};

        ColorGradient gradient;
        gradient.reserve(steps);

        for (size_t i = 0; i < steps; ++i) {
            const float t = steps > 1
                ? static_cast<float>(i) / (steps - 1)
                : 0.0f;
            gradient.push_back(InterpolateColors(start, end, t));
        }

        return gradient;
    }

    template<typename Derived>
    float BaseRenderer<Derived>::SmoothValue(
        float current,
        float target,
        float attackRate,
        float decayRate
    ) const {
        const float rate = (current < target)
            ? attackRate
            : (1.0f - decayRate);
        return Helpers::Math::Lerp(current, target, rate);
    }

    template<typename Derived>
    std::vector<float> BaseRenderer<Derived>::SmoothValues(
        const std::vector<float>& current,
        const SpectrumData& target,
        float attackRate,
        float decayRate
    ) const {
        std::vector<float> result = current;
        const size_t count = std::min(current.size(), target.size());

        for (size_t i = 0; i < count; ++i) {
            result[i] = SmoothValue(
                current[i],
                target[i],
                attackRate,
                decayRate
            );
        }

        return result;
    }

    template<typename Derived>
    float BaseRenderer<Derived>::SmoothStep(
        float edge0,
        float edge1,
        float x
    ) const {
        const float t = std::clamp(
            (x - edge0) / (edge1 - edge0),
            0.0f,
            1.0f
        );
        return t * t * (3.0f - 2.0f * t);
    }

    template<typename Derived>
    float BaseRenderer<Derived>::EaseInOut(float t) const {
        return t * t * (3.0f - 2.0f * t);
    }

    template<typename Derived>
    float BaseRenderer<Derived>::EaseIn(float t) const {
        return t * t;
    }

    template<typename Derived>
    float BaseRenderer<Derived>::EaseOut(float t) const {
        return t * (2.0f - t);
    }

    template<typename Derived>
    void BaseRenderer<Derived>::DrawRoundedRect(
        Canvas& canvas,
        const Rect& rect,
        float radius,
        const Paint& paint,
        RoundingMode mode
    ) const {
        if (radius <= 0.0f || mode == RoundingMode::None) {
            canvas.DrawRectangle(rect, paint);
            return;
        }

        switch (mode) {
        case RoundingMode::All:
            canvas.DrawRoundedRectangle(rect, radius, paint);
            break;

        case RoundingMode::Top: {
            if (rect.height < radius * 2.0f) {
                canvas.DrawRectangle(rect, paint);
                return;
            }

            const Rect bottomRect{
                rect.x,
                rect.y + radius,
                rect.width,
                rect.height - radius
            };
            canvas.DrawRectangle(bottomRect, paint);

            const Rect topRect{
                rect.x,
                rect.y,
                rect.width,
                radius * 2.0f
            };
            canvas.DrawRoundedRectangle(topRect, radius, paint);
            break;
        }

        case RoundingMode::Bottom: {
            if (rect.height < radius * 2.0f) {
                canvas.DrawRectangle(rect, paint);
                return;
            }

            const Rect topRect{
                rect.x,
                rect.y,
                rect.width,
                rect.height - radius
            };
            canvas.DrawRectangle(topRect, paint);

            const Rect bottomRect{
                rect.x,
                rect.y + rect.height - radius * 2.0f,
                rect.width,
                radius * 2.0f
            };
            canvas.DrawRoundedRectangle(bottomRect, radius, paint);
            break;
        }

        default:
            canvas.DrawRectangle(rect, paint);
            break;
        }
    }

    template<typename Derived>
    void BaseRenderer<Derived>::RenderWithShadow(
        Canvas& canvas,
        const std::function<void()>& drawCall,
        const Point& offset,
        float alpha
    ) const {
        canvas.DrawWithShadow(
            drawCall,
            offset,
            Color(0.0f, 0.0f, 0.0f, alpha)
        );
    }

    template<typename Derived>
    void BaseRenderer<Derived>::RenderWithGlow(
        Canvas& canvas,
        const std::function<void()>& drawCall,
        const Point& center,
        float radius,
        const Color& glowColor,
        float intensity
    ) const {
        canvas.DrawGlow(center, radius, glowColor, intensity);
        drawCall();
    }

    template<typename Derived>
    void BaseRenderer<Derived>::RenderRectBatches(
        Canvas& canvas,
        const RectBatch& batches,
        float cornerRadius,
        RoundingMode mode
    ) const {
        for (const auto& [color, rects] : batches) {
            if (rects.empty()) continue;

            const Paint paint = Paint::Fill(color);

            if (cornerRadius > 0.0f || mode != RoundingMode::None) {
                for (const auto& rect : rects) {
                    DrawRoundedRect(canvas, rect, cornerRadius, paint, mode);
                }
            }
            else {
                canvas.DrawRectangleBatch(rects, paint);
            }
        }
    }

    template<typename Derived>
    void BaseRenderer<Derived>::RenderCircleBatches(
        Canvas& canvas,
        const PointBatch& batches,
        float radius
    ) const {
        for (const auto& [color, points] : batches) {
            if (!points.empty()) {
                canvas.DrawCircleBatch(
                    points,
                    radius,
                    Paint::Fill(color)
                );
            }
        }
    }

    template<typename Derived>
    void BaseRenderer<Derived>::InitializePeakTracker(
        size_t size,
        float holdTime,
        float decayRate
    ) {
        m_peakTracker.emplace(
            size,
            CreatePeakConfig(holdTime, decayRate)
        );
    }

    template<typename Derived>
    PeakTracker::Config BaseRenderer<Derived>::CreatePeakConfig(
        float holdTime,
        float decayRate,
        float minVisible
    ) {
        PeakTracker::Config config;
        config.holdTime = holdTime;
        config.decayRate = decayRate;
        config.minVisible = minVisible;
        return config;
    }

    template<typename Derived>
    float BaseRenderer<Derived>::MapToRange(
        float value,
        float inMin,
        float inMax,
        float outMin,
        float outMax
    ) const {
        return Helpers::Math::Map(
            value,
            inMin,
            inMax,
            outMin,
            outMax
        );
    }

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