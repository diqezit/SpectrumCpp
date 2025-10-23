// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements common rendering utilities to reduce code duplication across
// renderer classes. Provides validated, exception-safe helper functions.
//
// Implementation details:
// - All validation functions are noexcept for performance
// - Resource creation includes comprehensive error checking
// - RAII wrappers ensure proper cleanup in all code paths
// - Caching utilities are thread-safe via mutable containers
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Helpers/Rendering/RenderHelpers.h"
#include "Graphics/API/D2DHelpers.h"
#include "Graphics/API/Helpers/Core/Validation.h"
#include <cmath>

namespace Spectrum {
    namespace Helpers {
        namespace Rendering {

            using namespace Helpers::TypeConversion;
            using namespace Helpers::HResult;
            using namespace Helpers::Scopes;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // RenderValidation Implementation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            bool RenderValidation::ValidateRenderTarget(ID2D1RenderTarget* target) noexcept
            {
                return target != nullptr;
            }

            bool RenderValidation::ValidateWriteFactory(IDWriteFactory* factory) noexcept
            {
                return factory != nullptr;
            }

            bool RenderValidation::ValidateBrush(ID2D1Brush* brush) noexcept
            {
                return brush != nullptr;
            }

            bool RenderValidation::ValidateGeometry(ID2D1Geometry* geometry) noexcept
            {
                return geometry != nullptr;
            }

            bool RenderValidation::ValidateRenderingContext(
                ID2D1RenderTarget* target,
                ID2D1Brush* brush
            ) noexcept {
                return ValidateRenderTarget(target) && ValidateBrush(brush);
            }

            bool RenderValidation::ValidateTextRenderingContext(
                ID2D1RenderTarget* target,
                IDWriteFactory* factory,
                const std::wstring& text
            ) noexcept {
                return ValidateRenderTarget(target) &&
                    ValidateWriteFactory(factory) &&
                    !text.empty();
            }

            bool RenderValidation::ValidatePointArray(
                const std::vector<Point>& points,
                size_t minSize
            ) noexcept {
                return Helpers::Validate::PointArray(points, minSize);
            }

            bool RenderValidation::ValidateGradientStops(
                const std::vector<D2D1_GRADIENT_STOP>& stops
            ) noexcept {
                return Helpers::Validate::GradientStops(stops);
            }

            bool RenderValidation::ValidatePositiveRadius(float radius) noexcept
            {
                return Helpers::Validate::PositiveRadius(radius);
            }

            bool RenderValidation::ValidateRadiusRange(
                float innerRadius,
                float outerRadius
            ) noexcept {
                return Helpers::Validate::RadiusRange(innerRadius, outerRadius);
            }

            bool RenderValidation::ValidateNonZeroAngle(float angle) noexcept
            {
                return Helpers::Validate::NonZeroAngle(angle);
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // BrushManager Implementation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            wrl::ComPtr<ID2D1SolidColorBrush> BrushManager::CreateSolidBrush(
                ID2D1RenderTarget* target,
                const Color& color
            ) {
                if (!target) {
                    return nullptr;
                }

                wrl::ComPtr<ID2D1SolidColorBrush> brush;
                HRESULT hr = target->CreateSolidColorBrush(
                    ToD2DColor(color),
                    &brush
                );

                if (!CheckComCreation(hr, "CreateSolidColorBrush", brush)) {
                    return nullptr;
                }

                return brush;
            }

            wrl::ComPtr<ID2D1LinearGradientBrush> BrushManager::CreateLinearGradientBrush(
                ID2D1RenderTarget* target,
                const Point& start,
                const Point& end,
                ID2D1GradientStopCollection* stops
            ) {
                if (!target || !stops) {
                    return nullptr;
                }

                wrl::ComPtr<ID2D1LinearGradientBrush> brush;
                HRESULT hr = target->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(
                        ToD2DPoint(start),
                        ToD2DPoint(end)
                    ),
                    stops,
                    &brush
                );

                if (!CheckComCreation(hr, "CreateLinearGradientBrush", brush)) {
                    return nullptr;
                }

                return brush;
            }

            wrl::ComPtr<ID2D1RadialGradientBrush> BrushManager::CreateRadialGradientBrush(
                ID2D1RenderTarget* target,
                const Point& center,
                float radiusX,
                float radiusY,
                ID2D1GradientStopCollection* stops
            ) {
                if (!target || !stops) {
                    return nullptr;
                }

                wrl::ComPtr<ID2D1RadialGradientBrush> brush;
                HRESULT hr = target->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                        ToD2DPoint(center),
                        D2D1::Point2F(0, 0),
                        radiusX,
                        radiusY
                    ),
                    stops,
                    &brush
                );

                if (!CheckComCreation(hr, "CreateRadialGradientBrush", brush)) {
                    return nullptr;
                }

                return brush;
            }

            wrl::ComPtr<ID2D1GradientStopCollection> BrushManager::CreateGradientStops(
                ID2D1RenderTarget* target,
                const std::vector<D2D1_GRADIENT_STOP>& stops
            ) {
                if (!target || stops.empty()) {
                    return nullptr;
                }

                wrl::ComPtr<ID2D1GradientStopCollection> collection;
                HRESULT hr = target->CreateGradientStopCollection(
                    stops.data(),
                    static_cast<UINT32>(stops.size()),
                    D2D1_GAMMA_2_2,
                    D2D1_EXTEND_MODE_CLAMP,
                    &collection
                );

                if (!CheckComCreation(hr, "CreateGradientStopCollection", collection)) {
                    return nullptr;
                }

                return collection;
            }

            BrushManager::BrushColorScope::BrushColorScope(ID2D1SolidColorBrush* brush)
                : m_brush(brush)
                , m_originalColor{}
            {
                if (m_brush) {
                    m_originalColor = m_brush->GetColor();
                }
            }

            BrushManager::BrushColorScope::~BrushColorScope()
            {
                if (m_brush) {
                    m_brush->SetColor(m_originalColor);
                }
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // HashGenerator Implementation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            uint64_t HashGenerator::GenerateStrokeStyleKey(
                D2D1_CAP_STYLE startCap,
                D2D1_CAP_STYLE endCap,
                D2D1_CAP_STYLE dashCap,
                D2D1_LINE_JOIN lineJoin,
                D2D1_DASH_STYLE dashStyle,
                float dashOffset
            ) noexcept {
                uint32_t offsetBits;
                std::memcpy(&offsetBits, &dashOffset, sizeof(float));

                return (static_cast<uint64_t>(startCap) << 0) |
                    (static_cast<uint64_t>(endCap) << 4) |
                    (static_cast<uint64_t>(dashCap) << 8) |
                    (static_cast<uint64_t>(lineJoin) << 12) |
                    (static_cast<uint64_t>(dashStyle) << 16) |
                    (static_cast<uint64_t>(offsetBits) << 32);
            }

            size_t HashGenerator::GenerateTextFormatKey(
                const std::wstring& fontFamily,
                float fontSize,
                DWRITE_FONT_WEIGHT weight,
                DWRITE_FONT_STYLE style,
                DWRITE_FONT_STRETCH stretch,
                DWRITE_TEXT_ALIGNMENT textAlign,
                DWRITE_PARAGRAPH_ALIGNMENT paragraphAlign
            ) noexcept {
                return GenerateHash(
                    fontFamily,
                    fontSize,
                    static_cast<size_t>(weight),
                    static_cast<size_t>(style),
                    static_cast<size_t>(stretch),
                    static_cast<size_t>(textAlign),
                    static_cast<size_t>(paragraphAlign)
                );
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // StrokeStyleManager Implementation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            wrl::ComPtr<ID2D1StrokeStyle> StrokeStyleManager::CreateStrokeStyle(
                ID2D1Factory* factory,
                const D2D1_STROKE_STYLE_PROPERTIES& properties,
                const float* dashes,
                UINT32 dashCount
            ) {
                if (!factory) {
                    return nullptr;
                }

                wrl::ComPtr<ID2D1StrokeStyle> strokeStyle;
                HRESULT hr = factory->CreateStrokeStyle(
                    properties,
                    dashes,
                    dashCount,
                    &strokeStyle
                );

                if (!CheckComCreation(hr, "CreateStrokeStyle", strokeStyle)) {
                    return nullptr;
                }

                return strokeStyle;
            }

            D2D1_STROKE_STYLE_PROPERTIES StrokeStyleManager::CreateStrokeProperties(
                D2D1_CAP_STYLE startCap,
                D2D1_CAP_STYLE endCap,
                D2D1_CAP_STYLE dashCap,
                D2D1_LINE_JOIN lineJoin,
                float miterLimit,
                D2D1_DASH_STYLE dashStyle,
                float dashOffset
            ) noexcept {
                return D2D1::StrokeStyleProperties(
                    startCap,
                    endCap,
                    dashCap,
                    lineJoin,
                    miterLimit,
                    dashStyle,
                    dashOffset
                );
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // FactoryHelper Implementation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            wrl::ComPtr<ID2D1Factory> FactoryHelper::GetFactoryFromRenderTarget(
                ID2D1RenderTarget* renderTarget
            ) {
                if (!renderTarget) {
                    return nullptr;
                }

                wrl::ComPtr<ID2D1Factory> factory;
                renderTarget->GetFactory(factory.GetAddressOf());
                return factory;
            }

            wrl::ComPtr<IDWriteTextFormat> FactoryHelper::CreateTextFormat(
                IDWriteFactory* writeFactory,
                const std::wstring& fontFamily,
                float fontSize,
                DWRITE_FONT_WEIGHT weight,
                DWRITE_FONT_STYLE style,
                DWRITE_FONT_STRETCH stretch
            ) {
                if (!writeFactory) {
                    return nullptr;
                }

                wrl::ComPtr<IDWriteTextFormat> textFormat;
                HRESULT hr = writeFactory->CreateTextFormat(
                    fontFamily.c_str(),
                    nullptr,
                    weight,
                    style,
                    stretch,
                    fontSize,
                    L"en-us",
                    &textFormat
                );

                if (!CheckComCreation(hr, "CreateTextFormat", textFormat)) {
                    return nullptr;
                }

                return textFormat;
            }

            wrl::ComPtr<IDWriteTextLayout> FactoryHelper::CreateTextLayout(
                IDWriteFactory* writeFactory,
                const std::wstring& text,
                IDWriteTextFormat* format,
                float maxWidth,
                float maxHeight
            ) {
                if (!writeFactory || !format || text.empty()) {
                    return nullptr;
                }

                wrl::ComPtr<IDWriteTextLayout> textLayout;
                HRESULT hr = writeFactory->CreateTextLayout(
                    text.c_str(),
                    static_cast<UINT32>(text.length()),
                    format,
                    maxWidth,
                    maxHeight,
                    &textLayout
                );

                if (!CheckComCreation(hr, "CreateTextLayout", textLayout)) {
                    return nullptr;
                }

                return textLayout;
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            // RenderPatterns Implementation
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            void RenderPatterns::DrawWithShadow(
                ID2D1RenderTarget* target,
                const std::function<void()>& drawFunc,
                const Point& shadowOffset,
                const Color& shadowColor,
                ID2D1SolidColorBrush* brush
            ) {
                if (!target || !drawFunc) return;

                D2D1_COLOR_F originalColor{};
                if (brush) {
                    originalColor = brush->GetColor();
                    brush->SetColor(ToD2DColor(shadowColor));
                }

                {
                    D2D1_MATRIX_3X2_F oldTransform;
                    target->GetTransform(&oldTransform);

                    D2D1_MATRIX_3X2_F shadowTransform = D2D1::Matrix3x2F::Translation(
                        shadowOffset.x,
                        shadowOffset.y
                    ) * oldTransform;

                    ScopedTransform transform(target, shadowTransform);
                    drawFunc();
                }

                if (brush) {
                    brush->SetColor(originalColor);
                }
                drawFunc();
            }

            void RenderPatterns::DrawWithOutline(
                const std::function<void()>& drawFunc,
                float /*outlineWidth*/,
                int passes
            ) {
                if (!drawFunc) return;

                for (int i = 0; i < passes; ++i) {
                    drawFunc();
                }

                drawFunc();
            }

            void RenderPatterns::DrawMirrored(
                ID2D1RenderTarget* target,
                const std::function<void()>& drawFunc,
                bool horizontal,
                bool vertical,
                const Point& pivot
            ) {
                if (!target || !drawFunc) return;

                drawFunc();

                D2D1_MATRIX_3X2_F oldTransform;
                target->GetTransform(&oldTransform);

                D2D1_MATRIX_3X2_F mirrorMatrix = D2D1::Matrix3x2F::Identity();

                if (horizontal) {
                    mirrorMatrix = D2D1::Matrix3x2F::Scale(
                        -1.0f, 1.0f,
                        D2D1::Point2F(pivot.x, pivot.y)
                    );
                }

                if (vertical) {
                    auto vMatrix = D2D1::Matrix3x2F::Scale(
                        1.0f, -1.0f,
                        D2D1::Point2F(pivot.x, pivot.y)
                    );
                    mirrorMatrix = mirrorMatrix * vMatrix;
                }

                ScopedTransform transform(target, mirrorMatrix * oldTransform);
                drawFunc();
            }

        } // namespace Rendering
    } // namespace Helpers
} // namespace Spectrum