#include "MainWindow.h"
#include "MessageHandler.h"
#include "Win32Utils.h"

namespace Spectrum::Platform {

    MainWindow::MainWindow(HINSTANCE hInstance)
        : WindowBase(hInstance)
        , m_running(false)
        , m_isOverlay(false) {
    }

    bool MainWindow::Initialize(
        const std::wstring& title,
        int width,
        int height,
        bool isOverlay,
        MessageHandler* messageHandler
    ) {
        m_isOverlay = isOverlay;
        m_className = isOverlay ? L"SpectrumOverlayClass" : L"SpectrumMainClass";

        if (!InitializeBase(title, width, height,
            WindowLimits::MainMinWidth, WindowLimits::MainMaxWidth,
            WindowLimits::MainMinHeight, WindowLimits::MainMaxHeight,
            messageHandler)) {
            return false;
        }

        m_running = true;
        LOG_INFO("MainWindow: Window created successfully (HWND: " << m_hwnd << ", "
            << (m_isOverlay ? "Overlay" : "Normal") << ")");
        return true;
    }

    void MainWindow::Show(int cmdShow) const {
        ShowWindowWithPosition(cmdShow, !m_isOverlay, false);
    }

    void MainWindow::SetRunning(bool running) noexcept {
        m_running = running;
    }

    [[nodiscard]] bool MainWindow::IsRunning() const noexcept {
        return m_running;
    }

    [[nodiscard]] const char* MainWindow::GetWindowTypeName() const noexcept {
        return "MainWindow";
    }

    void MainWindow::CustomizeWindowClass(WNDCLASSEXW& wcex) {
        wcex.hbrBackground = m_isOverlay
            ? nullptr
            : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    }

    [[nodiscard]] DWORD MainWindow::GetStyleFlags() const {
        return m_isOverlay ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    }

    [[nodiscard]] DWORD MainWindow::GetExStyleFlags() const {
        return m_isOverlay
            ? (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW)
            : WS_EX_APPWINDOW;
    }

    void MainWindow::OnWindowCreated(HWND hwnd) {
        if (m_isOverlay) {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        }
    }

    [[nodiscard]] bool MainWindow::CanClose() const {
        return m_running;
    }

    [[nodiscard]] WNDPROC MainWindow::GetWindowProc() const {
        return WndProc;
    }

    [[nodiscard]] bool MainWindow::ShouldAdjustWindowRect() const {
        return !m_isOverlay;
    }

    LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return CommonWndProc<MessageHandler>(hwnd, msg, wParam, lParam);
    }

}