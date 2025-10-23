// UIWindow.h
#ifndef SPECTRUM_CPP_UIWINDOW_H
#define SPECTRUM_CPP_UIWINDOW_H

#include "Common/Common.h"
#include <string>

namespace Spectrum::Platform {

    class UIMessageHandler;

    class UIWindow final
    {
    public:
        explicit UIWindow(HINSTANCE hInstance);
        ~UIWindow() noexcept;

        UIWindow(const UIWindow&) = delete;
        UIWindow& operator=(const UIWindow&) = delete;
        UIWindow(UIWindow&&) = delete;
        UIWindow& operator=(UIWindow&&) = delete;

        [[nodiscard]] bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            UIMessageHandler* messageHandler
        );

        void Show(int cmdShow = SW_SHOW) const;
        void Hide() const;
        void Close();

        [[nodiscard]] HWND GetHwnd() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        bool RegisterWindowClass();
        [[nodiscard]] HWND CreateWindowHandle(
            const std::wstring& title,
            int width,
            int height,
            UIMessageHandler* messageHandler
        );

        static LRESULT CALLBACK WndProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam
        );

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        std::wstring m_className;
        bool m_classRegistered;
        int m_width;
        int m_height;
    };

} // namespace Spectrum::Platform

#endif // SPECTRUM_CPP_UIWINDOW_H