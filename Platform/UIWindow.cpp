#include "UIWindow.h"
#include "UIMessageHandler.h"

namespace Spectrum::Platform {

    UIWindow::UIWindow(HINSTANCE hInstance)
        : WindowBase(hInstance) {
        m_className = L"SpectrumUIWindowClass";
    }

    bool UIWindow::Initialize(
        const std::wstring& title,
        int width,
        int height,
        UIMessageHandler* messageHandler
    ) {
        return InitializeBase(title, width, height,
            WindowLimits::UIMinWidth, WindowLimits::UIMaxWidth,
            WindowLimits::UIMinHeight, WindowLimits::UIMaxHeight,
            messageHandler);
    }

    void UIWindow::Show(int cmdShow) {
        if (m_firstShow) {
            ShowWindowWithPosition(cmdShow, false, true);
            m_firstShow = false;
        }
        else {
            ShowWindowWithPosition(cmdShow, false, false);
        }
    }

    [[nodiscard]] const char* UIWindow::GetWindowTypeName() const noexcept {
        return "UIWindow";
    }

    void UIWindow::CustomizeWindowClass(WNDCLASSEXW& wcex) {
        wcex.style |= CS_OWNDC;
        wcex.hbrBackground = nullptr;
    }

    [[nodiscard]] DWORD UIWindow::GetStyleFlags() const {
        return WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    }

    [[nodiscard]] DWORD UIWindow::GetExStyleFlags() const {
        return WS_EX_APPWINDOW;
    }

    [[nodiscard]] WNDPROC UIWindow::GetWindowProc() const {
        return WndProc;
    }

    LRESULT CALLBACK UIWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return CommonWndProc<UIMessageHandler>(hwnd, msg, wParam, lParam);
    }

}