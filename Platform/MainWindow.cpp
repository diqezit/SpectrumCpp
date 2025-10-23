// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the MainWindow class, providing a complete RAII wrapper
// around Win32 window management.
//
// This implementation handles the step-by-step process of Win32 window
// registration, creation, and message processing. The static WndProc acts
// as the entry point for all window messages, delegating them to the
// associated MessageHandler instance.
//
// Refactored to follow SRP and DRY principles with small, focused functions.
// Each function has a single, clear responsibility.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "MainWindow.h"
#include "Resources/Resource.h"
#include "MessageHandler.h"
#include "Win32Utils.h"
#include "Graphics/API/Helpers/Platform/WindowHelpers.h"

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Constants
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    namespace {
        constexpr int kMinWindowWidth = 320;
        constexpr int kMinWindowHeight = 240;
        constexpr int kMaxWindowWidth = 7680;
        constexpr int kMaxWindowHeight = 4320;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    MainWindow::MainWindow(HINSTANCE hInstance)
        : m_hInstance(hInstance)
        , m_hwnd(nullptr)
        , m_className()
        , m_running(false)
        , m_isOverlay(false)
        , m_classRegistered(false)
        , m_width(0)
        , m_height(0)
    {
    }

    MainWindow::~MainWindow() noexcept
    {
        Cleanup();
    }

    [[nodiscard]] bool MainWindow::Initialize(
        const std::wstring& title,
        int width,
        int height,
        bool isOverlay,
        MessageHandler* messageHandler
    )
    {
        LogInitialization(title, width, height);

        if (!ValidateInitializationParams(width, height, messageHandler)) {
            LogError("validation");
            return false;
        }

        StoreWindowDimensions(width, height);
        SetupWindowConfiguration(isOverlay);

        if (!RegisterWindowClass()) {
            LogError("window class registration");
            return false;
        }

        if (!CreateAndConfigureWindow(title, width, height, messageHandler)) {
            LogError("window creation");
            return false;
        }

        m_running = true;
        LogWindowCreated();
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MainWindow::ProcessMessages()
    {
        MSG msg{};
        while (m_running && PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MainWindow::Show(int cmdShow) const
    {
        if (!m_hwnd) {
            return;
        }

        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
    }

    void MainWindow::Hide() const
    {
        Helpers::Window::HideWindow(m_hwnd);
    }

    void MainWindow::Close()
    {
        if (m_running && m_hwnd) {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
    }

    void MainWindow::SetRunning(bool running) noexcept
    {
        m_running = running;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // State Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool MainWindow::IsRunning() const noexcept
    {
        return m_running;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] HWND MainWindow::GetHwnd() const noexcept
    {
        return m_hwnd;
    }

    [[nodiscard]] int MainWindow::GetWidth() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] int MainWindow::GetHeight() const noexcept
    {
        return m_height;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Initialization - High Level
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool MainWindow::ValidateInitializationParams(
        int width,
        int height,
        MessageHandler* handler
    ) const
    {
        if (!ValidateHInstance()) {
            LOG_ERROR("MainWindow: Invalid HINSTANCE");
            return false;
        }

        if (!ValidateDimensions(width, height)) {
            LOG_ERROR("MainWindow: Invalid dimensions: " << width << "x" << height);
            return false;
        }

        if (!ValidateMessageHandler(handler)) {
            LOG_ERROR("MainWindow: MessageHandler is null");
            return false;
        }

        return true;
    }

    void MainWindow::StoreWindowDimensions(int width, int height) noexcept
    {
        m_width = width;
        m_height = height;
    }

    void MainWindow::SetupWindowConfiguration(bool isOverlay)
    {
        m_isOverlay = isOverlay;
        m_className = GenerateClassName(isOverlay);
    }

    bool MainWindow::RegisterWindowClass()
    {
        if (!ValidateClassName()) {
            LOG_ERROR("MainWindow: Invalid class name");
            return false;
        }

        WNDCLASSEXW wcex = CreateWindowClass();

        if (RegisterClassExW(&wcex) == 0) {
            LOG_ERROR("MainWindow: Failed to register window class");
            return false;
        }

        m_classRegistered = true;
        LOG_INFO("MainWindow: Window class registered: " << (m_isOverlay ? "Overlay" : "Normal"));
        return true;
    }

    bool MainWindow::CreateAndConfigureWindow(
        const std::wstring& title,
        int width,
        int height,
        MessageHandler* messageHandler
    )
    {
        const auto styles = Win32Utils::MakeStyles(m_isOverlay);
        const auto params = CalculateWindowRect(width, height, styles.style, styles.exStyle);

        m_hwnd = CreateWindowHandle(title, params, styles.style, styles.exStyle, messageHandler);

        if (!ValidateWindowHandle(m_hwnd)) {
            LOG_ERROR("MainWindow: Failed to create window");
            return false;
        }

        ConfigureNewWindow();
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Class Registration
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] WNDCLASSEXW MainWindow::CreateWindowClass() const
    {
        WNDCLASSEXW wcex{};

        ConfigureWindowClassBase(wcex);
        SetWindowClassStyle(wcex);
        SetWindowClassBackground(wcex);

        WindowIcons icons = LoadWindowIcons();
        ApplyWindowIcons(wcex, icons);

        return wcex;
    }

    void MainWindow::ConfigureWindowClassBase(WNDCLASSEXW& wcex) const
    {
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = m_hInstance;
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = m_className.c_str();
    }

    void MainWindow::SetWindowClassStyle(WNDCLASSEXW& wcex) const
    {
        wcex.style = CS_HREDRAW | CS_VREDRAW;
    }

    void MainWindow::SetWindowClassBackground(WNDCLASSEXW& wcex) const
    {
        wcex.hbrBackground = m_isOverlay
            ? nullptr
            : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    }

    [[nodiscard]] MainWindow::WindowIcons MainWindow::LoadWindowIcons() const
    {
        WindowIcons icons{};

        icons.largeIcon = LoadApplicationIcon(m_hInstance, false);
        icons.smallIcon = LoadApplicationIcon(m_hInstance, true);

        // Fallback to default icons if application icons not found
        if (!icons.largeIcon) {
            icons.largeIcon = LoadDefaultIcon(false);
        }

        if (!icons.smallIcon) {
            icons.smallIcon = LoadDefaultIcon(true);
        }

        return icons;
    }

    void MainWindow::ApplyWindowIcons(WNDCLASSEXW& wcex, const WindowIcons& icons) const
    {
        wcex.hIcon = icons.largeIcon;
        wcex.hIconSm = icons.smallIcon;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Creation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] HWND MainWindow::CreateWindowHandle(
        const std::wstring& title,
        const WindowRectParams& params,
        DWORD style,
        DWORD exStyle,
        MessageHandler* messageHandler
    )
    {
        return CreateWindowExW(
            exStyle,
            m_className.c_str(),
            title.c_str(),
            style,
            params.x, params.y,
            params.w, params.h,
            nullptr, nullptr,
            m_hInstance,
            messageHandler
        );
    }

    bool MainWindow::ValidateWindowHandle(HWND hwnd) const
    {
        return hwnd != nullptr;
    }

    void MainWindow::ConfigureNewWindow()
    {
        ApplyPostCreationStyles();
    }

    [[nodiscard]] MainWindow::WindowRectParams MainWindow::CalculateWindowRect(
        int width,
        int height,
        DWORD style,
        DWORD exStyle
    ) const
    {
        if (m_isOverlay) {
            return CalculateOverlayRect(width, height);
        }

        return CalculateNormalRect(width, height, style, exStyle);
    }

    [[nodiscard]] MainWindow::WindowRectParams MainWindow::CalculateOverlayRect(
        int width,
        int height
    ) const
    {
        return { 0, 0, width, height };
    }

    [[nodiscard]] MainWindow::WindowRectParams MainWindow::CalculateNormalRect(
        int width,
        int height,
        DWORD style,
        DWORD exStyle
    ) const
    {
        RECT rect{ 0, 0, width, height };
        Win32Utils::AdjustRectForStyles(rect, { style, exStyle });

        return {
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top
        };
    }

    void MainWindow::ApplyPostCreationStyles() const
    {
        if (m_isOverlay) {
            ApplyOverlayTransparency();
        }
    }

    void MainWindow::ApplyOverlayTransparency() const
    {
        LONG_PTR currentExStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, currentExStyle | WS_EX_TRANSPARENT);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Cleanup
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MainWindow::Cleanup() noexcept
    {
        LogCleanup();

        DestroyWindowSafe();
        UnregisterClassSafe();
    }

    void MainWindow::DestroyWindowSafe() noexcept
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            LOG_INFO("MainWindow: Window destroyed");
        }
    }

    void MainWindow::UnregisterClassSafe() noexcept
    {
        if (m_classRegistered && !m_className.empty())
        {
            UnregisterClassW(m_className.c_str(), m_hInstance);
            m_classRegistered = false;
            LOG_INFO("MainWindow: Window class unregistered");
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool MainWindow::ValidateHInstance() const noexcept
    {
        return m_hInstance != nullptr;
    }

    [[nodiscard]] bool MainWindow::ValidateDimensions(int width, int height) const noexcept
    {
        if (width < kMinWindowWidth || width > kMaxWindowWidth) {
            return false;
        }

        if (height < kMinWindowHeight || height > kMaxWindowHeight) {
            return false;
        }

        return true;
    }

    [[nodiscard]] bool MainWindow::ValidateMessageHandler(MessageHandler* handler) const noexcept
    {
        return handler != nullptr;
    }

    [[nodiscard]] bool MainWindow::ValidateClassName() const noexcept
    {
        return !m_className.empty();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Win32 Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE) {
            StoreMessageHandlerPointer(hwnd, lParam);
        }

        if (MessageHandler* handler = GetMessageHandlerFromHwnd(hwnd))
        {
            return handler->HandleWindowMessage(hwnd, msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void MainWindow::StoreMessageHandlerPointer(HWND hwnd, LPARAM lParam)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams)
        );
    }

    MessageHandler* MainWindow::GetMessageHandlerFromHwnd(HWND hwnd)
    {
        return reinterpret_cast<MessageHandler*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helpers - Static to avoid const issues
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] std::wstring MainWindow::GenerateClassName(bool isOverlay) const
    {
        return isOverlay ? L"SpectrumOverlayClass" : L"SpectrumMainClass";
    }

    [[nodiscard]] HICON MainWindow::LoadApplicationIcon(HINSTANCE hInstance, bool isSmallIcon)
    {
        const int resourceId = isSmallIcon ? IDI_APP_ICON : IDI_APP_ICON;
        return LoadIconW(hInstance, MAKEINTRESOURCEW(resourceId));
    }

    [[nodiscard]] HICON MainWindow::LoadDefaultIcon(bool isSmallIcon)
    {
        return LoadIconW(nullptr, isSmallIcon ? IDI_APPLICATION : IDI_APPLICATION);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Logging
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void MainWindow::LogInitialization(
        const std::wstring& title,
        int width,
        int height
    ) const
    {
        char titleBuffer[256] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1,
            titleBuffer, sizeof(titleBuffer) - 1, nullptr, nullptr);

        LOG_INFO("MainWindow: Initializing window '" << titleBuffer
            << "' (" << width << "x" << height << ")");
    }

    void MainWindow::LogWindowCreated() const
    {
        LOG_INFO("MainWindow: Window created successfully (HWND: "
            << m_hwnd << ", " << (m_isOverlay ? "Overlay" : "Normal") << ")");
    }

    void MainWindow::LogCleanup() const
    {
        LOG_INFO("MainWindow: Starting cleanup");
    }

    void MainWindow::LogError(const char* operation) const
    {
        (void)operation; // Suppress warning
        LOG_ERROR("MainWindow: Failed during " << operation);
    }

} // namespace Spectrum::Platform