// TransformManager.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the TransformManager for 2D transformation handling.
//
// Implementation details:
// - Maintains a stack of transformation matrices for nested transforms
// - Provides relative transforms (multiply with current) and absolute
// - Includes stack depth limits to prevent overflow
// - All transform methods are const (modify render target, not manager state)
// - Uses D2DHelpers for validation and conversion
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "TransformManager.h"
#include "Graphics/API/D2DHelpers.h"
#include "Common/Logger.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    TransformManager::TransformManager()
        : m_renderTarget(nullptr)
    {
    }

    void TransformManager::OnRenderTargetChanged(ID2D1RenderTarget* renderTarget)
    {
        m_renderTarget = renderTarget;
        // Clear the stack as it relates to the old render target's state
        while (!m_transformStack.empty()) {
            m_transformStack.pop();
        }
    }

    void TransformManager::OnDeviceLost()
    {
        m_renderTarget = nullptr;
        while (!m_transformStack.empty()) {
            m_transformStack.pop();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transform Stack Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TransformManager::PushTransform() const
    {
        if (!m_renderTarget) return;

        if (m_transformStack.size() >= kMaxStackDepth) {
            LOG_ERROR("Transform stack depth exceeded maximum of " << kMaxStackDepth);
            return;
        }

        D2D1_MATRIX_3X2_F currentTransform;
        m_renderTarget->GetTransform(&currentTransform);
        m_transformStack.push(currentTransform);
    }

    void TransformManager::PopTransform() const
    {
        if (!m_renderTarget) return;

        if (m_transformStack.empty()) {
            LOG_WARNING("Attempted to pop from empty transform stack");
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            return;
        }

        m_renderTarget->SetTransform(m_transformStack.top());
        m_transformStack.pop();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transform Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TransformManager::RotateAt(const Point& center, float angleDegrees) const
    {
        if (!m_renderTarget) return;

        D2D1_MATRIX_3X2_F currentTransform;
        m_renderTarget->GetTransform(&currentTransform);

        const D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(angleDegrees, ToD2DPoint(center));
        m_renderTarget->SetTransform(rotation * currentTransform);
    }

    void TransformManager::ScaleAt(const Point& center, float scaleX, float scaleY) const
    {
        if (!m_renderTarget) return;

        D2D1_MATRIX_3X2_F currentTransform;
        m_renderTarget->GetTransform(&currentTransform);

        const D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(scaleX, scaleY, ToD2DPoint(center));
        m_renderTarget->SetTransform(scale * currentTransform);
    }

    void TransformManager::TranslateBy(float dx, float dy) const
    {
        if (!m_renderTarget) return;

        D2D1_MATRIX_3X2_F currentTransform;
        m_renderTarget->GetTransform(&currentTransform);

        const D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(dx, dy);
        m_renderTarget->SetTransform(translation * currentTransform);
    }

    void TransformManager::SetTransform(const D2D1_MATRIX_3X2_F& transform) const
    {
        if (m_renderTarget) {
            m_renderTarget->SetTransform(transform);
        }
    }

    void TransformManager::ResetTransform() const
    {
        if (m_renderTarget) {
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        }
    }

} // namespace Spectrum 