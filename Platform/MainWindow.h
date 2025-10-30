#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

#include "Common/Common.h"
#include "WindowBase.h"
#include <string>

namespace Spectrum::Platform {

    class MessageHandler;

    class MainWindow final : public WindowBase {
    public:
        explicit MainWindow(HINSTANCE hInstance);
        ~MainWindow() noexcept override = default;

        [[nodiscard]] bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            bool isOverlay,
            MessageHandler* messageHandler
        );

        void Show(int cmdShow = SW_SHOW) const;
        void SetRunning(bool running) noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;

    protected:
        [[nodiscard]] const char* GetWindowTypeName() const noexcept override;
        void CustomizeWindowClass(WNDCLASSEXW& wcex) override;
        [[nodiscard]] DWORD GetStyleFlags() const override;
        [[nodiscard]] DWORD GetExStyleFlags() const override;
        void OnWindowCreated(HWND hwnd) override;
        [[nodiscard]] bool CanClose() const override;
        [[nodiscard]] WNDPROC GetWindowProc() const override;
        [[nodiscard]] bool ShouldAdjustWindowRect() const override;

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        bool m_running;
        bool m_isOverlay;
    };

}

#endif