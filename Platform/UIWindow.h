#ifndef SPECTRUM_CPP_UIWINDOW_H
#define SPECTRUM_CPP_UIWINDOW_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Borderless popup window for ImGui settings panel.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Platform/WindowBase.h"
#include "Platform/UIMessageHandler.h"

namespace Spectrum::Platform {

    class UIWindow final : public WindowBase {
    public:
        explicit UIWindow(HINSTANCE h)
            : WindowBase(h) {
            m_className = L"SpectrumUIClass";
        }

        [[nodiscard]] bool Initialize(
            const std::wstring& title, int w, int h,
            UIMessageHandler* handler)
        {
            return Init(title, w, h,
                WindowLimits::UIMinW, WindowLimits::UIMaxW,
                WindowLimits::UIMinH, WindowLimits::UIMaxH,
                handler);
        }

        void Show(int cmd = SW_SHOW) {
            ShowAt(cmd, false, m_first);
            m_first = false;
        }

    protected:
        void CustomizeClass(WNDCLASSEXW& wc) override {
            wc.style |= CS_OWNDC;
            wc.hbrBackground = nullptr;
        }

        DWORD StyleFlags()   const override { return WS_POPUP | WS_CLIPCHILDREN; }
        DWORD ExStyleFlags() const override { return WS_EX_TOOLWINDOW; }

        WNDPROC WndProcFunc() const override {
            return [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
                return CommonWndProc<UIMessageHandler>(h, m, w, l);
                };
        }

        bool AdjustRect() const override { return false; }

    private:
        bool m_first = true;
    };

} // namespace Spectrum::Platform

#endif