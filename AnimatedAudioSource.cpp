#include "AnimatedAudioSource.h"
#include "Random.h"
#include "MathUtils.h"

namespace Spectrum {

    AnimatedAudioSource::AnimatedAudioSource(const AudioConfig& config)
        : m_barCount(config.barCount), m_postProcessor(config.barCount)
    {
        m_postProcessor.SetSmoothing(config.smoothing);
    }

    void AnimatedAudioSource::Update(float deltaTime) {
        m_animationTime += deltaTime;
        SpectrumData testData = GenerateTestSpectrum(m_animationTime);
        m_postProcessor.Process(testData);
    }

    SpectrumData AnimatedAudioSource::GetSpectrum() {
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

    SpectrumData AnimatedAudioSource::GenerateTestSpectrum(float timeOffset) {
        SpectrumData testData(m_barCount, 0.0f);
        for (size_t i = 0; i < m_barCount; ++i) {
            testData[i] = CalculateBarValue(i, timeOffset);
        }
        return testData;
    }

    float AnimatedAudioSource::CalculateBaseSineValue(float phase) const {
        return (std::sin(phase) + 1.0f) * 0.5f;
    }

    float AnimatedAudioSource::ApplyFrequencyFalloff(float value, float normalizedFrequency) const {
        return value * (1.0f - normalizedFrequency * 0.7f);
    }

    float AnimatedAudioSource::AddRandomNoise(float value) const {
        return value + Utils::Random::Instance().Float(-0.05f, 0.05f);
    }

    float AnimatedAudioSource::CalculateBarValue(size_t barIndex, float timeOffset) const {
        const float frequency = static_cast<float>(barIndex) / static_cast<float>(m_barCount);
        const float phase = timeOffset * 2.0f + static_cast<float>(barIndex) * 0.3f;

        float value = CalculateBaseSineValue(phase);
        value = ApplyFrequencyFalloff(value, frequency);
        value = AddRandomNoise(value);

        return Utils::Saturate(value);
    }

}