#ifndef SPECTRUM_CPP_UIWINDOW_H
#define SPECTRUM_CPP_UIWINDOW_H

#include "Common/Common.h"
#include "WindowBase.h"
#include <string>

namespace Spectrum::Platform {

    class UIMessageHandler;

    class UIWindow final : public WindowBase {
    public:
        explicit UIWindow(HINSTANCE hInstance);
        ~UIWindow() noexcept override = default;

        [[nodiscard]] bool Initialize(
            const std::wstring& title,
            int width,
            int height,
            UIMessageHandler* messageHandler
        );

        void Show(int cmdShow = SW_SHOW);

    protected:
        [[nodiscard]] const char* GetWindowTypeName() const noexcept override;
        void CustomizeWindowClass(WNDCLASSEXW& wcex) override;
        [[nodiscard]] DWORD GetStyleFlags() const override;
        [[nodiscard]] DWORD GetExStyleFlags() const override;
        [[nodiscard]] WNDPROC GetWindowProc() const override;

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        mutable bool m_firstShow = true;
    };

}

#endif