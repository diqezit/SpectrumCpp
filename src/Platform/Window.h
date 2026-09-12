#ifndef SPECTRUM_CPP_WINDOW_H
#define SPECTRUM_CPP_WINDOW_H

#include "Common/Common.h"
#include "Platform/Messages.h"

namespace Spectrum::Platform {

    class Window {
    public:
        enum Kind { Main, Overlay, UI, Count };

        explicit Window(HINSTANCE hInst) : m_hInst(hInst) {}

        ~Window() noexcept {
            if (m_hwnd)
                DestroyWindow(m_hwnd);
            if (m_registered)
                UnregisterClassW(m_className, m_hInst);
        }

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        // -=-=-=-=-=-=-=-= Lifecycle =-=-=-=-=-=-=-=

        [[nodiscard]] bool Initialize(
            const std::wstring& title, int w, int h,
            Kind kind, MessageHandlerBase* handler)
        {
            m_kind = kind;
            m_w = w;
            m_h = h;

            DWORD style = WS_OVERLAPPEDWINDOW;
            DWORD exStyle = WS_EX_APPWINDOW;

            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = &WndProc;
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hIcon = LoadIconW(m_hInst, MAKEINTRESOURCEW(101));
            wc.hIconSm = wc.hIcon;

            switch (kind) {
            case Main:
                m_className = L"SpectrumMainClass";
                wc.hbrBackground =
                    reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
                break;
            case Overlay:
                m_className = L"SpectrumOverlayClass";
                style = WS_POPUP;
                exStyle = WS_EX_LAYERED | WS_EX_TOPMOST |
                    WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
                break;
            case UI:
                m_className = L"SpectrumUIClass";
                style = WS_POPUP | WS_CLIPCHILDREN;
                exStyle = WS_EX_TOOLWINDOW;
                wc.style |= CS_OWNDC;
                break;
            default:
                return false;
            }

            wc.lpszClassName = m_className;
            if (!RegisterClassExW(&wc))
                return false;
            m_registered = true;

            if (kind == Main) {
                RECT rc{ 0, 0, w, h };
                AdjustWindowRectEx(&rc, style, FALSE, exStyle);
                w = rc.right - rc.left;
                h = rc.bottom - rc.top;
            }

            m_hwnd = CreateWindowExW(
                exStyle, m_className, title.c_str(), style,
                CW_USEDEFAULT, CW_USEDEFAULT, w, h,
                nullptr, nullptr, m_hInst, handler);
            if (!m_hwnd)
                return false;

            m_running = true;
            return true;
        }

        void Show(int cmd = SW_SHOW) {
            ::ShowWindow(m_hwnd, cmd);

            RECT wa{};
            SystemParametersInfo(SPI_GETWORKAREA, 0, &wa, 0);

            switch (m_kind) {
            case Main:
                SetWindowPos(
                    m_hwnd, nullptr,
                    wa.left + (wa.right - wa.left - m_w) / 2,
                    wa.top + (wa.bottom - wa.top - m_h) / 2,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
                break;
            case UI:
                if (!m_first)
                    break;
                m_first = false;
                SetWindowPos(
                    m_hwnd, HWND_TOP,
                    wa.right - m_w - 20, 50,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
                SetForegroundWindow(m_hwnd);
                break;
            default:
                break;
            }

            UpdateWindow(m_hwnd);
        }

        void Hide() const { ::ShowWindow(m_hwnd, SW_HIDE); }

        void SetRunning(bool v) noexcept { m_running = v; }

        [[nodiscard]] bool IsRunning() const noexcept { return m_running; }
        [[nodiscard]] HWND GetHwnd() const noexcept { return m_hwnd; }
        [[nodiscard]] int GetWidth() const noexcept { return m_w; }
        [[nodiscard]] int GetHeight() const noexcept { return m_h; }

    private:
        static LRESULT CALLBACK WndProc(
            HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            if (msg == WM_NCCREATE) {
                auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
                SetWindowLongPtr(
                    hwnd, GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                return DefWindowProc(hwnd, msg, wp, lp);
            }

            auto* h = reinterpret_cast<MessageHandlerBase*>(
                GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (!h)
                return DefWindowProc(hwnd, msg, wp, lp);
            return h->HandleWindowMessage(hwnd, msg, wp, lp);
        }

        // -=-=-=-=-=-=-=-= State =-=-=-=-=-=-=-=

        HINSTANCE      m_hInst;
        HWND           m_hwnd = nullptr;
        const wchar_t* m_className = nullptr;
        Kind           m_kind = Main;
        bool           m_registered = false;
        bool           m_running = false;
        bool           m_first = true;
        int            m_w = 0;
        int            m_h = 0;
    };

} // namespace Spectrum::Platform

#endif