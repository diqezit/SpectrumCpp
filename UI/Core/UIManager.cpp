#include "UI/Core/UIManager.h"
#include "UI/Panels/AudioSettingsPanel/AudioSettingsPanel.h"
#include "UI/Panels/ColorPicker/ColorPicker.h"
#include "UI/Panels/ControlPanel/ControlPanel.h"
#include "UI/Common/UILayout.h"
#include "UI/Components/UISlider.h"
#include "App/ControllerCore.h"
#include "Graphics/API/Canvas.h"
#include "Graphics/API/Core/RenderEngine.h"
#include "Graphics/API/Structs/Paint.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RendererManager.h"
#include "Platform/WindowManager.h"
#include "Common/MathUtils.h"
#include <stdexcept>

namespace Spectrum
{
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    UIManager::UIManager(
        ControllerCore* controller,
        Platform::WindowManager* windowManager
    ) :
        m_controller(controller),
        m_windowManager(windowManager),
        m_activeSlider(nullptr),
        m_modalOverlayAlpha(0.0f),
        m_colorPickerAlpha(0.0f),
        m_shouldShowColorPicker(false)
    {
        if (!m_controller) {
            throw std::invalid_argument("controller dependency cannot be null");
        }
        if (!m_windowManager) {
            throw std::invalid_argument("windowManager dependency cannot be null");
        }
    }

    UIManager::~UIManager() noexcept
    {
        if (m_activeSlider) {
            ReleaseMouseCapture();
        }
    }

    [[nodiscard]] bool UIManager::Initialize()
    {
        m_controlPanel = std::make_unique<ControlPanel>(m_controller);
        if (!m_controlPanel) {
            return false;
        }

        m_controlPanel->Initialize();
        m_controlPanel->SetOnShowAudioSettings([this] { ShowAudioSettings(); });

        m_audioSettingsPanel = std::make_unique<AudioSettingsPanel>(m_controller);
        if (!m_audioSettingsPanel) {
            return false;
        }

        m_audioSettingsPanel->Initialize();
        m_audioSettingsPanel->SetOnCloseCallback([this] { HideAudioSettings(); });

        if (auto* engine = m_windowManager->GetRenderEngine())
        {
            const Point pickerPos = {
                static_cast<float>(engine->GetWidth()) - UILayout::kPadding - 80.0f,
                UILayout::kPadding
            };
            m_colorPicker = std::make_unique<ColorPicker>(pickerPos, 40.0f);
            if (!m_colorPicker || !m_colorPicker->Initialize(engine->GetCanvas())) {
                return false;
            }
        }
        else
        {
            return false;
        }

        m_colorPicker->SetOnColorSelectedCallback([this](const Color& color) {
            if (m_controller) {
                m_controller->SetPrimaryColor(color);
            }
            });

        m_shouldShowColorPicker = ShouldDrawColorPicker();
        return true;
    }

    void UIManager::RecreateResources(
        Canvas& canvas,
        int width,
        int height
    )
    {
        if (m_colorPicker)
        {
            m_colorPicker->RecreateResources(canvas);
            RepositionColorPicker(width, height);
        }
        if (m_controlPanel) {
            m_controlPanel->Initialize();
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Main Execution
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::Update(
        const Point& mousePos,
        bool isMouseDown,
        float deltaTime
    )
    {
        UpdateModalOverlayAnimation(deltaTime);
        UpdateColorPickerVisibility(deltaTime);

        if (m_activeSlider)
        {
            if (isMouseDown) {
                m_activeSlider->Drag(mousePos);
            }
            else
            {
                m_activeSlider->EndDrag();
                m_activeSlider = nullptr;
                ReleaseMouseCapture();
            }
            return; // If a slider is active, it captures all input
        }

        if (IsModalActive())
        {
            if (m_audioSettingsPanel) {
                m_audioSettingsPanel->Update(mousePos, isMouseDown, deltaTime);
            }

            if (isMouseDown)
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

        if (m_controlPanel) {
            m_controlPanel->Update(mousePos, isMouseDown, deltaTime);
        }
        if (m_shouldShowColorPicker && m_colorPicker && m_colorPickerAlpha > 0.01f)
        {
            m_colorPicker->Update(mousePos, isMouseDown, deltaTime);
        }
    }

    void UIManager::Draw(Canvas& canvas) const
    {
        if (m_controlPanel) {
            m_controlPanel->Draw(canvas);
        }

        if (m_shouldShowColorPicker && m_colorPicker && m_colorPickerAlpha > 0.01f)
        {
            canvas.PushTransform();
            const float alpha = Utils::EaseOutCubic(m_colorPickerAlpha);
            const float scale = Utils::Lerp(0.8f, 1.0f, Utils::EaseOutBack(m_colorPickerAlpha));
            const Point pickerCenter = m_colorPicker->GetCenter();
            canvas.ScaleAt(pickerCenter, scale, scale);
            m_colorPicker->DrawWithAlpha(canvas, alpha);
            canvas.PopTransform();
        }

        if (IsModalActive() && m_modalOverlayAlpha > 0.01f)
        {
            const float alpha = GetModalOverlayAlpha();
            if (auto* engine = m_windowManager->GetRenderEngine())
            {
                const Rect screenRect = {
                    0.0f, 0.0f,
                    static_cast<float>(engine->GetWidth()),
                    static_cast<float>(engine->GetHeight())
                };

                const Paint paint = Paint::Fill(Color(0.0f, 0.0f, 0.0f, alpha));
                canvas.DrawRectangle(screenRect, paint);
            }

            if (m_audioSettingsPanel) {
                m_audioSettingsPanel->Draw(canvas);
            }
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Window Message Handling
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    [[nodiscard]] bool UIManager::HandleMessage(
        HWND /*hwnd*/,
        UINT msg,
        WPARAM /*wParam*/,
        LPARAM /*lParam*/
    )
    {
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MOUSEMOVE)
        {
            return false;
        }
        return false;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Private Implementation / Internal Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void UIManager::ShowAudioSettings()
    {
        if (m_audioSettingsPanel) {
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
        if (m_audioSettingsPanel) {
            m_audioSettingsPanel->Hide();
        }
    }

    void UIManager::ReleaseMouseCapture()
    {
        ::ReleaseCapture();
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
        if (shouldShow != m_shouldShowColorPicker) {
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

    void UIManager::RepositionColorPicker(
        int width,
        int /*height*/
    )
    {
        if (!m_colorPicker) {
            return;
        }
        const Point newPos = {
            static_cast<float>(width) - UILayout::kPadding - 80.0f,
            UILayout::kPadding
        };
        m_colorPicker->SetPosition(newPos);
    }

    [[nodiscard]] bool UIManager::IsModalActive() const noexcept
    {
        return m_audioSettingsPanel && m_audioSettingsPanel->IsVisible();
    }

    [[nodiscard]] bool UIManager::ShouldDrawColorPicker() const noexcept
    {
        if (!m_controller) {
            return false;
        }
        if (const auto* rendererManager = m_controller->GetRendererManager())
        {
            if (const auto* renderer = rendererManager->GetCurrentRenderer())
            {
                return renderer->SupportsPrimaryColor();
            }
        }
        return false;
    }

    [[nodiscard]] float UIManager::GetModalOverlayAlpha() const noexcept
    {
        const float easedAlpha = Utils::EaseInOutCubic(m_modalOverlayAlpha);
        return easedAlpha * kMaxModalOverlayAlpha;
    }

} // namespace Spectrum