// FFTProcessor.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// FFTProcessor.cpp: Implementation of the FFTProcessor class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "FFTProcessor.h"
#include "Utils.h"

namespace Spectrum {

    namespace {
        inline size_t IntegerLog2(size_t n) noexcept {
            size_t l = 0;
            while ((n >> l) > 1) ++l;
            return l;
        }
    }

    FFTProcessor::FFTProcessor(size_t fftSize)
        : m_fftSize(fftSize)
        , m_logSize(IntegerLog2(fftSize))
        , m_windowType(FFTWindowType::Hann) {

        if (!IsPowerOfTwo(m_fftSize)) {
            LOG_ERROR("FFT size must be a power of two. Got: " << m_fftSize);
        }

        m_fftBuffer.resize(m_fftSize);
        m_magnitudes.resize(m_fftSize / 2 + 1);
        m_phases.resize(m_fftSize / 2 + 1);
        m_window.resize(m_fftSize);

        InitializeTwiddleFactors();
        GenerateWindow();
    }

    bool FFTProcessor::IsPowerOfTwo(size_t n) noexcept {
        return n && ((n & (n - 1)) == 0);
    }

    void FFTProcessor::InitializeTwiddleFactors() {
        m_twiddleFactors.resize(m_fftSize / 2);
        for (size_t i = 0; i < m_fftSize / 2; ++i) {
            const float angle =
                -TWO_PI * static_cast<float>(i) / static_cast<float>(m_fftSize);
            m_twiddleFactors[i] = std::complex<float>(
                std::cos(angle), std::sin(angle)
            );
        }
    }

    void FFTProcessor::SetWindowType(FFTWindowType type) {
        if (type == m_windowType) return;
        m_windowType = type;
        GenerateWindow();
    }

    void FFTProcessor::GenerateWindow() {
        m_window = GenerateWindow(m_windowType, m_fftSize);
    }

    std::vector<float> FFTProcessor::GenerateWindow(
        FFTWindowType type,
        size_t size
    ) {
        std::vector<float> window(size);
        for (size_t i = 0; i < size; ++i)
            window[i] = ApplyWindowFunction(type, i, size);
        return window;
    }

    float FFTProcessor::ApplyWindowFunction(
        FFTWindowType type,
        size_t index,
        size_t size
    ) {
        const float N = static_cast<float>(size - 1);
        const float n = static_cast<float>(index);

        switch (type) {
        case FFTWindowType::Hann:
            return 0.5f * (1.0f - std::cos(TWO_PI * n / N));

        case FFTWindowType::Hamming:
            return 0.54f - 0.46f * std::cos(TWO_PI * n / N);

        case FFTWindowType::Blackman:
            return 0.42f
                - 0.5f * std::cos(TWO_PI * n / N)
                + 0.08f * std::cos(2.0f * TWO_PI * n / N);

        case FFTWindowType::Rectangular:
        default:
            return 1.0f;
        }
    }

    void FFTProcessor::ApplyWindow(const AudioBuffer& input) {
        const size_t N = m_fftSize;
        const size_t M = std::min(N, input.size());

        for (size_t i = 0; i < M; ++i)
            m_fftBuffer[i] = std::complex<float>(input[i] * m_window[i], 0.0f);

        for (size_t i = M; i < N; ++i)
            m_fftBuffer[i] = std::complex<float>(0.0f, 0.0f);
    }

    size_t FFTProcessor::ReverseBits(size_t num, size_t bitCount) const noexcept {
        size_t rev = 0;
        for (size_t i = 0; i < bitCount; ++i)
            rev |= ((num >> i) & 1ULL) << (bitCount - 1 - i);
        return rev;
    }

    void FFTProcessor::BitReversalPermutation() {
        for (size_t i = 0; i < m_fftSize; ++i) {
            const size_t j = ReverseBits(i, m_logSize);
            if (i < j) std::swap(m_fftBuffer[i], m_fftBuffer[j]);
        }
    }

    void FFTProcessor::StagePass(
        size_t m,
        size_t halfM,
        size_t step
    ) noexcept {
        for (size_t base = 0; base < m_fftSize; base += m)
            ButterflyBlock(base, halfM, step);
    }

    void FFTProcessor::ButterflyBlock(
        size_t base,
        size_t halfM,
        size_t step
    ) noexcept {
        for (size_t j = 0; j < halfM; ++j) {
            const size_t tw = j * step;

            const std::complex<float> t =
                m_twiddleFactors[tw] * m_fftBuffer[base + j + halfM];
            const std::complex<float> u = m_fftBuffer[base + j];

            m_fftBuffer[base + j] = u + t;
            m_fftBuffer[base + j + halfM] = u - t;
        }
    }

    void FFTProcessor::CooleyTukeyFFT() {
        for (size_t stage = 1; stage <= m_logSize; ++stage) {
            const size_t m = 1ULL << stage;
            const size_t halfM = m >> 1;
            const size_t step = m_fftSize / m;
            StagePass(m, halfM, step);
        }
    }

    void FFTProcessor::PerformFFT() {
        BitReversalPermutation();
        CooleyTukeyFFT();
    }

    float FFTProcessor::CalculateMagnitude(
        const std::complex<float>& c
    ) const noexcept {
        const float re = c.real();
        const float im = c.imag();
        return std::sqrt(re * re + im * im);
    }

    float FFTProcessor::CalculatePhase(
        const std::complex<float>& c
    ) const noexcept {
        return std::atan2(c.imag(), c.real());
    }

    void FFTProcessor::CalculateMagnitudesAndPhases() {
        const float norm = 2.0f / static_cast<float>(m_fftSize);
        const size_t bins = m_magnitudes.size(); // N/2 + 1

        for (size_t i = 0; i < bins; ++i) {
            m_magnitudes[i] = CalculateMagnitude(m_fftBuffer[i]) * norm;
            m_phases[i] = CalculatePhase(m_fftBuffer[i]);
        }

        if (!m_magnitudes.empty())
            m_magnitudes[0] *= 0.5f; // DC component
    }

    void FFTProcessor::Process(const AudioBuffer& input) {
        ApplyWindow(input);
        PerformFFT();
        CalculateMagnitudesAndPhases();
    }

} // namespace Spectrum