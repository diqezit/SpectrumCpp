// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// ControllerCore.cpp: Implementation of the Controller class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "ControllerCore.h"
#include "InputManager.h"

namespace Spectrum {

    ControllerCore::ControllerCore(HINSTANCE hInstance)
        : m_hInstance(hInstance) {
        m_windowManager = std::make_unique<WindowManager>(hInstance);
        m_inputManager = std::make_unique<InputManager>(*this);
        m_audioManager = std::make_unique<AudioManager>();
    }

    ControllerCore::~ControllerCore() = default;

    bool ControllerCore::Initialize() {
        if (!InitializeComponents()) return false;
        PrintWelcomeMessage();
        return true;
    }

    bool ControllerCore::InitializeComponents() {
        // Initialize window manager
        if (!m_windowManager->Initialize()) {
            LOG_ERROR("Failed to initialize window manager");
            return false;
        }

        // Setup window callbacks
        m_windowManager->SetKeyCallback([this](int key) {
            m_inputManager->OnKeyPress(key);
            });

        m_windowManager->SetMouseMoveCallback([this](int x, int y) {
            m_inputManager->OnMouseMove(x, y);
            });

        m_windowManager->SetMouseClickCallback([this](int x, int y) {
            m_inputManager->OnMouseClick(x, y);
            });

        m_windowManager->SetResizeCallback([this](int w, int h) {
            OnResize(w, h);
            });

        m_windowManager->SetCloseCallback([this]() {
            OnClose();
            });

        // Setup color picker callback
        if (auto* picker = m_windowManager->GetColorPicker()) {
            picker->SetOnColorSelectedCallback([this](const Color& color) {
                if (m_rendererManager && m_rendererManager->GetCurrentRenderer()) {
                    m_rendererManager->GetCurrentRenderer()->SetPrimaryColor(color);
                }
                });
        }

        // Initialize audio
        if (!m_audioManager->Initialize()) return false;

        // Initialize renderer
        m_rendererManager = std::make_unique<RendererManager>();
        if (!m_rendererManager->Initialize()) return false;

        if (auto* graphics = m_windowManager->GetGraphics()) {
            m_rendererManager->SetCurrentRenderer(
                m_rendererManager->GetCurrentStyle(),
                graphics
            );
        }

        return true;
    }

    void ControllerCore::PrintWelcomeMessage() {
        LOG_INFO("========================================");
        LOG_INFO("     Spectrum Visualizer C++");
        LOG_INFO("========================================");
        LOG_INFO("Controls:");
        LOG_INFO("  SPACE - Toggle audio capture");
        LOG_INFO("  A     - Toggle animation (test mode)");
        LOG_INFO("  R     - Switch renderer");
        LOG_INFO("  Q     - Change render quality");
        LOG_INFO("  O     - Toggle Overlay Mode");
        LOG_INFO("  S     - Switch Spectrum Scale");
        LOG_INFO("  UP/DOWN Arrow  - Change Amplification");
        LOG_INFO("  LEFT/RIGHT Arrow - Change FFT Window");
        LOG_INFO("  -/+ Keys       - Change Bar Count");
        LOG_INFO("  ESC   - Exit");
        LOG_INFO("========================================");
    }

    void ControllerCore::Run() {
        m_timer.Reset();
        MainLoop();
    }

    void ControllerCore::MainLoop() {
        while (m_windowManager->IsRunning()) {
            m_windowManager->ProcessMessages();

            float dt = m_timer.GetElapsedSeconds();
            if (dt >= FRAME_TIME) {
                m_timer.Reset();
                Update(dt);
                Render();
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void ControllerCore::Update(float deltaTime) {
        m_audioManager->Update(deltaTime);
    }

    void ControllerCore::Render() {
        auto* graphics = m_windowManager->GetGraphics();
        if (!graphics) return;

        if (auto rt = graphics->GetRenderTarget()) {
            auto state = rt->CheckWindowState();
            if (state & D2D1_WINDOW_STATE_OCCLUDED) return;
        }

        graphics->BeginDraw();

        const Color clear = m_windowManager->IsOverlayMode()
            ? Color::Transparent()
            : Color::FromRGB(13, 13, 26);
        graphics->Clear(clear);

        if (m_rendererManager) {
            SpectrumData spectrum = m_audioManager->GetSpectrum();
            m_rendererManager->Render(*graphics, spectrum);
        }

        // Draw color picker only in normal mode
        if (auto* picker = m_windowManager->GetColorPicker()) {
            if (picker->IsVisible() && !m_windowManager->IsOverlayMode()) {
                picker->Draw(*graphics);
            }
        }

        graphics->EndDraw();
    }

    void ControllerCore::OnResize(int width, int height) {
        if (m_rendererManager) {
            m_rendererManager->OnResize(width, height);
        }
    }

    void ControllerCore::OnClose() {
        LOG_INFO("Application closing.");
    }

    void ControllerCore::ToggleOverlay() {
        if (m_windowManager) {
            m_windowManager->ToggleOverlay();
        }
    }

} // namespace Spectrum