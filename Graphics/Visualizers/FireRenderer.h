#ifndef SPECTRUM_CPP_FIRE_RENDERER_H
#define SPECTRUM_CPP_FIRE_RENDERER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FireRenderer
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Draw.h"
#include "Graphics/Base/BaseRenderer.h"
#include "Graphics/Visualizers/Settings/QualityTraits.h"

namespace Spectrum {

    class FireRenderer final : public BaseRenderer<FireRenderer> {
    public:
        FireRenderer() {
            m_palette = {
                Color(0.0f, 0.0f, 0.0f, 0.0f),
                Color(0.2f, 0.0f, 0.0f, 1.0f),
                Color(0.5f, 0.0f, 0.0f, 1.0f),
                Color(0.8f, 0.2f, 0.0f, 1.0f),
                Color(1.0f, 0.5f, 0.0f, 1.0f),
                Color(1.0f, 0.8f, 0.0f, 1.0f),
                Color(1.0f, 1.0f, 0.5f, 1.0f),
                Color(1.0f, 1.0f, 1.0f, 1.0f)
            };
            UpdateSettings();
        }

        [[nodiscard]] std::string_view GetName() const override { return "Fire"; }
        void SetPrimaryColor(const Color&) override {}

        void OnActivate(int width, int height) override {
            BaseRenderer::OnActivate(width, height);
            RebuildGrid();
        }

    protected:
        void UpdateSettings() override {
            m_settings = GetQualitySettings<Settings::FireSettings>();
            RebuildGrid();
        }

        void UpdateAnimation(const SpectrumData& spectrum, float) override {
            if (m_cols <= 0 || m_rows <= 0) return;
            for (float& cell : m_grid) cell *= m_settings.decay;
            Inject(spectrum);
            Propagate();
        }

        void DoRender(BLContext& ctx, const SpectrumData&) override {
            if (m_cols <= 0 || m_rows <= 0) return;

            for (int y = 0; y < m_rows; ++y) {
                for (int x = 0; x < m_cols; ++x) {
                    const float intensity = At(x, y);
                    if (intensity < kMinVisible) continue;

                    Color color = SampleGradient(m_palette, Clamp(intensity, 0.0f, 1.0f));
                    color.a *= SmoothStep(0.0f, 0.1f, intensity);
                    if (color.a < 0.01f) continue;

                    Draw::FillRect(ctx, {
                        static_cast<float>(x) * m_settings.pixelSize,
                        static_cast<float>(y) * m_settings.pixelSize,
                        m_settings.pixelSize,
                        m_settings.pixelSize
                        }, color);
                }
            }
        }

    private:
        static constexpr float kWindSpeed = 2.0f;
        static constexpr float kWindAmplitude = 2.0f;
        static constexpr float kMinVisible = 0.01f;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Grid
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RebuildGrid() {
            if (GetWidth() <= 0 || GetHeight() <= 0 || m_settings.pixelSize <= 0.0f) {
                m_cols = m_rows = 0;
                m_grid.clear();
                return;
            }
            m_cols = static_cast<int>(static_cast<float>(GetWidth()) / m_settings.pixelSize);
            m_rows = static_cast<int>(static_cast<float>(GetHeight()) / m_settings.pixelSize);
            m_grid.assign(static_cast<size_t>(m_cols) * static_cast<size_t>(m_rows), 0.0f);
        }

        [[nodiscard]] size_t Index(int x, int y) const {
            return static_cast<size_t>(y) * static_cast<size_t>(m_cols) + static_cast<size_t>(x);
        }

        [[nodiscard]] float At(int x, int y) const {
            const size_t i = Index(x, y);
            return i < m_grid.size() ? m_grid[i] : 0.0f;
        }

        void Inject(const SpectrumData& spectrum) {
            if (spectrum.empty() || m_cols <= 0 || m_rows <= 0) return;

            const int bottom = m_rows - 1;
            const float last = static_cast<float>(std::max<size_t>(1, spectrum.size()) - 1);

            for (size_t i = 0; i < spectrum.size(); ++i) {
                const int x = Clamp(
                    static_cast<int>(Map(static_cast<float>(i), 0.0f, last, 0.0f,
                        static_cast<float>(m_cols - 1))),
                    0, m_cols - 1);
                const size_t idx = Index(x, bottom);
                if (idx < m_grid.size()) {
                    m_grid[idx] = std::max(
                        m_grid[idx],
                        Helpers::Sanitize::Normalized(spectrum[i]) * m_settings.heatMultiplier);
                }
            }
        }

        void Propagate() {
            const auto src = m_grid;
            for (int y = 0; y < m_rows - 1; ++y) {
                for (int x = 0; x < m_cols; ++x) {
                    int sx = x;
                    if (m_settings.useWind) {
                        sx += static_cast<int>(
                            std::sin(GetTime() * kWindSpeed + x * 0.5f) * kWindAmplitude);
                        sx = Clamp(sx, 0, m_cols - 1);
                    }

                    const size_t srcIdx = Index(sx, y + 1);
                    float value = srcIdx < src.size() ? src[srcIdx] : 0.0f;

                    if (m_settings.useSmoothing && x > 0 && x < m_cols - 1) {
                        const size_t left = Index(x - 1, y + 1);
                        const size_t right = Index(x + 1, y + 1);
                        const float l = left < src.size() ? src[left] : value;
                        const float r = right < src.size() ? src[right] : value;
                        value = value * 0.5f + (l + r) * 0.25f;
                    }
                    m_grid[Index(x, y)] = value;
                }
            }
        }

        Settings::FireSettings m_settings{};
        int m_cols = 0;
        int m_rows = 0;
        std::vector<float> m_grid;
        ColorGradient m_palette;
    };

} // namespace Spectrum

#endif