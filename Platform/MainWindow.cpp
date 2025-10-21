// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the MainWindow class, providing a complete RAII wrapper
// around Win32 window management.
//
// This implementation handles the step-by-step process of Win32 window
// registration, creation, and message processing. The static WndProc acts
// as the entry point for all window messages, delegating them to the
// associated WindowManager instance stored via GWLP_USERDATA.
//
// Key Implementation Details:
// - Two-phase initialization: constructor + Initialize()
// - Proper cleanup tracking via m_classRegistered flag
// - Window class name differentiation for main vs overlay windows
// - Message pump that respects m_running flag for graceful shutdown
// - WndProc uses WM_NCCREATE for early user data storage
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "MainWindow.h"
#include "Resource.h"
#include "WindowHelper.h"
#include "WindowManager.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    MainWindow::MainWindow(HINSTANCE hInstance) :
        m_hInstance(hInstance),
        m_hwnd(nullptr),
        m_running(false),
        m_isOverlay(false),
        m_classRegistered(false),
        m_width(0),
        m_height(0)
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
        void* userPtr
    )
    {
        m_width = width;
        m_height = height;
        m_isOverlay = isOverlay;
        m_className = isOverlay ? L"SpectrumOverlayClass" : L"SpectrumMainClass";

        if (!RegisterWindowClass()) return false;
        if (!CreateAndConfigureWindow(title, width, height, userPtr)) return false;

        m_running = true;
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
        if (!m_hwnd) return;

        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
    }

    void MainWindow::Hide() const
    {
        if (!m_hwnd) return;

        ShowWindow(m_hwnd, SW_HIDE);
    }

    void MainWindow::Close()
    {
        if (!m_running) return;
        if (!m_hwnd) return;

        PostMessage(m_hwnd, WM_CLOSE, 0, 0);
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
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool MainWindow::RegisterWindowClass()
    {
        const WNDCLASSEXW wcex = CreateWindowClass();

        const ATOM result = RegisterClassExW(&wcex);
        if (result == 0) return false;

        m_classRegistered = true;
        return true;
    }

    [[nodiscard]] WNDCLASSEXW MainWindow::CreateWindowClass() const
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = m_hInstance;
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = m_className.c_str();
        wcex.lpszMenuName = nullptr;

        wcex.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        if (!wcex.hIcon)
            wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

        wcex.hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        if (!wcex.hIconSm)
            wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

        wcex.hbrBackground = m_isOverlay ? nullptr : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        return wcex;
    }

    [[nodiscard]] bool MainWindow::CreateAndConfigureWindow(
        const std::wstring& title,
        int width,
        int height,
        void* userPtr
    )
    {
        const auto styles = WindowUtils::MakeStyles(m_isOverlay);
        const auto params = CalculateWindowRect(width, height, styles);

        m_hwnd = WindowUtils::CreateWindowWithStyles(
            m_hInstance,
            m_className.c_str(),
            title.c_str(),
            styles,
            params.x,
            params.y,
            params.w,
            params.h,
            userPtr
        );

        if (!m_hwnd) return false;

        ApplyPostCreationStyles();
        return true;
    }

    [[nodiscard]] MainWindow::WindowRectParams MainWindow::CalculateWindowRect(
        int width,
        int height,
        const WindowUtils::Styles& styles
    ) const
    {
        RECT rect{ 0, 0, width, height };
        WindowUtils::AdjustRectIfNeeded(rect, styles, m_isOverlay);

        if (m_isOverlay)
            return { 0, 0, width, height };

        return {
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top
        };
    }

    void MainWindow::ApplyPostCreationStyles() const
    {
        if (m_isOverlay)
            WindowUtils::ApplyOverlay(m_hwnd);
    }

    void MainWindow::Cleanup() noexcept
    {
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
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Win32 Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    LRESULT CALLBACK MainWindow::WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        if (msg == WM_NCCREATE)
            StoreManagerPointer(hwnd, lParam);

        WindowManager* manager = GetManagerFromHwnd(hwnd);

        if (manager)
            return manager->HandleWindowMessage(hwnd, msg, wParam, lParam);

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void MainWindow::StoreManagerPointer(HWND hwnd, LPARAM lParam)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams)
        );
    }

    WindowManager* MainWindow::GetManagerFromHwnd(HWND hwnd)
    {
        return reinterpret_cast<WindowManager*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA)
            );
    }

} // namespace Spectrum