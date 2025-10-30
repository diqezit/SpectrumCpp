#ifndef SPECTRUM_CPP_WINDOW_BASE_H
#define SPECTRUM_CPP_WINDOW_BASE_H

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <string>

namespace Spectrum::Platform {

    class WindowBase {
    public:
        virtual ~WindowBase() noexcept {
            LOG_INFO("WindowBase: Starting cleanup.");

            if (m_hwnd) {
                DestroyWindow(m_hwnd);
                m_hwnd = nullptr;
            }

            if (m_classRegistered && !m_className.empty()) {
                UnregisterClassW(m_className.c_str(), m_hInstance);
                m_classRegistered = false;
            }

            LOG_INFO("WindowBase: Cleanup complete.");
        }

        WindowBase(const WindowBase&) = delete;
        WindowBase& operator=(const WindowBase&) = delete;
        WindowBase(WindowBase&&) = delete;
        WindowBase& operator=(WindowBase&&) = delete;

        void Hide() const {
            Helpers::Window::HideWindow(m_hwnd);
        }

        [[nodiscard]] HWND GetHwnd() const noexcept { return m_hwnd; }
        [[nodiscard]] int GetWidth() const noexcept { return m_width; }
        [[nodiscard]] int GetHeight() const noexcept { return m_height; }

    protected:
        struct WindowLimits {
            static constexpr int MainMinWidth = 320;
            static constexpr int MainMinHeight = 240;
            static constexpr int MainMaxWidth = 7680;
            static constexpr int MainMaxHeight = 4320;

            static constexpr int UIMinWidth = 200;
            static constexpr int UIMinHeight = 200;
            static constexpr int UIMaxWidth = 2560;
            static constexpr int UIMaxHeight = 1440;
        };

        explicit WindowBase(HINSTANCE hInstance)
            : m_hInstance(hInstance)
            , m_hwnd(nullptr)
            , m_className()
            , m_classRegistered(false)
            , m_width(0)
            , m_height(0) {
        }

        [[nodiscard]] virtual const char* GetWindowTypeName() const noexcept { return "WindowBase"; }
        virtual void CustomizeWindowClass(WNDCLASSEXW& wcex) = 0;
        [[nodiscard]] virtual DWORD GetStyleFlags() const = 0;
        [[nodiscard]] virtual DWORD GetExStyleFlags() const = 0;
        virtual void OnWindowCreated(HWND hwnd) { (void)hwnd; }
        [[nodiscard]] virtual bool CanClose() const { return true; }
        [[nodiscard]] virtual WNDPROC GetWindowProc() const = 0;
        [[nodiscard]] virtual bool ShouldAdjustWindowRect() const { return true; }

        [[nodiscard]] bool InitializeBase(
            const std::wstring& title,
            int width,
            int height,
            int minWidth,
            int maxWidth,
            int minHeight,
            int maxHeight,
            void* messageHandler
        ) {
            char titleBuffer[256] = { 0 };
            WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, titleBuffer,
                sizeof(titleBuffer) - 1, nullptr, nullptr);
            LOG_INFO(GetWindowTypeName() << ": Initializing window '"
                << titleBuffer << "' (" << width << "x" << height << ")");

            if (!m_hInstance || !messageHandler) {
                LOG_ERROR(GetWindowTypeName()
                    << ": Invalid parameters (hInstance or messageHandler is null).");
                return false;
            }

            if (width < minWidth || width > maxWidth ||
                height < minHeight || height > maxHeight) {
                LOG_ERROR(GetWindowTypeName() << ": Invalid dimensions: "
                    << width << "x" << height);
                return false;
            }

            m_width = width;
            m_height = height;

            if (!RegisterWindowClassInternal()) {
                LOG_ERROR(GetWindowTypeName() << ": Failed to register window class.");
                return false;
            }

            m_hwnd = CreateWindowInternal(title, width, height, messageHandler);
            if (!m_hwnd) {
                LOG_ERROR(GetWindowTypeName() << ": Failed to create window handle.");
                return false;
            }

            OnWindowCreated(m_hwnd);

            LOG_INFO(GetWindowTypeName() << ": Window created successfully (HWND: "
                << m_hwnd << ")");
            return true;
        }

        void ShowWindowWithPosition(int cmdShow, bool centerWindow, bool topRight) const {
            VALIDATE_PTR_OR_RETURN(m_hwnd, GetWindowTypeName());

            ShowWindow(m_hwnd, cmdShow);

            if (centerWindow) {
                RECT workArea{};
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
                const int x = workArea.left + (workArea.right - workArea.left - m_width) / 2;
                const int y = workArea.top + (workArea.bottom - workArea.top - m_height) / 2;
                SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            else if (topRight) {
                RECT workArea{};
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
                const int x = workArea.right - m_width - 20;
                const int y = 50;
                SetWindowPos(m_hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                SetForegroundWindow(m_hwnd);
                SetFocus(m_hwnd);
            }

            UpdateWindow(m_hwnd);
        }

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        std::wstring m_className;
        bool m_classRegistered;
        int m_width;
        int m_height;

    private:
        bool RegisterWindowClassInternal() {
            WNDCLASSEXW wcex{};
            wcex.cbSize = sizeof(WNDCLASSEXW);
            wcex.style = CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc = GetWindowProc();
            wcex.hInstance = m_hInstance;
            wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wcex.lpszClassName = m_className.c_str();

            wcex.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(101));
            wcex.hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCEW(101));

            CustomizeWindowClass(wcex);

            if (!wcex.hIcon) {
                wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            }
            if (!wcex.hIconSm) {
                wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
            }

            if (RegisterClassExW(&wcex) == 0) {
                return false;
            }

            m_classRegistered = true;
            LOG_INFO(GetWindowTypeName() << ": Window class registered successfully.");
            return true;
        }

        [[nodiscard]] HWND CreateWindowInternal(
            const std::wstring& title,
            int width,
            int height,
            void* messageHandler
        ) {
            const DWORD style = GetStyleFlags();
            const DWORD exStyle = GetExStyleFlags();

            int x = CW_USEDEFAULT;
            int y = CW_USEDEFAULT;
            int w = width;
            int h = height;

            if (ShouldAdjustWindowRect()) {
                RECT rect{ 0, 0, width, height };
                AdjustWindowRectEx(&rect, style, FALSE, exStyle);
                w = rect.right - rect.left;
                h = rect.bottom - rect.top;
            }

            return CreateWindowExW(
                exStyle,
                m_className.c_str(),
                title.c_str(),
                style,
                x, y, w, h,
                nullptr,
                nullptr,
                m_hInstance,
                messageHandler
            );
        }
    };

    template <typename HandlerType>
    inline LRESULT CALLBACK CommonWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_NCCREATE) {
            auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        auto* handler = reinterpret_cast<HandlerType*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        return handler ? handler->HandleWindowMessage(hwnd, msg, wParam, lParam)
            : DefWindowProc(hwnd, msg, wParam, lParam);
    }

}

#endif