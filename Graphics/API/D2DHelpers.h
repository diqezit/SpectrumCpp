#ifndef SPECTRUM_CPP_D2D_HELPERS_H
#define SPECTRUM_CPP_D2D_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines centralized Direct2D utility functions and RAII wrappers for
// managing D2D render states. This eliminates code duplication across
// multiple renderer classes and ensures proper state restoration.
//
// THIS FILE IS NOW A BACKWARD-COMPATIBILITY FACADE.
// It includes the new modular helpers and re-exports their namespaces.
// New code should include specific helpers directly (e.g., "Helpers/TypeConversion.h").
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Helpers/TypeConversion.h"
#include "Helpers/MathConstants.h"
#include "Helpers/Validation.h"
#include "Helpers/Sanitization.h"
#include "Helpers/HResultHelpers.h"
#include "Helpers/D2DScopes.h"

namespace Spectrum {
    namespace D2DHelpers {

        // Re-export all modular helper namespaces for backward compatibility.
        using namespace Spectrum::Helpers::TypeConversion;
        using namespace Spectrum::Helpers::Math;
        using namespace Spectrum::Helpers::Validate;
        using namespace Spectrum::Helpers::Sanitize;
        using namespace Spectrum::Helpers::HResult;
        using namespace Spectrum::Helpers::Scopes;

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

    } // namespace D2DHelpers
} // namespace Spectrum

#endif // SPECTRUM_CPP_D2D_HELPERS_H