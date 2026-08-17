#pragma once

#include "IAudioSource.hpp"
#include "Audio/Capture/AudioCapture.hpp"
#include "Audio/Processing/Spectrum.hpp"

namespace Spectrum {

    class RealtimeAudioSource : public IAudioSource {
    public:
        explicit RealtimeAudioSource(const AudioConfig& cfg)
            : m_analyzer(cfg.barCount, cfg.fftSize)
        {
            m_analyzer.SetAmplification(cfg.amplification);
            m_analyzer.SetSmoothing(cfg.smoothing);
            m_analyzer.SetFFTWindow(cfg.windowType);
            m_analyzer.SetScaleType(cfg.scaleType);
        }

        SpectrumData GetSpectrum()         override { return m_analyzer.GetSpectrum(); }
        void SetAmplification(float a)     override { m_analyzer.SetAmplification(a); }
        void SetBarCount(size_t n)         override { m_analyzer.SetBarCount(n); }
        void SetFFTWindow(FFTWindowType t)  override { m_analyzer.SetFFTWindow(t); }
        void SetScaleType(SpectrumScale t)  override { m_analyzer.SetScaleType(t); }
        void SetSmoothing(float s)         override { m_analyzer.SetSmoothing(s); }

        bool Initialize() override {
            m_capture = std::make_unique<AudioCapture>();
            if (!m_capture->Initialize()) {
                m_capture.reset();
                return false;
            }
            m_capture->SetCallback(&m_analyzer);
            return true;
        }

        void Update(float) override {
            if (m_capturing && m_capture && m_capture->IsFaulted())
                StopCapture();
            m_analyzer.Update();
        }

        void StartCapture() override {
            if (m_capturing)
                return;
            if ((!m_capture || m_capture->IsFaulted()) && !Initialize())
                return;
            if (m_capture->Start())
                m_capturing = true;
        }

        void StopCapture() override {
            if (m_capture)
                m_capture->Stop();
            m_capturing = false;
        }

    private:
        Analyzer                      m_analyzer;
        std::unique_ptr<AudioCapture> m_capture;
        bool                          m_capturing = false;
    };

} // namespace Spectrum