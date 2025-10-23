// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the PeakTracker component for peak value tracking.
//
// Implementation details:
// - Update() processes all channels in single pass
// - Peak values are clamped to [0.0, 1.0] range
// - Hold timers decrement linearly with deltaTime
// - Decay is applied multiplicatively for smooth animation
// - Resize preserves existing values where possible
//
// Performance characteristics:
// - Update: O(n) where n is min(values.size(), peaks.size())
// - Memory: 2 * sizeof(float) * channelCount
// - Cache-friendly sequential access patterns
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/Base/PeakTracker.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <algorithm>

namespace Spectrum {

    using namespace Helpers::Math;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    PeakTracker::PeakTracker(size_t channelCount, Config config)
        : m_config(config)
    {
        Resize(channelCount);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Interface
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PeakTracker::Update(const SpectrumData& values, float deltaTime)
    {
        const size_t count = std::min(values.size(), m_peaks.size());

        for (size_t i = 0; i < count; ++i) {
            UpdateChannel(i, values[i], deltaTime);
        }
    }

    void PeakTracker::Reset()
    {
        std::fill(m_peaks.begin(), m_peaks.end(), 0.0f);
        std::fill(m_holdTimers.begin(), m_holdTimers.end(), 0.0f);
    }

    void PeakTracker::Resize(size_t newSize)
    {
        m_peaks.resize(newSize, 0.0f);
        m_holdTimers.resize(newSize, 0.0f);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    float PeakTracker::GetPeak(size_t index) const
    {
        return IsValidIndex(index) ? m_peaks[index] : 0.0f;
    }

    bool PeakTracker::IsPeakVisible(size_t index) const
    {
        return IsValidIndex(index) && m_peaks[index] > m_config.minVisible;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void PeakTracker::UpdateChannel(size_t index, float value, float deltaTime)
    {
        if (!IsValidIndex(index)) return;

        const float sanitized = Saturate(value);

        if (ShouldSetNewPeak(index, sanitized)) {
            SetNewPeak(index, sanitized);
        }
        else if (IsHolding(index)) {
            UpdateHoldTimer(index, deltaTime);
        }
        else {
            ApplyDecay(index);
        }
    }

    void PeakTracker::SetNewPeak(size_t index, float value)
    {
        m_peaks[index] = value;
        m_holdTimers[index] = m_config.holdTime;
    }

    void PeakTracker::UpdateHoldTimer(size_t index, float deltaTime)
    {
        m_holdTimers[index] = std::max(0.0f, m_holdTimers[index] - deltaTime);
    }

    void PeakTracker::ApplyDecay(size_t index)
    {
        m_peaks[index] *= m_config.decayRate;
    }

    bool PeakTracker::ShouldSetNewPeak(size_t index, float value) const
    {
        return value >= m_peaks[index];
    }

    bool PeakTracker::IsHolding(size_t index) const
    {
        return m_holdTimers[index] > 0.0f;
    }

    bool PeakTracker::IsValidIndex(size_t index) const
    {
        return index < m_peaks.size();
    }

} // namespace Spectrum