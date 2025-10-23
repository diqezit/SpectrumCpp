// TransformManager.h
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the TransformManager for handling 2D transformations.
//
// This class manages a stack of transformation matrices, enabling nested
// transforms for complex scene graphs. It provides both relative transforms
// (applied to current) and absolute transforms (replacing current).
//
// Key responsibilities:
// - Transform stack management with push/pop semantics
// - Relative transformations (rotate, scale, translate)
// - Absolute transform setting
// - Stack overflow protection
//
// Design notes:
// - All transform operations modify render target state
// - Stack depth limited to prevent overflow (kMaxStackDepth)
// - RAII TransformScope for automatic push/pop
// - Uses ComPtr for render target lifetime management
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_TRANSFORM_MANAGER_H
#define SPECTRUM_CPP_TRANSFORM_MANAGER_H

#include "Common/Common.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include <stack>

namespace Spectrum {

    class TransformManager final : public IRenderComponent
    {
    public:
        static constexpr size_t kMaxStackDepth = 32;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // RAII Transform Guard
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        class TransformScope final
        {
        public:
            explicit TransformScope(
                TransformManager& manager
            );

            TransformScope(
                TransformScope&& other
            ) noexcept;

            TransformScope& operator=(
                TransformScope&& other
                ) noexcept;

            ~TransformScope() noexcept;

            void Release() noexcept;
            void Reset() noexcept;

            TransformScope(const TransformScope&) = delete;
            TransformScope& operator=(const TransformScope&) = delete;

        private:
            TransformManager* m_manager;
            bool m_active;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit TransformManager();

        TransformManager(const TransformManager&) = delete;
        TransformManager& operator=(const TransformManager&) = delete;

        // IRenderComponent implementation
        void OnRenderTargetChanged(
            const wrl::ComPtr<ID2D1RenderTarget>& renderTarget
        ) override;

        void OnDeviceLost() override;

        Priority GetPriority() const override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Transform Stack Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void PushTransform();

        void PopTransform();

        [[nodiscard]] TransformScope CreateScope();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Transform Operations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RotateAt(
            const Point& center,
            float angleDegrees
        );

        void ScaleAt(
            const Point& center,
            float scaleX,
            float scaleY
        );

        void TranslateBy(
            float dx,
            float dy
        );

        void SetTransform(
            const D2D1_MATRIX_3X2_F& transform
        );

        void ResetTransform();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] size_t GetStackDepth() const noexcept;

        [[nodiscard]] bool IsStackEmpty() const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helper Methods (DRY)
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ApplyRelativeTransform(
            const D2D1_MATRIX_3X2_F& transform
        );

        void ClearTransformStack();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        wrl::ComPtr<ID2D1RenderTarget> m_renderTarget;
        std::stack<D2D1_MATRIX_3X2_F> m_transformStack;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_TRANSFORM_MANAGER_H