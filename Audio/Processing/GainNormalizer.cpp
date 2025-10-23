#include "GainNormalizer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <algorithm>
#include <numeric>

namespace Spectrum {

    using namespace Helpers::Math;

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
            m_peakLevel = Lerp(m_peakLevel, currentMax, kAttackRate);
        else
            m_peakLevel *= kDecayRate;

        m_peakLevel = std::max(m_peakLevel, kEpsilon);

        // Calculate and apply the dynamic gain.
        const float dynamicGain = Clamp(kTargetGainLevel / m_peakLevel, kMinGain, kMaxGain);

        for (float& val : spectrum)
            val *= dynamicGain;
    }

    void GainNormalizer::Reset() {
        m_peakLevel = 0.0f;
    }

} // namespace Spectrum