#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the UIManager for centralized UI orchestration and management.
//
// This class serves as the central coordinator for all UI components,
// managing their lifecycle, input distribution, and rendering order.
// Implements a modal-priority input system with smooth animations.
//
// Key features:
// - Modal-first input priority system
// - Smooth delta-time based animations for overlays
// - Mouse capture coordination for drag operations
// - Automatic ColorPicker visibility based on renderer support
// - Resource recreation on graphics device loss
//
// Design notes:
// - All rendering methods are const (state in animation variables)
// - Input flows: modal → control panel → color picker
// - Animations use exponential decay for smooth transitions
// - Dependencies validated at construction time
// - Refactored to follow SRP and DRY principles
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include <memory>
#include <functional>

namespace Spectrum {
    class AudioSettingsPanel;
    class ColorPicker;
    class ControlPanel;
    class ControllerCore;
    class Canvas;
    class UISlider;

    namespace Platform {
        class WindowManager;
    }

    class UIManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        explicit UIManager(
            ControllerCore* controller,
            Platform::WindowManager* windowManager
        );
        ~UIManager() noexcept;

        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;
        UIManager(UIManager&&) = delete;
        UIManager& operator=(UIManager&&) = delete;

        [[nodiscard]] bool Initialize();

        void RecreateResources(
            Canvas& canvas,
            int width,
            int height
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Main Update & Rendering
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void Update(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void Draw(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Window Message Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool HandleMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Component Initialization - High Level
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        bool InitializeControlPanel();
        bool InitializeAudioSettingsPanel();
        bool InitializeColorPicker();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Component Initialization - Low Level
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] std::unique_ptr<ControlPanel> CreateControlPanel();
        [[nodiscard]] std::unique_ptr<AudioSettingsPanel> CreateAudioSettingsPanel();
        [[nodiscard]] std::unique_ptr<ColorPicker> CreateColorPicker();

        bool ConfigureControlPanel(ControlPanel* panel);
        bool ConfigureAudioSettingsPanel(AudioSettingsPanel* panel);
        bool ConfigureColorPicker(ColorPicker* picker, Canvas& canvas);

        void SetupControlPanelCallbacks();
        void SetupAudioSettingsPanelCallbacks();
        void SetupColorPickerCallbacks();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Recreation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RecreateColorPickerResources(Canvas& canvas, int width, int height);
        void RecreateControlPanelResources();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Modal Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void ShowAudioSettings();
        void HideAudioSettings();
        void CleanupActiveSlider();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Update Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateAnimations(float deltaTime);
        void ProcessInput(const Point& mousePos, bool isMouseDown, float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Input Handling - Modal
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateModalInput(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void UpdateAudioSettingsPanel(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void HandleModalMouseDown(const Point& mousePos);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Input Handling - Normal
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateNormalInput(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void UpdateControlPanelInput(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        void UpdateColorPickerInput(
            const Point& mousePos,
            bool isMouseDown,
            float deltaTime
        );

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Slider Drag Handling
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateSliderDrag(const Point& mousePos, bool isMouseDown);
        void BeginSliderDrag(const Point& mousePos);
        void EndSliderDrag();

        [[nodiscard]] UISlider* FindSliderAtPosition(const Point& mousePos);
        void StartSliderDrag(UISlider* slider, const Point& mousePos);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Mouse Capture
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void CaptureMouseForSlider();
        void ReleaseMouseCapture();
        [[nodiscard]] HWND GetCurrentWindowHandle() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Animation Updates
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void UpdateModalOverlayAnimation(float deltaTime);
        void UpdateColorPickerVisibility(float deltaTime);
        void UpdateColorPickerVisibilityState();
        void AnimateColorPickerAlpha(float targetAlpha, float deltaTime);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Rendering Pipeline
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void DrawControlPanel(Canvas& canvas) const;
        void DrawColorPicker(Canvas& canvas) const;
        void DrawModalOverlay(Canvas& canvas) const;
        void DrawAudioSettings(Canvas& canvas) const;
        void DrawModalBackground(Canvas& canvas) const;

        void DrawColorPickerWithTransform(Canvas& canvas) const;
        void ApplyColorPickerTransform(Canvas& canvas) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Layout & Positioning
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RepositionColorPicker(int width, int height);
        void UpdateColorPickerPosition(const Point& newPos);
        [[nodiscard]] Point CalculateColorPickerPosition(int width) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool IsModalActive() const noexcept;
        [[nodiscard]] bool ShouldDrawColorPicker() const noexcept;
        [[nodiscard]] bool ShouldDrawColorPickerNow() const noexcept;
        [[nodiscard]] bool HasActiveSlider() const noexcept;
        [[nodiscard]] bool IsColorPickerVisible() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Renderer Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CurrentRendererSupportsPrimaryColor() const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Calculation Helpers
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] float GetModalOverlayAlpha() const noexcept;
        [[nodiscard]] float GetColorPickerScale() const noexcept;
        [[nodiscard]] float CalculateColorPickerAlpha() const noexcept;
        [[nodiscard]] float CalculateColorPickerTransformScale() const noexcept;

        [[nodiscard]] Rect GetScreenRect() const;
        [[nodiscard]] int GetRenderEngineWidth() const;
        [[nodiscard]] int GetRenderEngineHeight() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Easing Functions
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] static float EaseModalOverlayAlpha(float t) noexcept;
        [[nodiscard]] static float EaseColorPickerAlpha(float t) noexcept;
        [[nodiscard]] static float EaseColorPickerScale(float t) noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Validation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool ValidateDependencies() const noexcept;
        [[nodiscard]] bool ValidateController() const noexcept;
        [[nodiscard]] bool ValidateWindowManager() const noexcept;
        [[nodiscard]] bool ValidateRenderEngine() const noexcept;
        [[nodiscard]] bool ValidateComponent(void* component, const char* name) const noexcept;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Logging
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void LogComponentInitialization(const char* componentName) const;
        void LogComponentInitialized(const char* componentName) const;
        void LogComponentFailed(const char* componentName) const;
        void LogColorPickerVisibilityChange(bool visible) const;
        void LogSliderDragStart() const;
        void LogSliderDragEnd() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        // Animation parameters
        static constexpr float kModalFadeSpeed = 12.0f;
        static constexpr float kColorPickerFadeSpeed = 8.0f;
        static constexpr float kMaxModalOverlayAlpha = 0.6f;
        static constexpr float kColorPickerMinScale = 0.8f;
        static constexpr float kColorPickerMaxScale = 1.0f;
        static constexpr float kMinVisibleAlpha = 0.01f;

        // Dependencies (non-owning)
        ControllerCore* m_controller;
        Platform::WindowManager* m_windowManager;

        // UI Components (owning)
        std::unique_ptr<ControlPanel> m_controlPanel;
        std::unique_ptr<AudioSettingsPanel> m_audioSettingsPanel;
        std::unique_ptr<ColorPicker> m_colorPicker;

        // Active slider (non-owning)
        UISlider* m_activeSlider;

        // Animation state
        float m_modalOverlayAlpha;
        float m_colorPickerAlpha;
        bool m_shouldShowColorPicker;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_UI_MANAGER_H