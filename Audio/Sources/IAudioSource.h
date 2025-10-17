// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the IAudioSource interface, which provides a contract
// for any class that can supply spectrum data to the application, be it
// from a live capture, a pre-recorded file, or procedural generation.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_IAUDIOSOURCE_H
#define SPECTRUM_CPP_IAUDIOSOURCE_H

#include "Common/Common.h"

namespace Spectrum {

    class IAudioSource {
    public:
        virtual ~IAudioSource() = default;

        virtual bool Initialize() = 0;
        virtual void Update(float deltaTime) = 0;
        [[nodiscard]] virtual SpectrumData GetSpectrum() = 0;

        virtual void SetAmplification(float /*amp*/) {}
        virtual void SetBarCount(size_t /*count*/) {}
        virtual void SetFFTWindow(FFTWindowType /*type*/) {}
        virtual void SetScaleType(SpectrumScale /*type*/) {}
        virtual void SetSmoothing(float /*smoothing*/) {}

        virtual void StartCapture() {}
        virtual void StopCapture() {}
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_IAUDIOSOURCE_H