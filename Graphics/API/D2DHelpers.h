#ifndef SPECTRUM_CPP_D2D_HELPERS_H
#define SPECTRUM_CPP_D2D_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// D2DHelpers - Unified facade for all Direct2D utility functions
//
// This header maintains backward compatibility by including all modular
// helper headers. Existing code continues to work, while new code can
// include specific headers for faster compilation.
//
// Migration path:
//   OLD: #include "D2DHelpers.h"
//   NEW: #include "Helpers/TypeConversion.h"  // Include only what you need
//
// Version 2.0: Added Enums, Structs, and EnumConversion support
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Core types (must be first)
#include "Common/Types.h"

// NEW: Enums (v2.0)
#include "Graphics/API/Enums/PaintEnums.h"
#include "Graphics/API/Enums/RenderEnums.h"
#include "Graphics/API/Enums/TextEnums.h"

// NEW: Structs (v2.0)
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/API/Structs/StrokeOptions.h"
#include "Graphics/API/Structs/TextStyle.h"

// Helpers (modular)
#include "Graphics/API/Helpers/TypeConversion.h"
#include "Graphics/API/Helpers/EnumConversion.h"
#include "Graphics/API/Helpers/MathConstants.h"
#include "Graphics/API/Helpers/Validation.h"
#include "Graphics/API/Helpers/Sanitization.h"
#include "Graphics/API/Helpers/HResultHelpers.h"
#include "Graphics/API/Helpers/D2DScopes.h"

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Backward Compatibility Namespace
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

namespace Spectrum::D2DHelpers {

    // Re-export all helper namespaces for backward compatibility
    using namespace Helpers::TypeConversion;
    using namespace Helpers::EnumConversion;    // NEW: v2.0
    using namespace Helpers::Math;
    using namespace Helpers::Validate;
    using namespace Helpers::Sanitize;
    using namespace Helpers::HResult;
    using namespace Helpers::Scopes;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Geometry Building Helpers (kept in facade for legacy compatibility)
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
#define SPECTRUM_D2DHELPERS_VERSION_MINOR 0
#define SPECTRUM_D2DHELPERS_HAS_PAINT_V2 1
#define SPECTRUM_D2DHELPERS_HAS_ENUMS 1

#endif // SPECTRUM_CPP_D2D_HELPERS_H