#ifndef SPECTRUM_CPP_RENDERER_MANAGER_H
#define SPECTRUM_CPP_RENDERER_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the RendererManager, responsible for managing the lifecycle
// of all available visualizers (IRenderer implementations).
//
// This class handles the creation, switching, and configuration of
// renderers, acting as a central authority for visualization style and
// quality settings.
//
// Error handling:
// - Uses centralized validation system (Graphics/API/Helpers/Core/Validation.h)
// - Provides detailed logging for debugging
// - Gracefully handles missing or invalid renderers
// - Ensures application stability during renderer switches
// - Guarantees transactional state changes (ACID-like behavior)
//
// Validation strategy:
// - All pointer validation via unified Validation.h helpers
// - Domain-specific validation (dimensions) kept local
// - Consistent error logging format across application
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <Common/Logger.h>
#include <map>
#include <memory>
#include <string_view>

namespace Spectrum
{
    class EventBus;
    class IRenderer;

    namespace Platform {
        class WindowManager;
    }

    class RendererManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit RendererManager(EventBus* bus, Platform::WindowManager* windowManager);
        ~RendererManager();

        RendererManager(const RendererManager&) = delete;
        RendererManager& operator=(const RendererManager&) = delete;
        RendererManager(RendererManager&&) = delete;
        RendererManager& operator=(RendererManager&&) = delete;

        [[nodiscard]] bool Initialize();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Event Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void OnResize(int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Configuration & Setters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetCurrentRenderer(RenderStyle style);
        void SwitchToNextRenderer();
        void SwitchToPrevRenderer();
        void CycleQuality(int direction);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Public Getters
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] IRenderer* GetCurrentRenderer() const noexcept;
        [[nodiscard]] RenderStyle GetCurrentStyle() const noexcept;
        [[nodiscard]] RenderQuality GetQuality() const noexcept;
        [[nodiscard]] std::string_view GetCurrentRendererName() const noexcept;
        [[nodiscard]] std::string_view GetQualityName() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsRendererActive() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Diagnostics
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] size_t GetRendererCount() const noexcept;
        [[nodiscard]] bool IsRendererAvailable(RenderStyle style) const noexcept;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Internal State Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct RendererState {
            IRenderer* renderer = nullptr;
            RenderStyle style = RenderStyle::Bars;
            bool isActive = false;

            void Clear() noexcept {
                renderer = nullptr;
                style = RenderStyle::Bars;
                isActive = false;
            }
        };

        struct ActivationContext {
            int width = 0;
            int height = 0;
            IRenderer* renderer = nullptr;
            RenderStyle style = RenderStyle::Bars;
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SubscribeToEvents(EventBus* bus);
        bool CreateRenderers();
        bool ActivateInitialRenderer();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Renderer Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        bool SwitchRenderer(RenderStyle newStyle);
        bool ActivateNewRenderer(RenderStyle style);
        bool PrepareActivationContext(RenderStyle style, ActivationContext& outContext) const;
        bool TryActivateRenderer(const ActivationContext& context) const noexcept;
        void CommitRendererState(const ActivationContext& context) noexcept;
        bool AttemptRendererRecovery(RenderStyle fallbackStyle);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Quality Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void SetQuality(RenderQuality quality);
        void ApplyQualityToAllRenderers(RenderQuality quality);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helper Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ValidateDimensions(int width, int height) const noexcept;
        [[nodiscard]] IRenderer* FindRenderer(RenderStyle style) const noexcept;
        [[nodiscard]] bool GetEngineDimensions(int& outWidth, int& outHeight) const noexcept;
        void SafeDeactivateRenderer(IRenderer* renderer) const noexcept;
        void SafeSetQuality(IRenderer* renderer, RenderQuality quality) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Logging Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void LogRendererSwitch(RenderStyle from, RenderStyle to) const;
        void LogRendererCreation() const;
        void LogActivationSuccess(const ActivationContext& context) const;
        void LogActivationFailure(RenderStyle style, const char* reason) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        std::map<RenderStyle, std::unique_ptr<IRenderer>> m_renderers;
        RendererState m_currentState;
        RenderQuality m_currentQuality;
        Platform::WindowManager* m_windowManager;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_RENDERER_MANAGER_H