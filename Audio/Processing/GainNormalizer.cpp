// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the GainNormalizer class. It uses a peak-tracking
// algorithm with attack and decay to smoothly adjust the gain, preventing
// abrupt changes in the visualization.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "GainNormalizer.h"
#include "Common/MathUtils.h"
#include <algorithm>
#include <numeric>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    GainNormalizer::GainNormalizer() : m_peakLevel(0.0f) {}

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Public Interface
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void GainNormalizer::Process(SpectrumData& spectrum) {
        float currentMax = 0.0f;
        if (!spectrum.empty())
            currentMax = *std::max_element(spectrum.begin(), spectrum.end());

        // Update the running peak level with attack/decay behavior.
        if (currentMax > m_peakLevel)
            m_peakLevel = Utils::Lerp(m_peakLevel, currentMax, kAttackRate);
        else
            m_peakLevel *= kDecayRate;

        m_peakLevel = std::max(m_peakLevel, kEpsilon);

        // Calculate and apply the dynamic gain.
        const float dynamicGain = Utils::Clamp(kTargetGainLevel / m_peakLevel, kMinGain, kMaxGain);

        for (float& val : spectrum)
            val *= dynamicGain;
    }

    void GainNormalizer::Reset() {
        m_peakLevel = 0.0f;
    }

} // namespace Spectrum