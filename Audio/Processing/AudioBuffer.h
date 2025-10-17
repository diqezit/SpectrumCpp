#ifndef SPECTRUM_CPP_AUDIO_BUFFER_H
#define SPECTRUM_CPP_AUDIO_BUFFER_H

#include "Common/Common.h"

namespace Spectrum {

    class ThreadSafeAudioBuffer {
    public:
        void Add(const float* data, size_t frames, int channels);
        bool HasEnoughData(size_t required) const;
        void CopyTo(AudioBuffer& dest, size_t size);
        void Consume(size_t size);

    private:
        float MixdownMonoFrame(const float* frameData, int channels) const;

        AudioBuffer m_buffer;
        mutable std::mutex m_mutex;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_AUDIO_BUFFER_H