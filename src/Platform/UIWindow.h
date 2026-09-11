#ifndef SPECTRUM_CPP_UIWINDOW_H
#define SPECTRUM_CPP_UIWINDOW_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Borderless popup window for ImGui settings panel.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Platform/WindowBase.h"
#include "Platform/Messages.h"
#include <utility>

namespace Spectrum::Platform {

    class UIWindow final : public WindowBase {
    public:
        explicit UIWindow(HINSTANCE h) : WindowBase(h) {
            m_className = L"SpectrumUIClass";
        }

        [[nodiscard]] bool Initialize(
            const std::wstring& title, int w, int h, UIMessageHandler* handler)
        {
            return Init(title, w, h, handler);
        }

        void Show(int cmd = SW_SHOW) {
            ShowAt(cmd, false, std::exchange(m_first, false));
        }

    protected:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Class
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        void CustomizeClass(WNDCLASSEXW& wc) override {
            wc.style |= CS_OWNDC;
            wc.hbrBackground = nullptr;
        }

        WNDPROC WndProcFunc() const override { return &WndProc; }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Style
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        DWORD StyleFlags()   const override { return WS_POPUP | WS_CLIPCHILDREN; }
        DWORD ExStyleFlags() const override { return WS_EX_TOOLWINDOW; }
        bool  AdjustRect()   const override { return false; }

    private:
        static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
            return CommonWndProc<UIMessageHandler>(h, m, w, l);
        }

        bool m_first = true;
    };

} // namespace Spectrum::Platform

#endif