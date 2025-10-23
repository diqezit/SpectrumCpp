#include "MainWindow.h"
#include "Resources/Resource.h"
#include "MessageHandler.h"
#include "Win32Utils.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum::Platform {

    namespace {
        constexpr int kMinWindowWidth = 320;
        constexpr int kMinWindowHeight = 240;
        constexpr int kMaxWindowWidth = 7680;
        constexpr int kMaxWindowHeight = 4320;
    }

    MainWindow::MainWindow(HINSTANCE hInstance) :
        m_hInstance(hInstance),
        m_hwnd(nullptr),
        m_className(),
        m_running(false),
        m_isOverlay(false),
        m_classRegistered(false),
        m_width(0),
        m_height(0)
    {
    }

    MainWindow::~MainWindow() noexcept
    {
        LOG_INFO("MainWindow: Starting cleanup.");
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (m_classRegistered && !m_className.empty())
        {
            UnregisterClassW(m_className.c_str(), m_hInstance);
            m_classRegistered = false;
        }
        LOG_INFO("MainWindow: Cleanup complete.");
    }

    [[nodiscard]] bool MainWindow::Initialize(
        const std::wstring& title,
        int width,
        int height,
        bool isOverlay,
        MessageHandler* messageHandler
    )
    {
        char titleBuffer[256] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, titleBuffer, sizeof(titleBuffer) - 1, nullptr, nullptr);
        LOG_INFO("MainWindow: Initializing window '" << titleBuffer << "' (" << width << "x" << height << ")");

        if (!m_hInstance || !messageHandler)
        {
            LOG_ERROR("MainWindow: Invalid parameters (hInstance or messageHandler is null).");
            return false;
        }

        if (width < kMinWindowWidth || width > kMaxWindowWidth || height < kMinWindowHeight || height > kMaxWindowHeight)
        {
            LOG_ERROR("MainWindow: Invalid dimensions: " << width << "x" << height);
            return false;
        }

        m_width = width;
        m_height = height;
        m_isOverlay = isOverlay;
        m_className = isOverlay ? L"SpectrumOverlayClass" : L"SpectrumMainClass";

        if (!RegisterWindowClass())
        {
            LOG_ERROR("MainWindow: Failed to register window class.");
            return false;
        }

        m_hwnd = CreateWindowHandle(title, width, height, messageHandler);
        if (!m_hwnd)
        {
            LOG_ERROR("MainWindow: Failed to create window handle.");
            return false;
        }

        m_running = true;
        LOG_INFO("MainWindow: Window created successfully (HWND: " << m_hwnd << ", " << (m_isOverlay ? "Overlay" : "Normal") << ")");
        return true;
    }

    void MainWindow::Show(int cmdShow) const
    {
        VALIDATE_PTR_OR_RETURN(m_hwnd, "MainWindow");
        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
    }

    void MainWindow::Hide() const
    {
        Helpers::Window::HideWindow(m_hwnd);
    }

    void MainWindow::Close()
    {
        if (m_running && m_hwnd)
        {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
    }

    void MainWindow::SetRunning(bool running) noexcept
    {
        m_running = running;
    }

    [[nodiscard]] bool MainWindow::IsRunning() const noexcept
    {
        return m_running;
    }

    [[nodiscard]] HWND MainWindow::GetHwnd() const noexcept { return m_hwnd; }
    [[nodiscard]] int MainWindow::GetWidth() const noexcept { return m_width; }
    [[nodiscard]] int MainWindow::GetHeight() const noexcept { return m_height; }

    // Overlay mode needs null background to allow rendering transparent visualization
    bool MainWindow::RegisterWindowClass()
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = m_hInstance;
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = m_className.c_str();
        wcex.hbrBackground = m_isOverlay ? nullptr : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wcex.hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));

        if (!wcex.hIcon) { wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION); }
        if (!wcex.hIconSm) { wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION); }

        if (RegisterClassExW(&wcex) == 0)
        {
            return false;
        }

        m_classRegistered = true;
        LOG_INFO("MainWindow: Window class registered: " << (m_isOverlay ? "Overlay" : "Normal"));
        return true;
    }

    // Overlay skips AdjustWindowRect because borderless window already matches client area
    // WS_EX_TRANSPARENT added after creation to let mouse events pass through to desktop
    [[nodiscard]] HWND MainWindow::CreateWindowHandle(
        const std::wstring& title,
        int width,
        int height,
        MessageHandler* messageHandler
    )
    {
        const auto styles = Win32Utils::MakeStyles(m_isOverlay);
        int x = 0, y = 0, w = width, h = height;

        if (!m_isOverlay)
        {
            RECT rect{ 0, 0, width, height };
            Win32Utils::AdjustRectForStyles(rect, { styles.style, styles.exStyle });
            x = CW_USEDEFAULT;
            y = CW_USEDEFAULT;
            w = rect.right - rect.left;
            h = rect.bottom - rect.top;
        }

        HWND hwnd = CreateWindowExW(
            styles.exStyle,
            m_className.c_str(),
            title.c_str(),
            styles.style,
            x, y, w, h,
            nullptr,
            nullptr,
            m_hInstance,
            messageHandler
        );

        if (hwnd && m_isOverlay)
        {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        }

        return hwnd;
    }

    // Handler must be set during WM_NCCREATE before WM_CREATE arrives
    LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        auto* handler = reinterpret_cast<MessageHandler*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (handler)
        {
            return handler->HandleWindowMessage(hwnd, msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

} // namespace Spectrum::Platform