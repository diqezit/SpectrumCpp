// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the AnimatedAudioSource, which provides procedurally
// generated spectrum data for testing and demonstration purposes. It does
// not require a real audio input.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_ANIMATEDAUDIOSOURCE_H
#define SPECTRUM_CPP_ANIMATEDAUDIOSOURCE_H

#include "IAudioSource.h"
#include "Audio/Processing/SpectrumPostProcessor.h"

namespace Spectrum {

    class AnimatedAudioSource : public IAudioSource {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Public Interface
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        explicit AnimatedAudioSource(const AudioConfig& config);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // IAudioSource Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        bool Initialize() override;
        void Update(float deltaTime) override;
        [[nodiscard]] SpectrumData GetSpectrum() override;

        void SetBarCount(size_t count) override;
        void SetSmoothing(float smoothing) override;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Private Implementation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        [[nodiscard]] SpectrumData GenerateTestSpectrum(float timeOffset) const;
        [[nodiscard]] float CalculateBarValue(size_t barIndex, float timeOffset) const;
        [[nodiscard]] float CalculateBaseSineValue(float phase) const noexcept;
        [[nodiscard]] float ApplyFrequencyFalloff(float value, float normalizedFrequency) const noexcept;
        [[nodiscard]] float AddRandomNoise(float value) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        float m_animationTime;
        size_t m_barCount;
        SpectrumPostProcessor m_postProcessor;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_ANIMATEDAUDIOSOURCE_H