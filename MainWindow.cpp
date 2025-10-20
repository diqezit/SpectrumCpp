// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the MainWindow class
// It abstracts the complexities of Win32 window management, providing a
// clean interface for creating, showing, and processing window messages
//
// Implements the MainWindow class by defining the step-by-step process of
// Win32 window registration and creation. It also contains the static WndProc
// which acts as the entry point for all window messages, delegating them
// to the associated WindowManager instance.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "MainWindow.h"
#include "ControllerCore.h"
#include "Resource.h"
#include "WindowManager.h"
#include "WindowHelper.h"

namespace Spectrum {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    MainWindow::MainWindow(HINSTANCE hInstance) :
        m_hInstance(hInstance),
        m_hwnd(nullptr),
        m_running(false),
        m_isOverlay(false),
        m_width(0),
        m_height(0)
    {
    }

    MainWindow::~MainWindow() noexcept
    {
        if (m_hwnd) DestroyWindow(m_hwnd);
        if (!m_className.empty()) UnregisterClassW(m_className.c_str(), m_hInstance);
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Main Execution Loop
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void MainWindow::ProcessMessages()
    {
        MSG msg = {};
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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // State Queries & Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void MainWindow::Show(int cmdShow) const
    {
        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
    }

    void MainWindow::Hide() const
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }

    void MainWindow::Close()
    {
        if (m_running)
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
    }

    [[nodiscard]] bool MainWindow::IsRunning() const noexcept
    {
        return m_running;
    }

    void MainWindow::SetRunning(bool running)
    {
        m_running = running;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Public Getters
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

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

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    [[nodiscard]] bool MainWindow::RegisterWindowClass()
    {
        WNDCLASSEXW wcex = CreateWindowClass();
        return RegisterClassExW(&wcex) != 0;
    }

    [[nodiscard]] WNDCLASSEXW MainWindow::CreateWindowClass() const
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = m_hInstance;
        wcex.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wcex.hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = m_className.c_str();
        wcex.hbrBackground = m_isOverlay
            ? CreateSolidBrush(RGB(0, 0, 0))
            : (HBRUSH)(COLOR_WINDOW + 1);
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
            params.x, params.y, params.w, params.h,
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
        RECT rc = { 0, 0, width, height };
        WindowUtils::AdjustRectIfNeeded(rc, styles, m_isOverlay);

        if (m_isOverlay) return { 0, 0, width, height };

        return {
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rc.right - rc.left,
            rc.bottom - rc.top
        };
    }

    void MainWindow::ApplyPostCreationStyles() const
    {
        if (m_isOverlay)
            WindowUtils::ApplyOverlay(m_hwnd);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Win32 Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    LRESULT CALLBACK MainWindow::WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        if (msg == WM_NCCREATE)
            StoreManagerPointer(hwnd, lParam);

        WindowManager* wm = GetManagerFromHwnd(hwnd);

        if (wm)
            return wm->HandleWindowMessage(hwnd, msg, wParam, lParam);

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void MainWindow::StoreManagerPointer(HWND hwnd, LPARAM lParam)
    {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
    }

    WindowManager* MainWindow::GetManagerFromHwnd(HWND hwnd)
    {
        return reinterpret_cast<WindowManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

} // namespace Spectrum