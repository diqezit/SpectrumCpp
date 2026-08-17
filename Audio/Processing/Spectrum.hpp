#pragma once

#include "Common/Common.h"
#include "Audio/Capture/AudioCapture.hpp"
#include "Graphics/API/GraphicsHelpers.h"

#include <kiss_fftr.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

namespace Spectrum {

    class PostProcessor {
    public:
        static constexpr float kAmpMin      = 0.1f;
        static constexpr float kAmpMax      = 5.0f;
        static constexpr float kSmoothMin   = 0.0f;
        static constexpr float kSmoothMax   = 1.0f;
        static constexpr float kMinLevel    = 1e-6f;
        static constexpr float kLevelAttack = 0.01f;
        static constexpr float kLevelDecay  = 0.999f;
        static constexpr float kPeakDecay   = 0.98f;
        static constexpr float kTargetLevel = 0.8f;
        static constexpr float kGainMin     = 0.1f;
        static constexpr float kGainMax     = 20.0f;
        static constexpr float kLogRange    = 150.0f;

        explicit PostProcessor(size_t n) { SetBarCount(n); }

        void SetBarCount(size_t n) {
            m_n = n;
            m_bars.assign(n, 0.0f);
            m_peaks.assign(n, 0.0f);
            m_lvl = 0.0f;
        }

        void SetAmplification(float a) { m_amp   = std::clamp(a, kAmpMin,    kAmpMax);    }
        void SetSmoothing(float s)     { m_smooth = std::clamp(s, kSmoothMin, kSmoothMax); }

        const SpectrumData& GetSmoothedBars() const { return m_bars;  }
        const SpectrumData& GetPeakValues()   const { return m_peaks; }
        float GetAmplification()              const { return m_amp;   }
        float GetSmoothing()                  const { return m_smooth; }

        void Process(SpectrumData& spec) {
            float* s = spec.data();
            const float peak = *std::max_element(s, s + m_n);
            m_lvl = std::max(
                peak > m_lvl ? Lerp(m_lvl, peak, kLevelAttack) : m_lvl * kLevelDecay,
                kMinLevel);

            const float gain        = std::clamp(kTargetLevel / m_lvl, kGainMin, kGainMax);
            const float invLogRange = 1.0f / std::log1p(kLogRange);
            const float attack      = m_smooth * 0.5f;

            float* pk = m_peaks.data();
            float* br = m_bars.data();
            for (size_t i = 0; i < m_n; ++i) {
                const float v = std::min(
                    std::pow(std::log1p(s[i] * gain * kLogRange) * invLogRange, m_amp),
                    1.0f);
                s[i]  = v;
                pk[i] = std::max(v, pk[i] * kPeakDecay);
                br[i] = Lerp(v, br[i], v > br[i] ? attack : m_smooth);
            }
        }

    private:
        size_t m_n     = 0;
        float  m_amp   = DEFAULT_AMPLIFICATION;
        float  m_smooth= DEFAULT_SMOOTHING;
        float  m_lvl   = 0.0f;
        SpectrumData m_bars, m_peaks;
    };

    class RingBuffer {
    public:
        explicit RingBuffer(size_t capacity) : m_data(capacity) {}

        void Write(const float* samples, uint32_t frames, uint32_t channels) {
            if (channels == 0 || frames == 0)
                return;

            const float  inv = 1.0f / float(channels);
            const size_t cap = m_data.size();
            size_t i = size_t(m_w.load(std::memory_order_relaxed) % cap);

            for (const float* end = samples + frames * channels; samples < end; samples += channels) {
                m_data[i] = std::accumulate(samples, samples + channels, 0.0f) * inv;
                if (++i == cap)
                    i = 0;
            }

            m_w.fetch_add(frames, std::memory_order_release);
        }

        uint64_t Written() const { return m_w.load(std::memory_order_acquire); }

        void Read(size_t count, float* out) const {
            const size_t cap = m_data.size();
            size_t r = size_t((Written() + cap - count) % cap);
            for (size_t i = 0; i < count; ++i) {
                out[i] = m_data[r];
                if (++r == cap)
                    r = 0;
            }
        }

    private:
        std::vector<float>    m_data;
        std::atomic<uint64_t> m_w{ 0 };
    };

    class Window {
    public:
        void Build(size_t n, FFTWindowType type) {
            m_data.resize(n);
            const float N = std::max(1.0f, float(n - 1));
            const auto formula = [type](float a) {
                switch (type) {
                case FFTWindowType::Hann:     return 0.5f * (1.0f - std::cos(a));
                case FFTWindowType::Hamming:  return 0.54f - 0.46f * std::cos(a);
                case FFTWindowType::Blackman: return 0.42f - 0.5f * std::cos(a) + 0.08f * std::cos(2.0f * a);
                default:                      return 1.0f;
                }
            };
            for (size_t i = 0; i < n; ++i)
                m_data[i] = formula(Helpers::Constants::kTwoPi * float(i) / N);
        }

        const float* Data() const { return m_data.data(); }

    private:
        std::vector<float> m_data;
    };

    class BandMapper {
    public:
        static constexpr float kMinLogFreq = 20.0f;

        void Setup(size_t bars, size_t fftSize, size_t sampleRate, SpectrumScale scale) {
            m_bars    = bars;
            m_fftSize = fftSize;
            m_rate    = sampleRate;
            m_scale   = scale;
            Rebuild();
        }

        void Map(const float* mag, float* out) const {
            const bool avg = m_scale == SpectrumScale::Logarithmic;
            for (size_t i = 0; i < m_bars; ++i) {
                const auto& [a, b] = m_bands[i];
                out[i] = avg
                    ? std::accumulate(mag + a, mag + b, 0.0f) / float(b - a)
                    : *std::max_element(mag + a, mag + b);
            }
        }

    private:
        struct Band { size_t a, b; };

        float Edge(size_t i) const {
            const float t   = float(i) / float(m_bars);
            const float nyq = float(m_rate) * 0.5f;
            switch (m_scale) {
            case SpectrumScale::Logarithmic: {
                const float lo = std::log10(kMinLogFreq);
                const float hi = std::log10(nyq);
                return std::pow(10.0f, lo + t * (hi - lo));
            }
            case SpectrumScale::Mel:
                return MelToFreq(t * FreqToMel(nyq));
            default:
                return t * nyq;
            }
        }

        void Rebuild() {
            const auto toBin = [this](float f) {
                return std::min(size_t(f * float(m_fftSize) / float(m_rate)), m_fftSize / 2);
            };

            std::vector<size_t> edge(m_bars + 1);
            for (size_t i = 0; i <= m_bars; ++i)
                edge[i] = std::max(toBin(Edge(i)), size_t(1));

            m_bands.resize(m_bars);
            for (size_t i = 0; i < m_bars; ++i) {
                const size_t a = edge[i];
                m_bands[i] = { a, std::max(edge[i + 1] + 1, a + 1) };
            }
        }

        size_t       m_bars = 0, m_fftSize = 0, m_rate = 0;
        SpectrumScale m_scale = SpectrumScale::Logarithmic;
        std::vector<Band> m_bands;
    };

    class Analyzer : public IAudioCaptureCallback {
    public:
        Analyzer(size_t bars = DEFAULT_BAR_COUNT, size_t n = DEFAULT_FFT_SIZE)
            : m_bars(bars)
            , m_n(n)
            , m_fft(kiss_fftr_alloc(int(n), 0, nullptr, nullptr))
            , m_post(bars)
            , m_ring(std::max<size_t>(n * 4, 2048))
            , m_td(n)
            , m_fd(n / 2 + 1)
            , m_mag(n / 2 + 1)
            , m_out(bars)
        {
            m_window.Build(m_n, m_wt);
            m_mapper.Setup(m_bars, m_n, m_rate, m_scale);
        }

        Analyzer(const Analyzer&)            = delete;
        Analyzer& operator=(const Analyzer&) = delete;

        void OnAudioData(const float* p, uint32_t frames, uint32_t ch) override {
            m_ring.Write(p, frames, ch);
        }

        void Update() {
            if (!m_fft)
                return;

            const uint64_t w = m_ring.Written();
            if (w == m_last)
                return;
            m_last = w;

            m_ring.Read(m_n, m_td.data());

            const float* win = m_window.Data();
            for (size_t i = 0; i < m_n; ++i)
                m_td[i] *= win[i];

            kiss_fftr(m_fft.get(), m_td.data(), m_fd.data());

            const float norm       = 2.0f / float(m_n);
            const kiss_fft_cpx* fd = m_fd.data();
            float* mag             = m_mag.data();
            for (size_t i = 0; i < m_fd.size(); ++i)
                mag[i] = std::hypot(fd[i].r, fd[i].i) * norm;
            mag[0] = 0.0f;

            m_mapper.Map(mag, m_out.data());
            m_post.Process(m_out);
        }

        void SetBarCount(size_t n) {
            if (!n || n == m_bars)
                return;
            m_bars = n;
            m_out.resize(n);
            m_post.SetBarCount(n);
            m_mapper.Setup(m_bars, m_n, m_rate, m_scale);
        }

        void SetSampleRate(size_t rate) {
            if (!rate || rate == m_rate)
                return;
            m_rate = rate;
            m_mapper.Setup(m_bars, m_n, m_rate, m_scale);
        }

        void SetAmplification(float a) { m_post.SetAmplification(a); }
        void SetSmoothing(float s)     { m_post.SetSmoothing(s);     }

        void SetFFTWindow(FFTWindowType t) {
            if (t == m_wt)
                return;
            m_wt = t;
            m_window.Build(m_n, m_wt);
        }

        void SetScaleType(SpectrumScale t) {
            if (t == m_scale)
                return;
            m_scale = t;
            m_mapper.Setup(m_bars, m_n, m_rate, m_scale);
        }

        const SpectrumData& GetSpectrum()    const { return m_post.GetSmoothedBars(); }
        const SpectrumData& GetPeakValues()  const { return m_post.GetPeakValues();   }
        size_t              GetBarCount()    const { return m_bars;                   }
        float               GetAmplification() const { return m_post.GetAmplification(); }
        float               GetSmoothing()   const { return m_post.GetSmoothing();    }
        SpectrumScale       GetScaleType()   const { return m_scale;                  }

    private:
        struct FftFree {
            void operator()(kiss_fftr_state* p) const { kiss_fftr_free(p); }
        };

        size_t        m_bars, m_n, m_rate = DEFAULT_SAMPLE_RATE;
        SpectrumScale m_scale = SpectrumScale::Logarithmic;
        FFTWindowType m_wt    = FFTWindowType::Hann;
        std::unique_ptr<kiss_fftr_state, FftFree> m_fft;
        PostProcessor m_post;
        RingBuffer    m_ring;
        BandMapper    m_mapper;
        Window        m_window;
        uint64_t      m_last = 0;
        std::vector<float>        m_td;
        std::vector<kiss_fft_cpx> m_fd;
        SpectrumData  m_mag, m_out;
    };

} // namespace Spectrum