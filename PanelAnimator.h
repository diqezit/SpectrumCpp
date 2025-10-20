// PanelAnimator.h
#ifndef SPECTRUM_CPP_PANEL_ANIMATOR_H
#define SPECTRUM_CPP_PANEL_ANIMATOR_H

namespace Spectrum {

    enum class AnimationState {
        Closed,
        Opening,
        Open,
        Closing
    };

    class PanelAnimator {
    public:
        explicit PanelAnimator(float speed = 8.0f)
            : m_state(AnimationState::Closed)
            , m_progress(0.0f)
            , m_speed(speed)
        {
        }

        void Open() {
            if (m_state == AnimationState::Open || m_state == AnimationState::Opening) return;
            m_state = AnimationState::Opening;
        }

        void Close() {
            if (m_state == AnimationState::Closed || m_state == AnimationState::Closing) return;
            m_state = AnimationState::Closing;
        }

        void Update(float deltaTime) {
            const float step = m_speed * deltaTime;

            if (m_state == AnimationState::Opening) {
                m_progress = std::min(1.0f, m_progress + step);
                if (m_progress >= 1.0f) m_state = AnimationState::Open;
            }
            else if (m_state == AnimationState::Closing) {
                m_progress = std::max(0.0f, m_progress - step);
                if (m_progress <= 0.0f) m_state = AnimationState::Closed;
            }
        }

        [[nodiscard]] bool IsVisible() const {
            return m_state != AnimationState::Closed;
        }

        [[nodiscard]] float GetProgress() const { return m_progress; }
        [[nodiscard]] AnimationState GetState() const { return m_state; }

    private:
        AnimationState m_state;
        float m_progress;
        float m_speed;
    };

}

#endif