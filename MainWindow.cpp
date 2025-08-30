// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// MainWindow.cpp: Implementation of the MainWindow class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "MainWindow.h"
#include "WindowHelper.h"
#include "Utils.h"

namespace Spectrum {

    MainWindow::MainWindow(HINSTANCE hInstance)
        : m_hInstance(hInstance)
        , m_hwnd(nullptr)
        , m_width(800)
        , m_height(600)
        , m_isOverlay(false)
        , m_running(false)
        , m_isMinimized(false)
        , m_className(L"SpectrumVisualizerWindow")
        , m_title(L"Spectrum Visualizer") {
    }

    MainWindow::~MainWindow() {
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
        UnregisterClassW(m_className.c_str(), m_hInstance);
    }

    bool MainWindow::Initialize(bool isOverlay,
        int width,
        int height) {
        m_isOverlay = isOverlay;
        m_width = width;
        m_height = height;

        if (isOverlay) {
            m_className = L"SpectrumOverlayWindow";
            m_title = L"Spectrum Overlay";
        }

        if (!RegisterWindowClass()) {
            LOG_ERROR("Failed to register window class");
            return false;
        }

        if (!CreateWindowInstance()) {
            LOG_ERROR("Failed to create window instance");
            return false;
        }

        ApplyWindowStyles();
        Show();
        m_running = true;

        return true;
    }

    bool MainWindow::RegisterWindowClass() const {
        return WindowUtils::RegisterWindowClass(m_hInstance,
            m_className.c_str(),
            WindowProc,
            m_isOverlay);
    }

    bool MainWindow::CreateWindowInstance() {
        auto st = WindowUtils::MakeStyles(m_isOverlay);

        RECT rect = { 0, 0, m_width, m_height };
        WindowUtils::AdjustRectIfNeeded(rect, st, m_isOverlay);

        int x = m_isOverlay ? 0 : CW_USEDEFAULT;
        int y = m_isOverlay ? 0 : CW_USEDEFAULT;
        int w = m_isOverlay ? m_width : (rect.right - rect.left);
        int h = m_isOverlay ? m_height : (rect.bottom - rect.top);

        m_hwnd = WindowUtils::CreateWindowWithStyles(m_hInstance,
            m_className.c_str(),
            m_title.c_str(),
            st,
            x,
            y,
            w,
            h,
            this);

        if (!m_hwnd) {
            LOG_ERROR("CreateWindowEx failed: " << GetLastError());
            return false;
        }

        return true;
    }

    void MainWindow::ApplyWindowStyles() const {
        if (m_isOverlay)
            WindowUtils::ApplyOverlay(m_hwnd);
    }

    DWORD MainWindow::GetWindowStyleFlags() const {
        return m_isOverlay ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    }

    DWORD MainWindow::GetWindowExStyleFlags() const {
        if (m_isOverlay)
            return WS_EX_LAYERED | WS_EX_TRANSPARENT |
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
        return WS_EX_APPWINDOW;
    }

    void MainWindow::UpdateWindowStyles() const {
        if (!m_hwnd) return;

        SetWindowLongPtr(m_hwnd, GWL_STYLE, GetWindowStyleFlags());
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, GetWindowExStyleFlags());

        SetWindowPos(m_hwnd,
            m_isOverlay ? HWND_TOPMOST : HWND_NOTOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    }

    void MainWindow::ProcessMessages() {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void MainWindow::Show() const {
        if (m_hwnd) {
            ShowWindow(m_hwnd, SW_SHOW);
            UpdateWindow(m_hwnd);
        }
    }

    void MainWindow::Hide() const {
        if (m_hwnd)
            ShowWindow(m_hwnd, SW_HIDE);
    }

    void MainWindow::Close() {
        m_running = false;
        if (m_hwnd)
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
    }

    void MainWindow::Minimize() {
        if (m_hwnd) {
            ShowWindow(m_hwnd, SW_MINIMIZE);
            m_isMinimized = true;
        }
    }

    void MainWindow::Restore() {
        if (m_hwnd) {
            ShowWindow(m_hwnd, SW_RESTORE);
            m_isMinimized = false;
        }
    }

    void MainWindow::SetTitle(const std::wstring& title) {
        m_title = title;
        if (m_hwnd)
            SetWindowTextW(m_hwnd, title.c_str());
    }

    void MainWindow::SetPosition(int x, int y) const {
        if (m_hwnd)
            SetWindowPos(m_hwnd,
                nullptr,
                x,
                y,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);
    }

    void MainWindow::CenterOnScreen() const {
        if (!m_hwnd) return;

        RECT rect{};
        if (!GetWindowRect(m_hwnd, &rect)) return;

        int ww = rect.right - rect.left;
        int hh = rect.bottom - rect.top;

        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);

        int x = (sw - ww) / 2;
        int y = (sh - hh) / 2;

        SetWindowPos(m_hwnd,
            nullptr,
            x,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOZORDER);
    }

    void MainWindow::MakeTransparent() const {
        if (!m_hwnd) return;
        LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
    }

    void MainWindow::MakeOpaque() const {
        if (!m_hwnd) return;
        LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
    }

    LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam) {
        MainWindow* window = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            window = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(window));
            window->m_hwnd = hwnd;
        }
        else {
            window = reinterpret_cast<MainWindow*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA)
                );
        }

        if (window)
            return window->HandleMessage(msg, wParam, lParam);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT MainWindow::HandleMessage(UINT msg,
        WPARAM wParam,
        LPARAM lParam) {
        switch (msg) {
        case WM_DESTROY:
            HandleClose();
            return 0;

        case WM_SIZE:
            HandleResize(wParam, lParam);
            return 0;

        case WM_KEYDOWN:
            HandleKeyDown(wParam);
            return 0;

        case WM_LBUTTONDOWN:
            HandleMouseClick(lParam);
            return 0;

        case WM_MOUSEMOVE:
            HandleMouseMove(lParam);
            return 0;

        case WM_NCHITTEST:
            if (m_isOverlay)
                return HTTRANSPARENT;
            break;

        case WM_ERASEBKGND:
            return 1;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void MainWindow::HandleResize(WPARAM wParam, LPARAM lParam) {
        WindowUtils::ExtractSize(lParam, m_width, m_height);
        WindowUtils::UpdateMinimizeFlagOnSize(wParam, m_isMinimized);

        if (m_resizeCallback && !m_isMinimized)
            m_resizeCallback(m_width, m_height);
    }

    void MainWindow::HandleKeyDown(WPARAM wParam) {
        if (m_keyCallback)
            m_keyCallback(static_cast<int>(wParam));
    }

    void MainWindow::HandleMouseMove(LPARAM lParam) {
        if (m_mouseMoveCallback) {
            int x, y;
            WindowUtils::ExtractMouse(lParam, x, y);
            m_mouseMoveCallback(x, y);
        }
    }

    void MainWindow::HandleMouseClick(LPARAM lParam) {
        if (m_mouseClickCallback) {
            int x, y;
            WindowUtils::ExtractMouse(lParam, x, y);
            m_mouseClickCallback(x, y);
        }
    }

    void MainWindow::HandleClose() {
        m_running = false;
        m_hwnd = nullptr;

        if (!m_isOverlay)
            PostQuitMessage(0);

        if (m_closeCallback)
            m_closeCallback();
    }

} // namespace Spectrum