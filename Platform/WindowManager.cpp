// WindowManager.cpp
#include "WindowManager.h"

#include "App/ControllerCore.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "MainWindow.h"
#include "UIWindow.h"
#include "MessageHandler.h"
#include "UIMessageHandler.h"
#include "Win32Utils.h"
#include "UI/Core/UIManager.h"

#include <stdexcept>

namespace Spectrum::Platform {

    using namespace Helpers::Validate;

    namespace {
        constexpr int kMainWindowWidth = 800;
        constexpr int kMainWindowHeight = 600;
        constexpr const wchar_t* kMainWindowTitle = L"Spectrum Visualizer";

        constexpr int kOverlayWindowHeight = 300;
        constexpr const wchar_t* kOverlayWindowTitle = L"Spectrum Overlay";

        constexpr int kUIWindowWidth = 340;
        constexpr int kUIWindowHeight = 480;
        constexpr const wchar_t* kUIWindowTitle = L"Spectrum Control Panel";
    }

    WindowManager::WindowManager(
        HINSTANCE hInstance,
        ControllerCore* controller,
        EventBus* bus)
        : m_hInstance(hInstance)
        , m_controller(controller)
        , m_isOverlay(false)
        , m_isResizing(false)
        , m_isUIResizing(false)
        , m_lastWidth(0)
        , m_lastHeight(0)
        , m_lastUIWidth(0)
        , m_lastUIHeight(0)
    {
        LOG_INFO("WindowManager: Initializing dual-window architecture...");

        if (!ValidateDependencies())
        {
            throw std::invalid_argument("WindowManager: Controller dependency cannot be null.");
        }

        m_uiManager = std::make_unique<UIManager>(m_controller, this);
        if (!Pointer(m_uiManager.get(), "UIManager", "WindowManager"))
        {
            throw std::runtime_error("WindowManager: Failed to create UIManager.");
        }

        m_messageHandler = std::make_unique<MessageHandler>(controller, this, nullptr, bus);
        if (!Pointer(m_messageHandler.get(), "MessageHandler", "WindowManager"))
        {
            throw std::runtime_error("WindowManager: Failed to create main MessageHandler.");
        }

        m_uiMessageHandler = std::make_unique<UIMessageHandler>(controller, this, m_uiManager.get(), bus);
        if (!Pointer(m_uiMessageHandler.get(), "UIMessageHandler", "WindowManager"))
        {
            throw std::runtime_error("WindowManager: Failed to create UI MessageHandler.");
        }

        LOG_INFO("WindowManager: Construction completed.");
    }

    WindowManager::~WindowManager() noexcept
    {
        LOG_INFO("WindowManager: Shutting down...");

        HideUIWindow();

        if (m_isOverlay)
        {
            HideWindow(m_overlayWnd.get());
        }
        else
        {
            HideWindow(m_mainWnd.get());
        }

        LOG_INFO("WindowManager: Shutdown complete.");
    }

    bool WindowManager::Initialize()
    {
        LOG_INFO("WindowManager: Starting initialization sequence...");

        if (!InitializeWindows())
        {
            LOG_ERROR("WindowManager: Window initialization failed.");
            return false;
        }

        if (!InitializeGraphics())
        {
            LOG_ERROR("WindowManager: Graphics initialization failed.");
            return false;
        }

        if (!InitializeUI())
        {
            LOG_ERROR("WindowManager: UI initialization failed.");
            return false;
        }

        if (m_mainWnd)
        {
            if (!Helpers::Window::CenterWindow(m_mainWnd->GetHwnd()))
            {
                LOG_WARNING("WindowManager: Failed to center main window.");
            }
            ShowWindow(m_mainWnd.get());
        }

        ShowUIWindow();
        ForceUIRender();

        LOG_INFO("WindowManager: Initialization completed successfully.");
        return true;
    }

    bool WindowManager::HandleVisualizationResize(int width, int height, bool recreateContext)
    {
        LOG_INFO("WindowManager: Handling visualization resize ("
            << width << "x" << height << ", recreate=" << recreateContext << ")");

        if (!Helpers::Window::IsValidSize(width, height))
        {
            LOG_ERROR("WindowManager: Invalid dimensions for visualization resize.");
            return false;
        }

        if (recreateContext)
        {
            if (!RecreateVisualizationContext(GetCurrentHwnd()))
            {
                LOG_ERROR("WindowManager: Visualization context recreation failed.");
                return false;
            }
        }

        if (m_engine)
        {
            m_engine->Resize(width, height);
        }

        if (m_controller)
        {
            m_controller->OnResize(width, height);
        }

        LOG_INFO("WindowManager: Visualization resize handled successfully.");
        return true;
    }

    bool WindowManager::HandleUIResize(int width, int height, bool recreateContext)
    {
        LOG_INFO("WindowManager: Handling UI resize ("
            << width << "x" << height << ", recreate=" << recreateContext << ")");

        if (!Helpers::Window::IsValidSize(width, height))
        {
            LOG_ERROR("WindowManager: Invalid dimensions for UI resize.");
            return false;
        }

        if (recreateContext)
        {
            if (!m_uiWnd)
            {
                LOG_ERROR("WindowManager: UI window not available for context recreation.");
                return false;
            }

            if (!RecreateUIContext(m_uiWnd->GetHwnd()))
            {
                LOG_ERROR("WindowManager: UI context recreation failed.");
                return false;
            }

            if (m_uiManager)
            {
                LOG_INFO("WindowManager: Re-initializing UIManager after context recreation.");
                m_uiManager->Shutdown();

                if (!m_uiManager->Initialize())
                {
                    LOG_ERROR("WindowManager: Failed to re-initialize UIManager.");
                    return false;
                }
            }
        }

        if (m_uiEngine)
        {
            m_uiEngine->Resize(width, height);
        }

        if (m_controller)
        {
            m_controller->OnUIResize(width, height);
        }

        LOG_INFO("WindowManager: UI resize handled successfully.");
        return true;
    }

    void WindowManager::OnResizeStart()
    {
        m_isResizing = true;
        LOG_INFO("WindowManager: Main resize started.");
    }

    void WindowManager::OnUIResizeStart()
    {
        m_isUIResizing = true;
        LOG_INFO("WindowManager: UI resize started.");
    }

    void WindowManager::OnResizeEnd(HWND hwnd)
    {
        m_isResizing = false;

        auto clientRect = Helpers::Window::GetClientRect(hwnd);
        if (!clientRect)
        {
            LOG_WARNING("WindowManager: Cannot get main window dimensions on resize end.");
            return;
        }

        const int width = clientRect->Width();
        const int height = clientRect->Height();

        if (!HandleVisualizationResize(width, height, false))
        {
            LOG_ERROR("WindowManager: Failed to handle main window resize end.");
        }

        LOG_INFO("WindowManager: Main resize completed at " << width << "x" << height);
    }

    void WindowManager::OnUIResizeEnd(HWND hwnd)
    {
        m_isUIResizing = false;

        auto clientRect = Helpers::Window::GetClientRect(hwnd);
        if (!clientRect)
        {
            LOG_WARNING("WindowManager: Cannot get UI window dimensions on resize end.");
            return;
        }

        const int width = clientRect->Width();
        const int height = clientRect->Height();

        if (!HandleUIResize(width, height, false))
        {
            LOG_ERROR("WindowManager: Failed to handle UI window resize end.");
        }

        LOG_INFO("WindowManager: UI resize completed at " << width << "x" << height);
    }

    void WindowManager::OnResize(HWND hwnd, int width, int height)
    {
        if (ShouldSkipResize(width, height))
        {
            return;
        }

        m_lastWidth = width;
        m_lastHeight = height;

        if (m_isResizing && m_engine)
        {
            m_engine->Resize(width, height);
        }
        else if (!HandleVisualizationResize(width, height, false))
        {
            LOG_ERROR("WindowManager: Failed to handle main window resize.");
        }
    }

    void WindowManager::OnUIResize(HWND hwnd, int width, int height)
    {
        if (width == m_lastUIWidth && height == m_lastUIHeight)
        {
            return;
        }

        m_lastUIWidth = width;
        m_lastUIHeight = height;

        if (m_isUIResizing && m_uiEngine)
        {
            m_uiEngine->Resize(width, height);
        }
        else if (!HandleUIResize(width, height, false))
        {
            LOG_ERROR("WindowManager: Failed to handle UI window resize.");
        }
    }

    void WindowManager::ToggleOverlay()
    {
        m_isOverlay = !m_isOverlay;

        LOG_INFO("WindowManager: Switching to " << (m_isOverlay ? "OVERLAY" : "NORMAL") << " mode.");

        if (m_isOverlay)
        {
            SwitchActiveWindow(m_mainWnd.get(), m_overlayWnd.get());
        }
        else
        {
            SwitchActiveWindow(m_overlayWnd.get(), m_mainWnd.get());
        }

        NotifyRendererOfModeChange();
        LOG_INFO("WindowManager: Mode switch completed.");
    }

    void WindowManager::ShowUIWindow() const
    {
        if (m_uiWnd)
        {
            m_uiWnd->Show();
        }
    }

    void WindowManager::HideUIWindow() const
    {
        if (m_uiWnd)
        {
            m_uiWnd->Hide();
        }
    }

    void WindowManager::ForceUIRender()
    {
        if (m_uiManager && m_uiEngine && m_uiWnd && IsUIWindowVisible())
        {
            m_uiEngine->ClearD3D11(Color(0.05f, 0.05f, 0.10f, 1.0f));
            m_uiManager->BeginFrame();
            m_uiManager->Render();
            m_uiManager->EndFrame();
            m_uiEngine->Present();
        }
    }

    bool WindowManager::IsRunning() const
    {
        return m_mainWnd && m_mainWnd->IsRunning();
    }

    bool WindowManager::IsOverlayMode() const noexcept
    {
        return m_isOverlay;
    }

    bool WindowManager::IsActive() const
    {
        return IsRunning() && Helpers::Window::IsActiveAndVisible(GetCurrentHwnd());
    }

    bool WindowManager::IsResizing() const noexcept
    {
        return m_isResizing || m_isUIResizing;
    }

    bool WindowManager::IsUIWindowVisible() const
    {
        return m_uiWnd && IsWindowVisible(m_uiWnd->GetHwnd());
    }

    RenderEngine* WindowManager::GetVisualizationEngine() const noexcept
    {
        return m_engine.get();
    }

    RenderEngine* WindowManager::GetUIEngine() const noexcept
    {
        return m_uiEngine.get();
    }

    UIManager* WindowManager::GetUIManager() const noexcept
    {
        return m_uiManager.get();
    }

    MessageHandler* WindowManager::GetMessageHandler() const noexcept
    {
        return m_messageHandler.get();
    }

    MainWindow* WindowManager::GetMainWindow() const noexcept
    {
        return m_mainWnd.get();
    }

    UIWindow* WindowManager::GetUIWindow() const noexcept
    {
        return m_uiWnd.get();
    }

    HWND WindowManager::GetCurrentHwnd() const
    {
        return m_isOverlay
            ? (m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr)
            : (m_mainWnd ? m_mainWnd->GetHwnd() : nullptr);
    }

    bool WindowManager::InitializeWindows()
    {
        LOG_INFO("WindowManager: Creating all windows...");

        m_mainWnd = CreateMainWindowInstance(kMainWindowTitle, kMainWindowWidth, kMainWindowHeight, false);
        VALIDATE_PTR_OR_RETURN_FALSE(m_mainWnd.get(), "WindowManager");

        const auto screenSize = Win32Utils::GetScreenSize();
        m_overlayWnd = CreateMainWindowInstance(kOverlayWindowTitle, screenSize.w, kOverlayWindowHeight, true);
        VALIDATE_PTR_OR_RETURN_FALSE(m_overlayWnd.get(), "WindowManager");

        m_uiWnd = CreateUIWindowInstance();
        VALIDATE_PTR_OR_RETURN_FALSE(m_uiWnd.get(), "WindowManager");

        LOG_INFO("WindowManager: All windows created successfully.");
        return true;
    }

    bool WindowManager::InitializeGraphics()
    {
        LOG_INFO("WindowManager: Initializing graphics contexts...");

        VALIDATE_PTR_OR_RETURN_FALSE(m_mainWnd.get(), "WindowManager");
        if (!RecreateVisualizationContext(m_mainWnd->GetHwnd()))
        {
            LOG_ERROR("WindowManager: Failed to create visualization context.");
            return false;
        }

        VALIDATE_PTR_OR_RETURN_FALSE(m_uiWnd.get(), "WindowManager");
        if (!RecreateUIContext(m_uiWnd->GetHwnd()))
        {
            LOG_ERROR("WindowManager: Failed to create UI context.");
            return false;
        }

        LOG_INFO("WindowManager: Graphics initialized.");
        return true;
    }

    bool WindowManager::InitializeUI()
    {
        LOG_INFO("WindowManager: Initializing UI components...");

        VALIDATE_PTR_OR_RETURN_FALSE(m_uiManager.get(), "WindowManager");

        if (!m_uiManager->Initialize())
        {
            LOG_ERROR("WindowManager: UIManager initialization failed.");
            return false;
        }

        LOG_INFO("WindowManager: UI components initialized.");
        return true;
    }

    std::unique_ptr<MainWindow> WindowManager::CreateMainWindowInstance(
        const wchar_t* title,
        int width,
        int height,
        bool isOverlay) const
    {
        LOG_INFO("WindowManager: Creating main window: " << (isOverlay ? "overlay" : "normal"));

        auto window = std::make_unique<MainWindow>(m_hInstance);

        if (!window->Initialize(title, width, height, isOverlay, m_messageHandler.get()))
        {
            LOG_ERROR("WindowManager: Failed to initialize main window instance.");
            return nullptr;
        }

        LOG_INFO("WindowManager: Main window created successfully.");
        return window;
    }

    std::unique_ptr<UIWindow> WindowManager::CreateUIWindowInstance() const
    {
        LOG_INFO("WindowManager: Creating UI window...");

        auto window = std::make_unique<UIWindow>(m_hInstance);

        if (!window->Initialize(kUIWindowTitle, kUIWindowWidth, kUIWindowHeight, m_uiMessageHandler.get()))
        {
            LOG_ERROR("WindowManager: Failed to initialize UI window instance.");
            return nullptr;
        }

        LOG_INFO("WindowManager: UI window created successfully.");
        return window;
    }

    void WindowManager::SwitchActiveWindow(MainWindow* hide, MainWindow* show)
    {
        if (!hide || !show)
        {
            LOG_WARNING("WindowManager: Invalid windows for switch.");
            return;
        }

        HideWindow(hide);

        auto clientRect = Helpers::Window::GetClientRect(show->GetHwnd());
        if (!clientRect)
        {
            LOG_ERROR("WindowManager: Cannot get window dimensions on switch.");
            return;
        }

        if (!HandleVisualizationResize(clientRect->Width(), clientRect->Height(), true))
        {
            LOG_ERROR("WindowManager: Failed to switch active window.");
            return;
        }

        if (m_isOverlay)
        {
            PositionOverlayWindow();
        }

        ShowWindow(show);
    }

    void WindowManager::ShowWindow(MainWindow* window) const
    {
        if (window)
        {
            Helpers::Window::ShowWindowState(window->GetHwnd());
        }
    }

    void WindowManager::HideWindow(MainWindow* window) const
    {
        if (window)
        {
            Helpers::Window::HideWindow(window->GetHwnd());
        }
    }

    void WindowManager::PositionOverlayWindow() const
    {
        if (!m_overlayWnd)
        {
            return;
        }

        if (!Helpers::Window::PositionAtBottom(m_overlayWnd->GetHwnd(), m_overlayWnd->GetHeight()))
        {
            LOG_WARNING("WindowManager: Failed to position overlay window.");
        }
    }

    bool WindowManager::RecreateVisualizationContext(HWND hwnd)
    {
        if (!Helpers::Window::IsWindowValid(hwnd))
        {
            LOG_ERROR("WindowManager: Invalid HWND for visualization context.");
            return false;
        }

        LOG_INFO("WindowManager: Creating Visualization RenderEngine (D2D-only)...");

        m_engine = std::make_unique<RenderEngine>(hwnd, m_isOverlay, true);
        VALIDATE_PTR_OR_RETURN_FALSE(m_engine.get(), "WindowManager");

        if (!m_engine->Initialize())
        {
            LOG_ERROR("WindowManager: Visualization RenderEngine initialization failed.");
            return false;
        }

        LOG_INFO("WindowManager: Visualization RenderEngine created successfully.");
        return true;
    }

    bool WindowManager::RecreateUIContext(HWND hwnd)
    {
        if (!Helpers::Window::IsWindowValid(hwnd))
        {
            LOG_ERROR("WindowManager: Invalid HWND for UI context.");
            return false;
        }

        LOG_INFO("WindowManager: Creating UI RenderEngine (D3D11-only)...");

        m_uiEngine = std::make_unique<RenderEngine>(hwnd, false, false);
        VALIDATE_PTR_OR_RETURN_FALSE(m_uiEngine.get(), "WindowManager");

        if (!m_uiEngine->Initialize())
        {
            LOG_ERROR("WindowManager: UI RenderEngine initialization failed.");
            return false;
        }

        LOG_INFO("WindowManager: UI RenderEngine created successfully.");
        return true;
    }

    void WindowManager::NotifyRendererOfModeChange() const
    {
        if (!m_controller)
        {
            return;
        }

        auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager)
        {
            return;
        }

        auto* renderer = rendererManager->GetCurrentRenderer();
        if (renderer)
        {
            renderer->SetOverlayMode(m_isOverlay);
            LOG_INFO("WindowManager: Renderer notified of mode change.");
        }
    }

    bool WindowManager::ShouldSkipResize(int width, int height) const noexcept
    {
        return !Helpers::Window::IsValidSize(width, height) ||
            (width == m_lastWidth && height == m_lastHeight);
    }

    bool WindowManager::ValidateDependencies() const noexcept
    {
        return Pointer(m_controller, "Controller", "WindowManager");
    }

}  // namespace Spectrum::Platform