#ifndef SPECTRUM_CPP_PANEL_ANIMATOR_H
#define SPECTRUM_CPP_PANEL_ANIMATOR_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the PanelAnimator, a reusable animation state machine for UI panels
// with configurable easing functions for smooth, professional transitions.
//
// This class uses cubic easing by default for more natural-feeling animations
// compared to linear or quadratic curves.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Helpers/Math/MathHelpers.h"
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

        explicit PanelAnimator(float speed = 8.0f) noexcept
            : m_state(AnimationState::Closed)
            , m_progress(0.0f)
            , m_speed(speed)
        {
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Open() noexcept
        {
            if (ShouldIgnoreOpenRequest()) return;
            TransitionToOpening();
        }

        void Close() noexcept
        {
            if (ShouldIgnoreCloseRequest()) return;
            TransitionToClosing();
        }

        void Update(float deltaTime) noexcept
        {
            if (IsOpening()) {
                UpdateOpeningAnimation(deltaTime);
            }
            else if (IsClosing()) {
                UpdateClosingAnimation(deltaTime);
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsVisible() const noexcept
        {
            return !IsClosed();
        }

        [[nodiscard]] bool IsClosed() const noexcept
        {
            return m_state == AnimationState::Closed;
        }

        [[nodiscard]] bool IsOpening() const noexcept
        {
            return m_state == AnimationState::Opening;
        }

        [[nodiscard]] bool IsOpen() const noexcept
        {
            return m_state == AnimationState::Open;
        }

        [[nodiscard]] bool IsClosing() const noexcept
        {
            return m_state == AnimationState::Closing;
        }

        [[nodiscard]] bool IsFullyOpen() const noexcept
        {
            return m_progress >= 1.0f;
        }

        [[nodiscard]] bool IsFullyClosed() const noexcept
        {
            return m_progress <= 0.0f;
        }

        [[nodiscard]] float GetProgress() const noexcept
        {
            return ApplyEasing(m_progress);
        }

        [[nodiscard]] float GetRawProgress() const noexcept
        {
            return m_progress;
        }

        [[nodiscard]] AnimationState GetState() const noexcept
        {
            return m_state;
        }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Transition Guards
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ShouldIgnoreOpenRequest() const noexcept
        {
            return IsOpen() || IsOpening();
        }

        [[nodiscard]] bool ShouldIgnoreCloseRequest() const noexcept
        {
            return IsClosed() || IsClosing();
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Transitions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void TransitionToOpening() noexcept
        {
            m_state = AnimationState::Opening;
        }

        void TransitionToClosing() noexcept
        {
            m_state = AnimationState::Closing;
        }

        void TransitionToOpen() noexcept
        {
            m_state = AnimationState::Open;
        }

        void TransitionToClosed() noexcept
        {
            m_state = AnimationState::Closed;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateOpeningAnimation(float deltaTime) noexcept
        {
            IncreaseProgress(deltaTime);

            if (IsFullyOpen()) {
                TransitionToOpen();
            }
        }

        void UpdateClosingAnimation(float deltaTime) noexcept
        {
            DecreaseProgress(deltaTime);

            if (IsFullyClosed()) {
                TransitionToClosed();
            }
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Progress Manipulation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void IncreaseProgress(float deltaTime) noexcept
        {
            const float step = CalculateAnimationStep(deltaTime);
            m_progress = ClampProgressToMax(m_progress + step);
        }

        void DecreaseProgress(float deltaTime) noexcept
        {
            const float step = CalculateAnimationStep(deltaTime);
            m_progress = ClampProgressToMin(m_progress - step);
        }

        [[nodiscard]] float CalculateAnimationStep(float deltaTime) const noexcept
        {
            return m_speed * deltaTime;
        }

        [[nodiscard]] static float ClampProgressToMax(float progress) noexcept
        {
            return std::min(1.0f, progress);
        }

        [[nodiscard]] static float ClampProgressToMin(float progress) noexcept
        {
            return std::max(0.0f, progress);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Easing
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] static float ApplyEasing(float rawProgress) noexcept
        {
            return Helpers::Math::EaseInOutCubic(rawProgress);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        AnimationState m_state;
        float m_progress;
        float m_speed;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_PANEL_ANIMATOR_H