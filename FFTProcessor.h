// FFTProcessor.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// FFTProcessor.h: Performs the Fast Fourier Transform on audio data.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_FFT_PROCESSOR_H
#define SPECTRUM_CPP_FFT_PROCESSOR_H

#include "Common.h"

namespace Spectrum {

    class FFTProcessor {
    public:
        explicit FFTProcessor(size_t fftSize = DEFAULT_FFT_SIZE);
        ~FFTProcessor() = default;

        // Main processing
        void Process(const AudioBuffer& input);
        void SetWindowType(FFTWindowType type);

        // Getters
        const SpectrumData& GetMagnitudes() const noexcept { return m_magnitudes; }
        const SpectrumData& GetPhases() const noexcept { return m_phases; }
        size_t GetFFTSize() const noexcept { return m_fftSize; }
        FFTWindowType GetWindowType() const noexcept { return m_windowType; }

        // Static window function generators
        static std::vector<float> GenerateWindow(FFTWindowType type, size_t size);
        static float ApplyWindowFunction(FFTWindowType type, size_t index, size_t size);

    private:
        // Window and input preparation
        void GenerateWindow();
        void ApplyWindow(const AudioBuffer& input);

        // FFT processing
        void PerformFFT();
        void BitReversalPermutation();
        void CooleyTukeyFFT();

        // Cooley–Tukey helpers
        void StagePass(size_t m, size_t halfM, size_t step) noexcept;
        void ButterflyBlock(size_t base, size_t halfM, size_t step) noexcept;

        // Result calculation
        void CalculateMagnitudesAndPhases();
        float CalculateMagnitude(const std::complex<float>& c) const noexcept;
        float CalculatePhase(const std::complex<float>& c) const noexcept;

        // Helpers
        size_t ReverseBits(size_t num, size_t bitCount) const noexcept;
        void InitializeTwiddleFactors();
        static bool IsPowerOfTwo(size_t n) noexcept;

    private:
        // FFT parameters
        size_t m_fftSize;
        size_t m_logSize;

        // Buffers
        std::vector<std::complex<float>> m_fftBuffer;
        std::vector<std::complex<float>> m_twiddleFactors;

        // Results
        SpectrumData m_magnitudes;
        SpectrumData m_phases;

        // Window
        std::vector<float> m_window;
        FFTWindowType m_windowType;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_FFT_PROCESSOR_H