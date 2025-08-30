// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// InputManager.cpp: Implementation of the InputManager class.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "InputManager.h"
#include "ControllerCore.h"
#include "WindowManager.h"
#include "Utils.h"

namespace Spectrum {

    InputManager::InputManager(ControllerCore& controller)
        : m_controller(controller) {
    }

    void InputManager::OnKeyPress(int key) {
        switch (key) {
        case VK_SPACE:
            m_controller.m_audioManager->ToggleCapture();
            break;

        case 'A':
            m_controller.m_audioManager->ToggleAnimation();
            break;

        case 'R':
            if (m_controller.m_rendererManager) {
                auto* graphics = m_controller.m_windowManager->GetGraphics();
                m_controller.m_rendererManager->SwitchRenderer(1, graphics);
            }
            break;

        case 'Q':
            if (m_controller.m_rendererManager) {
                m_controller.m_rendererManager->CycleQuality();
            }
            break;

        case 'O':
            m_controller.ToggleOverlay();
            break;

        case 'S':
            m_controller.m_audioManager->ChangeSpectrumScale(1);
            break;

        case VK_UP:
            m_controller.m_audioManager->ChangeAmplification(-0.1f);
            break;

        case VK_DOWN:
            m_controller.m_audioManager->ChangeAmplification(0.1f);
            break;

        case VK_LEFT:
            m_controller.m_audioManager->ChangeFFTWindow(-1);
            break;

        case VK_RIGHT:
            m_controller.m_audioManager->ChangeFFTWindow(1);
            break;

        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            m_controller.m_audioManager->ChangeBarCount(-4);
            break;

        case VK_ADD:
        case VK_OEM_PLUS:
            m_controller.m_audioManager->ChangeBarCount(4);
            break;

        case VK_ESCAPE:
            if (m_controller.m_windowManager &&
                m_controller.m_windowManager->IsOverlayMode()) {
                m_controller.ToggleOverlay();
            }
            else if (m_controller.m_windowManager) {
                auto* mainWindow = m_controller.m_windowManager->GetMainWindow();
                if (mainWindow) {
                    mainWindow->Close();
                }
            }
            break;
        }
    }

    void InputManager::OnMouseMove(int x, int y) {
        if (m_controller.m_windowManager) {
            auto* colorPicker = m_controller.m_windowManager->GetColorPicker();
            if (colorPicker && colorPicker->IsVisible()) {
                colorPicker->HandleMouseMove(x, y);
            }
        }
    }

    void InputManager::OnMouseClick(int x, int y) {
        if (m_controller.m_windowManager) {
            auto* colorPicker = m_controller.m_windowManager->GetColorPicker();
            if (colorPicker && colorPicker->IsVisible()) {
                colorPicker->HandleMouseClick(x, y);
            }
        }
    }

} // namespace Spectrum