#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common/Common.h"
#include <string>

namespace Spectrum::Platform {

    class MessageHandler;

    class MainWindow final
    {
    public:
        explicit MainWindow(HINSTANCE hInstance);
        ~MainWindow() noexcept;

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator=(const MainWindow&) = delete;
        MainWindow(MainWindow&&) = delete;
        MainWindow& operator=(MainWindow&&) = delete;

        [[nodiscard]] bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            bool isOverlay,
            MessageHandler* messageHandler
        );

        void Show(int cmdShow = SW_SHOW) const;
        void Hide() const;
        void Close();
        void SetRunning(bool running) noexcept;

        [[nodiscard]] bool IsRunning() const noexcept;

        [[nodiscard]] HWND GetHwnd() const noexcept;
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        bool RegisterWindowClass();
        [[nodiscard]] HWND CreateWindowHandle(
            const std::wstring& title,
            int width,
            int height,
            MessageHandler* messageHandler
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

        bool m_running;
        bool m_isOverlay;
        bool m_classRegistered;

        int m_width;
        int m_height;
    };

} // namespace Spectrum::Platform

#endif // SPECTRUM_CPP_MAINWINDOW_H