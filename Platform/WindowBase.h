#ifndef SPECTRUM_CPP_WINDOW_BASE_H
#define SPECTRUM_CPP_WINDOW_BASE_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Abstract base for Win32 window creation with class registration.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <string>

namespace Spectrum::Platform {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // WndProc template dispatcher
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Handler>
    LRESULT CALLBACK CommonWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
            SetWindowLongPtr(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return DefWindowProc(hwnd, msg, wp, lp);
        }
        auto* h = reinterpret_cast<Handler*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        return h ? h->HandleWindowMessage(hwnd, msg, wp, lp)
            : DefWindowProc(hwnd, msg, wp, lp);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Size limits
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    namespace WindowLimits {
        constexpr int MainMinW = 320, MainMaxW = 7680;
        constexpr int MainMinH = 240, MainMaxH = 4320;
        constexpr int UIMinW = 200, UIMaxW = 2560;
        constexpr int UIMinH = 200, UIMaxH = 1440;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Base class
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    class WindowBase {
    public:
        virtual ~WindowBase() noexcept {
            if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
            if (m_registered && !m_className.empty())
                UnregisterClassW(m_className.c_str(), m_hInst);
        }

        WindowBase(const WindowBase&) = delete;
        WindowBase& operator=(const WindowBase&) = delete;

        void Hide() const { Helpers::Window::HideWindow(m_hwnd); }

        [[nodiscard]] HWND GetHwnd()   const noexcept { return m_hwnd; }
        [[nodiscard]] int  GetWidth()  const noexcept { return m_w; }
        [[nodiscard]] int  GetHeight() const noexcept { return m_h; }

    protected:
        explicit WindowBase(HINSTANCE hInst) : m_hInst(hInst) {}

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Hooks
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        virtual void    CustomizeClass(WNDCLASSEXW&) = 0;
        virtual DWORD   StyleFlags()   const = 0;
        virtual DWORD   ExStyleFlags() const = 0;
        virtual WNDPROC WndProcFunc()  const = 0;
        virtual void    OnCreated(HWND) {}
        virtual bool    AdjustRect()   const { return true; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        [[nodiscard]] bool Init(
            const std::wstring& title,
            int w, int h,
            int minW, int maxW, int minH, int maxH,
            void* handler)
        {
            if (!m_hInst || !handler) return false;
            if (w < minW || w > maxW || h < minH || h > maxH) return false;

            m_w = w;
            m_h = h;

            if (!RegisterWndClass()) return false;

            m_hwnd = CreateWnd(title, w, h, handler);
            if (!m_hwnd) return false;

            OnCreated(m_hwnd);
            return true;
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Show with positioning
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void ShowAt(int cmd, bool center, bool topRight) const {
            if (!m_hwnd) return;
            ::ShowWindow(m_hwnd, cmd);

            RECT wa{};
            SystemParametersInfo(SPI_GETWORKAREA, 0, &wa, 0);

            if (center) {
                SetWindowPos(m_hwnd, nullptr,
                    wa.left + (wa.right - wa.left - m_w) / 2,
                    wa.top + (wa.bottom - wa.top - m_h) / 2,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            else if (topRight) {
                SetWindowPos(m_hwnd, HWND_TOP,
                    wa.right - m_w - 20, 50,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER);
                SetForegroundWindow(m_hwnd);
            }

            UpdateWindow(m_hwnd);
        }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Members
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        HINSTANCE    m_hInst;
        HWND         m_hwnd = nullptr;
        std::wstring m_className;
        bool         m_registered = false;
        int          m_w = 0, m_h = 0;

    private:
        bool RegisterWndClass() {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WndProcFunc();
            wc.hInstance = m_hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.lpszClassName = m_className.c_str();
            wc.hIcon = LoadIconW(m_hInst, MAKEINTRESOURCEW(101));
            wc.hIconSm = wc.hIcon;

            CustomizeClass(wc);

            if (!wc.hIcon)   wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            if (!wc.hIconSm) wc.hIconSm = wc.hIcon;

            m_registered = (RegisterClassExW(&wc) != 0);
            return m_registered;
        }

        HWND CreateWnd(const std::wstring& title, int w, int h, void* handler) {
            const DWORD s = StyleFlags();
            const DWORD ex = ExStyleFlags();

            if (AdjustRect()) {
                RECT rc{ 0, 0, w, h };
                AdjustWindowRectEx(&rc, s, FALSE, ex);
                w = rc.right - rc.left;
                h = rc.bottom - rc.top;
            }

            return CreateWindowExW(
                ex, m_className.c_str(), title.c_str(), s,
                CW_USEDEFAULT, CW_USEDEFAULT, w, h,
                nullptr, nullptr, m_hInst, handler);
        }
    };

} // namespace Spectrum::Platform

#endif