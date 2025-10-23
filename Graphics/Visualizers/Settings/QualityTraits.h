#ifndef SPECTRUM_CPP_QUALITY_TRAITS_H
#define SPECTRUM_CPP_QUALITY_TRAITS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines quality traits system for renderer-specific settings.
//
// This file provides compile-time mapping between renderer types and their
// corresponding settings structures. Each visualizer has a dedicated settings
// type that defines all configurable quality-dependent parameters.
//
// Architecture:
// - Settings structures defined in Settings namespace
// - QualityTraits template maps renderer type to settings type
// - SettingsFor<T> alias provides convenient access
// - Zero runtime overhead (compile-time resolution)
//
// Usage pattern:
//   using MySettings = SettingsFor<BarsRenderer>;
//   MySettings settings = QualityPresets::Get<BarsRenderer>(quality);
//
// Adding new renderer:
// 1. Define Settings::NewRendererSettings struct
// 2. Add QualityTraits<NewRenderer> specialization
// 3. Create preset table in QualityPresets.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Forward Declarations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Settings Structures
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace Settings {

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // BarsRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct BarsSettings {
            float barSpacing;
            float cornerRadius;
            bool useShadow;
            bool useHighlight;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // CircularWaveRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct CircularWaveSettings {
            bool useGlow;
            float maxStroke;
            int maxRings;
            float rotationSpeed;
            float waveSpeed;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // CubesRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct CubesSettings {
            bool useTopFace;
            bool useSideFace;
            bool useShadow;
            float topHeightRatio;
            float sideFaceBrightness;
            float perspective;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // FireRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct FireSettings {
            bool useSmoothing;
            bool useWind;
            float pixelSize;
            float decay;
            float heatMultiplier;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // GaugeRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct GaugeSettings {
            float smoothingFactorInc;
            float smoothingFactorDec;
            float riseSpeed;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // KenwoodBarsRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct KenwoodBarsSettings {
            float barSpacing;
            float cornerRadius;
            bool useGradient;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // LedPanelRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct LedPanelSettings {
            bool usePeakHold;
            int maxRows;
            float smoothingMultiplier;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // MatrixLedRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct MatrixLedSettings {
            bool enableGlow;       // was usePeakHold
            int ledDensity;        // was maxRows
            float blurAmount;      // was smoothingMultiplier
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // ParticlesRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct ParticlesSettings {
            int maxParticles;
            float spawnRate;
            float particleDetail;
            bool useBatchRendering;
            float particleSize;    // additional field for spawn chance calculation
            bool useTrails;        // for batch vs individual rendering
            float trailLength;     // for size calculation
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // PolylineWaveRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct PolylineWaveSettings {
            bool useGradientBars;
            bool useGlow;
            bool useHighlight;
            bool usePulsingCore;
            float glowIntensity;
            float highlightIntensity;
            bool useFill;          // additional for core rendering
            float lineWidth;       // additional for glow calculation
            float smoothness;      // additional for highlight threshold
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // SphereRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct SphereSettings {
            bool useGradient;
            float responseSpeed;
            bool useGlow;          // additional field (same as useGradient in practice)
            float rotationSpeed;   // additional field (same as responseSpeed)
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // WaveRenderer Settings
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct WaveSettings {
            float waveHeight;
            bool useFill;
            bool useMirror;
            float smoothness;
            int points;
        };

    } // namespace Settings

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Quality Traits Template
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<typename TRenderer>
    struct QualityTraits;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Quality Traits Specializations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<>
    struct QualityTraits<BarsRenderer> {
        using SettingsType = Settings::BarsSettings;
    };

    template<>
    struct QualityTraits<CircularWaveRenderer> {
        using SettingsType = Settings::CircularWaveSettings;
    };

    template<>
    struct QualityTraits<CubesRenderer> {
        using SettingsType = Settings::CubesSettings;
    };

    template<>
    struct QualityTraits<FireRenderer> {
        using SettingsType = Settings::FireSettings;
    };

    template<>
    struct QualityTraits<GaugeRenderer> {
        using SettingsType = Settings::GaugeSettings;
    };

    template<>
    struct QualityTraits<KenwoodBarsRenderer> {
        using SettingsType = Settings::KenwoodBarsSettings;
    };

    template<>
    struct QualityTraits<LedPanelRenderer> {
        using SettingsType = Settings::LedPanelSettings;
    };

    template<>
    struct QualityTraits<MatrixLedRenderer> {
        using SettingsType = Settings::MatrixLedSettings;
    };

    template<>
    struct QualityTraits<ParticlesRenderer> {
        using SettingsType = Settings::ParticlesSettings;
    };

    template<>
    struct QualityTraits<PolylineWaveRenderer> {
        using SettingsType = Settings::PolylineWaveSettings;
    };

    template<>
    struct QualityTraits<SphereRenderer> {
        using SettingsType = Settings::SphereSettings;
    };

    template<>
    struct QualityTraits<WaveRenderer> {
        using SettingsType = Settings::WaveSettings;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Convenience Alias
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<typename TRenderer>
    using SettingsFor = typename QualityTraits<TRenderer>::SettingsType;

} // namespace Spectrum

#endif // SPECTRUM_CPP_QUALITY_TRAITS_H