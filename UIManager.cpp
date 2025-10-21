// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements the UIManager with comprehensive delta-time animation support
// for all UI components and smooth modal transitions.
// 
// Key implementation details:
// - Smooth modal overlay fade with cubic easing
// - ColorPicker fade-in/fade-out animations
// - Optimized resource recreation (reposition instead of recreate)
// - Frame-rate independent animations throughout
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UIManager.h"
#include "AudioSettingsPanel.h"
#include "ColorPicker.h"
#include "ControlPanel.h"
#include "ControllerCore.h"
#include "GraphicsContext.h"
#include "IRenderer.h"
#include "MathUtils.h"
#include "RendererManager.h"
#include "UILayout.h"
#include "UISlider.h"
#include "WindowManager.h"

namespace Spectrum
{
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UIManager::UIManager(ControllerCore* controller, WindowManager* windowManager) :
        m_controller(controller),
        m_windowManager(windowManager),
        m_activeSlider(nullptr),
        m_modalOverlayAlpha(0.0f),
        m_colorPickerAlpha(0.0f),
        m_shouldShowColorPicker(false)
    {
    }

    UIManager::~UIManager() noexcept
    {
        ReleaseMouseCapture();
    }

    bool UIManager::Initialize()
    {
        if (!m_controller || !m_windowManager) return false;

        m_controlPanel = std::make_unique<ControlPanel>(m_controller);
        if (!m_controlPanel) return false;

        m_controlPanel->Initialize();
        m_controlPanel->SetOnShowAudioSettings([this] { ShowAudioSettings(); });

        m_audioSettingsPanel = std::make_unique<AudioSettingsPanel>(m_controller);
        if (!m_audioSettingsPanel) return false;

        m_audioSettingsPanel->Initialize();
        m_audioSettingsPanel->SetOnCloseCallback([this] { HideAudioSettings(); });

        GraphicsContext* graphics = m_windowManager->GetGraphics();
        if (!graphics) return false;

        const Point pickerPos = {
            static_cast<float>(graphics->GetWidth()) - UILayout::kPadding - 80.0f,
            UILayout::kPadding
        };

        m_colorPicker = std::make_unique<ColorPicker>(pickerPos, 40.0f);
        if (!m_colorPicker) return false;

        if (!m_colorPicker->Initialize(*graphics)) return false;

        m_colorPicker->SetOnColorSelectedCallback([this](const Color& color) {
            if (m_controller) m_controller->SetPrimaryColor(color);
            });

        m_shouldShowColorPicker = ShouldDrawColorPicker();

        return true;
    }

    void UIManager::RecreateResources(GraphicsContext& context, int width, int height)
    {
        if (m_colorPicker)
        {
            m_colorPicker->RecreateResources(context);
            RepositionColorPicker(width, height);
        }

        if (m_controlPanel)
        {
            m_controlPanel->Initialize();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::Update(const Point& mousePos, bool isMouseDown, float deltaTime)
    {
        UpdateModalOverlayAnimation(deltaTime);
        UpdateColorPickerVisibility(deltaTime);

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
                ReleaseMouseCapture();
            }
        }

        if (IsModalActive())
        {
            if (m_audioSettingsPanel)
            {
                m_audioSettingsPanel->Update(mousePos, isMouseDown, deltaTime);
            }

            if (isMouseDown && !m_activeSlider)
            {
                m_activeSlider = m_audioSettingsPanel->GetSliderAt(mousePos);
                if (m_activeSlider)
                {
                    if (m_windowManager)
                    {
                        SetCapture(m_windowManager->GetCurrentHwnd());
                    }
                    m_activeSlider->BeginDrag(mousePos);
                }
            }
            return;
        }

        if (m_controlPanel)
        {
            m_controlPanel->Update(mousePos, isMouseDown, deltaTime);
        }

        if (m_shouldShowColorPicker && m_colorPicker && m_colorPickerAlpha > 0.01f)
        {
            m_colorPicker->Update(mousePos, isMouseDown, deltaTime);
        }
    }

    void UIManager::Draw(GraphicsContext& context) const
    {
        if (m_controlPanel)
        {
            m_controlPanel->Draw(context);
        }

        if (m_shouldShowColorPicker && m_colorPicker && m_colorPickerAlpha > 0.01f)
        {
            context.PushTransform();

            const float alpha = Utils::EaseOutCubic(m_colorPickerAlpha);
            const float scale = Utils::Lerp(0.8f, 1.0f, Utils::EaseOutBack(m_colorPickerAlpha));

            const Point pickerCenter = m_colorPicker->GetCenter();
            context.ScaleAt(pickerCenter, scale, scale);

            m_colorPicker->DrawWithAlpha(context, alpha);

            context.PopTransform();
        }

        if (IsModalActive() && m_modalOverlayAlpha > 0.01f)
        {
            const float alpha = GetModalOverlayAlpha();

            const Rect screenRect = {
                0.0f, 0.0f,
                static_cast<float>(context.GetWidth()),
                static_cast<float>(context.GetHeight())
            };

            context.DrawRectangle(screenRect, Color(0.0f, 0.0f, 0.0f, alpha));

            if (m_audioSettingsPanel)
            {
                m_audioSettingsPanel->Draw(context);
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::ShowAudioSettings()
    {
        if (m_audioSettingsPanel)
        {
            m_audioSettingsPanel->Show();
        }
    }

    void UIManager::HideAudioSettings()
    {
        if (m_activeSlider)
        {
            m_activeSlider->EndDrag();
            m_activeSlider = nullptr;
            ReleaseMouseCapture();
        }
        if (m_audioSettingsPanel)
        {
            m_audioSettingsPanel->Hide();
        }
    }

    void UIManager::ReleaseMouseCapture()
    {
        ReleaseCapture();
    }

    void UIManager::UpdateModalOverlayAnimation(float deltaTime)
    {
        const float targetAlpha = IsModalActive() ? 1.0f : 0.0f;

        m_modalOverlayAlpha = Utils::ExponentialDecay(
            m_modalOverlayAlpha,
            targetAlpha,
            kModalFadeSpeed,
            deltaTime
        );
    }

    void UIManager::UpdateColorPickerVisibility(float deltaTime)
    {
        const bool shouldShow = ShouldDrawColorPicker();

        if (shouldShow != m_shouldShowColorPicker)
        {
            m_shouldShowColorPicker = shouldShow;
        }

        const float targetAlpha = m_shouldShowColorPicker ? 1.0f : 0.0f;

        m_colorPickerAlpha = Utils::ExponentialDecay(
            m_colorPickerAlpha,
            targetAlpha,
            kColorPickerFadeSpeed,
            deltaTime
        );
    }

    void UIManager::RepositionColorPicker(int width, int height)
    {
        if (!m_colorPicker) return;

        const Point newPos = {
            static_cast<float>(width) - UILayout::kPadding - 80.0f,
            UILayout::kPadding
        };

        m_colorPicker->SetPosition(newPos);
    }

    bool UIManager::IsModalActive() const noexcept
    {
        return m_audioSettingsPanel && m_audioSettingsPanel->IsVisible();
    }

    bool UIManager::ShouldDrawColorPicker() const noexcept
    {
        if (!m_controller) return false;

        const auto* rendererManager = m_controller->GetRendererManager();
        if (!rendererManager) return false;

        const auto* renderer = rendererManager->GetCurrentRenderer();
        return renderer && renderer->SupportsPrimaryColor();
    }

    float UIManager::GetModalOverlayAlpha() const noexcept
    {
        const float easedAlpha = Utils::EaseInOutCubic(m_modalOverlayAlpha);
        return easedAlpha * kMaxModalOverlayAlpha;
    }

} // namespace Spectrum