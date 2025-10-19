// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Implements the UIManager, which creates and manages all UI components,
// forwarding interactions to the appropriate handlers.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "UIManager.h"
#include "GraphicsContext.h"
#include "ControllerCore.h"
#include "ColorPicker.h"

namespace Spectrum {

    UIManager::UIManager(ControllerCore* controller)
        : m_controller(controller) {
    }

    bool UIManager::Initialize(GraphicsContext& context) {
        if (!CreateUIComponents(context)) return false;
        SetupCallbacks();
        return true;
    }

    bool UIManager::CreateUIComponents(GraphicsContext& context) {
        m_colorPicker = std::make_unique<ColorPicker>(Point{ 20.0f, 20.0f }, 40.0f);
        if (!m_colorPicker->Initialize(context)) return false;

        // Future UI elements will be created here
        return true;
    }

    void UIManager::SetupCallbacks() {
        if (m_colorPicker) {
            m_colorPicker->SetOnColorSelectedCallback([this](const Color& color) {
                if (m_controller) m_controller->SetPrimaryColor(color);
                });
        }
        // Future UI element callbacks will be set up here
    }

    void UIManager::RecreateResources(GraphicsContext& context) {
        if (m_colorPicker) {
            m_colorPicker->RecreateResources(context);
        }
    }

    void UIManager::Draw(GraphicsContext& context) {
        if (m_colorPicker && m_colorPicker->IsVisible()) {
            m_colorPicker->Draw(context);
        }
        // Future elements will be drawn here
    }

    bool UIManager::HandleMouseMove(int x, int y) {
        if (m_colorPicker && m_colorPicker->IsVisible()) {
            return m_colorPicker->HandleMouseMove(x, y);
        }
        return false;
    }

    bool UIManager::HandleMouseClick(int x, int y) {
        if (m_colorPicker && m_colorPicker->IsVisible()) {
            return m_colorPicker->HandleMouseClick(x, y);
        }
        return false;
    }

    bool UIManager::HandleMouseMessage(UINT msg, int x, int y) {
        switch (msg) {
        case WM_MOUSEMOVE:
            return HandleMouseMove(x, y);
        case WM_LBUTTONDOWN:
            return HandleMouseClick(x, y);
        default:
            return false;
        }
    }

}