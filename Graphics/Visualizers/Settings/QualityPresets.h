#ifndef SPECTRUM_CPP_QUALITY_PRESETS_H
#define SPECTRUM_CPP_QUALITY_PRESETS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Quality presets registry for all visualizers.
//
// This file defines preset configurations for each quality level across all
// visualizer types. Provides centralized, single-source-of-truth for quality
// settings, eliminating duplication and simplifying maintenance.
//
// Design principles:
// - Preset tables use aggregate initialization (C++17 compatible)
// - Each preset defines Low/Medium/High/Ultra configurations
// - Type-safe access via template Get<RendererType>() function
// - Support for overlay mode where applicable
// - Compile-time constant tables for zero runtime cost
//
// Quality level guidelines:
// - Low:    Minimal effects, maximum performance
// - Medium: Balanced quality and performance
// - High:   Enhanced visuals, moderate performance cost
// - Ultra:  Maximum visual quality, highest resource usage
//
// Usage pattern:
//   auto settings = QualityPresets::Get<BarsRenderer>(quality);
//   auto overlaySettings = QualityPresets::Get<CircularWaveRenderer>(
//       quality, true);
//
// Modifying presets:
// - Adjust values in preset tables below
// - Changes apply to all instances using that quality level
// - No need to modify individual renderer code
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "QualityTraits.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum::QualityPresets {

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Preset Table Container
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BarsRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::BarsSettings> BarsPresets = {
    { 1.0f, 0.0f, false, false },
    { 2.0f, 3.0f, false, true  },
    { 2.0f, 5.0f, true,  true  },
    { 3.0f, 6.0f, true,  true  }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CircularWaveRenderer Presets (Normal Mode)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::CircularWaveSettings> CircularWavePresets = {
    { false,  6.0f, 16, 0.5f, 2.0f },
    { true,   7.0f, 24, 0.5f, 2.0f },
    { true,   8.0f, 32, 0.5f, 2.0f },
    { true,  10.0f, 48, 0.5f, 2.0f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CircularWaveRenderer Presets (Overlay Mode)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::CircularWaveSettings> CircularWaveOverlayPresets = {
    { false, 4.0f, 12, 0.4f, 1.5f },
    { true,  5.0f, 16, 0.4f, 1.5f },
    { true,  6.0f, 20, 0.4f, 1.5f },
    { true,  7.0f, 24, 0.4f, 1.5f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CubesRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::CubesSettings> CubesPresets = {
    { false, true, false, 0.2f,  0.7f,  0.15f },
    { true,  true, true,  0.25f, 0.6f,  0.25f },
    { true,  true, true,  0.3f,  0.5f,  0.35f },
    { true,  true, true,  0.35f, 0.45f, 0.4f  }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FireRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::FireSettings> FirePresets = {
    { false, false, 12.0f, 0.93f, 1.2f },
    { true,  true,   8.0f, 0.95f, 1.5f },
    { true,  true,   6.0f, 0.97f, 1.8f },
    { true,  true,   4.0f, 0.98f, 2.0f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GaugeRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::GaugeSettings> GaugePresets = {
    { 0.25f, 0.06f, 0.12f },
    { 0.20f, 0.05f, 0.15f },
    { 0.15f, 0.04f, 0.20f },
    { 0.12f, 0.03f, 0.25f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// KenwoodBarsRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// LedPanelRenderer Presets (Normal Mode)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::LedPanelSettings> LedPanelPresets = {
    { false, 16, 1.0f },
    { true,  24, 0.9f },
    { true,  32, 0.8f },
    { true,  48, 0.7f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// LedPanelRenderer Presets (Overlay Mode)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::LedPanelSettings> LedPanelOverlayPresets = {
    { true,  8, 1.2f },
    { true, 12, 1.1f },
    { true, 16, 1.0f },
    { true, 20, 0.9f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// MatrixLedRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::MatrixLedSettings> MatrixLedPresets = {
    { false, 16, 1.0f },
    { true,  24, 0.9f },
    { true,  32, 0.8f },
    { true,  48, 0.6f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// MatrixLedRenderer Presets (Overlay Mode)
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::MatrixLedSettings> MatrixLedOverlayPresets = {
    { false, 16, 1.0f },
    { true,  24, 0.9f },
    { true,  32, 0.8f },
    { true,  48, 0.6f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ParticlesRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::ParticlesSettings> ParticlesPresets = {
    // Low: maxParticles, spawnRate, particleDetail, useBatchRendering, particleSize, useTrails, trailLength
    {  5000, 0.7f,  0.6f, true, 0.7f,  false, 0.6f },
    { 10000, 0.85f, 0.8f, true, 0.85f, true,  0.8f },
    { 15000, 1.0f,  1.0f, true, 1.0f,  true,  1.0f },
    { 20000, 1.0f,  1.2f, true, 1.0f,  true,  1.0f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PolylineWaveRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::PolylineWaveSettings> PolylineWavePresets = {
    // Low: useGradientBars, useGlow, useHighlight, usePulsingCore, glowIntensity, highlightIntensity, useFill, lineWidth, smoothness
    { false, false, false, false, 0.0f, 0.0f, false, 0.0f, 0.0f  },
    { true,  true,  false, true,  0.4f, 0.0f, true,  0.4f, 0.0f  },
    { true,  true,  true,  true,  0.6f, 0.8f, true,  0.6f, 0.8f  },
    { true,  true,  true,  true,  0.8f, 1.0f, true,  0.8f, 0.95f }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SphereRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::SphereSettings> SpherePresets = {
    // Low: useGradient, responseSpeed, useGlow, rotationSpeed
    { false, 0.15f, false, 0.15f },
    { true,  0.2f,  true,  0.2f  },
    { true,  0.25f, true,  0.25f },
    { true,  0.3f,  true,  0.3f  }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// WaveRenderer Presets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline const PresetTable<Settings::WaveSettings> WavePresets = {
    { 0.7f,  false, false, 0.6f,  64  },
    { 0.85f, true,  false, 0.8f,  128 },
    { 0.95f, true,  true,  0.9f,  256 },
    { 1.0f,  true,  true,  0.95f, 512 }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Generic Getter Template Declaration
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

template<typename TRenderer>
struct PresetSource;

template<typename TRenderer>
[[nodiscard]] SettingsFor<TRenderer> Get(RenderQuality quality, bool isOverlay = false) {
    return PresetSource<TRenderer>::Table(isOverlay).Get(quality);
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Getter Template Specializations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

} // namespace Spectrum::QualityPresets

#endif // SPECTRUM_CPP_QUALITY_PRESETS_H