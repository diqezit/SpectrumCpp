// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the WindowManager for window lifecycle orchestration.
//
// Implementation details:
// - Manages two windows: main (normal) and overlay (fullscreen transparent)
// - Graphics context recreated on every mode switch
// - All subsystems notified of resize events
// - Window positioning calculated based on screen dimensions
// - Message processing delegated to MessageHandler
// - Resize events optimized with debouncing for performance
// - Functions refactored to follow SRP and DRY principles
//
// Refactored to use WindowHelpers.h for cleaner, safer window operations
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "WindowManager.h"
#include "App/ControllerCore.h"
#include "Graphics/Api/Core/RenderEngine.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "Graphics/API/Helpers/Platform/WindowHelpers.h"
#include "MainWindow.h"
#include "MessageHandler.h"
#include "Win32Utils.h"
#include "UI/Core/UIManager.h"
#include <stdexcept>

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {

        constexpr int kMainWindowWidth = 800;
        constexpr int kMainWindowHeight = 600;
        constexpr const wchar_t* kMainWindowTitle = L"Spectrum Visualizer";

        constexpr int kOverlayWindowHeight = 300;
        constexpr const wchar_t* kOverlayWindowTitle = L"Spectrum Overlay";

    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WindowManager::WindowManager(
        HINSTANCE hInstance,
        ControllerCore* controller,
        EventBus* bus
    )
        : m_hInstance(hInstance)
        , m_controller(controller)
        , m_isOverlay(false)
        , m_isResizing(false)
        , m_lastWidth(0)
        , m_lastHeight(0)
    {
        LOG_INFO("WindowManager: Initializing...");

        if (!ValidateDependencies()) {
            throw std::invalid_argument("WindowManager: Required dependencies are null");
        }

        LOG_INFO("WindowManager: Creating UI and message handling components...");

        m_uiManager = std::make_unique<UIManager>(m_controller, this);
        if (!m_uiManager) {
            throw std::runtime_error("WindowManager: Failed to create UIManager");
        }

        m_messageHandler = std::make_unique<MessageHandler>(
            controller,
            this,
            m_uiManager.get(),
            bus
        );
        if (!m_messageHandler) {
            throw std::runtime_error("WindowManager: Failed to create MessageHandler");
        }

        LOG_INFO("WindowManager: Construction completed");
    }

    WindowManager::~WindowManager() noexcept
    {
        LOG_INFO("WindowManager: Shutting down...");

        if (m_isOverlay) {
            HideOverlayWindow();
        }
        else {
            HideMainWindow();
        }

        LOG_INFO("WindowManager: Destroyed");
    }

    [[nodiscard]] bool WindowManager::Initialize()
    {
        LOG_INFO("WindowManager: Starting initialization sequence...");

        if (!InitializeMainWindow()) {
            LOG_ERROR("WindowManager: Failed to initialize main window");
            return false;
        }

        if (!InitializeOverlayWindow()) {
            LOG_ERROR("WindowManager: Failed to initialize overlay window");
            return false;
        }

        if (!InitializeGraphics()) {
            LOG_ERROR("WindowManager: Failed to initialize graphics");
            return false;
        }

        if (!InitializeUIComponents()) {
            LOG_ERROR("WindowManager: Failed to initialize UI components");
            return false;
        }

        CenterMainWindow();
        ShowMainWindow();

        LOG_INFO("WindowManager: Initialization completed successfully");
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ProcessMessages()
    {
        if (m_mainWnd && m_mainWnd->IsRunning()) {
            m_mainWnd->ProcessMessages();
        }
    }

    void WindowManager::PropagateResizeToSubsystems(HWND hwnd)
    {
        auto clientRect = Helpers::Window::GetClientRect(hwnd);
        if (!clientRect) {
            LOG_WARNING("WindowManager: Cannot propagate resize - invalid HWND");
            return;
        }

        const int width = clientRect->Width();
        const int height = clientRect->Height();

        OnResize(hwnd, width, height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resize Management (Optimized)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::OnResizeStart()
    {
        m_isResizing = true;
        LOG_INFO("WindowManager: Resize operation started");
    }

    void WindowManager::OnResizeEnd(HWND hwnd)
    {
        m_isResizing = false;

        if (!ValidateWindow(hwnd)) {
            LOG_WARNING("WindowManager: Invalid HWND in OnResizeEnd");
            return;
        }

        int width = 0;
        int height = 0;
        if (!ExtractResizeDimensions(hwnd, width, height)) {
            LOG_WARNING("WindowManager: Cannot get dimensions in OnResizeEnd");
            return;
        }

        PerformFullResize(hwnd, width, height);

        LogResizeOperation("completed", width, height);
    }

    void WindowManager::OnResize(HWND hwnd, int width, int height)
    {
        if (ShouldSkipResize(width, height)) {
            return;
        }

        m_lastWidth = width;
        m_lastHeight = height;

        if (m_isResizing) {
            PerformLightweightResize(width, height);
        }
        else {
            PerformFullResize(hwnd, width, height);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mode Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ToggleOverlay()
    {
        m_isOverlay = !m_isOverlay;

        LogModeSwitch(m_isOverlay);

        if (m_isOverlay) {
            ActivateOverlayMode();
        }
        else {
            DeactivateOverlayMode();
        }

        NotifyRendererOfModeChange();

        LOG_INFO("WindowManager: Mode switch completed (overlay: "
            << (m_isOverlay ? "ON" : "OFF") << ")");
    }

    [[nodiscard]] bool WindowManager::RecreateGraphicsAndNotify(HWND hwnd)
    {
        if (!ValidateWindow(hwnd)) {
            LOG_ERROR("WindowManager: Cannot recreate graphics - invalid HWND");
            return false;
        }

        LogGraphicsRecreation();

        if (!RecreateGraphicsContext(hwnd)) {
            LOG_ERROR("WindowManager: Graphics context recreation failed");
            return false;
        }

        if (!RecreateUIResources()) {
            LOG_ERROR("WindowManager: UI resources recreation failed");
            return false;
        }

        PropagateResizeToSubsystems(hwnd);

        LOG_INFO("WindowManager: Graphics recreation and notification completed");
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool WindowManager::IsRunning() const
    {
        return m_mainWnd && m_mainWnd->IsRunning();
    }

    [[nodiscard]] bool WindowManager::IsOverlayMode() const noexcept
    {
        return m_isOverlay;
    }

    [[nodiscard]] bool WindowManager::IsActive() const
    {
        if (!IsRunning()) {
            return false;
        }

        const HWND hwnd = GetCurrentHwnd();

        return Helpers::Window::IsActiveAndVisible(hwnd);
    }

    [[nodiscard]] bool WindowManager::IsResizing() const noexcept
    {
        return m_isResizing;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] RenderEngine* WindowManager::GetRenderEngine() const noexcept
    {
        return m_engine.get();
    }

    [[nodiscard]] UIManager* WindowManager::GetUIManager() const noexcept
    {
        return m_uiManager.get();
    }

    [[nodiscard]] MessageHandler* WindowManager::GetMessageHandler() const noexcept
    {
        return m_messageHandler.get();
    }

    [[nodiscard]] MainWindow* WindowManager::GetMainWindow() const noexcept
    {
        return m_mainWnd.get();
    }

    [[nodiscard]] HWND WindowManager::GetCurrentHwnd() const
    {
        if (m_isOverlay) {
            return m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr;
        }

        return m_mainWnd ? m_mainWnd->GetHwnd() : nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Initialization - High Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::InitializeMainWindow()
    {
        LogWindowInitialization(L"Main Window");

        m_mainWnd = CreateMainWindowInstance();
        if (!m_mainWnd) {
            LOG_ERROR("WindowManager: Failed to create MainWindow instance");
            return false;
        }

        if (!ConfigureMainWindow(m_mainWnd.get())) {
            LOG_ERROR("WindowManager: MainWindow configuration failed");
            return false;
        }

        LogWindowCreated(L"Main window", kMainWindowWidth, kMainWindowHeight);
        return true;
    }

    bool WindowManager::InitializeOverlayWindow()
    {
        LogWindowInitialization(L"Overlay Window");

        m_overlayWnd = CreateOverlayWindowInstance();
        if (!m_overlayWnd) {
            LOG_ERROR("WindowManager: Failed to create overlay MainWindow instance");
            return false;
        }

        if (!ConfigureOverlayWindow(m_overlayWnd.get())) {
            LOG_ERROR("WindowManager: Overlay window configuration failed");
            return false;
        }

        const int screenWidth = GetScreenWidth();
        LogWindowCreated(L"Overlay window", screenWidth, kOverlayWindowHeight);
        return true;
    }

    bool WindowManager::InitializeGraphics()
    {
        LOG_INFO("WindowManager: Initializing graphics context...");

        if (!m_mainWnd) {
            LOG_ERROR("WindowManager: Cannot initialize graphics - main window is null");
            return false;
        }

        const HWND hwnd = m_mainWnd->GetHwnd();
        if (!RecreateGraphicsAndNotify(hwnd)) {
            LOG_ERROR("WindowManager: Graphics initialization failed");
            return false;
        }

        LOG_INFO("WindowManager: Graphics context initialized");
        return true;
    }

    bool WindowManager::InitializeUIComponents()
    {
        LOG_INFO("WindowManager: Initializing UI components...");

        if (!ValidateUIManager()) {
            LOG_ERROR("WindowManager: UIManager is null");
            return false;
        }

        if (!m_uiManager->Initialize()) {
            LOG_ERROR("WindowManager: UIManager initialization failed");
            return false;
        }

        LOG_INFO("WindowManager: UI components initialized");
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Initialization - Low Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::unique_ptr<MainWindow> WindowManager::CreateMainWindowInstance() const
    {
        return std::make_unique<MainWindow>(m_hInstance);
    }

    [[nodiscard]] std::unique_ptr<MainWindow> WindowManager::CreateOverlayWindowInstance() const
    {
        return std::make_unique<MainWindow>(m_hInstance);
    }

    bool WindowManager::ConfigureMainWindow(MainWindow* window)
    {
        if (!window) {
            return false;
        }

        return window->Initialize(
            kMainWindowTitle,
            kMainWindowWidth,
            kMainWindowHeight,
            false,
            m_messageHandler.get()
        );
    }

    bool WindowManager::ConfigureOverlayWindow(MainWindow* window)
    {
        if (!window) {
            return false;
        }

        const int screenWidth = GetScreenWidth();

        return window->Initialize(
            kOverlayWindowTitle,
            screenWidth,
            kOverlayWindowHeight,
            true,
            m_messageHandler.get()
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Mode Transitions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ActivateOverlayMode()
    {
        LOG_INFO("WindowManager: Activating overlay mode...");

        HideMainWindow();
        SwitchToOverlayWindow();

        LOG_INFO("WindowManager: Overlay mode activated");
    }

    void WindowManager::DeactivateOverlayMode()
    {
        LOG_INFO("WindowManager: Deactivating overlay mode...");

        HideOverlayWindow();
        SwitchToMainWindow();

        LOG_INFO("WindowManager: Overlay mode deactivated");
    }

    void WindowManager::SwitchToMainWindow()
    {
        if (!m_mainWnd) {
            LOG_ERROR("WindowManager: Cannot switch to main window - window is null");
            return;
        }

        const HWND hwnd = m_mainWnd->GetHwnd();
        if (!RecreateGraphicsAndNotify(hwnd)) {
            LOG_ERROR("WindowManager: Failed to recreate graphics for main window");
            return;
        }

        ShowMainWindow();
    }

    void WindowManager::SwitchToOverlayWindow()
    {
        if (!m_overlayWnd) {
            LOG_ERROR("WindowManager: Cannot switch to overlay - window is null");
            return;
        }

        const HWND hwnd = m_overlayWnd->GetHwnd();
        if (!RecreateGraphicsAndNotify(hwnd)) {
            LOG_ERROR("WindowManager: Failed to recreate graphics for overlay");
            return;
        }

        ShowOverlayWindow();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::HideMainWindow() const
    {
        if (m_mainWnd) {
            Helpers::Window::HideWindow(m_mainWnd->GetHwnd());
            LOG_INFO("WindowManager: Main window hidden");
        }
    }

    void WindowManager::ShowMainWindow() const
    {
        if (!m_mainWnd) {
            return;
        }

        const HWND hwnd = m_mainWnd->GetHwnd();

        Helpers::Window::ShowWindowState(hwnd);
        Helpers::Window::BringToFront(hwnd);

        LOG_INFO("WindowManager: Main window shown");
    }

    void WindowManager::HideOverlayWindow() const
    {
        if (m_overlayWnd) {
            Helpers::Window::HideWindow(m_overlayWnd->GetHwnd());
            LOG_INFO("WindowManager: Overlay window hidden");
        }
    }

    void WindowManager::ShowOverlayWindow() const
    {
        if (!m_overlayWnd) {
            return;
        }

        PositionOverlayWindow();

        LOG_INFO("WindowManager: Overlay window shown");
    }

    void WindowManager::CenterMainWindow()
    {
        if (!m_mainWnd) {
            return;
        }

        const HWND hwnd = m_mainWnd->GetHwnd();

        if (!Helpers::Window::CenterWindow(hwnd)) {
            LOG_WARNING("WindowManager: Failed to center main window");
            return;
        }

        LOG_INFO("WindowManager: Main window centered");
    }

    void WindowManager::PositionOverlayWindow() const
    {
        if (!m_overlayWnd) {
            return;
        }

        const HWND hwnd = m_overlayWnd->GetHwnd();
        const int overlayHeight = CalculateOverlayHeight();

        if (!Helpers::Window::PositionAtBottom(hwnd, overlayHeight)) {
            LOG_WARNING("WindowManager: Failed to position overlay");
            return;
        }

        LOG_INFO("WindowManager: Overlay positioned at bottom");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Graphics Management - High Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::RecreateGraphicsContext(HWND hwnd)
    {
        if (!ValidateWindow(hwnd)) {
            LOG_ERROR("WindowManager: Cannot recreate graphics context - invalid HWND");
            return false;
        }

        LOG_INFO("WindowManager: Creating RenderEngine (overlay: "
            << (m_isOverlay ? "ON" : "OFF") << ")...");

        m_engine = CreateRenderEngine(hwnd);
        if (!m_engine) {
            LOG_ERROR("WindowManager: Failed to create RenderEngine instance");
            return false;
        }

        if (!InitializeRenderEngine(m_engine.get())) {
            LOG_ERROR("WindowManager: RenderEngine initialization failed");
            return false;
        }

        LOG_INFO("WindowManager: RenderEngine created successfully");
        return true;
    }

    bool WindowManager::RecreateUIResources()
    {
        if (!ValidateUIManager() || !ValidateRenderEngine()) {
            LOG_ERROR("WindowManager: Cannot recreate UI resources - missing dependencies");
            return false;
        }

        LOG_INFO("WindowManager: Recreating UI resources...");

        return UpdateUIResourcesFromEngine();
    }

    void WindowManager::NotifyRendererOfModeChange() const
    {
        if (!ValidateController()) {
            return;
        }

        auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager) {
            LOG_WARNING("WindowManager: RendererManager is null");
            return;
        }

        auto* renderer = rendererManager->GetCurrentRenderer();
        if (!renderer) {
            LOG_WARNING("WindowManager: Current renderer is null");
            return;
        }

        renderer->SetOverlayMode(m_isOverlay);

        LOG_INFO("WindowManager: Renderer notified of mode change");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Graphics Management - Low Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::unique_ptr<RenderEngine> WindowManager::CreateRenderEngine(HWND hwnd) const
    {
        return std::make_unique<RenderEngine>(hwnd, m_isOverlay);
    }

    bool WindowManager::InitializeRenderEngine(RenderEngine* engine) const
    {
        if (!engine) {
            return false;
        }

        return engine->Initialize();
    }

    bool WindowManager::UpdateUIResourcesFromEngine()
    {
        m_uiManager->RecreateResources(
            m_engine->GetCanvas(),
            m_engine->GetWidth(),
            m_engine->GetHeight()
        );

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resize Handling (Internal)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::PerformLightweightResize(int width, int height)
    {
        ResizeRenderEngine(width, height);
    }

    void WindowManager::PerformFullResize(HWND hwnd, int width, int height)
    {
        (void)hwnd; // unused parameter

        LOG_INFO("WindowManager: Propagating resize to subsystems: "
            << width << "x" << height);

        ResizeRenderEngine(width, height);
        ResizeUIManager(width, height);
        NotifyControllerResize(width, height);

        LOG_INFO("WindowManager: Resize propagation completed");
    }

    void WindowManager::ResizeRenderEngine(int width, int height)
    {
        if (m_engine) {
            m_engine->Resize(width, height);
            LOG_INFO("WindowManager: RenderEngine resized to " << width << "x" << height);
        }
    }

    void WindowManager::ResizeUIManager(int width, int height)
    {
        if (m_uiManager && m_engine) {
            m_uiManager->RecreateResources(
                m_engine->GetCanvas(),
                width,
                height
            );
            LOG_INFO("WindowManager: UIManager resources recreated for "
                << width << "x" << height);
        }
    }

    void WindowManager::NotifyControllerResize(int width, int height)
    {
        if (m_controller) {
            m_controller->OnResize(width, height);
            LOG_INFO("WindowManager: Controller notified of resize");
        }
    }

    bool WindowManager::ShouldSkipResize(int width, int height) const noexcept
    {
        if (!Helpers::Window::IsValidSize(width, height)) {
            return true;
        }

        if (width == m_lastWidth && height == m_lastHeight) {
            return true;
        }

        return false;
    }

    bool WindowManager::ExtractResizeDimensions(HWND hwnd, int& outWidth, int& outHeight) const
    {
        auto clientRect = Helpers::Window::GetClientRect(hwnd);
        if (!clientRect) {
            return false;
        }

        outWidth = clientRect->Width();
        outHeight = clientRect->Height();

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Calculation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] int WindowManager::CalculateOverlayHeight() const
    {
        return m_overlayWnd ? m_overlayWnd->GetHeight() : kOverlayWindowHeight;
    }

    [[nodiscard]] Point WindowManager::CalculateMainWindowCenter() const
    {
        const auto screenSize = Win32Utils::GetScreenSize();
        return {
            static_cast<float>(screenSize.w) * 0.5f,
            static_cast<float>(screenSize.h) * 0.5f
        };
    }

    [[nodiscard]] int WindowManager::GetScreenWidth() const
    {
        const auto screenSize = Win32Utils::GetScreenSize();
        return screenSize.w;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool WindowManager::ValidateDependencies() const noexcept
    {
        if (!ValidateController()) {
            LOG_ERROR("WindowManager: Controller dependency is null");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool WindowManager::ValidateController() const noexcept
    {
        return m_controller != nullptr;
    }

    [[nodiscard]] bool WindowManager::ValidateUIManager() const noexcept
    {
        return m_uiManager != nullptr;
    }

    [[nodiscard]] bool WindowManager::ValidateRenderEngine() const noexcept
    {
        return m_engine != nullptr;
    }

    [[nodiscard]] bool WindowManager::ValidateWindow(HWND hwnd) const noexcept
    {
        return Helpers::Window::IsWindowValid(hwnd);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Logging Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::LogModeSwitch(bool toOverlay) const
    {
        LOG_INFO("WindowManager: Switching mode to "
            << (toOverlay ? "OVERLAY" : "NORMAL"));
    }

    void WindowManager::LogWindowInitialization(const wchar_t* windowType) const
    {
        char buffer[64] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, windowType, -1, buffer, sizeof(buffer) - 1, nullptr, nullptr);
        LOG_INFO("WindowManager: Initializing " << buffer);
    }

    void WindowManager::LogGraphicsRecreation() const
    {
        LOG_INFO("WindowManager: Recreating graphics context");
    }

    void WindowManager::LogWindowCreated(const wchar_t* windowType, int width, int height) const
    {
        char buffer[64] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, windowType, -1, buffer, sizeof(buffer) - 1, nullptr, nullptr);
        LOG_INFO("WindowManager: " << buffer << " created (" << width << "x" << height << ")");
    }

    void WindowManager::LogResizeOperation(const char* operationType, int width, int height) const
    {
        LOG_INFO("WindowManager: Resize operation " << operationType
            << " at " << width << "x" << height);
    }

} // namespace Spectrum::Platform