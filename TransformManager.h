#ifndef SPECTRUM_CPP_TRANSFORM_MANAGER_H
#define SPECTRUM_CPP_TRANSFORM_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file defines the TransformManager class, which handles all 2D
// transformations for the render target. It uses a stack to manage
// nested transforms, allowing for complex scene graph manipulations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common.h"
#include <stack>

namespace Spectrum {

    class TransformManager {
    public:
        explicit TransformManager(ID2D1RenderTarget* renderTarget);

        void PushTransform();
        void PopTransform();

        void RotateAt(const Point& center, float angleDegrees);
        void ScaleAt(const Point& center, float scaleX, float scaleY);
        void TranslateBy(float dx, float dy);

        void SetTransform(const D2D1_MATRIX_3X2_F& transform);
        void ResetTransform();

        void UpdateRenderTarget(ID2D1RenderTarget* renderTarget);

    private:
        ID2D1RenderTarget* m_renderTarget;
        std::stack<D2D1_MATRIX_3X2_F> m_transformStack;
    };

} // namespace Spectrum

#endif