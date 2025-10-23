// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the UIManager for centralized UI orchestration.
//
// Implementation details:
// - Three-tier UI hierarchy: modals → panels → widgets
// - Modal inputs have absolute priority over normal UI
// - Smooth animations using exponential decay
// - ColorPicker automatically shown/hidden based on renderer support
// - Mouse capture ensures smooth drag operations
//
// Update pipeline:
// 1. Update animations (overlay fade, picker visibility)
// 2. Process active slider drag if present
// 3. Route input based on modal state (modal-first priority)
// 4. Update individual components with delta time
//
// Rendering pipeline:
// 1. Control panel (always visible in normal mode)
// 2. Color picker (with fade animation)
// 3. Modal overlay (dark background with fade)
// 4. Audio settings panel (if modal active)
//
// Refactored to follow SRP and DRY principles with small, focused functions.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/Core/UIManager.h"
#include "UI/Panels/AudioSettingsPanel/AudioSettingsPanel.h"
#include "UI/Panels/ColorPicker/ColorPicker.h"
#include "UI/Panels/ControlPanel/ControlPanel.h"
#include "UI/Common/UILayout.h"
#include "UI/Components/UISlider.h"
#include "App/ControllerCore.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/RenderEngine.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "Platform/WindowManager.h"
#include "Graphics/API/Helpers/Math/MathHelpers.h"
#include <stdexcept>

namespace Spectrum
{
    using namespace Helpers::Math;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UIManager::UIManager(
        ControllerCore* controller,
        Platform::WindowManager* windowManager
    )
        : m_controller(controller)
        , m_windowManager(windowManager)
        , m_activeSlider(nullptr)
        , m_modalOverlayAlpha(0.0f)
        , m_colorPickerAlpha(0.0f)
        , m_shouldShowColorPicker(false)
    {
        LOG_INFO("UIManager: Initializing...");

        if (!ValidateDependencies()) {
            throw std::invalid_argument("UIManager: Required dependencies are null");
        }
    }

    UIManager::~UIManager() noexcept
    {
        LOG_INFO("UIManager: Shutting down...");

        CleanupActiveSlider();
    }

    [[nodiscard]] bool UIManager::Initialize()
    {
        LOG_INFO("UIManager: Starting component initialization...");

        if (!InitializeControlPanel()) {
            LogComponentFailed("ControlPanel");
            return false;
        }

        if (!InitializeAudioSettingsPanel()) {
            LogComponentFailed("AudioSettingsPanel");
            return false;
        }

        if (!InitializeColorPicker()) {
            LogComponentFailed("ColorPicker");
            return false;
        }

        m_shouldShowColorPicker = ShouldDrawColorPicker();

        LOG_INFO("UIManager: Initialization completed successfully");
        return true;
    }

    void UIManager::RecreateResources(
        Canvas& canvas,
        int width,
        int height
    )
    {
        LOG_INFO("UIManager: Recreating resources for " << width << "x" << height);

        RecreateColorPickerResources(canvas, width, height);
        RecreateControlPanelResources();

        LOG_INFO("UIManager: Resource recreation completed");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Update & Rendering
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::Update(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        UpdateAnimations(deltaTime);
        ProcessInput(mousePos, isMouseDown, deltaTime);
    }

    void UIManager::Draw(Canvas& canvas) const
    {
        DrawControlPanel(canvas);
        DrawColorPicker(canvas);
        DrawModalOverlay(canvas);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIManager::HandleMessage(
        HWND /*hwnd*/,
        UINT msg,
        WPARAM /*wParam*/,
        LPARAM /*lParam*/
    )
    {
        // Reserved for future keyboard shortcuts or special handling
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEMOVE) {
            return false; // Handled in Update()
        }

        return false;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Component Initialization - High Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool UIManager::InitializeControlPanel()
    {
        LogComponentInitialization("ControlPanel");

        m_controlPanel = CreateControlPanel();
        if (!ValidateComponent(m_controlPanel.get(), "ControlPanel")) {
            return false;
        }

        if (!ConfigureControlPanel(m_controlPanel.get())) {
            return false;
        }

        SetupControlPanelCallbacks();

        LogComponentInitialized("ControlPanel");
        return true;
    }

    bool UIManager::InitializeAudioSettingsPanel()
    {
        LogComponentInitialization("AudioSettingsPanel");

        m_audioSettingsPanel = CreateAudioSettingsPanel();
        if (!ValidateComponent(m_audioSettingsPanel.get(), "AudioSettingsPanel")) {
            return false;
        }

        if (!ConfigureAudioSettingsPanel(m_audioSettingsPanel.get())) {
            return false;
        }

        SetupAudioSettingsPanelCallbacks();

        LogComponentInitialized("AudioSettingsPanel");
        return true;
    }

    bool UIManager::InitializeColorPicker()
    {
        LogComponentInitialization("ColorPicker");

        if (!ValidateRenderEngine()) {
            LOG_ERROR("UIManager: RenderEngine is null");
            return false;
        }

        m_colorPicker = CreateColorPicker();
        if (!ValidateComponent(m_colorPicker.get(), "ColorPicker")) {
            return false;
        }

        auto* engine = m_windowManager->GetRenderEngine();
        if (!ConfigureColorPicker(m_colorPicker.get(), engine->GetCanvas())) {
            return false;
        }

        SetupColorPickerCallbacks();

        LogComponentInitialized("ColorPicker");
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Component Initialization - Low Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::unique_ptr<ControlPanel> UIManager::CreateControlPanel()
    {
        return std::make_unique<ControlPanel>(m_controller);
    }

    [[nodiscard]] std::unique_ptr<AudioSettingsPanel> UIManager::CreateAudioSettingsPanel()
    {
        return std::make_unique<AudioSettingsPanel>(m_controller);
    }

    [[nodiscard]] std::unique_ptr<ColorPicker> UIManager::CreateColorPicker()
    {
        const int width = GetRenderEngineWidth();
        const Point pickerPos = CalculateColorPickerPosition(width);

        return std::make_unique<ColorPicker>(pickerPos, 40.0f);
    }

    bool UIManager::ConfigureControlPanel(ControlPanel* panel)
    {
        if (!panel) {
            return false;
        }

        panel->Initialize();
        return true;
    }

    bool UIManager::ConfigureAudioSettingsPanel(AudioSettingsPanel* panel)
    {
        if (!panel) {
            return false;
        }

        panel->Initialize();
        return true;
    }

    bool UIManager::ConfigureColorPicker(ColorPicker* picker, Canvas& canvas)
    {
        if (!picker) {
            return false;
        }

        if (!picker->Initialize(canvas)) {
            LOG_ERROR("UIManager: Failed to initialize ColorPicker resources");
            return false;
        }

        return true;
    }

    void UIManager::SetupControlPanelCallbacks()
    {
        if (!m_controlPanel) {
            return;
        }

        m_controlPanel->SetOnShowAudioSettings([this]() {
            ShowAudioSettings();
            });
    }

    void UIManager::SetupAudioSettingsPanelCallbacks()
    {
        if (!m_audioSettingsPanel) {
            return;
        }

        m_audioSettingsPanel->SetOnCloseCallback([this]() {
            HideAudioSettings();
            });
    }

    void UIManager::SetupColorPickerCallbacks()
    {
        if (!m_colorPicker) {
            return;
        }

        m_colorPicker->SetOnColorSelectedCallback([this](const Color& color) {
            if (m_controller) {
                m_controller->SetPrimaryColor(color);
                LOG_INFO("UIManager: Color changed via picker");
            }
            });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Recreation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::RecreateColorPickerResources(Canvas& canvas, int width, int height)
    {
        if (m_colorPicker) {
            m_colorPicker->RecreateResources(canvas);
            RepositionColorPicker(width, height);
        }
    }

    void UIManager::RecreateControlPanelResources()
    {
        if (m_controlPanel) {
            m_controlPanel->Initialize();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Modal Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::ShowAudioSettings()
    {
        LOG_INFO("UIManager: Showing AudioSettings modal");

        if (m_audioSettingsPanel) {
            m_audioSettingsPanel->Show();
        }
    }

    void UIManager::HideAudioSettings()
    {
        LOG_INFO("UIManager: Hiding AudioSettings modal");

        CleanupActiveSlider();

        if (m_audioSettingsPanel) {
            m_audioSettingsPanel->Hide();
        }
    }

    void UIManager::CleanupActiveSlider()
    {
        if (m_activeSlider) {
            m_activeSlider->EndDrag();
            m_activeSlider = nullptr;
            ReleaseMouseCapture();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Update Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::UpdateAnimations(float deltaTime)
    {
        UpdateModalOverlayAnimation(deltaTime);
        UpdateColorPickerVisibility(deltaTime);
    }

    void UIManager::ProcessInput(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        if (HasActiveSlider()) {
            UpdateSliderDrag(mousePos, isMouseDown);
            return; // Slider captures all input
        }

        if (IsModalActive()) {
            UpdateModalInput(mousePos, isMouseDown, deltaTime);
        }
        else {
            UpdateNormalInput(mousePos, isMouseDown, deltaTime);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Input Handling - Modal
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::UpdateModalInput(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        UpdateAudioSettingsPanel(mousePos, isMouseDown, deltaTime);

        if (isMouseDown) {
            HandleModalMouseDown(mousePos);
        }
    }

    void UIManager::UpdateAudioSettingsPanel(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        if (m_audioSettingsPanel) {
            m_audioSettingsPanel->Update(mousePos, isMouseDown, deltaTime);
        }
    }

    void UIManager::HandleModalMouseDown(const Point& mousePos)
    {
        if (m_audioSettingsPanel) {
            BeginSliderDrag(mousePos);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Input Handling - Normal
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::UpdateNormalInput(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        UpdateControlPanelInput(mousePos, isMouseDown, deltaTime);
        UpdateColorPickerInput(mousePos, isMouseDown, deltaTime);
    }

    void UIManager::UpdateControlPanelInput(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        if (m_controlPanel) {
            m_controlPanel->Update(mousePos, isMouseDown, deltaTime);
        }
    }

    void UIManager::UpdateColorPickerInput(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        if (ShouldDrawColorPickerNow() && m_colorPicker) {
            m_colorPicker->Update(mousePos, isMouseDown, deltaTime);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Slider Drag Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::UpdateSliderDrag(
        const Point& mousePos,
        bool isMouseDown
    )
    {
        if (!m_activeSlider) {
            return;
        }

        if (isMouseDown) {
            m_activeSlider->Drag(mousePos);
        }
        else {
            EndSliderDrag();
        }
    }

    void UIManager::BeginSliderDrag(const Point& mousePos)
    {
        UISlider* slider = FindSliderAtPosition(mousePos);

        if (slider) {
            StartSliderDrag(slider, mousePos);
        }
    }

    void UIManager::EndSliderDrag()
    {
        if (m_activeSlider) {
            m_activeSlider->EndDrag();
            m_activeSlider = nullptr;
            ReleaseMouseCapture();
            LogSliderDragEnd();
        }
    }

    [[nodiscard]] UISlider* UIManager::FindSliderAtPosition(const Point& mousePos)
    {
        if (!m_audioSettingsPanel) {
            return nullptr;
        }

        return m_audioSettingsPanel->GetSliderAt(mousePos);
    }

    void UIManager::StartSliderDrag(UISlider* slider, const Point& mousePos)
    {
        m_activeSlider = slider;
        CaptureMouseForSlider();
        m_activeSlider->BeginDrag(mousePos);
        LogSliderDragStart();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mouse Capture
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::CaptureMouseForSlider()
    {
        const HWND hwnd = GetCurrentWindowHandle();
        if (hwnd) {
            SetCapture(hwnd);
        }
    }

    void UIManager::ReleaseMouseCapture()
    {
        ::ReleaseCapture();
    }

    [[nodiscard]] HWND UIManager::GetCurrentWindowHandle() const
    {
        return m_windowManager->GetCurrentHwnd();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Animation Updates
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::UpdateModalOverlayAnimation(float deltaTime)
    {
        const float targetAlpha = IsModalActive() ? 1.0f : 0.0f;

        m_modalOverlayAlpha = ExponentialDecay(
            m_modalOverlayAlpha,
            targetAlpha,
            kModalFadeSpeed,
            deltaTime
        );
    }

    void UIManager::UpdateColorPickerVisibility(float deltaTime)
    {
        UpdateColorPickerVisibilityState();

        const float targetAlpha = m_shouldShowColorPicker ? 1.0f : 0.0f;
        AnimateColorPickerAlpha(targetAlpha, deltaTime);
    }

    void UIManager::UpdateColorPickerVisibilityState()
    {
        const bool shouldShow = ShouldDrawColorPicker();

        if (shouldShow != m_shouldShowColorPicker) {
            m_shouldShowColorPicker = shouldShow;
            LogColorPickerVisibilityChange(shouldShow);
        }
    }

    void UIManager::AnimateColorPickerAlpha(float targetAlpha, float deltaTime)
    {
        m_colorPickerAlpha = ExponentialDecay(
            m_colorPickerAlpha,
            targetAlpha,
            kColorPickerFadeSpeed,
            deltaTime
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Rendering Pipeline
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::DrawControlPanel(Canvas& canvas) const
    {
        if (m_controlPanel) {
            m_controlPanel->Draw(canvas);
        }
    }

    void UIManager::DrawColorPicker(Canvas& canvas) const
    {
        if (!ShouldDrawColorPickerNow() || !m_colorPicker) {
            return;
        }

        DrawColorPickerWithTransform(canvas);
    }

    void UIManager::DrawModalOverlay(Canvas& canvas) const
    {
        if (!IsModalActive() || m_modalOverlayAlpha <= kMinVisibleAlpha) {
            return;
        }

        DrawModalBackground(canvas);
        DrawAudioSettings(canvas);
    }

    void UIManager::DrawAudioSettings(Canvas& canvas) const
    {
        if (m_audioSettingsPanel) {
            m_audioSettingsPanel->Draw(canvas);
        }
    }

    void UIManager::DrawModalBackground(Canvas& canvas) const
    {
        const float alpha = GetModalOverlayAlpha();
        const Rect screenRect = GetScreenRect();
        const Paint paint = Paint::Fill(Color(0.0f, 0.0f, 0.0f, alpha));

        canvas.DrawRectangle(screenRect, paint);
    }

    void UIManager::DrawColorPickerWithTransform(Canvas& canvas) const
    {
        const float alpha = CalculateColorPickerAlpha();
        const Point pickerCenter = m_colorPicker->GetCenter();

        canvas.PushTransform();
        ApplyColorPickerTransform(canvas);
        m_colorPicker->DrawWithAlpha(canvas, alpha);
        canvas.PopTransform();
    }

    void UIManager::ApplyColorPickerTransform(Canvas& canvas) const
    {
        const float scale = CalculateColorPickerTransformScale();
        const Point pickerCenter = m_colorPicker->GetCenter();

        canvas.ScaleAt(pickerCenter, scale, scale);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Layout & Positioning
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::RepositionColorPicker(int width, int height)
    {
        (void)height; // Suppress unused parameter warning

        if (!m_colorPicker) {
            return;
        }

        const Point newPos = CalculateColorPickerPosition(width);
        UpdateColorPickerPosition(newPos);
    }

    void UIManager::UpdateColorPickerPosition(const Point& newPos)
    {
        m_colorPicker->SetPosition(newPos);

        LOG_INFO("UIManager: ColorPicker repositioned to ("
            << newPos.x << ", " << newPos.y << ")");
    }

    [[nodiscard]] Point UIManager::CalculateColorPickerPosition(int width) const
    {
        return {
            static_cast<float>(width) - UILayout::kPadding - 80.0f,
            UILayout::kPadding
        };
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIManager::IsModalActive() const noexcept
    {
        return m_audioSettingsPanel && m_audioSettingsPanel->IsVisible();
    }

    [[nodiscard]] bool UIManager::ShouldDrawColorPicker() const noexcept
    {
        return CurrentRendererSupportsPrimaryColor();
    }

    [[nodiscard]] bool UIManager::ShouldDrawColorPickerNow() const noexcept
    {
        return m_shouldShowColorPicker &&
            m_colorPicker &&
            IsColorPickerVisible();
    }

    [[nodiscard]] bool UIManager::HasActiveSlider() const noexcept
    {
        return m_activeSlider != nullptr;
    }

    [[nodiscard]] bool UIManager::IsColorPickerVisible() const noexcept
    {
        return m_colorPickerAlpha > kMinVisibleAlpha;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Renderer Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIManager::CurrentRendererSupportsPrimaryColor() const noexcept
    {
        if (!ValidateController()) {
            return false;
        }

        const auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager) {
            return false;
        }

        const auto* renderer = rendererManager->GetCurrentRenderer();
        if (!renderer) {
            return false;
        }

        return renderer->SupportsPrimaryColor();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float UIManager::GetModalOverlayAlpha() const noexcept
    {
        const float easedAlpha = EaseModalOverlayAlpha(m_modalOverlayAlpha);
        return easedAlpha * kMaxModalOverlayAlpha;
    }

    [[nodiscard]] float UIManager::GetColorPickerScale() const noexcept
    {
        return CalculateColorPickerTransformScale();
    }

    [[nodiscard]] float UIManager::CalculateColorPickerAlpha() const noexcept
    {
        return EaseColorPickerAlpha(m_colorPickerAlpha);
    }

    [[nodiscard]] float UIManager::CalculateColorPickerTransformScale() const noexcept
    {
        const float t = EaseColorPickerScale(m_colorPickerAlpha);
        return Lerp(kColorPickerMinScale, kColorPickerMaxScale, t);
    }

    [[nodiscard]] Rect UIManager::GetScreenRect() const
    {
        const int width = GetRenderEngineWidth();
        const int height = GetRenderEngineHeight();

        return {
            0.0f,
            0.0f,
            static_cast<float>(width),
            static_cast<float>(height)
        };
    }

    [[nodiscard]] int UIManager::GetRenderEngineWidth() const
    {
        auto* engine = m_windowManager->GetRenderEngine();
        return engine ? engine->GetWidth() : 0;
    }

    [[nodiscard]] int UIManager::GetRenderEngineHeight() const
    {
        auto* engine = m_windowManager->GetRenderEngine();
        return engine ? engine->GetHeight() : 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Easing Functions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] float UIManager::EaseModalOverlayAlpha(float t) noexcept
    {
        return EaseInOutCubic(t);
    }

    [[nodiscard]] float UIManager::EaseColorPickerAlpha(float t) noexcept
    {
        return EaseOutCubic(t);
    }

    [[nodiscard]] float UIManager::EaseColorPickerScale(float t) noexcept
    {
        return EaseOutBack(t);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIManager::ValidateDependencies() const noexcept
    {
        if (!ValidateController()) {
            LOG_ERROR("UIManager: Controller dependency is null");
            return false;
        }

        if (!ValidateWindowManager()) {
            LOG_ERROR("UIManager: WindowManager dependency is null");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool UIManager::ValidateController() const noexcept
    {
        return m_controller != nullptr;
    }

    [[nodiscard]] bool UIManager::ValidateWindowManager() const noexcept
    {
        return m_windowManager != nullptr;
    }

    [[nodiscard]] bool UIManager::ValidateRenderEngine() const noexcept
    {
        if (!m_windowManager) {
            return false;
        }

        return m_windowManager->GetRenderEngine() != nullptr;
    }

    [[nodiscard]] bool UIManager::ValidateComponent(
        void* component,
        const char* name
    ) const noexcept
    {
        (void)name;

        if (!component) {
            LOG_ERROR("UIManager: Failed to create " << name);
            return false;
        }

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Logging
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::LogComponentInitialization(const char* componentName) const
    {
        LOG_INFO("UIManager: Initializing " << componentName << "...");
    }

    void UIManager::LogComponentInitialized(const char* componentName) const
    {
        LOG_INFO("UIManager: " << componentName << " initialized");
    }

    void UIManager::LogComponentFailed(const char* componentName) const
    {
        (void)componentName; // Suppress unused
        LOG_ERROR("UIManager: Failed to initialize " << componentName);
    }

    void UIManager::LogColorPickerVisibilityChange(bool visible) const
    {
        LOG_INFO("UIManager: ColorPicker visibility changed to "
            << (visible ? "visible" : "hidden"));
    }

    void UIManager::LogSliderDragStart() const
    {
        LOG_INFO("UIManager: Slider drag started");
    }

    void UIManager::LogSliderDragEnd() const
    {
        LOG_INFO("UIManager: Slider drag ended");
    }

} // namespace Spectrum