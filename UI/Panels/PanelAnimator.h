#ifndef SPECTRUM_CPP_PANEL_ANIMATOR_H
#define SPECTRUM_CPP_PANEL_ANIMATOR_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PanelAnimator, a reusable animation state machine for UI panels
// with configurable easing functions for smooth, professional transitions.
//
// This class uses cubic easing by default for more natural-feeling animations
// compared to linear or quadratic curves.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/MathUtils.h"
#include <algorithm>

namespace Spectrum
{
    enum class AnimationState
    {
        Closed,
        Opening,
        Open,
        Closing
    };

    class PanelAnimator final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit PanelAnimator(float speed = 8.0f) noexcept :
            m_state(AnimationState::Closed),
            m_progress(0.0f),
            m_speed(speed)
        {
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Open() noexcept
        {
            if (m_state == AnimationState::Open || m_state == AnimationState::Opening) return;
            m_state = AnimationState::Opening;
        }

        void Close() noexcept
        {
            if (m_state == AnimationState::Closed || m_state == AnimationState::Closing) return;
            m_state = AnimationState::Closing;
        }

        void Update(float deltaTime) noexcept
        {
            const float step = m_speed * deltaTime;

            if (m_state == AnimationState::Opening)
            {
                m_progress = std::min(1.0f, m_progress + step);
                if (m_progress >= 1.0f) m_state = AnimationState::Open;
            }
            else if (m_state == AnimationState::Closing)
            {
                m_progress = std::max(0.0f, m_progress - step);
                if (m_progress <= 0.0f) m_state = AnimationState::Closed;
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsVisible() const noexcept { return m_state != AnimationState::Closed; }
        [[nodiscard]] float GetProgress() const noexcept { return Utils::EaseInOutCubic(m_progress); }
        [[nodiscard]] float GetRawProgress() const noexcept { return m_progress; }
        [[nodiscard]] AnimationState GetState() const noexcept { return m_state; }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        AnimationState m_state;
        float m_progress;
        float m_speed;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PANEL_ANIMATOR_H