// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the RendererManager, which handles the lifecycle and
// configuration of all visualization renderers.
//
// Key implementation details:
// - Manages a collection of IRenderer implementations
// - Handles switching between different visualization styles
// - Applies global quality settings across all renderers
// - Uses centralized validation system (Validation.h)
// - Logs all operations for debugging and diagnostics
// - Guarantees transactional state changes with rollback support
//
// Validation approach:
// - All pointer validation via VALIDATE_PTR_OR_RETURN* macros
// - Condition validation via VALIDATE_CONDITION_OR_RETURN_FALSE
// - Domain-specific validation (dimensions) kept local
// - No duplicate validation logic across components
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RendererManager.h"
#include "Graphics/Visualizers/BarsRenderer.h"
#include "Graphics/Visualizers/CircularWaveRenderer.h"
#include "Graphics/Visualizers/CubesRenderer.h"
#include "Common/EventBus.h"
#include "Graphics/Visualizers/FireRenderer.h"
#include "Graphics/Visualizers/GaugeRenderer.h"
#include "Graphics/Visualizers/KenwoodBarsRenderer.h"
#include "Graphics/Visualizers/LedPanelRenderer.h"
#include "Graphics/Visualizers/MatrixLedRenderer.h"
#include "Graphics/Visualizers/ParticlesRenderer.h"
#include "Graphics/Visualizers/PolylineWaveRenderer.h"
#include "Graphics/Visualizers/SphereRenderer.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/Visualizers/WaveRenderer.h"
#include "Platform/WindowManager.h"

namespace Spectrum
{
    using namespace Helpers::Validate;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    RendererManager::RendererManager(
        EventBus* bus,
        Platform::WindowManager* windowManager
    )
        : m_currentState()
        , m_currentQuality(RenderQuality::Medium)
        , m_windowManager(windowManager)
    {
        LOG_INFO("RendererManager: Initializing...");

        if (!Pointer(windowManager, "WindowManager", "RendererManager")) {
            LOG_ERROR("RendererManager: WindowManager is null!");
        }

        SubscribeToEvents(bus);
    }

    RendererManager::~RendererManager()
    {
        LOG_INFO("RendererManager: Shutting down...");

        if (m_currentState.renderer) {
            SafeDeactivateRenderer(m_currentState.renderer);
        }

        LOG_INFO("RendererManager: Destroyed " << m_renderers.size() << " renderer(s)");
    }

    [[nodiscard]] bool RendererManager::Initialize()
    {
        LOG_INFO("RendererManager: Starting initialization...");

        VALIDATE_PTR_OR_RETURN_FALSE(m_windowManager, "RendererManager");

        if (!CreateRenderers()) {
            LOG_ERROR("RendererManager: Renderer creation failed");
            return false;
        }

        if (!ActivateInitialRenderer()) {
            LOG_ERROR("RendererManager: Initial renderer activation failed");
            return false;
        }

        LOG_INFO("RendererManager: Initialization completed successfully");
        LOG_INFO("RendererManager: " << m_renderers.size() << " renderer(s) available");

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Event Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::OnResize(int width, int height)
    {
        if (!ValidateDimensions(width, height)) {
            LOG_WARNING("RendererManager: Invalid resize dimensions ("
                << width << "x" << height << ")");
            return;
        }

        LOG_INFO("RendererManager: Resizing to " << width << "x" << height);

        if (!m_currentState.isActive || !m_currentState.renderer) {
            LOG_WARNING("RendererManager: No active renderer to resize");
            return;
        }

        try {
            m_currentState.renderer->OnResize(width, height);
            m_currentState.renderer->SetQuality(m_currentQuality);
            LOG_INFO("RendererManager: Resize completed successfully");
        }
        catch (const std::exception&) {
            LOG_ERROR("RendererManager: Resize failed, attempting recovery");
            if (!AttemptRendererRecovery(m_currentState.style)) {
                LOG_CRITICAL("RendererManager: Failed to recover from resize error!");
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Configuration & Setters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::SetCurrentRenderer(RenderStyle style)
    {
        if (style == m_currentState.style && m_currentState.isActive) {
            LOG_INFO("RendererManager: Already using requested renderer");
            return;
        }

        if (!IsRendererAvailable(style)) {
            LOG_ERROR("RendererManager: Renderer style "
                << static_cast<int>(style) << " not available");
            return;
        }

        if (!SwitchRenderer(style)) {
            LOG_ERROR("RendererManager: Failed to switch renderer");
        }
    }

    void RendererManager::SwitchToNextRenderer()
    {
        LOG_INFO("RendererManager: Cycling to next renderer");
        SetCurrentRenderer(Helpers::Utils::CycleEnum(m_currentState.style, 1));
    }

    void RendererManager::SwitchToPrevRenderer()
    {
        LOG_INFO("RendererManager: Cycling to previous renderer");
        SetCurrentRenderer(Helpers::Utils::CycleEnum(m_currentState.style, -1));
    }

    void RendererManager::CycleQuality(int direction)
    {
        LOG_INFO("RendererManager: Cycling quality (direction: " << direction << ")");
        SetQuality(Helpers::Utils::CycleEnum(m_currentQuality, direction));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] IRenderer* RendererManager::GetCurrentRenderer() const noexcept
    {
        return m_currentState.isActive ? m_currentState.renderer : nullptr;
    }

    [[nodiscard]] RenderStyle RendererManager::GetCurrentStyle() const noexcept
    {
        return m_currentState.style;
    }

    [[nodiscard]] RenderQuality RendererManager::GetQuality() const noexcept
    {
        return m_currentQuality;
    }

    [[nodiscard]] std::string_view RendererManager::GetCurrentRendererName() const noexcept
    {
        if (!m_currentState.isActive || !m_currentState.renderer) {
            return "None";
        }

        try {
            return m_currentState.renderer->GetName();
        }
        catch (...) {
            LOG_ERROR("RendererManager: Exception while getting renderer name");
            return "Error";
        }
    }

    [[nodiscard]] std::string_view RendererManager::GetQualityName() const noexcept
    {
        switch (m_currentQuality)
        {
        case RenderQuality::Low:    return "Low";
        case RenderQuality::Medium: return "Medium";
        case RenderQuality::High:   return "High";
        case RenderQuality::Ultra:  return "Ultra";
        default:                    return "Unknown";
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool RendererManager::IsRendererActive() const noexcept
    {
        return m_currentState.isActive && m_currentState.renderer != nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Diagnostics
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] size_t RendererManager::GetRendererCount() const noexcept
    {
        return m_renderers.size();
    }

    [[nodiscard]] bool RendererManager::IsRendererAvailable(RenderStyle style) const noexcept
    {
        return m_renderers.find(style) != m_renderers.end();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::SubscribeToEvents(EventBus* bus)
    {
        if (!Pointer(bus, "EventBus", "RendererManager")) {
            LOG_WARNING("RendererManager: EventBus is null, skipping event subscription");
            return;
        }

        LOG_INFO("RendererManager: Subscribing to events...");

        try {
            bus->Subscribe(InputAction::SwitchRenderer, [this]() {
                SwitchToNextRenderer();
                });

            bus->Subscribe(InputAction::CycleQuality, [this]() {
                CycleQuality(1);
                });

            LOG_INFO("RendererManager: Event subscription completed");
        }
        catch (const std::exception&) {
            LOG_ERROR("RendererManager: Event subscription failed");
        }
    }

    bool RendererManager::CreateRenderers()
    {
        LOG_INFO("RendererManager: Creating renderers...");

        try {
            m_renderers[RenderStyle::Bars] = std::make_unique<BarsRenderer>();
            m_renderers[RenderStyle::Wave] = std::make_unique<WaveRenderer>();
            m_renderers[RenderStyle::CircularWave] = std::make_unique<CircularWaveRenderer>();
            m_renderers[RenderStyle::Cubes] = std::make_unique<CubesRenderer>();
            m_renderers[RenderStyle::Fire] = std::make_unique<FireRenderer>();
            m_renderers[RenderStyle::LedPanel] = std::make_unique<LedPanelRenderer>();
            m_renderers[RenderStyle::Gauge] = std::make_unique<GaugeRenderer>();
            m_renderers[RenderStyle::KenwoodBars] = std::make_unique<KenwoodBarsRenderer>();
            m_renderers[RenderStyle::Particles] = std::make_unique<ParticlesRenderer>();
            m_renderers[RenderStyle::MatrixLed] = std::make_unique<MatrixLedRenderer>();
            m_renderers[RenderStyle::Sphere] = std::make_unique<SphereRenderer>();
            m_renderers[RenderStyle::PolylineWave] = std::make_unique<PolylineWaveRenderer>();

            LogRendererCreation();
            return true;
        }
        catch (const std::exception&) {
            LOG_ERROR("RendererManager: Exception during renderer creation");
            return false;
        }
    }

    bool RendererManager::ActivateInitialRenderer()
    {
        LOG_INFO("RendererManager: Activating initial renderer...");

        const RenderStyle initialStyle = RenderStyle::Bars;

        if (!ActivateNewRenderer(initialStyle)) {
            LOG_ERROR("RendererManager: Failed to activate initial renderer, trying fallbacks");

            // Try to activate any available renderer as fallback
            for (const auto& [style, renderer] : m_renderers) {
                if (renderer && ActivateNewRenderer(style)) {
                    LOG_INFO("RendererManager: Fallback renderer activated: "
                        << renderer->GetName().data());
                    return true;
                }
            }

            LOG_CRITICAL("RendererManager: No renderer could be activated!");
            return false;
        }

        LOG_INFO("RendererManager: Initial renderer activated: "
            << GetCurrentRendererName().data());

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Renderer Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool RendererManager::SwitchRenderer(RenderStyle newStyle)
    {
        // Save current state for potential rollback
        const RendererState previousState = m_currentState;

        // Prepare activation context WITHOUT modifying current state
        ActivationContext context;
        if (!PrepareActivationContext(newStyle, context)) {
            LogActivationFailure(newStyle, "Failed to prepare activation context");
            return false;
        }

        // Try to activate new renderer WITHOUT deactivating current
        if (!TryActivateRenderer(context)) {
            LogActivationFailure(newStyle, "Activation failed");
            return false;
        }

        // Deactivate previous renderer (if any) after successful activation
        if (previousState.isActive && previousState.renderer) {
            SafeDeactivateRenderer(previousState.renderer);
        }

        // Commit new state atomically
        CommitRendererState(context);

        // Log successful switch
        LogRendererSwitch(previousState.style, newStyle);

        return true;
    }

    bool RendererManager::ActivateNewRenderer(RenderStyle style)
    {
        // Prepare activation context
        ActivationContext context;
        if (!PrepareActivationContext(style, context)) {
            return false;
        }

        // Try activation
        if (!TryActivateRenderer(context)) {
            return false;
        }

        // Commit state
        CommitRendererState(context);

        return true;
    }

    bool RendererManager::PrepareActivationContext(
        RenderStyle style,
        ActivationContext& outContext
    ) const
    {
        IRenderer* renderer = FindRenderer(style);
        if (!renderer) {
            LogActivationFailure(style, "Renderer not found");
            return false;
        }

        // Get dimensions from engine
        int width = 0, height = 0;
        if (!GetEngineDimensions(width, height)) {
            LogActivationFailure(style, "Failed to get engine dimensions");
            return false;
        }

        if (!ValidateDimensions(width, height)) {
            LogActivationFailure(style, "Invalid dimensions");
            return false;
        }

        outContext.renderer = renderer;
        outContext.style = style;
        outContext.width = width;
        outContext.height = height;

        return true;
    }

    bool RendererManager::TryActivateRenderer(const ActivationContext& context) const noexcept
    {
        try {
            context.renderer->OnActivate(context.width, context.height);
            context.renderer->SetQuality(m_currentQuality);
            return true;
        }
        catch (const std::exception&) {
            LOG_ERROR("RendererManager: Exception during activation");
            return false;
        }
    }

    void RendererManager::CommitRendererState(const ActivationContext& context) noexcept
    {
        m_currentState.renderer = context.renderer;
        m_currentState.style = context.style;
        m_currentState.isActive = true;

        LogActivationSuccess(context);
    }

    bool RendererManager::AttemptRendererRecovery(RenderStyle fallbackStyle)
    {
        LOG_INFO("RendererManager: Attempting recovery with fallback style: "
            << static_cast<int>(fallbackStyle));

        // Clear current broken state
        m_currentState.Clear();

        // Try to activate fallback
        if (ActivateNewRenderer(fallbackStyle)) {
            LOG_INFO("RendererManager: Recovery successful");
            return true;
        }

        LOG_CRITICAL("RendererManager: Recovery failed!");
        return false;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Quality Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::SetQuality(RenderQuality quality)
    {
        m_currentQuality = quality;

        LOG_INFO("RendererManager: Setting quality to " << GetQualityName().data());

        ApplyQualityToAllRenderers(quality);
    }

    void RendererManager::ApplyQualityToAllRenderers(RenderQuality quality)
    {
        int successCount = 0;
        int failCount = 0;

        for (auto& [style, renderer] : m_renderers)
        {
            if (!renderer) {
                LOG_WARNING("RendererManager: Null renderer for style "
                    << static_cast<int>(style));
                ++failCount;
                continue;
            }

            SafeSetQuality(renderer.get(), quality);
            ++successCount;
        }

        LOG_INFO("RendererManager: Quality update completed - "
            << successCount << " succeeded, " << failCount << " failed");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helper Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool RendererManager::ValidateDimensions(
        int width,
        int height
    ) const noexcept
    {
        constexpr int kMinDimension = 1;
        constexpr int kMaxDimension = 16384;

        return width >= kMinDimension && width <= kMaxDimension &&
            height >= kMinDimension && height <= kMaxDimension;
    }

    [[nodiscard]] IRenderer* RendererManager::FindRenderer(RenderStyle style) const noexcept
    {
        auto it = m_renderers.find(style);

        if (it == m_renderers.end() || !it->second) {
            LOG_ERROR("RendererManager: Renderer not found for style "
                << static_cast<int>(style));
            return nullptr;
        }

        return it->second.get();
    }

    [[nodiscard]] bool RendererManager::GetEngineDimensions(
        int& outWidth,
        int& outHeight
    ) const noexcept
    {
        if (!m_windowManager) {
            return false;
        }

        auto* engine = m_windowManager->GetVisualizationEngine();
        if (!engine) {
            return false;
        }

        try {
            outWidth = engine->GetWidth();
            outHeight = engine->GetHeight();
            return true;
        }
        catch (...) {
            LOG_ERROR("RendererManager: Exception while getting engine dimensions");
            return false;
        }
    }

    void RendererManager::SafeDeactivateRenderer(IRenderer* renderer) const noexcept
    {
        if (!renderer) return;

        try {
            LOG_INFO("RendererManager: Deactivating renderer: "
                << renderer->GetName().data());
            renderer->OnDeactivate();
        }
        catch (...) {
            LOG_ERROR("RendererManager: Exception during deactivation");
        }
    }

    void RendererManager::SafeSetQuality(IRenderer* renderer, RenderQuality quality) const noexcept
    {
        if (!renderer) return;

        try {
            renderer->SetQuality(quality);
        }
        catch (...) {
            LOG_ERROR("RendererManager: Failed to set quality for renderer");
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Logging Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void RendererManager::LogRendererSwitch(RenderStyle from, RenderStyle to) const
    {
        LOG_INFO("RendererManager: Renderer switched from "
            << static_cast<int>(from) << " to " << static_cast<int>(to));
    }

    void RendererManager::LogRendererCreation() const
    {
        LOG_INFO("RendererManager: Created " << m_renderers.size() << " renderer(s):");

        for (const auto& [style, renderer] : m_renderers) {
            if (renderer) {
                LOG_INFO("  - " << renderer->GetName().data()
                    << " (style: " << static_cast<int>(style) << ")");
            }
        }
    }

    void RendererManager::LogActivationSuccess(const ActivationContext& context) const
    {
        if (!context.renderer) return;

        try {
            LOG_INFO("RendererManager: Activated '"
                << context.renderer->GetName().data()
                << "' at " << context.width << "x" << context.height);
        }
        catch (...) {
            LOG_ERROR("RendererManager: Exception while logging activation success");
        }
    }

    void RendererManager::LogActivationFailure(
        RenderStyle style,
        const char* reason
    ) const
    {
        LOG_ERROR("RendererManager: Failed to activate renderer (style: "
            << static_cast<int>(style) << ") - " << reason);
    }

} // namespace Spectrum