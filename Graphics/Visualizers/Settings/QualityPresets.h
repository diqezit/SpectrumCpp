#ifndef SPECTRUM_CPP_QUALITY_PRESETS_H
#define SPECTRUM_CPP_QUALITY_PRESETS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Quality presets for each visualizer (Low / Medium / High / Ultra)
// Overlay tables are used when isOverlay is true
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"

namespace Spectrum {

    class BarsRenderer;
    class CircularWaveRenderer;
    class CubesRenderer;
    class FireRenderer;
    class GaugeRenderer;
    class KenwoodBarsRenderer;
    class LedPanelRenderer;
    class MatrixLedRenderer;
    class ParticlesRenderer;
    class PolylineWaveRenderer;
    class SphereRenderer;
    class WaveRenderer;
    class WaterfallRenderer;

    namespace Settings {

        struct BarsSettings {
            float barSpacing;
            float cornerRadius;
            bool useShadow;
            bool useHighlight;
        };

        struct CircularWaveSettings {
            bool useGlow;
            float maxStroke;
            int maxRings;
            float rotationSpeed;
            float waveSpeed;
        };

        struct CubesSettings {
            bool useTopFace;
            bool useSideFace;
            bool useShadow;
            float topHeightRatio;
            float sideFaceBrightness;
            float perspective;
        };

        struct FireSettings {
            bool useSmoothing;
            bool useWind;
            float pixelSize;
            float decay;
            float heatMultiplier;
        };

        struct GaugeSettings {
            float smoothingFactorInc;
            float smoothingFactorDec;
            float riseSpeed;
        };

        struct KenwoodBarsSettings {
            float barSpacing;
            float cornerRadius;
            bool useGradient;
        };

        struct LedPanelSettings {
            bool usePeakHold;
            int maxRows;
            float smoothingMultiplier;
        };

        struct MatrixLedSettings {
            bool enableGlow;
            int ledDensity;
            float blurAmount;
        };

        struct ParticlesSettings {
            int maxParticles;
            float spawnRate;
            float particleDetail;
            bool useBatchRendering;
            float particleSize;
            bool useTrails;
            float trailLength;
        };

        struct PolylineWaveSettings {
            bool useGradientBars;
            bool useGlow;
            bool useHighlight;
            bool usePulsingCore;
            float glowIntensity;
            float highlightIntensity;
            bool useFill;
            float lineWidth;
            float smoothness;
        };

        struct SphereSettings {
            bool useGradient;
            float responseSpeed;
            bool useGlow;
            float rotationSpeed;
        };

        struct WaveSettings {
            float waveHeight;
            bool useFill;
            bool useMirror;
            float smoothness;
            int points;
        };

        struct WaterfallSettings {
            float perspectiveDepth;
            float lineWidth;
        };

    } // namespace Settings

} // namespace Spectrum

namespace Spectrum::QualityPresets {

    template<typename TSettings>
    struct PresetTable {
        TSettings low;
        TSettings medium;
        TSettings high;
        TSettings ultra;

        [[nodiscard]] constexpr const TSettings& Get(RenderQuality quality) const {
            switch (quality) {
            case RenderQuality::Low:    return low;
            case RenderQuality::High:   return high;
            case RenderQuality::Ultra:  return ultra;
            default:                    return medium;
            }
        }
    };

    inline const PresetTable<Settings::BarsSettings> BarsPresets = {
        { 1.0f, 0.0f, false, false },
        { 2.0f, 3.0f, false, true  },
        { 2.0f, 5.0f, true,  true  },
        { 3.0f, 6.0f, true,  true  }
    };

    inline const PresetTable<Settings::CircularWaveSettings> CircularWavePresets = {
        { false,  6.0f, 16, 0.5f, 2.0f },
        { true,   7.0f, 24, 0.5f, 2.0f },
        { true,   8.0f, 32, 0.5f, 2.0f },
        { true,  10.0f, 48, 0.5f, 2.0f }
    };

    inline const PresetTable<Settings::CircularWaveSettings> CircularWaveOverlayPresets = {
        { false, 4.0f, 12, 0.4f, 1.5f },
        { true,  5.0f, 16, 0.4f, 1.5f },
        { true,  6.0f, 20, 0.4f, 1.5f },
        { true,  7.0f, 24, 0.4f, 1.5f }
    };

    inline const PresetTable<Settings::CubesSettings> CubesPresets = {
        { false, true, false, 0.2f,  0.7f,  0.15f },
        { true,  true, true,  0.25f, 0.6f,  0.25f },
        { true,  true, true,  0.3f,  0.5f,  0.35f },
        { true,  true, true,  0.35f, 0.45f, 0.4f  }
    };

    inline const PresetTable<Settings::FireSettings> FirePresets = {
        { false, false, 12.0f, 0.93f, 1.2f },
        { true,  true,   8.0f, 0.95f, 1.5f },
        { true,  true,   6.0f, 0.97f, 1.8f },
        { true,  true,   4.0f, 0.98f, 2.0f }
    };

    inline const PresetTable<Settings::GaugeSettings> GaugePresets = {
        { 0.25f, 0.06f, 0.12f },
        { 0.20f, 0.05f, 0.15f },
        { 0.15f, 0.04f, 0.20f },
        { 0.12f, 0.03f, 0.25f }
    };

    inline const PresetTable<Settings::KenwoodBarsSettings> KenwoodBarsPresets = {
        { 2.0f, 0.0f, false },
        { 2.0f, 1.5f, true  },
        { 2.0f, 2.0f, true  },
        { 3.0f, 2.5f, true  }
    };

    inline const PresetTable<Settings::KenwoodBarsSettings> KenwoodBarsOverlayPresets = {
        { 2.0f, 0.0f, false },
        { 2.0f, 1.5f, true  },
        { 2.0f, 1.5f, true  },
        { 3.0f, 2.0f, true  }
    };

    inline const PresetTable<Settings::LedPanelSettings> LedPanelPresets = {
        { false, 16, 1.0f },
        { true,  24, 0.9f },
        { true,  32, 0.8f },
        { true,  48, 0.7f }
    };

    inline const PresetTable<Settings::LedPanelSettings> LedPanelOverlayPresets = {
        { true,  8, 1.2f },
        { true, 12, 1.1f },
        { true, 16, 1.0f },
        { true, 20, 0.9f }
    };

    inline const PresetTable<Settings::MatrixLedSettings> MatrixLedPresets = {
        { false, 16, 1.0f },
        { true,  24, 0.9f },
        { true,  32, 0.8f },
        { true,  48, 0.6f }
    };

    inline const PresetTable<Settings::MatrixLedSettings> MatrixLedOverlayPresets = {
        { false, 16, 1.0f },
        { true,  24, 0.9f },
        { true,  32, 0.8f },
        { true,  48, 0.6f }
    };

    inline const PresetTable<Settings::ParticlesSettings> ParticlesPresets = {
        {  5000, 0.7f,  0.6f, true, 0.7f,  false, 0.6f },
        { 10000, 0.85f, 0.8f, true, 0.85f, true,  0.8f },
        { 15000, 1.0f,  1.0f, true, 1.0f,  true,  1.0f },
        { 20000, 1.0f,  1.2f, true, 1.0f,  true,  1.0f }
    };

    inline const PresetTable<Settings::PolylineWaveSettings> PolylineWavePresets = {
        { false, false, false, false, 0.0f, 0.0f, false, 0.0f, 0.0f  },
        { true,  true,  false, true,  0.4f, 0.0f, true,  0.4f, 0.0f  },
        { true,  true,  true,  true,  0.6f, 0.8f, true,  0.6f, 0.8f  },
        { true,  true,  true,  true,  0.8f, 1.0f, true,  0.8f, 0.95f }
    };

    inline const PresetTable<Settings::SphereSettings> SpherePresets = {
        { false, 0.15f, false, 0.15f },
        { true,  0.2f,  true,  0.2f  },
        { true,  0.25f, true,  0.25f },
        { true,  0.3f,  true,  0.3f  }
    };

    inline const PresetTable<Settings::WaveSettings> WavePresets = {
        { 0.7f,  false, false, 0.6f,  64  },
        { 0.85f, true,  false, 0.8f,  128 },
        { 0.95f, true,  true,  0.9f,  256 },
        { 1.0f,  true,  true,  0.95f, 512 }
    };

    inline const PresetTable<Settings::WaterfallSettings> WaterfallPresets = {
        { 0.50f, 1.0f },
        { 0.60f, 1.2f },
        { 0.72f, 1.5f },
        { 0.85f, 1.8f }
    };

    template<typename TRenderer>
    struct PresetSource;

    template<typename TRenderer>
    [[nodiscard]] auto Get(RenderQuality quality, bool isOverlay = false) {
        return PresetSource<TRenderer>::Table(isOverlay).Get(quality);
    }

    template<> struct PresetSource<BarsRenderer> {
        static const auto& Table(bool) { return BarsPresets; }
    };

    template<> struct PresetSource<CircularWaveRenderer> {
        static const auto& Table(bool overlay) {
            return overlay ? CircularWaveOverlayPresets : CircularWavePresets;
        }
    };

    template<> struct PresetSource<CubesRenderer> {
        static const auto& Table(bool) { return CubesPresets; }
    };

    template<> struct PresetSource<FireRenderer> {
        static const auto& Table(bool) { return FirePresets; }
    };

    template<> struct PresetSource<GaugeRenderer> {
        static const auto& Table(bool) { return GaugePresets; }
    };

    template<> struct PresetSource<KenwoodBarsRenderer> {
        static const auto& Table(bool overlay) {
            return overlay ? KenwoodBarsOverlayPresets : KenwoodBarsPresets;
        }
    };

    template<> struct PresetSource<LedPanelRenderer> {
        static const auto& Table(bool overlay) {
            return overlay ? LedPanelOverlayPresets : LedPanelPresets;
        }
    };

    template<> struct PresetSource<MatrixLedRenderer> {
        static const auto& Table(bool overlay) {
            return overlay ? MatrixLedOverlayPresets : MatrixLedPresets;
        }
    };

    template<> struct PresetSource<ParticlesRenderer> {
        static const auto& Table(bool) { return ParticlesPresets; }
    };

    template<> struct PresetSource<PolylineWaveRenderer> {
        static const auto& Table(bool) { return PolylineWavePresets; }
    };

    template<> struct PresetSource<SphereRenderer> {
        static const auto& Table(bool) { return SpherePresets; }
    };

    template<> struct PresetSource<WaveRenderer> {
        static const auto& Table(bool) { return WavePresets; }
    };

    template<> struct PresetSource<WaterfallRenderer> {
        static const auto& Table(bool) { return WaterfallPresets; }
    };

} // namespace Spectrum::QualityPresets

#endif