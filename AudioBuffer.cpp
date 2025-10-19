#include "AudioBuffer.h"

namespace Spectrum {

    float ThreadSafeAudioBuffer::MixdownMonoFrame(
        const float* frameData,
        int channels
    ) const {
        float monoSample = 0.0f;
        for (int ch = 0; ch < channels; ++ch) {
            monoSample += frameData[ch];
        }
        return monoSample / static_cast<float>(channels);
    }

    void ThreadSafeAudioBuffer::Add(
        const float* data,
        size_t frames,
        int channels
    ) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.reserve(m_buffer.size() + frames);

        for (size_t frame = 0; frame < frames; ++frame) {
            const float* currentFrameData = data + frame * static_cast<size_t>(channels);
            m_buffer.push_back(MixdownMonoFrame(currentFrameData, channels));
        }
    }

    bool ThreadSafeAudioBuffer::HasEnoughData(size_t required) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_buffer.size() >= required;
    }

    void ThreadSafeAudioBuffer::CopyTo(
        AudioBuffer& dest,
        size_t size
    ) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_buffer.size() >= size) {
            std::copy_n(m_buffer.begin(), size, dest.begin());
        }
    }

    void ThreadSafeAudioBuffer::Consume(size_t size) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_buffer.size() >= size) {
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + size);
        }
    }

} // namespace Spectrum