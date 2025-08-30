// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// OverlayWindow.cpp: Implementation of the overlay window manager.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "OverlayWindow.h"
#include "WindowHelper.h"
#include "Utils.h"

namespace Spectrum {

    OverlayWindow::OverlayWindow(HINSTANCE hInstance)
        : m_hInstance(hInstance)
        , m_exitRequested(false)
        , m_originalWindowState{} {
    }

    OverlayWindow::~OverlayWindow() {
        if (m_window)
            m_window->Close();
    }

    bool OverlayWindow::Enter(MainWindow& mainWindow,
        std::unique_ptr<GraphicsContext>& graphics,
        ColorPicker* colorPicker,
        const std::function<void(int)>& onKey,
        const std::function<void(int, int)>& onResize,
        const std::function<void()>& onCloseRequest) {
        m_exitRequested = false;

        m_originalWindowState.width = mainWindow.GetWidth();
        m_originalWindowState.height = mainWindow.GetHeight();

        mainWindow.Minimize();

        if (!CreateOverlayWindow()) {
            LOG_ERROR("Failed to create overlay window");
            mainWindow.Restore();
            return false;
        }

        SetupCallbacks(onKey, onResize, onCloseRequest);

        if (colorPicker)
            colorPicker->SetVisible(false);

        if (!RebindGraphics(graphics, m_window->GetHwnd())) {
            LOG_ERROR("Failed to rebind graphics to overlay window");
            m_window.reset();
            mainWindow.Restore();
            return false;
        }

        if (colorPicker && graphics)
            colorPicker->RecreateResources(*graphics);

        m_window->Show();

        if (onResize) {
            int width, height;
            GetScreenDimensions(width, height);
            onResize(width, height);
        }

        LOG_INFO("Entered overlay mode");
        return true;
    }

    void OverlayWindow::RequestExit() {
        m_exitRequested = true;
        if (m_window && m_window->IsRunning())
            m_window->Close();
    }

    bool OverlayWindow::FinalizeExit(MainWindow& mainWindow,
        std::unique_ptr<GraphicsContext>& graphics,
        ColorPicker* colorPicker) {
        m_window.reset();

        mainWindow.Restore();

        if (!RebindGraphics(graphics, mainWindow.GetHwnd())) {
            LOG_ERROR("Failed to rebind graphics to main window");
            return false;
        }

        if (colorPicker) {
            colorPicker->SetVisible(true);
            if (graphics)
                colorPicker->RecreateResources(*graphics);
        }

        SetForegroundWindow(mainWindow.GetHwnd());

        LOG_INFO("Exited overlay mode");
        return true;
    }

    void OverlayWindow::ProcessMessages() {
        if (m_window)
            m_window->ProcessMessages();
    }

    bool OverlayWindow::IsRunning() const {
        return m_window && m_window->IsRunning();
    }

    bool OverlayWindow::CreateOverlayWindow() {
        int width, height;
        GetScreenDimensions(width, height);

        m_window = std::make_unique<MainWindow>(m_hInstance);

        if (!m_window->Initialize(true, width, height)) {
            LOG_ERROR("Failed to initialize overlay window");
            m_window.reset();
            return false;
        }

        return true;
    }

    void OverlayWindow::SetupCallbacks(const std::function<void(int)>& onKey,
        const std::function<void(int, int)>& onResize,
        const std::function<void()>& onCloseRequest) {
        if (!m_window) return;

        if (onKey)
            m_window->SetKeyCallback([this, onKey](int key) {
            if (key == VK_ESCAPE || key == 'O' || key == 'o')
                RequestExit();
            else
                onKey(key);
                });

        if (onResize)
            m_window->SetResizeCallback(onResize);

        m_window->SetCloseCallback([this, onCloseRequest]() {
            m_exitRequested = true;
            if (onCloseRequest)
                onCloseRequest();
            });
    }

    bool OverlayWindow::RebindGraphics(std::unique_ptr<GraphicsContext>& graphics,
        HWND newHwnd) {
        graphics.reset();
        graphics = std::make_unique<GraphicsContext>(newHwnd);

        if (!graphics->Initialize()) {
            LOG_ERROR("Failed to initialize graphics for HWND: " << newHwnd);
            graphics.reset();
            return false;
        }

        return true;
    }

    void OverlayWindow::GetScreenDimensions(int& width, int& height) const {
        WindowUtils::GetScreenSize(width, height);
    }

} // namespace Spectrum