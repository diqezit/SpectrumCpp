// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the AnimatedAudioSource. It generates a dynamic,
// wave-like spectrum using sine functions and noise, which is then passed
// through a post-processor for smoothing.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "AnimatedAudioSource.h"
#include "Common/Random.h"
#include "Common/MathUtils.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    AnimatedAudioSource::AnimatedAudioSource(
        const AudioConfig& config
    ) :
        m_animationTime(0.0f),
        m_barCount(config.barCount),
        m_postProcessor(config.barCount)
    {
        m_postProcessor.SetSmoothing(config.smoothing);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // IAudioSource Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    bool AnimatedAudioSource::Initialize() {
        return true;
    }

    void AnimatedAudioSource::Update(float deltaTime) {
        m_animationTime += deltaTime;
        SpectrumData testData = GenerateTestSpectrum(m_animationTime);
        m_postProcessor.Process(testData);
    }

    [[nodiscard]] SpectrumData AnimatedAudioSource::GetSpectrum() {
        return m_postProcessor.GetSmoothedBars();
    }

    void AnimatedAudioSource::SetBarCount(size_t count) {
        if (m_barCount == count) return;
        m_barCount = count;
        m_postProcessor.SetBarCount(count);
    }

    void AnimatedAudioSource::SetSmoothing(float smoothing) {
        m_postProcessor.SetSmoothing(smoothing);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Private Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] SpectrumData AnimatedAudioSource::GenerateTestSpectrum(float timeOffset) const {
        SpectrumData testData(m_barCount, 0.0f);
        for (size_t i = 0; i < m_barCount; ++i)
            testData[i] = CalculateBarValue(i, timeOffset);

        return testData;
    }

    [[nodiscard]] float AnimatedAudioSource::CalculateBarValue(
        size_t barIndex,
        float timeOffset
    ) const {
        const float frequency = static_cast<float>(barIndex) / static_cast<float>(m_barCount);
        const float phase = timeOffset * 2.0f + static_cast<float>(barIndex) * 0.3f;

        float value = CalculateBaseSineValue(phase);
        value = ApplyFrequencyFalloff(value, frequency);
        value = AddRandomNoise(value);

        return Utils::Saturate(value);
    }

    [[nodiscard]] float AnimatedAudioSource::CalculateBaseSineValue(float phase) const noexcept {
        return (std::sin(phase) + 1.0f) * 0.5f;
    }

    [[nodiscard]] float AnimatedAudioSource::ApplyFrequencyFalloff(
        float value,
        float normalizedFrequency
    ) const noexcept {
        return value * (1.0f - normalizedFrequency * 0.7f);
    }

    [[nodiscard]] float AnimatedAudioSource::AddRandomNoise(float value) const {
        return value + Utils::Random::Instance().Float(-0.05f, 0.05f);
    }

} // namespace Spectrum