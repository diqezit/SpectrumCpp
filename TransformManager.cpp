// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the TransformManager class
// It provides a stateful wrapper around Direct2D's transformation matrix
// to simplify rotation, scaling, and translation operations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "TransformManager.h"
#include "MathUtils.h"

namespace Spectrum {

    namespace {
        inline D2D1_POINT_2F ToD2DPoint(const Point& p) {
            return D2D1::Point2F(p.x, p.y);
        }
    }

    TransformManager::TransformManager(ID2D1RenderTarget* renderTarget)
        : m_renderTarget(renderTarget)
    {
    }

    // save current transform to apply nested transforms
    void TransformManager::PushTransform() {
        if (m_renderTarget) {
            D2D1_MATRIX_3X2_F transform;
            m_renderTarget->GetTransform(&transform);
            m_transformStack.push(transform);
        }
    }

    // restore previous transform after nested operations
    void TransformManager::PopTransform() {
        if (m_renderTarget) {
            if (!m_transformStack.empty()) {
                m_renderTarget->SetTransform(m_transformStack.top());
                m_transformStack.pop();
            }
            else {
                // fall back to identity if stack is empty to prevent errors
                m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
            }
        }
    }

    // apply rotation relative to current transform
    void TransformManager::RotateAt(const Point& center, float angleDegrees) {
        if (m_renderTarget) {
            D2D1_MATRIX_3X2_F current, rotation;
            m_renderTarget->GetTransform(&current);
            rotation = D2D1::Matrix3x2F::Rotation(angleDegrees, ToD2DPoint(center));
            m_renderTarget->SetTransform(rotation * current);
        }
    }

    // apply scaling relative to current transform
    void TransformManager::ScaleAt(const Point& center, float scaleX, float scaleY) {
        if (m_renderTarget) {
            D2D1_MATRIX_3X2_F current, scale;
            m_renderTarget->GetTransform(&current);
            scale = D2D1::Matrix3x2F::Scale(scaleX, scaleY, ToD2DPoint(center));
            m_renderTarget->SetTransform(scale * current);
        }
    }

    // apply translation relative to current transform
    void TransformManager::TranslateBy(float dx, float dy) {
        if (m_renderTarget) {
            D2D1_MATRIX_3X2_F current, translation;
            m_renderTarget->GetTransform(&current);
            translation = D2D1::Matrix3x2F::Translation(dx, dy);
            m_renderTarget->SetTransform(translation * current);
        }
    }

    void TransformManager::SetTransform(const D2D1_MATRIX_3X2_F& transform) {
        if (m_renderTarget) {
            m_renderTarget->SetTransform(transform);
        }
    }

    void TransformManager::ResetTransform() {
        if (m_renderTarget) {
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        }
    }

    // when render target is recreated, transform stack must be cleared
    // otherwise old transforms could be applied to new target
    void TransformManager::UpdateRenderTarget(ID2D1RenderTarget* renderTarget) {
        m_renderTarget = renderTarget;

        while (!m_transformStack.empty()) {
            m_transformStack.pop();
        }
    }

} // namespace Spectrum