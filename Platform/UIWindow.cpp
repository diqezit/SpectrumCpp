// UIWindow.cpp
#include "UIWindow.h"
#include "UIMessageHandler.h"
#include "Resources/Resource.h"
#include "Graphics/API/GraphicsHelpers.h"

namespace Spectrum::Platform {

    namespace {
        constexpr int kMinWindowWidth = 200;
        constexpr int kMinWindowHeight = 200;
        constexpr int kMaxWindowWidth = 2560;
        constexpr int kMaxWindowHeight = 1440;
    }

    UIWindow::UIWindow(HINSTANCE hInstance) :
        m_hInstance(hInstance),
        m_hwnd(nullptr),
        m_className(L"SpectrumUIWindowClass"),
        m_classRegistered(false),
        m_width(0),
        m_height(0)
    {
    }

    UIWindow::~UIWindow() noexcept
    {
        LOG_INFO("UIWindow: Starting cleanup.");
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        if (m_classRegistered && !m_className.empty())
        {
            UnregisterClassW(m_className.c_str(), m_hInstance);
            m_classRegistered = false;
        }
        LOG_INFO("UIWindow: Cleanup complete.");
    }

    bool UIWindow::Initialize(
        const std::wstring& title,
        int width,
        int height,
        UIMessageHandler* messageHandler
    )
    {
        char titleBuffer[256] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, title.c_str(), -1, titleBuffer, sizeof(titleBuffer) - 1, nullptr, nullptr);
        LOG_INFO("UIWindow: Initializing window '" << titleBuffer << "' (" << width << "x" << height << ")");

        if (!m_hInstance || !messageHandler)
        {
            LOG_ERROR("UIWindow: Invalid parameters (hInstance or messageHandler is null).");
            return false;
        }

        if (width < kMinWindowWidth || width > kMaxWindowWidth || height < kMinWindowHeight || height > kMaxWindowHeight)
        {
            LOG_ERROR("UIWindow: Invalid dimensions: " << width << "x" << height);
            return false;
        }

        m_width = width;
        m_height = height;

        if (!RegisterWindowClass())
        {
            LOG_ERROR("UIWindow: Failed to register window class.");
            return false;
        }

        m_hwnd = CreateWindowHandle(title, width, height, messageHandler);
        if (!m_hwnd)
        {
            LOG_ERROR("UIWindow: Failed to create window handle.");
            return false;
        }

        LOG_INFO("UIWindow: Window created successfully (HWND: " << m_hwnd << ")");
        return true;
    }

    void UIWindow::Show(int cmdShow) const
    {
        VALIDATE_PTR_OR_RETURN(m_hwnd, "UIWindow");

        ShowWindow(m_hwnd, cmdShow);

        RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
        int x = workArea.right - m_width - 20;
        int y = 50;

        SetWindowPos(
            m_hwnd,
            HWND_TOP,
            x, y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER
        );

        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);
        UpdateWindow(m_hwnd);
    }

    void UIWindow::Hide() const
    {
        Helpers::Window::HideWindow(m_hwnd);
    }

    void UIWindow::Close()
    {
        if (m_hwnd)
        {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
    }

    HWND UIWindow::GetHwnd() const noexcept { return m_hwnd; }
    int UIWindow::GetWidth() const noexcept { return m_width; }
    int UIWindow::GetHeight() const noexcept { return m_height; }

    bool UIWindow::RegisterWindowClass()
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = m_hInstance;
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.lpszClassName = m_className.c_str();
        wcex.hbrBackground = nullptr; // No background brush
        wcex.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
        wcex.hIconSm = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));

        if (!wcex.hIcon) { wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION); }
        if (!wcex.hIconSm) { wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION); }

        if (RegisterClassExW(&wcex) == 0)
        {
            return false;
        }

        m_classRegistered = true;
        LOG_INFO("UIWindow: Window class registered successfully.");
        return true;
    }

    HWND UIWindow::CreateWindowHandle(
        const std::wstring& title,
        int width,
        int height,
        UIMessageHandler* messageHandler
    )
    {
        DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
        DWORD exStyle = WS_EX_APPWINDOW;

        RECT rect{ 0, 0, width, height };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;

        HWND hwnd = CreateWindowExW(
            exStyle,
            m_className.c_str(),
            title.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT, w, h,
            nullptr,
            nullptr,
            m_hInstance,
            messageHandler
        );

        return hwnd;
    }

    LRESULT CALLBACK UIWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        auto* handler = reinterpret_cast<UIMessageHandler*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (handler)
        {
            return handler->HandleWindowMessage(hwnd, msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

} // namespace Spectrum::Platform