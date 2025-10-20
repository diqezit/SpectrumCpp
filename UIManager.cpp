// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file implements the UIManager. It manages the primary control panel,
// the audio settings popup, and the color picker. It dispatches input to
// the currently active component and handles mouse capture for dragging.
//
// Implements the UIManager by defining the creation of UI components and
// the state-driven logic for routing mouse input. It ensures that input is
// handled by the correct component based on a modal-first priority system.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UIManager.h"
#include "AudioSettingsPanel.h"
#include "ColorPicker.h"
#include "ControlPanel.h"
#include "ControllerCore.h"
#include "GraphicsContext.h"
#include "IRenderer.h"
#include "RendererManager.h"
#include "UILayout.h"
#include "UISlider.h"
#include "WindowManager.h"

namespace Spectrum {

    UIManager::UIManager(
        ControllerCore* controller,
        WindowManager* windowManager
    ) :
        m_controller(controller),
        m_windowManager(windowManager),
        m_activeSlider(nullptr)
    {
    }

    UIManager::~UIManager() noexcept = default;

    [[nodiscard]] bool UIManager::Initialize()
    {
        m_controlPanel = std::make_unique<ControlPanel>(m_controller);
        m_controlPanel->Initialize();
        m_controlPanel->SetOnShowAudioSettings([this]() { this->ShowAudioSettings(); });

        m_audioSettingsPanel = std::make_unique<AudioSettingsPanel>(m_controller);
        m_audioSettingsPanel->Initialize();
        m_audioSettingsPanel->SetOnCloseCallback([this]() { this->HideAudioSettings(); });

        GraphicsContext* gfx = m_windowManager->GetGraphics();
        if (!gfx) return false;

        const Point pickerPos = { gfx->GetWidth() - UILayout::kPadding - 80.0f, UILayout::kPadding };

        m_colorPicker = std::make_unique<ColorPicker>(pickerPos, 40.0f);
        if (!m_colorPicker->Initialize(*gfx))
            return false;

        m_colorPicker->SetOnColorSelectedCallback([this](const Color& c) {
            if (m_controller) m_controller->SetPrimaryColor(c);
            });

        return true;
    }

    void UIManager::RecreateResources(GraphicsContext& context, int width, int height)
    {
        if (m_colorPicker)
        {
            m_colorPicker->RecreateResources(context);
        }

        if (m_controlPanel)
        {
            m_controlPanel->Initialize();
        }

        if (m_colorPicker)
        {
            const Point newPos = {
                static_cast<float>(width) - UILayout::kPadding - 80.0f,
                UILayout::kPadding
            };

            m_colorPicker = std::make_unique<ColorPicker>(newPos, 40.0f);

            if (m_colorPicker->Initialize(context))
            {
                m_colorPicker->SetOnColorSelectedCallback([this](const Color& c) {
                    if (m_controller) m_controller->SetPrimaryColor(c);
                    });
            }
        }
    }

    void UIManager::Update(const Point& mousePos, bool isMouseDown, float deltaTime)
    {
        if (m_activeSlider)
        {
            if (isMouseDown)
            {
                m_activeSlider->Drag(mousePos);
            }
            else
            {
                m_activeSlider->EndDrag();
                m_activeSlider = nullptr;
                ReleaseCapture();
            }
            return;
        }

        if (IsModalActive())
        {
            m_audioSettingsPanel->Update(mousePos, isMouseDown, deltaTime);

            if (isMouseDown && !m_activeSlider)
            {
                m_activeSlider = m_audioSettingsPanel->GetSliderAt(mousePos);
                if (m_activeSlider)
                {
                    SetCapture(m_windowManager->GetCurrentHwnd());
                    m_activeSlider->BeginDrag(mousePos);
                }
            }
            return;
        }

        if (m_controlPanel)
            m_controlPanel->Update(mousePos, isMouseDown, deltaTime);

        if (ShouldDrawColorPicker() && m_colorPicker)
            m_colorPicker->Update(mousePos, isMouseDown);
    }

    void UIManager::Draw(GraphicsContext& context) const
    {
        if (m_controlPanel)
            m_controlPanel->Draw(context);

        if (ShouldDrawColorPicker())
            if (m_colorPicker)
                m_colorPicker->Draw(context);

        if (IsModalActive())
        {
            const Rect screenRect = {
                0.0f, 0.0f,
                static_cast<float>(context.GetWidth()),
                static_cast<float>(context.GetHeight())
            };
            context.DrawRectangle(screenRect, Color(0.0f, 0.0f, 0.0f, 0.5f));
            m_audioSettingsPanel->Draw(context);
        }
    }

    void UIManager::ShowAudioSettings()
    {
        if (m_audioSettingsPanel)
            m_audioSettingsPanel->Show();
    }

    void UIManager::HideAudioSettings()
    {
        if (m_audioSettingsPanel)
            m_audioSettingsPanel->Hide();
    }

    [[nodiscard]] bool UIManager::IsModalActive() const
    {
        return m_audioSettingsPanel && m_audioSettingsPanel->IsVisible();
    }

    [[nodiscard]] bool UIManager::ShouldDrawColorPicker() const
    {
        if (!m_controller) return false;

        const auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager) return false;

        const auto* renderer = rendererManager->GetCurrentRenderer();
        return renderer && renderer->SupportsPrimaryColor();
    }

}