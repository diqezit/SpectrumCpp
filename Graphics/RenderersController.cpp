// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// RenderersController.cpp: Implementation of the RenderersController class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "RenderersController.h"
#include "BarsRenderer.h"
#include "WaveRenderer.h"
#include "CircularWaveRenderer.h"
#include "CubesRenderer.h"
#include "FireRenderer.h"
#include "LedPanelRenderer.h"
#include "Utils.h"

namespace Spectrum {

    RenderersController::RenderersController()
        : m_currentRenderer(nullptr)
        , m_currentStyle(RenderStyle::Bars)
        , m_initialized(false) {
    }

    RenderersController::~RenderersController() {
        Shutdown();
    }

    bool RenderersController::InitializeRenderers() {
        if (m_initialized) return true;

        try {
            InitializeRendererInstances();
            SetCurrentRenderer(RenderStyle::Bars);
            m_initialized = true;
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize renderers: " << e.what());
            return false;
        }
    }

    bool RenderersController::InitializeColorPicker(GraphicsContext& graphics) {
        if (!m_initialized) {
            LOG_ERROR("Renderers must be initialized before color picker");
            return false;
        }

        try {
            m_colorPicker = std::make_unique<ColorPicker>(Point(20.0f, 20.0f), 40.0f);
            if (!m_colorPicker->Initialize(graphics)) {
                LOG_ERROR("Failed to initialize color picker");
                return false;
            }
            SetupColorPickerCallback();
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize color picker: " << e.what());
            return false;
        }
    }

    void RenderersController::Shutdown() {
        if (m_currentRenderer) {
            m_currentRenderer->OnDeactivate();
            m_currentRenderer = nullptr;
        }
        m_renderers.clear();
        m_colorPicker.reset();
        m_initialized = false;
    }

    void RenderersController::RenderCurrentVisualizer(
        GraphicsContext& graphics,
        const SpectrumData& spectrum
    ) {
        if (!m_currentRenderer) return;
        m_currentRenderer->Render(graphics, spectrum);
    }

    void RenderersController::RenderColorPicker(GraphicsContext& graphics) {
        if (m_colorPicker && m_colorPicker->IsVisible()) {
            m_colorPicker->Draw(graphics);
        }
    }

    void RenderersController::SetCurrentRenderer(RenderStyle style) {
        if (m_currentRenderer) {
            m_currentRenderer->OnDeactivate();
        }

        auto it = m_renderers.find(style);
        if (it == m_renderers.end()) {
            LOG_ERROR("Renderer style not found: " << static_cast<int>(style));
            return;
        }

        m_currentRenderer = it->second.get();
        m_currentStyle = style;

        if (m_currentRenderer) {
            // Window size will be set by OnWindowResize call
            m_currentRenderer->OnActivate(0, 0);
            LOG_INFO("Switched to " << m_currentRenderer->GetName().data() << " renderer");
        }
    }

    void RenderersController::SwitchRenderer(int direction) {
        int currentIndex = static_cast<int>(m_currentStyle);
        const int totalCount = static_cast<int>(RenderStyle::Count);

        currentIndex = (currentIndex + direction + totalCount) % totalCount;
        SetCurrentRenderer(static_cast<RenderStyle>(currentIndex));
    }

    RenderStyle RenderersController::GetCurrentRendererStyle() const {
        return m_currentStyle;
    }

    IRenderer* RenderersController::GetCurrentRenderer() const {
        return m_currentRenderer;
    }

    void RenderersController::OnWindowResize(int width, int height) {
        if (m_currentRenderer) {
            m_currentRenderer->OnActivate(width, height);
        }
    }

    void RenderersController::SetPrimaryColor(const Color& color) {
        if (m_currentRenderer) {
            m_currentRenderer->SetPrimaryColor(color);
        }
    }

    ColorPicker* RenderersController::GetColorPicker() const {
        return m_colorPicker.get();
    }

    bool RenderersController::IsColorPickerVisible() const {
        return m_colorPicker && m_colorPicker->IsVisible();
    }

    void RenderersController::SetColorPickerVisible(bool visible) {
        if (m_colorPicker) {
            if (visible) {
                m_colorPicker->Show();
            }
            else {
                m_colorPicker->Hide();
            }
        }
    }

    void RenderersController::InitializeRendererInstances() {
        m_renderers[RenderStyle::Bars] = std::make_unique<BarsRenderer>();
        m_renderers[RenderStyle::Wave] = std::make_unique<WaveRenderer>();
        m_renderers[RenderStyle::CircularWave] = std::make_unique<CircularWaveRenderer>();
        m_renderers[RenderStyle::Cubes] = std::make_unique<CubesRenderer>();
        m_renderers[RenderStyle::Fire] = std::make_unique<FireRenderer>();
        m_renderers[RenderStyle::LedPanel] = std::make_unique<LedPanelRenderer>();
    }

    void RenderersController::SetupColorPickerCallback() {
        if (!m_colorPicker) return;

        m_colorPicker->SetOnColorSelectedCallback(
            [this](const Color& color) {
                SetPrimaryColor(color);
            }
        );
    }

} // namespace Spectrum