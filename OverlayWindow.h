// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// OverlayWindow.h: Manages overlay window lifecycle and graphics rebind.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_OVERLAY_WINDOW_H
#define SPECTRUM_CPP_OVERLAY_WINDOW_H

#include "Common.h"
#include "MainWindow.h"
#include "GraphicsContext.h"
#include "ColorPicker.h"

namespace Spectrum {

    class OverlayWindow {
    public:
        explicit OverlayWindow(HINSTANCE hInstance);
        ~OverlayWindow();

        bool Enter(MainWindow& mainWindow,
            std::unique_ptr<GraphicsContext>& graphics,
            ColorPicker* colorPicker,
            const std::function<void(int)>& onKey,
            const std::function<void(int, int)>& onResize,
            const std::function<void()>& onCloseRequest = {});

        void RequestExit();

        bool FinalizeExit(MainWindow& mainWindow,
            std::unique_ptr<GraphicsContext>& graphics,
            ColorPicker* colorPicker);

        void ProcessMessages();

        bool IsActive() const { return m_window != nullptr; }
        bool IsRunning() const;
        bool ExitRequested() const { return m_exitRequested; }

        MainWindow* GetWindow() const { return m_window.get(); }

    private:
        bool CreateOverlayWindow();
        void SetupCallbacks(const std::function<void(int)>& onKey,
            const std::function<void(int, int)>& onResize,
            const std::function<void()>& onCloseRequest);

        bool RebindGraphics(std::unique_ptr<GraphicsContext>& graphics,
            HWND newHwnd);

        void GetScreenDimensions(int& width, int& height) const;

    private:
        HINSTANCE m_hInstance;
        std::unique_ptr<MainWindow> m_window;
        std::atomic<bool> m_exitRequested;

        struct {
            int width;
            int height;
        } m_originalWindowState;
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_OVERLAY_WINDOW_H