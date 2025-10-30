#include "Graphics/Visualizers/FireRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Base/RenderUtils.h"

namespace Spectrum {

    using namespace Helpers::Sanitize;
    using namespace Helpers::Math;

    namespace {
        constexpr float kWindSpeed = 2.0f;
        constexpr float kWindAmplitude = 2.0f;
        constexpr float kSmoothingCenter = 0.5f;
        constexpr float kSmoothingSide = 0.25f;
        constexpr float kMinVisibleIntensity = 0.01f;
    }

    FireRenderer::FireRenderer()
        : m_gridWidth(0)
        , m_gridHeight(0)
    {
        CreateFirePalette();
        UpdateSettings();
    }

    void FireRenderer::OnActivate(int width, int height) {
        BaseRenderer::OnActivate(width, height);
        InitializeGrid();
    }

    void FireRenderer::UpdateSettings() {
        m_settings = GetQualitySettings<Settings>();
        InitializeGrid();
    }

    void FireRenderer::UpdateAnimation(
        const SpectrumData& spectrum,
        float
    ) {
        if (m_gridWidth <= 0 || m_gridHeight <= 0) return;

        for (float& value : m_fireGrid) {
            value *= m_settings.decay;
        }

        InjectHeat(spectrum);
        PropagateFire();
    }

    void FireRenderer::DoRender(Canvas& canvas, const SpectrumData&) {
        if (m_gridWidth <= 0 || m_gridHeight <= 0) return;

        for (int y = 0; y < m_gridHeight; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                const size_t idx = y * m_gridWidth + x;
                if (idx >= m_fireGrid.size()) continue;

                const float intensity = m_fireGrid[idx];
                if (intensity < kMinVisibleIntensity) continue;

                Color color = SampleGradient(
                    m_firePalette,
                    Clamp(intensity, 0.0f, 1.0f)
                );

                color.a *= SmoothStep(0.0f, 0.1f, intensity);

                if (color.a >= 0.01f) {
                    const Rect pixelRect{
                        static_cast<float>(x) * m_settings.pixelSize,
                        static_cast<float>(y) * m_settings.pixelSize,
                        m_settings.pixelSize,
                        m_settings.pixelSize
                    };
                    canvas.DrawRectangle(pixelRect, Paint::Fill(color));
                }
            }
        }
    }

    void FireRenderer::InitializeGrid() {
        if (GetWidth() <= 0 || GetHeight() <= 0 || m_settings.pixelSize <= 0.0f) {
            m_gridWidth = m_gridHeight = 0;
            m_fireGrid.clear();
            return;
        }

        m_gridWidth = static_cast<int>(GetWidth() / m_settings.pixelSize);
        m_gridHeight = static_cast<int>(GetHeight() / m_settings.pixelSize);

        const size_t gridSize = static_cast<size_t>(m_gridWidth) * m_gridHeight;
        m_fireGrid.assign(gridSize, 0.0f);
    }

    void FireRenderer::CreateFirePalette() {
        m_firePalette = {
            Color(0.0f, 0.0f, 0.0f, 0.0f),
            Color(0.2f, 0.0f, 0.0f, 1.0f),
            Color(0.5f, 0.0f, 0.0f, 1.0f),
            Color(0.8f, 0.2f, 0.0f, 1.0f),
            Color(1.0f, 0.5f, 0.0f, 1.0f),
            Color(1.0f, 0.8f, 0.0f, 1.0f),
            Color(1.0f, 1.0f, 0.5f, 1.0f),
            Color(1.0f, 1.0f, 1.0f, 1.0f)
        };
    }

    void FireRenderer::InjectHeat(const SpectrumData& spectrum) {
        const int bottomY = m_gridHeight - 1;
        if (bottomY < 0) return;

        for (size_t i = 0; i < spectrum.size(); ++i) {
            const float normalized = NormalizedFloat(spectrum[i]);
            const int x = static_cast<int>(MapToRange(
                static_cast<float>(i),
                0.0f,
                static_cast<float>(spectrum.size() - 1),
                0.0f,
                static_cast<float>(m_gridWidth - 1)
            ));

            const size_t idx = bottomY * m_gridWidth + Clamp(x, 0, m_gridWidth - 1);
            if (idx < m_fireGrid.size()) {
                m_fireGrid[idx] = std::max(
                    m_fireGrid[idx],
                    normalized * m_settings.heatMultiplier
                );
            }
        }
    }

    void FireRenderer::PropagateFire() {
        const std::vector<float> readGrid = m_fireGrid;

        for (int y = 0; y < m_gridHeight - 1; ++y) {
            for (int x = 0; x < m_gridWidth; ++x) {
                int srcX = x;
                const int srcY = y + 1;

                if (m_settings.useWind) {
                    srcX += static_cast<int>(
                        std::sin(GetTime() * kWindSpeed + x * 0.5f) * kWindAmplitude
                        );
                    srcX = Clamp(srcX, 0, m_gridWidth - 1);
                }

                const size_t srcIdx = srcY * m_gridWidth + srcX;
                float value = (srcIdx < readGrid.size()) ? readGrid[srcIdx] : 0.0f;

                if (m_settings.useSmoothing && x > 0 && x < m_gridWidth - 1) {
                    const size_t leftIdx = srcY * m_gridWidth + x - 1;
                    const size_t rightIdx = srcY * m_gridWidth + x + 1;
                    const float left = (leftIdx < readGrid.size()) ? readGrid[leftIdx] : value;
                    const float right = (rightIdx < readGrid.size()) ? readGrid[rightIdx] : value;
                    value = value * kSmoothingCenter + (left + right) * kSmoothingSide;
                }

                const size_t destIdx = y * m_gridWidth + x;
                if (destIdx < m_fireGrid.size()) {
                    m_fireGrid[destIdx] = value;
                }
            }
        }
    }

} // namespace Spectrum