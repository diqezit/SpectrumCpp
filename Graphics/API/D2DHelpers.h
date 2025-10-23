#ifndef SPECTRUM_CPP_D2D_HELPERS_H
#define SPECTRUM_CPP_D2D_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// D2DHelpers - Unified facade for all Direct2D utility functions
//
// This header maintains backward compatibility by including all modular
// helper headers. Existing code continues to work, while new code can
// include specific headers for faster compilation.
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Core types (must be first)
#include "Common/Types.h"

// Enums
#include "Graphics/API/Enums/PaintEnums.h"
#include "Graphics/API/Enums/RenderEnums.h"
#include "Graphics/API/Enums/TextEnums.h"

// Structs
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/StrokeOptions.h"
#include "Graphics/API/Structs/TextStyle.h"

// Helpers
#include "Graphics/API/Helpers/Conversion/TypeConversion.h"
#include "Graphics/API/Helpers/Conversion/EnumConversion.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include "Graphics/API/Helpers/Core/Validation.h"
#include "Graphics/API/Helpers/Core/Sanitization.h"
#include "Graphics/API/Helpers/Core/HResultHelpers.h"
#include "Graphics/API/Helpers/D2D/D2DScopes.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Backward Compatibility Namespace
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace Spectrum::D2DHelpers {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::EnumConversion;
    using namespace Helpers::Math;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;
    using namespace Helpers::HResult;
    using namespace Helpers::Scopes;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Building Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    namespace Geometry {

        inline void BeginFigure(ID2D1GeometrySink* sink, const Point& startPoint, bool filled)
        {
            if (!sink) return;

            sink->BeginFigure(
                ToD2DPoint(startPoint),
                filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW
            );
        }

        inline void EndFigure(ID2D1GeometrySink* sink, bool closed)
        {
            if (!sink) return;

            sink->EndFigure(
                closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN
            );
        }

        inline void AddLine(ID2D1GeometrySink* sink, const Point& point)
        {
            if (!sink) return;
            sink->AddLine(ToD2DPoint(point));
        }

    } // namespace Geometry

} // namespace Spectrum::D2DHelpers

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Version Macros
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#define SPECTRUM_D2DHELPERS_VERSION_MAJOR 2
#define SPECTRUM_D2DHELPERS_VERSION_MINOR 1
#define SPECTRUM_D2DHELPERS_HAS_PAINT_V2 1
#define SPECTRUM_D2DHELPERS_HAS_ENUMS 1
#define SPECTRUM_D2DHELPERS_HIERARCHICAL_HELPERS 1

#endif // SPECTRUM_CPP_D2D_HELPERS_H