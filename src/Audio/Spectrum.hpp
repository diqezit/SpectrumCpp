#pragma once

#include "Common/Common.h"
#include "Audio/AudioCapture.hpp"

#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp/window.hpp>
#include <kfr/math.hpp>

namespace Spectrum {

    class Analyzer : public IAudioCaptureCallback {
    public:

        // -=-=-=-=-=-=-=-= Constants =-=-=-=-=-=-=-=
        static constexpr float kMinLogFreq = 20.0f;
        static constexpr float kMinLevel = 1e-6f;
        static constexpr float kLevelAttack = 0.01f;
        static constexpr float kLevelDecay = 0.999f;
        static constexpr float kTargetLevel = 0.8f;
        static constexpr float kGainMin = 0.1f;
        static constexpr float kGainMax = 20.0f;
        static constexpr float kLogRange = 150.0f;
        static constexpr float kInvLog = 1.0f / 5.017279836814924f;
        static constexpr float kBarMorph = 0.25f;

        enum {
            MAX_BARS = 256,
            RING_MIN = 2048,
            MIN_BIN = 1
        };

        enum Result { FAIL, DONE };

        enum Stage {
            MORPH, FFT, BANDS, AGC, COMPRESS, SMOOTH,
            REBUILD, WIN_BUILD
        };

        Analyzer(size_t bars = DEFAULT_BAR_COUNT, size_t fftSize = DEFAULT_FFT_SIZE)
            : m_fftSize(fftSize)
            , m_barCount(Clamp(bars, size_t(1), size_t(MAX_BARS)))
            , m_targetBarCount(m_barCount)
            , m_barCountF(float(m_barCount))
            , m_fft(m_fftSize)
            , m_tmp(m_fft.temp_size)
            , m_cap(std::max<size_t>(m_fftSize * 4, RING_MIN))
            , m_ring(m_cap)
            , m_td(m_fftSize)
            , m_fd(m_fftSize / 2 + 1)
            , m_mag(m_fftSize / 2 + 1)
            , m_rawBars(m_barCount, 0.0f)
            , m_bars(m_barCount, 0.0f)
        {
            Act(WIN_BUILD);
            Act(REBUILD);
        }

        Analyzer(const Analyzer&) = delete;
        Analyzer& operator=(const Analyzer&) = delete;
        Analyzer(Analyzer&&) = delete;
        Analyzer& operator=(Analyzer&&) = delete;

        // Audio thread. Capture delivers mono.
        void OnAudioData(const float* samples, uint32_t frames, uint32_t) override {
            size_t pos = size_t(m_written.load(std::memory_order_relaxed) % m_cap);
            m_ring.ringbuf_write(pos, samples, frames);
            m_written.fetch_add(frames, std::memory_order_release);
        }

        // -=-=-=-=-=-=-=-= Frame (UI thread) =-=-=-=-=-=-=-=
        // FFT FAIL = no full window, skip the rest.

        void Update(float dt) {
            Act(MORPH, dt);
            if (Act(FFT) == FAIL)
                return;
            Act(BANDS);
            Act(AGC);
            Act(COMPRESS);
            Act(SMOOTH);
        }

        // -=-=-=-=-=-=-=-= Settings =-=-=-=-=-=-=-=

        void SetBarCount(size_t n) {
            m_targetBarCount = Clamp(n, size_t(1), size_t(MAX_BARS));
        }

        void SetAmplification(float a) { m_amp = Clamp(a, AMP_MIN, AMP_MAX); }
        void SetSmoothing(float s) { m_smooth = Clamp(s, SMOOTH_MIN, SMOOTH_MAX); }

        void SetSampleRate(size_t rate) {
            if (rate == 0 || rate == m_rate)
                return;
            m_rate = rate;
            Act(REBUILD);
        }

        void SetFFTWindow(FFTWindowType t) {
            if (t == FFTWindowType::Count)
                t = FFTWindowType::Hann;
            if (t == m_windowType)
                return;
            m_windowType = t;
            Act(WIN_BUILD);
        }

        [[nodiscard]] const SpectrumData& GetSpectrum() const { return m_bars; }

    private:

        int Act(int how, float dt = 0.0f) {
            switch (how) {

            case MORPH: {
                if (m_barCount == m_targetBarCount)
                    return DONE;

                const float t = SmoothDt(kBarMorph, dt);
                if (t <= 0.0f)
                    return DONE;

                m_barCountF = Lerp(m_barCountF, float(m_targetBarCount), t);
                const size_t n = size_t(m_barCountF + 0.5f);
                if (n == m_barCount)
                    return DONE;

                m_scratch.resize(n);
                ResampleSpectrum(m_bars, m_scratch.data(), n, 1.0f);
                m_bars.swap(m_scratch);
                m_barCount = n;
                m_rawBars.resize(n);
                return Act(REBUILD);
            }

            case FFT: {
                const uint64_t written = m_written.load(std::memory_order_acquire);
                if (written == m_last || written < m_fftSize)
                    return FAIL;
                m_last = written;

                size_t start = size_t((written + m_cap - m_fftSize) % m_cap);
                m_ring.ringbuf_read(start, m_td.data(), m_fftSize);

                m_td *= m_win;
                m_fft.execute(m_fd, m_td, m_tmp);

                m_mag = kfr::cabs(m_fd) * (2.0f / float(m_fftSize));
                m_mag[0] = 0.0f; // zero DC component
                return DONE;
            }

            case BANDS: {
                for (size_t i = 0; i < m_barCount; ++i)
                    m_rawBars[i] = static_cast<float>(
                        kfr::mean(m_mag.slice(m_edge[i], m_len[i])));
                return DONE;
            }

            case AGC: {
                auto s = kfr::make_univector(m_rawBars.data(), m_barCount);
                const float peak = static_cast<float>(kfr::maxof(s));
                if (peak > m_level)
                    m_level = Lerp(m_level, peak, kLevelAttack);
                else
                    m_level *= kLevelDecay;
                m_level = std::max(m_level, kMinLevel);
                return DONE;
            }

            case COMPRESS: {
                auto s = kfr::make_univector(m_rawBars.data(), m_barCount);
                const float factor =
                    Clamp(kTargetLevel / m_level, kGainMin, kGainMax) * kLogRange;
                s = kfr::clamp(
                    kfr::logm(1.0f + s * factor, kInvLog) * m_amp,
                    0.0f, 1.0f);
                return DONE;
            }

            case SMOOTH: {
                // kfr::mix(c, x, y) = x * (1-c) + y * c
                // c=0 raw spectrum, c=1 old bars; attack twice as fast
                auto s = kfr::make_univector(m_rawBars.data(), m_barCount);
                auto bars = kfr::make_univector(m_bars.data(), m_barCount);
                bars = kfr::mix(
                    kfr::select(s > bars, m_smooth * 0.5f, m_smooth), s, bars);
                return DONE;
            }

            case REBUILD: {
                m_edge.resize(m_barCount + 1);
                m_len.resize(m_barCount);

                const size_t maxBin = m_fftSize / 2;
                const float stop = float(maxBin);

                float start = Clamp(
                    kMinLogFreq * float(m_fftSize) / float(m_rate),
                    float(MIN_BIN),
                    stop);

                const float step = std::pow(stop / start, 1.0f / float(m_barCount));

                float bin = start;
                size_t prev = MIN_BIN;

                for (size_t i = 0; i <= m_barCount; ++i) {
                    size_t edge = size_t(Clamp(bin, float(MIN_BIN), stop) + 0.5f);
                    if (i == m_barCount)
                        edge = maxBin;
                    edge = std::max(edge, prev);

                    m_edge[i] = edge;
                    if (i > 0)
                        m_len[i - 1] = std::max(edge - prev, size_t(1));

                    prev = edge;
                    bin *= step;
                }
                return DONE;
            }

            case WIN_BUILD: {
                // Indexed by FFTWindowType
                static constexpr struct {
                    kfr::window_type type;
                    float arg;
                } kSpec[] = {
                    { kfr::window_type::rectangular,     0.00f },
                    { kfr::window_type::triangular,      0.00f },
                    { kfr::window_type::bartlett,        0.00f },
                    { kfr::window_type::cosine,          0.00f },
                    { kfr::window_type::hann,            0.00f },
                    { kfr::window_type::bartlett_hann,   0.00f },
                    { kfr::window_type::hamming,         0.54f },
                    { kfr::window_type::bohman,          0.00f },
                    { kfr::window_type::blackman,        0.16f },
                    { kfr::window_type::blackman_harris, 0.00f },
                    { kfr::window_type::kaiser,          0.50f },
                    { kfr::window_type::flattop,         0.00f },
                    { kfr::window_type::gaussian,        2.50f },
                    { kfr::window_type::lanczos,         0.00f },
                    { kfr::window_type::cosine_np,       0.00f },
                    { kfr::window_type::planck_taper,    0.10f },
                    { kfr::window_type::tukey,           0.50f },
                };
                static_assert(
                    sizeof(kSpec) / sizeof(kSpec[0]) == size_t(FFTWindowType::Count),
                    "kSpec must match FFTWindowType");

                const size_t idx = size_t(m_windowType);
                m_win = kfr::window<float>(
                    m_fftSize, kSpec[idx].type, kSpec[idx].arg,
                    kfr::window_symmetry::periodic);
                return DONE;
            }

            default:
                return FAIL;
            }
        }

        // -=-=-=-=-=-=-=-= State =-=-=-=-=-=-=-=
        // Ring: m_ring + m_written (written by audio thread)
        // Spectrum: m_rawBars raw, m_bars smoothed (read by UI)
        // Bands: m_edge start bin, m_len number of bins per bar

        size_t        m_fftSize = 0;
        size_t        m_barCount = 0;
        size_t        m_targetBarCount = 0;
        float         m_barCountF = 0.0f;
        size_t        m_rate = DEFAULT_SAMPLE_RATE;
        FFTWindowType m_windowType = FFTWindowType::Hann;

        float m_amp = DEFAULT_AMPLIFICATION;
        float m_smooth = DEFAULT_SMOOTHING;
        float m_level = 0.0f;

        kfr::dft_plan_real<float> m_fft;
        kfr::univector<kfr::u8>   m_tmp;

        size_t                m_cap = 0;
        kfr::univector<float> m_ring;
        std::atomic<uint64_t> m_written{ 0 };
        uint64_t              m_last = 0;

        kfr::univector<float>  m_win;
        kfr::univector<size_t> m_edge;
        kfr::univector<size_t> m_len;

        kfr::univector<float>               m_td;
        kfr::univector<kfr::complex<float>> m_fd;
        kfr::univector<float>               m_mag;

        SpectrumData m_rawBars;
        SpectrumData m_bars;
        SpectrumData m_scratch;
    };

} // namespace Spectrum