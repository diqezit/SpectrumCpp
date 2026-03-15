#include "Platform/WindowManager.h"

#include "App/ControllerCore.h"
#include "Audio/AudioManager.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "Platform/MainWindow.h"
#include "Platform/UIWindow.h"
#include "UI/UIManager.h"

#include <stdexcept>

namespace Spectrum::Platform {

    using namespace Helpers::Window;

    namespace {
        constexpr int            kMainW = 800;
        constexpr int            kMainH = 600;
        constexpr const wchar_t* kMainTitle = L"Spectrum Visualizer";

        constexpr int            kOverlayH = 300;
        constexpr const wchar_t* kOverlayTitle = L"Spectrum Overlay";

        constexpr int            kUIW = 340;
        constexpr int            kUIH = 480;
        constexpr const wchar_t* kUITitle = L"Spectrum Control Panel";

        constexpr int            kWarmupFrames = 2;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    WindowManager::WindowManager(
        HINSTANCE hInst,
        ControllerCore* ctrl,
        EventBus* bus)
        : m_hInstance(hInst)
        , m_controller(ctrl)
    {
        if (!ctrl)
            throw std::invalid_argument(
                "WindowManager: null controller");

        m_uiManager = std::make_unique<UIManager>(ctrl, this);

        m_msgHandler = std::make_unique<MessageHandler>(
            ctrl, this, bus);

        m_uiMsgHandler = std::make_unique<UIMessageHandler>(
            ctrl, this, m_uiManager.get(), bus);
    }

    WindowManager::~WindowManager() noexcept {
        HideUIWindow();

        if (auto* w = m_isOverlay
            ? m_overlayWnd.get()
            : m_mainWnd.get())
            HideWindow(w->GetHwnd());

        m_uiWnd.reset();
        m_overlayWnd.reset();
        m_mainWnd.reset();
    }

    bool WindowManager::Initialize() {
        if (!InitializeWindows()
            || !InitializeGraphics()
            || !InitializeUI())
            return false;

        if (m_mainWnd) {
            CenterWindow(m_mainWnd->GetHwnd());
            m_mainWnd->Show();
        }

        ShowUIWindow();

        // ImGui warmup
        if (m_uiManager && m_ui.engine) {
            for (int i = 0; i < kWarmupFrames; ++i) {
                m_uiManager->BeginFrame();
                m_uiManager->Render();
                m_uiManager->EndFrame();
            }
        }

        ForceUIRender();
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window creation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::InitializeWindows() {
        m_mainWnd = CreateMainWnd(
            kMainTitle, kMainW, kMainH, false);

        m_overlayWnd = CreateMainWnd(
            kOverlayTitle,
            GetSystemMetrics(SM_CXSCREEN),
            kOverlayH, true);

        m_uiWnd = CreateUIWnd();

        return m_mainWnd && m_overlayWnd && m_uiWnd;
    }

    bool WindowManager::InitializeGraphics() {
        return m_mainWnd
            && RecreateEngine(
                m_viz, m_mainWnd->GetHwnd(), false, true)
            && m_uiWnd
            && RecreateEngine(
                m_ui, m_uiWnd->GetHwnd(), false, false);
    }

    bool WindowManager::InitializeUI() {
        return m_uiManager && m_uiManager->Initialize();
    }

    std::unique_ptr<MainWindow> WindowManager::CreateMainWnd(
        const wchar_t* title,
        int w, int h, bool overlay) const
    {
        auto wnd = std::make_unique<MainWindow>(m_hInstance);
        return wnd->Initialize(
            title, w, h, overlay, m_msgHandler.get())
            ? std::move(wnd) : nullptr;
    }

    std::unique_ptr<UIWindow> WindowManager::CreateUIWnd() const {
        auto wnd = std::make_unique<UIWindow>(m_hInstance);
        return wnd->Initialize(
            kUITitle, kUIW, kUIH, m_uiMsgHandler.get())
            ? std::move(wnd) : nullptr;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Engine management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::RecreateEngine(
        EngineSlot& slot, HWND hwnd,
        bool overlay, bool d2dOnly)
    {
        if (!IsWindowValid(hwnd))
            return false;

        slot.engine = std::make_unique<RenderEngine>(
            hwnd, overlay, d2dOnly);

        return slot.engine && slot.engine->Initialize();
    }

    void WindowManager::OnSlotResize(
        EngineSlot& slot, int w, int h,
        const std::function<bool(int, int)>& handler)
    {
        if (w == slot.lastW && h == slot.lastH)
            return;

        slot.lastW = w;
        slot.lastH = h;

        if (slot.engine)
            slot.engine->Resize(w, h);

        if (!slot.resizing)
            handler(w, h);
    }

    void WindowManager::OnSlotResizeEnd(
        EngineSlot& slot, HWND hwnd,
        const std::function<bool(int, int)>& handler)
    {
        slot.resizing = false;

        if (auto rc = GetClientRect(hwnd))
            handler(rc->Width(), rc->Height());
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resize — visualization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::HandleVisualizationResize(
        int w, int h, bool recreate)
    {
        if (!IsValidSize(w, h))
            return false;

        if (recreate && !RecreateEngine(
            m_viz, GetCurrentHwnd(), m_isOverlay, true))
            return false;

        if (m_viz.engine)
            m_viz.engine->Resize(w, h);

        if (m_controller)
            m_controller->OnResize(w, h);

        return true;
    }

    void WindowManager::OnResizeStart() {
        m_viz.resizing = true;
    }

    void WindowManager::OnResizeEnd(HWND h) {
        OnSlotResizeEnd(m_viz, h,
            [this](int w, int ht) {
                return HandleVisualizationResize(w, ht);
            });
    }

    void WindowManager::OnResize(HWND, int w, int h) {
        OnSlotResize(m_viz, w, h,
            [this](int a, int b) {
                return HandleVisualizationResize(a, b);
            });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resize — UI
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::HandleUIResize(
        int w, int h, bool recreate)
    {
        if (!IsValidSize(w, h))
            return false;

        if (recreate) {
            if (!m_uiWnd || !RecreateEngine(
                m_ui, m_uiWnd->GetHwnd(), false, false))
                return false;

            if (m_uiManager) {
                m_uiManager->Shutdown();
                if (!m_uiManager->Initialize())
                    return false;
            }
        }

        if (m_ui.engine)
            m_ui.engine->Resize(w, h);

        if (m_controller)
            m_controller->OnUIResize(w, h);

        return true;
    }

    void WindowManager::OnUIResizeStart() {
        m_ui.resizing = true;
    }

    void WindowManager::OnUIResizeEnd(HWND h) {
        OnSlotResizeEnd(m_ui, h,
            [this](int w, int ht) {
                return HandleUIResize(w, ht);
            });
    }

    void WindowManager::OnUIResize(HWND, int w, int h) {
        OnSlotResize(m_ui, w, h,
            [this](int a, int b) {
                return HandleUIResize(a, b);
            });
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Overlay
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ToggleOverlay() {
        m_isOverlay = !m_isOverlay;

        LOG_INFO("WindowManager: "
            << (m_isOverlay ? "OVERLAY" : "NORMAL")
            << " mode");

        SwitchActiveWindow(
            m_isOverlay ? m_mainWnd.get() : m_overlayWnd.get(),
            m_isOverlay ? m_overlayWnd.get() : m_mainWnd.get());

        NotifyRendererOfModeChange();
    }

    void WindowManager::SwitchActiveWindow(
        MainWindow* hide, MainWindow* show)
    {
        if (!hide || !show) return;

        HideWindow(hide->GetHwnd());

        if (auto rc = GetClientRect(show->GetHwnd()))
            HandleVisualizationResize(
                rc->Width(), rc->Height(), true);

        if (m_isOverlay)
            PositionOverlayWindow();

        ShowWindowState(show->GetHwnd());
    }

    void WindowManager::PositionOverlayWindow() const {
        if (m_overlayWnd)
            PositionAtBottom(
                m_overlayWnd->GetHwnd(),
                m_overlayWnd->GetHeight());
    }

    void WindowManager::NotifyRendererOfModeChange() const {
        if (!m_controller) return;

        if (auto* rm = m_controller->GetRendererManager())
            if (auto* r = rm->GetCurrentRenderer())
                r->SetOverlayMode(m_isOverlay);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // UI window
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void WindowManager::ShowUIWindow() const {
        if (m_uiWnd) m_uiWnd->Show();
    }

    void WindowManager::HideUIWindow() const {
        if (m_uiWnd) m_uiWnd->Hide();
    }

    void WindowManager::ForceUIRender() {
        if (!m_uiManager || !m_ui.engine || !IsUIWindowVisible())
            return;

        m_ui.engine->ClearD3D11(kUIBackgroundColor);
        m_uiManager->BeginFrame();
        m_uiManager->Render();
        m_uiManager->EndFrame();
        m_ui.engine->Present();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Queries
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool WindowManager::IsRunning() const {
        return m_mainWnd && m_mainWnd->IsRunning();
    }

    bool WindowManager::IsActive() const {
        return IsRunning()
            && IsActiveAndVisible(GetCurrentHwnd());
    }

    bool WindowManager::IsUIWindowVisible() const {
        return m_uiWnd
            && ::IsWindowVisible(m_uiWnd->GetHwnd());
    }

    HWND WindowManager::GetCurrentHwnd() const {
        return m_isOverlay
            ? (m_overlayWnd ? m_overlayWnd->GetHwnd() : nullptr)
            : (m_mainWnd ? m_mainWnd->GetHwnd() : nullptr);
    }

    HWND WindowManager::GetUIHwnd() const noexcept {
        return m_uiWnd ? m_uiWnd->GetHwnd() : nullptr;
    }

} // namespace Spectrum::Platform