// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the GainNormalizer, a DSP component responsible for
// applying automatic gain control (AGC). It analyzes the incoming spectrum
// and adjusts its level to ensure a consistent volume, making the
// visualization independent of the source's loudness.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_GAIN_NORMALIZER_H
#define SPECTRUM_CPP_GAIN_NORMALIZER_H

#include "Common/Common.h"

namespace Spectrum {

    class GainNormalizer {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        GainNormalizer();

        void Process(SpectrumData& spectrum);
        void Reset();

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Constants
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        static constexpr float kTargetGainLevel = 0.8f;
        static constexpr float kMinGain = 0.1f;
        static constexpr float kMaxGain = 20.0f;
        static constexpr float kEpsilon = 1e-6f;
        static constexpr float kAttackRate = 0.01f;
        static constexpr float kDecayRate = 0.999f;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        float m_peakLevel;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_GAIN_NORMALIZER_H