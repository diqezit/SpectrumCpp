// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BaseRenderer.cpp: Implementation of the BaseRenderer class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "BaseRenderer.h"
#include <algorithm>
#include <numeric>

namespace Spectrum {

    BaseRenderer::BaseRenderer()
        : m_quality(RenderQuality::Medium)
        , m_primaryColor(Color::FromRGB(33, 150, 243))
        , m_backgroundColor(Color::FromRGB(13, 13, 26))
        , m_width(0)
        , m_height(0)
        , m_time(0.0f) {
    }

    void BaseRenderer::SetQuality(RenderQuality quality) {
        m_quality = quality;
        UpdateSettings();
    }

    void BaseRenderer::SetPrimaryColor(const Color& color) {
        m_primaryColor = color;
    }

    void BaseRenderer::SetBackgroundColor(const Color& color) {
        m_backgroundColor = color;
    }

    void BaseRenderer::OnActivate(int width, int height) {
        SetViewport(width, height);
    }

    void BaseRenderer::Render(GraphicsContext& context,
        const SpectrumData& spectrum) {
        if (!IsRenderable(spectrum)) return;

        UpdateTime(FRAME_TIME);
        UpdateAnimation(spectrum, FRAME_TIME);
        DoRender(context, spectrum);
    }

    void BaseRenderer::UpdateTime(float deltaTime) {
        m_time += deltaTime;
        if (m_time > TIME_RESET_THRESHOLD) m_time = 0.0f;
    }

    void BaseRenderer::SetViewport(int width, int height) noexcept {
        m_width = width;
        m_height = height;
    }

    bool BaseRenderer::IsRenderable(
        const SpectrumData& spectrum) const noexcept {
        if (spectrum.empty()) return false;
        if (m_width <= 0 || m_height <= 0) return false;
        return true;
    }

    float BaseRenderer::AverageRange(const SpectrumData& spectrum,
        size_t begin,
        size_t end) {
        if (spectrum.empty()) return 0.0f;

        const size_t n = spectrum.size();
        begin = std::min(begin, n);
        end = std::min(end, n);
        if (begin >= end) return 0.0f;

        const float sum = std::accumulate(
            spectrum.begin() + begin,
            spectrum.begin() + end,
            0.0f
        );
        return sum / static_cast<float>(end - begin);
    }

    float BaseRenderer::SegmentAverage(const SpectrumData& spectrum,
        size_t segments,
        size_t index) {
        if (spectrum.empty() || segments == 0) return 0.0f;

        const size_t start = (index * spectrum.size()) / segments;
        size_t end = ((index + 1) * spectrum.size()) / segments;
        return AverageRange(spectrum, start, end);
    }

    float BaseRenderer::GetAverageMagnitude(
        const SpectrumData& spectrum) const {
        return AverageRange(spectrum, 0, spectrum.size());
    }

    float BaseRenderer::GetBassMagnitude(
        const SpectrumData& spectrum) const {
        if (spectrum.empty()) return 0.0f;
        const size_t end = std::max<size_t>(1, spectrum.size() / 8);
        return AverageRange(spectrum, 0, end);
    }

    float BaseRenderer::GetMidMagnitude(
        const SpectrumData& spectrum) const {
        if (spectrum.empty()) return 0.0f;
        const size_t start = spectrum.size() / 8;
        const size_t end = std::min(spectrum.size(),
            start + spectrum.size() / 2);
        return AverageRange(spectrum, start, end);
    }

    float BaseRenderer::GetHighMagnitude(
        const SpectrumData& spectrum) const {
        if (spectrum.empty()) return 0.0f;
        const size_t start = std::min(spectrum.size(),
            (spectrum.size() * 5) / 8);
        return AverageRange(spectrum, start, spectrum.size());
    }

    BaseRenderer::GridMetrics BaseRenderer::ComputeCenteredGrid(
        int cols, int rows) const {
        GridMetrics gm{};
        gm.cols = cols;
        gm.rows = rows;
        if (cols <= 0 || rows <= 0 ||
            m_width <= 0 || m_height <= 0) return gm;

        gm.cellSize = std::min(
            static_cast<float>(m_width) / cols,
            static_cast<float>(m_height) / rows
        );
        const float gridW = cols * gm.cellSize;
        const float gridH = rows * gm.cellSize;

        gm.startX = (m_width - gridW) * 0.5f;
        gm.startY = (m_height - gridH) * 0.5f;
        return gm;
    }

    BaseRenderer::BarLayout BaseRenderer::ComputeBarLayout(
        size_t count, float spacing) const {
        BarLayout bl{};
        bl.spacing = spacing;
        if (count == 0 || m_width <= 0) return bl;

        bl.totalBarWidth = static_cast<float>(m_width) /
            static_cast<float>(count);
        bl.barWidth = bl.totalBarWidth - spacing;
        if (bl.barWidth < 0.0f) bl.barWidth = 0.0f;
        return bl;
    }

    void BaseRenderer::BuildPolylineFromSpectrum(
        const SpectrumData& spectrum,
        float midlineFrac,
        float amplitudeFrac,
        std::vector<Point>& out) const {
        const size_t n = spectrum.size();
        out.resize(n);
        const float centerY = m_height * midlineFrac;
        const float amp = m_height * amplitudeFrac;

        for (size_t i = 0; i < n; ++i) {
            out[i].x = (static_cast<float>(i) /
                std::max<size_t>(1, n - 1)) * m_width;
            out[i].y = centerY - spectrum[i] * amp;
        }
    }

    float BaseRenderer::MagnitudeToHeight(float magnitude,
        float scale) const {
        const float h = magnitude * static_cast<float>(m_height) * scale;
        return std::clamp(h, 0.0f, static_cast<float>(m_height));
    }

} // namespace Spectrum