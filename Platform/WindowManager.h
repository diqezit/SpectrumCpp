#ifndef SPECTRUM_CPP_WINDOW_MANAGER_H
#define SPECTRUM_CPP_WINDOW_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Owns main / overlay / UI windows and their GPU surfaces.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/API/GraphicsSurface.h"
#include "Graphics/API/D3D11Backend.h"
#include "Platform/MainWindow.h"
#include "Platform/UIWindow.h"
#include "UI/UI.h"
#include "Platform/Messages.h"

#include <functional>
#include <memory>

namespace Spectrum {

    class Core;
    class EventBus;

    namespace Platform {

        using namespace Helpers::Window;

        class WindowManager final {
        public:
            using ResizeFn  = std::function<void(int, int)>;
            using OverlayFn = std::function<void(bool)>;

            WindowManager(
                HINSTANCE hInst, Core* core, EventBus* bus,
                ResizeFn onVizResize, ResizeFn onUiResize, OverlayFn onOverlay)
                : m_hInstance(hInst)
                , m_onVizResize(std::move(onVizResize))
                , m_onUiResize(std::move(onUiResize))
                , m_onOverlay(std::move(onOverlay))
                , m_uiManager(std::make_unique<UIManager>(
                    [this] { HideUIWindow(); },
                    [this] { ToggleOverlay(); }))
                , m_msgHandler(std::make_unique<MessageHandler>(core, this, bus))
                , m_uiMsgHandler(std::make_unique<UIMessageHandler>(core, this, m_uiManager.get(), bus)) {
            }

            ~WindowManager() noexcept {
                HideUIWindow();
                HideWindow(GetCurrentHwnd());
                m_uiWnd.reset();
                m_overlayWnd.reset();
                m_mainWnd.reset();
            }

            WindowManager(const WindowManager&) = delete;
            WindowManager& operator=(const WindowManager&) = delete;

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Lifecycle
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            bool Initialize() {
                m_mainWnd    = MakeWnd<MainWindow>(kMainTitle, kMainW, kMainH, false, m_msgHandler.get());
                m_overlayWnd = MakeWnd<MainWindow>(kOverlayTitle, GetSystemMetrics(SM_CXSCREEN), kOverlayH, true, m_msgHandler.get());
                m_uiWnd      = MakeWnd<UIWindow>(kUITitle, kUIW, kUIH, m_uiMsgHandler.get());

                static_cast<void>(Recreate(m_viz, m_mainWnd->GetHwnd(), false));
                static_cast<void>(Recreate(m_ui, m_uiWnd->GetHwnd()));
                InitUI();

                CenterWindow(m_mainWnd->GetHwnd());
                m_mainWnd->Show();
                ShowUIWindow();

                for (int i = 0; i < kWarmupFrames; ++i) {
                    m_uiManager->BeginFrame();
                    m_uiManager->EndFrame();
                }
                return true;
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Visualization resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            bool HandleVisualizationResize(int w, int h, bool recreate = false) {
                if (recreate) static_cast<void>(Recreate(m_viz, GetCurrentHwnd(), m_isOverlay));
                m_viz.Resize(w, h);
                m_onVizResize(w, h);
                return true;
            }

            void OnResizeStart() { m_viz.resizing = true; }
            void OnResizeEnd(HWND hwnd) { FinishResize(m_viz, hwnd, [this](int w, int h) { HandleVisualizationResize(w, h); }); }
            void OnResize(HWND, int w, int h) { if (m_viz.LiveResize(w, h)) static_cast<void>(HandleVisualizationResize(w, h)); }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // UI resize
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            bool HandleUIResize(int w, int h, bool recreate = false) {
                if (recreate) {
                    static_cast<void>(Recreate(m_ui, m_uiWnd->GetHwnd()));
                    m_uiManager->Shutdown();
                    InitUI();
                }
                m_ui.Resize(w, h);
                m_onUiResize(w, h);
                return true;
            }

            void OnUIResizeStart() { m_ui.resizing = true; }
            void OnUIResizeEnd(HWND hwnd) { FinishResize(m_ui, hwnd, [this](int w, int h) { HandleUIResize(w, h); }); }
            void OnUIResize(HWND, int w, int h) { if (m_ui.LiveResize(w, h)) static_cast<void>(HandleUIResize(w, h)); }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Overlay / UI visibility
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            void ToggleOverlay() {
                m_isOverlay = !m_isOverlay;
                SwitchActiveWindow(InactiveWnd(), ActiveWnd());
                m_onOverlay(m_isOverlay);
            }

            void ShowUIWindow() const { m_uiWnd->Show(); }
            void HideUIWindow() const { m_uiWnd->Hide(); }

            void ForceUIRender() {
                if (!IsUIWindowVisible()) return;
                m_ui.obj->Clear(Color::FromRGB(30, 30, 40));
                m_uiManager->BeginFrame();
                m_uiManager->Render();
                m_uiManager->EndFrame();
                static_cast<void>(m_ui.obj->Present());
            }

            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
            // Queries
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

            [[nodiscard]] bool IsRunning()         const { return m_mainWnd->IsRunning(); }
            [[nodiscard]] bool IsActive()          const { return IsRunning() && IsActiveAndVisible(GetCurrentHwnd()); }
            [[nodiscard]] bool IsUIWindowVisible() const { return ::IsWindowVisible(m_uiWnd->GetHwnd()); }
            [[nodiscard]] bool IsOverlayMode()     const noexcept { return m_isOverlay; }

            [[nodiscard]] HWND GetCurrentHwnd() const { return ActiveWnd()->GetHwnd(); }
            [[nodiscard]] HWND GetUIHwnd()      const noexcept { return m_uiWnd->GetHwnd(); }

            [[nodiscard]] GraphicsSurface* GetVisualizationSurface() const noexcept { return m_viz.obj.get(); }
            [[nodiscard]] D3D11Backend*    GetUIBackend()            const noexcept { return m_ui.obj.get(); }
            [[nodiscard]] UIManager*       GetUIManager()            const noexcept { return m_uiManager.get(); }
            [[nodiscard]] MainWindow*      GetMainWindow()           const noexcept { return m_mainWnd.get(); }
            [[nodiscard]] MessageHandler*  GetMessageHandler()       const noexcept { return m_msgHandler.get(); }

        private:
            static constexpr int            kMainW        = 800;
            static constexpr int            kMainH        = 600;
            static constexpr const wchar_t* kMainTitle    = L"Spectrum Visualizer";
            static constexpr int            kOverlayH     = 300;
            static constexpr const wchar_t* kOverlayTitle = L"Spectrum Overlay";
            static constexpr int            kUIW          = 340;
            static constexpr int            kUIH          = 480;
            static constexpr const wchar_t* kUITitle      = L"Spectrum Control Panel";
            static constexpr int            kWarmupFrames = 2;

            template<typename T>
            struct Slot {
                std::unique_ptr<T> obj;
                int  lastW    = 0;
                int  lastH    = 0;
                bool resizing = false;

                explicit operator bool() const { return static_cast<bool>(obj); }

                bool SizeChanged(int w, int h) {
                    if (w == lastW && h == lastH) return false;
                    lastW = w;
                    lastH = h;
                    return true;
                }

                void Resize(int w, int h) { obj->Resize(w, h); }

                bool LiveResize(int w, int h) {
                    if (!obj || !SizeChanged(w, h)) return false;
                    Resize(w, h);
                    return !resizing;
                }
            };

            template<typename W, typename... Args>
            std::unique_ptr<W> MakeWnd(Args... args) const {
                auto wnd = std::make_unique<W>(m_hInstance);
                static_cast<void>(wnd->Initialize(args...));
                return wnd;
            }

            template<typename T, typename... Args>
            bool Recreate(Slot<T>& slot, Args... args) {
                slot.obj = std::make_unique<T>();
                return slot.obj->Initialize(args...);
            }

            template<typename T, typename Fn>
            void FinishResize(Slot<T>& slot, HWND hwnd, Fn&& fn) {
                slot.resizing = false;
                if (!slot) return;
                const auto rc = *GetClientRect(hwnd);
                fn(rc.Width(), rc.Height());
            }

            MainWindow* ActiveWnd()   const { return (m_isOverlay ? m_overlayWnd : m_mainWnd).get(); }
            MainWindow* InactiveWnd() const { return (m_isOverlay ? m_mainWnd : m_overlayWnd).get(); }

            void InitUI() {
                static_cast<void>(m_uiManager->Initialize(
                    m_uiWnd->GetHwnd(),
                    m_ui.obj->GetD3D11Device(),
                    m_ui.obj->GetD3D11DeviceContext(),
                    m_ui.obj->GetD3D11RenderTargetView()));
            }

            void SwitchActiveWindow(MainWindow* hide, MainWindow* show) {
                HideWindow(hide->GetHwnd());
                const auto rc = *GetClientRect(show->GetHwnd());
                static_cast<void>(HandleVisualizationResize(rc.Width(), rc.Height(), true));
                if (m_isOverlay) PositionAtBottom(show->GetHwnd(), show->GetHeight());
                ShowWindowState(show->GetHwnd());
            }

            HINSTANCE m_hInstance;
            ResizeFn  m_onVizResize;
            ResizeFn  m_onUiResize;
            OverlayFn m_onOverlay;
            bool      m_isOverlay = false;

            std::unique_ptr<UIManager>        m_uiManager;
            std::unique_ptr<MessageHandler>   m_msgHandler;
            std::unique_ptr<UIMessageHandler> m_uiMsgHandler;
            std::unique_ptr<MainWindow>       m_mainWnd;
            std::unique_ptr<MainWindow>       m_overlayWnd;
            std::unique_ptr<UIWindow>         m_uiWnd;

            Slot<GraphicsSurface> m_viz;
            Slot<D3D11Backend>    m_ui;
        };

    } // namespace Platform
} // namespace Spectrum

#endif