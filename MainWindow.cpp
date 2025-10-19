// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the MainWindow class
// It abstracts the complexities of Win32 window management, providing a
// clean interface for creating, showing, and processing window messages
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "MainWindow.h"
#include "WindowHelper.h"
#include "ControllerCore.h"
#include "WindowManager.h"
#include "Resource.h"

namespace Spectrum {

    MainWindow::MainWindow(HINSTANCE hInstance)
        : m_hInstance(hInstance),
        m_hwnd(nullptr),
        m_running(false),
        m_isOverlay(false),
        m_width(0),
        m_height(0) {
    }

    MainWindow::~MainWindow() {
        if (m_hwnd) DestroyWindow(m_hwnd);
        if (!m_className.empty()) UnregisterClassW(m_className.c_str(), m_hInstance);
    }

    bool MainWindow::Initialize(
        const std::wstring& title,
        int width,
        int height,
        bool isOverlay,
        void* userPtr
    ) {
        m_width = width;
        m_height = height;
        m_isOverlay = isOverlay;
        m_className = isOverlay ? L"SpectrumOverlayClass" : L"SpectrumMainClass";

        if (!Register()) return false;
        if (!CreateWindowInstance(title, width, height, userPtr)) return false;

        ApplyStyles();
        m_running = true;
        return true;
    }

    WNDCLASSEXW MainWindow::CreateWindowClass() const {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = m_hInstance;
        wcex.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wcex.hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = m_className.c_str();

        if (m_isOverlay) {
            wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
        }
        else {
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        }
        return wcex;
    }

    bool MainWindow::Register() {
        WNDCLASSEXW wcex = CreateWindowClass();
        return RegisterClassExW(&wcex) != 0;
    }

    MainWindow::WindowRectParams MainWindow::CalculateWindowRect(
        int width,
        int height,
        const WindowUtils::Styles& styles
    ) const {
        RECT rc = { 0, 0, width, height };
        WindowUtils::AdjustRectIfNeeded(rc, styles, m_isOverlay);

        WindowRectParams params{};
        params.w = m_isOverlay ? width : rc.right - rc.left;
        params.h = m_isOverlay ? height : rc.bottom - rc.top;
        params.x = m_isOverlay ? 0 : CW_USEDEFAULT;
        params.y = m_isOverlay ? 0 : CW_USEDEFAULT;
        return params;
    }

    bool MainWindow::CreateWindowInstance(
        const std::wstring& title,
        int width,
        int height,
        void* userPtr
    ) {
        auto styles = WindowUtils::MakeStyles(m_isOverlay);
        auto params = CalculateWindowRect(width, height, styles);

        m_hwnd = WindowUtils::CreateWindowWithStyles(
            m_hInstance,
            m_className.c_str(),
            title.c_str(),
            styles,
            params.x, params.y, params.w, params.h,
            userPtr
        );

        return m_hwnd != nullptr;
    }

    void MainWindow::ApplyStyles() const {
        if (m_isOverlay) WindowUtils::ApplyOverlay(m_hwnd);
    }

    void MainWindow::ProcessMessages() {
        MSG msg = {};
        while (m_running && PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void MainWindow::Show(int cmdShow) const {
        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
    }

    void MainWindow::Hide() const {
        ShowWindow(m_hwnd, SW_HIDE);
    }

    void MainWindow::Close() {
        if (m_running) PostMessage(m_hwnd, WM_CLOSE, 0, 0);
    }

    void MainWindow::StoreManagerPointer(HWND hwnd, LPARAM lParam) {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto* wm = reinterpret_cast<WindowManager*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wm));
    }

    WindowManager* MainWindow::GetManagerFromHwnd(HWND hwnd) {
        return reinterpret_cast<WindowManager*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA)
            );
    }

    LRESULT CALLBACK MainWindow::WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    ) {
        WindowManager* wm = nullptr;
        if (msg == WM_NCCREATE) {
            StoreManagerPointer(hwnd, lParam);
            wm = GetManagerFromHwnd(hwnd);
        }
        else {
            wm = GetManagerFromHwnd(hwnd);
        }

        if (wm && wm->GetController()) {
            return wm->GetController()->HandleWindowMessage(
                hwnd, msg, wParam, lParam
            );
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

} // namespace Spectrum