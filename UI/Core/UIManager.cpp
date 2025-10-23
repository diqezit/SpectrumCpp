// UIManager.cpp
#include "UI/Core/UIManager.h"
#include "UI/Core/ImGuiContext.h"
#include "App/ControllerCore.h"
#include "Audio/AudioManager.h"
#include "Graphics/RendererManager.h"
#include "Graphics/API/GraphicsHelpers.h"
#include "Platform/WindowManager.h"
#include "Platform/UIWindow.h"
#include "imgui.h"
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace Spectrum {

    UIManager::UIManager(
        ControllerCore* controller,
        Platform::WindowManager* windowManager
    ) :
        m_controller(controller),
        m_windowManager(windowManager)
    {
        if (!controller || !windowManager)
        {
            throw std::invalid_argument("UIManager: Dependencies cannot be null");
        }
    }

    UIManager::~UIManager() noexcept
    {
        Shutdown();
    }

    bool UIManager::Initialize()
    {
        auto* uiEngine = m_windowManager->GetUIEngine();
        auto* uiWindow = m_windowManager->GetUIWindow();

        if (!uiEngine || !uiEngine->IsD3D11Mode() || !uiWindow)
        {
            LOG_ERROR("UIManager: Invalid dependencies for initialization");
            return false;
        }

        m_imguiContext = std::make_unique<ImGuiContext>();
        if (!m_imguiContext->Initialize(
            uiWindow->GetHwnd(),
            uiEngine->GetD3D11Device(),
            uiEngine->GetD3D11DeviceContext()
        ))
        {
            LOG_ERROR("UIManager: ImGui context initialization failed");
            return false;
        }

        m_imguiContext->SetRenderTargetView(uiEngine->GetD3D11RenderTargetView());
        return true;
    }

    void UIManager::Shutdown()
    {
        if (m_imguiContext)
        {
            m_imguiContext->Shutdown();
            m_imguiContext.reset();
        }
    }

    void UIManager::BeginFrame()
    {
        if (m_imguiContext && m_imguiContext->IsInitialized())
        {
            m_imguiContext->NewFrame();
        }
    }

    void UIManager::Render()
    {
        if (!m_imguiContext || !m_imguiContext->IsInitialized()) return;

        RenderControlPanel();
        RenderAudioSettings();
        RenderColorPicker();
    }

    void UIManager::EndFrame()
    {
        if (m_imguiContext && m_imguiContext->IsInitialized())
        {
            m_imguiContext->Render();
        }
    }

    void UIManager::OnResize(int width, int height)
    {
        (void)width;
        (void)height;

        if (m_imguiContext && m_imguiContext->IsInitialized())
        {
            auto* uiEngine = m_windowManager->GetUIEngine();
            if (uiEngine)
            {
                auto* rtv = uiEngine->GetD3D11RenderTargetView();
                if (rtv)
                {
                    m_imguiContext->SetRenderTargetView(rtv);
                }
            }
        }
    }

    bool UIManager::HandleMessage(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        if (m_imguiContext && m_imguiContext->IsInitialized())
        {
            return m_imguiContext->ProcessMessage(hwnd, msg, wParam, lParam);
        }
        return false;
    }

    AudioManager* UIManager::GetAudioManager() const
    {
        return m_controller ? m_controller->GetAudioManager() : nullptr;
    }

    RendererManager* UIManager::GetRendererManager() const
    {
        return m_controller ? m_controller->GetRendererManager() : nullptr;
    }

    RenderStyle UIManager::StringToRenderStyle(const std::string& name) const
    {
        static const std::unordered_map<std::string, RenderStyle> map = {
            { "Bars",          RenderStyle::Bars          },
            { "Wave",          RenderStyle::Wave          },
            { "Circular Wave", RenderStyle::CircularWave  },
            { "Polyline Wave", RenderStyle::PolylineWave  },
            { "Kenwood Bars",  RenderStyle::KenwoodBars   },
            { "Cubes",         RenderStyle::Cubes         },
            { "Fire",          RenderStyle::Fire          },
            { "LED Panel",     RenderStyle::LedPanel      },
            { "Matrix LED",    RenderStyle::MatrixLed     },
            { "Particles",     RenderStyle::Particles     },
            { "Sphere",        RenderStyle::Sphere        },
            { "Gauge",         RenderStyle::Gauge         }
        };
        auto it = map.find(name);
        return it != map.end() ? it->second : RenderStyle::Bars;
    }

    const char* UIManager::RenderStyleToString(RenderStyle style) const
    {
        switch (style)
        {
        case RenderStyle::Bars:         return "Bars";
        case RenderStyle::Wave:         return "Wave";
        case RenderStyle::CircularWave: return "Circular Wave";
        case RenderStyle::PolylineWave: return "Polyline Wave";
        case RenderStyle::KenwoodBars:  return "Kenwood Bars";
        case RenderStyle::Cubes:        return "Cubes";
        case RenderStyle::Fire:         return "Fire";
        case RenderStyle::LedPanel:     return "LED Panel";
        case RenderStyle::MatrixLed:    return "Matrix LED";
        case RenderStyle::Particles:    return "Particles";
        case RenderStyle::Sphere:       return "Sphere";
        case RenderStyle::Gauge:        return "Gauge";
        default:                        return "Unknown";
        }
    }

    void UIManager::RenderControlPanel()
    {
        ImGui::SetNextWindowSize(ImVec2(320, 285), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);

        if (!ImGui::Begin("Control Panel", nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return;
        }

        auto* rendererMgr = GetRendererManager();
        if (rendererMgr)
        {
            const char* label = "Renderer";
            const float labelWidth = ImGui::CalcTextSize(label).x;
            const float comboWidth = ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemInnerSpacing.x;

            ImGui::PushItemWidth(comboWidth);
            const char* currentName = RenderStyleToString(rendererMgr->GetCurrentStyle());
            if (ImGui::BeginCombo("##Renderer", currentName))
            {
                static const std::vector<RenderStyle> allStyles = {
                    RenderStyle::PolylineWave, RenderStyle::Bars, RenderStyle::Wave,
                    RenderStyle::CircularWave, RenderStyle::KenwoodBars, RenderStyle::Cubes,
                    RenderStyle::Fire, RenderStyle::LedPanel, RenderStyle::MatrixLed,
                    RenderStyle::Particles, RenderStyle::Sphere, RenderStyle::Gauge
                };

                for (const auto style : allStyles)
                {
                    const char* name = RenderStyleToString(style);
                    bool isSelected = (rendererMgr->GetCurrentStyle() == style);
                    if (ImGui::Selectable(name, isSelected))
                    {
                        rendererMgr->SetCurrentRenderer(style);
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::TextUnformatted(label);
        }

        ImGui::Spacing();

        if (ImGui::Button("Audio Settings", ImVec2(-1, 0))) m_showAudioSettings = true;
        if (ImGui::Button("Color Picker", ImVec2(-1, 0))) m_showColorPicker = true;
        if (ImGui::Button("Toggle Overlay", ImVec2(-1, 0)) && m_windowManager)
        {
            m_windowManager->ToggleOverlay();
        }

        ImGui::Separator();

        auto* audioMgr = GetAudioManager();
        if (audioMgr)
        {
            const float labelColumnWidth = ImGui::CalcTextSize("Amplification:").x + 10.0f;
            ImGui::Columns(2, "AudioInfo", false);
            ImGui::SetColumnWidth(0, labelColumnWidth);

            ImGui::TextUnformatted("Amplification:"); ImGui::NextColumn(); ImGui::Text("%.2f", audioMgr->GetAmplification()); ImGui::NextColumn();
            ImGui::TextUnformatted("Smoothing:");     ImGui::NextColumn(); ImGui::Text("%.2f", audioMgr->GetSmoothing()); ImGui::NextColumn();
            ImGui::TextUnformatted("Bar Count:");     ImGui::NextColumn(); ImGui::Text("%zu", audioMgr->GetBarCount()); ImGui::NextColumn();
            ImGui::TextUnformatted("FFT:");           ImGui::NextColumn(); ImGui::TextUnformatted(audioMgr->GetFFTWindowName().data()); ImGui::NextColumn();
            ImGui::TextUnformatted("Scale:");         ImGui::NextColumn(); ImGui::TextUnformatted(audioMgr->GetSpectrumScaleName().data()); ImGui::NextColumn();
            ImGui::TextUnformatted("Mode:");          ImGui::NextColumn(); ImGui::TextUnformatted(audioMgr->IsCapturing() ? "Capturing" : audioMgr->IsAnimating() ? "Animation" : "Idle"); ImGui::NextColumn();

            ImGui::Columns(1);
        }

        ImGui::End();
    }

    void UIManager::RenderAudioSettings()
    {
        if (!m_showAudioSettings) return;

        auto* audioMgr = GetAudioManager();
        if (!audioMgr)
        {
            m_showAudioSettings = false;
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(10, 305), ImGuiCond_Always);

        if (!ImGui::Begin("Audio Settings", &m_showAudioSettings,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize))
        {
            ImGui::End();
            return;
        }

        const float labelWidth = ImGui::CalcTextSize("Amplification").x;
        const float sliderWidth = ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemInnerSpacing.x;

        ImGui::PushItemWidth(sliderWidth);
        float amp = audioMgr->GetAmplification();
        if (ImGui::SliderFloat("##Amp", &amp, 0.1f, 5.0f, "%.2f")) audioMgr->SetAmplification(amp);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextUnformatted("Amplification");

        ImGui::PushItemWidth(sliderWidth);
        float smooth = audioMgr->GetSmoothing();
        if (ImGui::SliderFloat("##Smooth", &smooth, 0.0f, 0.95f, "%.2f")) audioMgr->SetSmoothing(smooth);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextUnformatted("Smoothing");

        ImGui::PushItemWidth(sliderWidth);
        int bars = static_cast<int>(audioMgr->GetBarCount());
        if (ImGui::SliderInt("##Bars", &bars, 16, 128)) audioMgr->SetBarCount(static_cast<size_t>(bars));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextUnformatted("Bar Count");

        ImGui::Separator();

        const char* fftLabel = "FFT Window";
        const float fftLabelWidth = ImGui::CalcTextSize(fftLabel).x;
        const float fftComboWidth = ImGui::GetContentRegionAvail().x - fftLabelWidth - ImGui::GetStyle().ItemInnerSpacing.x;

        ImGui::PushItemWidth(fftComboWidth);
        std::string currentFFT(audioMgr->GetFFTWindowName());
        if (ImGui::BeginCombo("##FFT", currentFFT.c_str()))
        {
            const auto fftWindows = audioMgr->GetAvailableFFTWindows();
            for (const auto& name : fftWindows)
            {
                bool isSelected = (currentFFT == name);
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    audioMgr->SetFFTWindowByName(name);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextUnformatted(fftLabel);

        const char* scaleLabel = "Scale";
        const float scaleLabelWidth = ImGui::CalcTextSize(scaleLabel).x;
        const float scaleComboWidth = ImGui::GetContentRegionAvail().x - scaleLabelWidth - ImGui::GetStyle().ItemInnerSpacing.x;

        ImGui::PushItemWidth(scaleComboWidth);
        std::string currentScale(audioMgr->GetSpectrumScaleName());
        if (ImGui::BeginCombo("##Scale", currentScale.c_str()))
        {
            const auto scaleTypes = audioMgr->GetAvailableSpectrumScales();
            for (const auto& name : scaleTypes)
            {
                bool isSelected = (currentScale == name);
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    audioMgr->SetSpectrumScaleByName(name);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextUnformatted(scaleLabel);

        ImGui::Separator();

        const float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
        if (ImGui::Button("Reset", ImVec2(buttonWidth, 0)))
        {
            audioMgr->SetAmplification(1.0f);
            audioMgr->SetSmoothing(0.7f);
            audioMgr->SetBarCount(64);
            audioMgr->SetFFTWindowByName("Hanning");
            audioMgr->SetSpectrumScaleByName("Linear");
        }

        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(buttonWidth, 0)))
        {
            m_showAudioSettings = false;
        }

        ImGui::End();
    }

    void UIManager::RenderColorPicker()
    {
        if (!m_showColorPicker) return;

        ImGui::SetNextWindowSize(ImVec2(240, 280), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(340, 10), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Color Picker", &m_showColorPicker, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        float color[3] = { m_selectedColor.r, m_selectedColor.g, m_selectedColor.b };

        if (ImGui::ColorPicker3(
            "##picker",
            color,
            ImGuiColorEditFlags_PickerHueWheel |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_InputRGB |
            ImGuiColorEditFlags_NoSidePreview
        ))
        {
            m_selectedColor = Color(color[0], color[1], color[2], 1.0f);
            if (m_controller)
            {
                m_controller->SetPrimaryColor(m_selectedColor);
            }
        }

        ImGui::End();
    }

} // namespace Spectrum