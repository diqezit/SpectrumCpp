// TransformManager.cpp
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the TransformManager for 2D transformation handling.
//
// Implementation details:
// - Maintains a stack of transformation matrices for nested transforms
// - Provides relative transforms (multiply with current) and absolute
// - Includes stack depth limits to prevent overflow
// - Uses ComPtr for safe render target lifetime management
// - Uses D2DHelpers for validation and conversion
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "TransformManager.h"
#include "Graphics/API/D2DHelpers.h"
#include "Common/Logger.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // TransformScope Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    TransformManager::TransformScope::TransformScope(
        TransformManager& manager
    )
        : m_manager(&manager)
        , m_active(false)
    {
        m_manager->PushTransform();
        m_active = true;
    }

    TransformManager::TransformScope::TransformScope(
        TransformScope&& other
    ) noexcept
        : m_manager(other.m_manager)
        , m_active(other.m_active)
    {
        other.m_active = false;
    }

    TransformManager::TransformScope&
        TransformManager::TransformScope::operator=(
            TransformScope&& other
            ) noexcept
    {
        if (this != &other)
        {
            Reset();
            m_manager = other.m_manager;
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }

    TransformManager::TransformScope::~TransformScope() noexcept
    {
        Reset();
    }

    void TransformManager::TransformScope::Release() noexcept
    {
        m_active = false;
    }

    void TransformManager::TransformScope::Reset() noexcept
    {
        if (m_active && m_manager)
        {
            m_manager->PopTransform();
            m_active = false;
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    TransformManager::TransformManager()
        : m_renderTarget(nullptr)
    {
    }

    void TransformManager::OnRenderTargetChanged(
        const wrl::ComPtr<ID2D1RenderTarget>& renderTarget
    )
    {
        m_renderTarget = renderTarget;
        ClearTransformStack();
    }

    void TransformManager::OnDeviceLost()
    {
        m_renderTarget.Reset();
        ClearTransformStack();
    }

    IRenderComponent::Priority TransformManager::GetPriority() const
    {
        return Priority::High;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transform Stack Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TransformManager::PushTransform()
    {
        if (!m_renderTarget)
        {
            throw std::runtime_error("No render target");
        }

        if (m_transformStack.size() >= kMaxStackDepth)
        {
            throw std::runtime_error("Transform stack overflow");
        }

        D2D1_MATRIX_3X2_F currentTransform;
        m_renderTarget->GetTransform(&currentTransform);
        m_transformStack.push(currentTransform);
    }

    void TransformManager::PopTransform()
    {
        if (!m_renderTarget)
        {
            return;
        }

        if (m_transformStack.empty())
        {
            LOG_WARNING("Attempted to pop from empty transform stack");
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            return;
        }

        m_renderTarget->SetTransform(m_transformStack.top());
        m_transformStack.pop();
    }

    TransformManager::TransformScope TransformManager::CreateScope()
    {
        return TransformScope(*this);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Transform Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TransformManager::RotateAt(
        const Point& center,
        float angleDegrees
    )
    {
        const D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
            angleDegrees,
            ToD2DPoint(center)
        );

        ApplyRelativeTransform(rotation);
    }

    void TransformManager::ScaleAt(
        const Point& center,
        float scaleX,
        float scaleY
    )
    {
        const D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
            scaleX,
            scaleY,
            ToD2DPoint(center)
        );

        ApplyRelativeTransform(scale);
    }

    void TransformManager::TranslateBy(
        float dx,
        float dy
    )
    {
        const D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation(
            dx,
            dy
        );

        ApplyRelativeTransform(translation);
    }

    void TransformManager::SetTransform(
        const D2D1_MATRIX_3X2_F& transform
    )
    {
        if (m_renderTarget)
        {
            m_renderTarget->SetTransform(transform);
        }
    }

    void TransformManager::ResetTransform()
    {
        SetTransform(D2D1::Matrix3x2F::Identity());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    size_t TransformManager::GetStackDepth() const noexcept
    {
        return m_transformStack.size();
    }

    bool TransformManager::IsStackEmpty() const noexcept
    {
        return m_transformStack.empty();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helper Methods (DRY)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void TransformManager::ApplyRelativeTransform(
        const D2D1_MATRIX_3X2_F& transform
    )
    {
        if (!m_renderTarget)
        {
            return;
        }

        D2D1_MATRIX_3X2_F currentTransform;
        m_renderTarget->GetTransform(&currentTransform);
        m_renderTarget->SetTransform(transform * currentTransform);
    }

    void TransformManager::ClearTransformStack()
    {
        while (!m_transformStack.empty())
        {
            m_transformStack.pop();
        }
    }

} // namespace Spectrum