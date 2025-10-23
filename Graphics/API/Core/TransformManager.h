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
// - All transform operations are const (modify render target state, not manager)
// - Stack depth limited to prevent overflow (kMaxStackDepth)
// - RAII TransformScope for automatic push/pop
// - Non-owning pointer to render target (lifetime managed externally)
//
// Usage pattern:
//   {
//       auto scope = transformManager.CreateScope();
//       transformManager.RotateAt(center, 45.0f);
//       DrawComplexShape();
//   } // Automatic transform restoration
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
            explicit TransformScope(TransformManager& manager)
                : m_manager(manager)
            {
                m_manager.PushTransform();
            }

            ~TransformScope() noexcept
            {
                m_manager.PopTransform();
            }

            TransformScope(const TransformScope&) = delete;
            TransformScope& operator=(const TransformScope&) = delete;

        private:
            TransformManager& m_manager;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit TransformManager();

        TransformManager(const TransformManager&) = delete;
        TransformManager& operator=(const TransformManager&) = delete;

        // IRenderComponent implementation
        void OnRenderTargetChanged(ID2D1RenderTarget* renderTarget) override;
        void OnDeviceLost() override;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Transform Stack Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void PushTransform() const;
        void PopTransform() const;
        [[nodiscard]] TransformScope CreateScope() { return TransformScope(*this); }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Transform Operations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RotateAt(const Point& center, float angleDegrees) const;
        void ScaleAt(const Point& center, float scaleX, float scaleY) const;
        void TranslateBy(float dx, float dy) const;

        void SetTransform(const D2D1_MATRIX_3X2_F& transform) const;
        void ResetTransform() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] size_t GetStackDepth() const noexcept { return m_transformStack.size(); }
        [[nodiscard]] bool IsStackEmpty() const noexcept { return m_transformStack.empty(); }

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        ID2D1RenderTarget* m_renderTarget;
        mutable std::stack<D2D1_MATRIX_3X2_F> m_transformStack;
    };

} // namespace Spectrum

#endif