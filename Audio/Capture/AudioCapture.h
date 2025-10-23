// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// AudioCapture.h: Captures a single audio session and reports its status.
// Uses the PIMPL idiom to hide complex WASAPI implementation details.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifndef SPECTRUM_CPP_AUDIO_CAPTURE_H
#define SPECTRUM_CPP_AUDIO_CAPTURE_H

#include "Common/Common.h"
#include <memory>

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Interface for receiving processed audio data from the capture session.
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    class IAudioCaptureCallback {
    public:
        virtual ~IAudioCaptureCallback() = default;
        virtual void OnAudioData(
            const float* data,
            size_t samples,
            int channels
        ) = 0;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Manages a WASAPI audio capture session in a separate thread.
    // This class is non-copyable and non-movable to ensure single ownership.
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    class AudioCapture {
    public:
        AudioCapture();
        ~AudioCapture();

        // Prohibit copying and moving to prevent resource management issues
        AudioCapture(const AudioCapture&) = delete;
        AudioCapture& operator=(const AudioCapture&) = delete;
        AudioCapture(AudioCapture&&) = delete;
        AudioCapture& operator=(AudioCapture&&) = delete;

        // Prepares the audio device and all necessary resources for capture
        bool Initialize();
        // Starts the capture thread and begins processing audio data
        bool Start();
        // Signals the capture thread to stop and cleans up resources
        void Stop() noexcept;

        // Query the state of the capture session
        bool IsCapturing() const noexcept;
        bool IsInitialized() const noexcept;
        // Check if a non-recoverable error has occurred (e.g. device lost)
        bool IsFaulted() const noexcept;
        HRESULT GetLastError() const noexcept;

        // Register a callback to receive audio data
        void SetCallback(IAudioCaptureCallback* callback) noexcept;

        // Get properties of the captured audio stream
        int GetSampleRate() const noexcept;
        int GetChannels() const noexcept;
        int GetBitsPerSample() const noexcept;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> m_pimpl;
    };

} // namespace Spectrum

#endif