#ifndef SPECTRUM_CPP_IAUDIOSOURCE_H
#define SPECTRUM_CPP_IAUDIOSOURCE_H

#include "Common/Common.h"

namespace Spectrum {

    class IAudioSource {
    public:
        virtual ~IAudioSource() = default;

        virtual bool Initialize() = 0;
        virtual void Update(float dt) = 0;
        virtual SpectrumData GetSpectrum() = 0;

        virtual void SetAmplification(float) {}
        virtual void SetBarCount(size_t) {}
        virtual void SetFFTWindow(FFTWindowType) {}
        virtual void SetScaleType(SpectrumScale) {}
        virtual void SetSmoothing(float) {}

        virtual void StartCapture() {}
        virtual void StopCapture() {}
    };

} // namespace Spectrum

#endif