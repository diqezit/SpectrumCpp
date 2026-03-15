#ifndef SPECTRUM_CPP_MAINWINDOW_H
#define SPECTRUM_CPP_MAINWINDOW_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Main application window — supports normal and overlay modes.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Platform/WindowBase.h"
#include "Platform/MessageHandler.h"

namespace Spectrum::Platform {

    class MainWindow final : public WindowBase {
    public:
        explicit MainWindow(HINSTANCE h) : WindowBase(h) {}

        [[nodiscard]] bool Initialize(
            const std::wstring& title, int w, int h,
            bool overlay, MessageHandler* handler)
        {
            m_overlay = overlay;
            m_className = overlay ? L"SpectrumOverlayClass" : L"SpectrumMainClass";

            if (!Init(title, w, h,
                WindowLimits::MainMinW, WindowLimits::MainMaxW,
                WindowLimits::MainMinH, WindowLimits::MainMaxH,
                handler))
                return false;

            m_running = true;
            return true;
        }

        void Show(int cmd = SW_SHOW) const { ShowAt(cmd, !m_overlay, false); }

        void SetRunning(bool v) noexcept { m_running = v; }
        [[nodiscard]] bool IsRunning() const noexcept { return m_running; }

    protected:
        void CustomizeClass(WNDCLASSEXW& wc) override {
            wc.hbrBackground = m_overlay
                ? nullptr
                : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        }

        DWORD StyleFlags() const override {
            return m_overlay ? WS_POPUP : WS_OVERLAPPEDWINDOW;
        }

        DWORD ExStyleFlags() const override {
            return m_overlay
                ? (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW)
                : WS_EX_APPWINDOW;
        }

        WNDPROC WndProcFunc() const override {
            return [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
                return CommonWndProc<MessageHandler>(h, m, w, l);
                };
        }

        void OnCreated(HWND hwnd) override {
            if (m_overlay)
                SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                    GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
        }

        bool AdjustRect() const override { return !m_overlay; }

    private:
        bool m_running = false;
        bool m_overlay = false;
    };

} // namespace Spectrum::Platform

#endif