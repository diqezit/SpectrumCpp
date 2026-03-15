#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UIManager — adaptive ImGui settings panel.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "imgui.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>

namespace Spectrum {

    class ImGuiContext;
    class ControllerCore;
    class AudioManager;
    class RendererManager;
    namespace Platform { class WindowManager; }

    class UIManager final {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Theme
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        struct Theme {
            // Accent
            static constexpr ImVec4 accent = { 0.35f, 0.40f, 0.95f, 1.0f };
            static constexpr ImVec4 accentDim = { 0.20f, 0.24f, 0.60f, 1.0f };

            // Surfaces
            static constexpr ImVec4 bg = { 0.055f, 0.055f, 0.09f,  1.0f };
            static constexpr ImVec4 surface = { 0.08f,  0.08f,  0.12f,  1.0f };
            static constexpr ImVec4 surfaceHov = { 0.10f,  0.10f,  0.155f, 1.0f };
            static constexpr ImVec4 popup = { 0.07f,  0.07f,  0.11f,  0.98f };
            static constexpr ImVec4 cardBorder = { 0.14f,  0.14f,  0.21f,  0.4f };
            static constexpr ImVec4 statusBg = { 0.04f,  0.04f,  0.07f,  1.0f };

            // Text
            static constexpr ImVec4 textPri = { 0.92f, 0.92f, 0.96f, 1.0f };
            static constexpr ImVec4 textSec = { 0.55f, 0.55f, 0.65f, 1.0f };
            static constexpr ImVec4 textDim = { 0.38f, 0.38f, 0.48f, 1.0f };
            static constexpr ImVec4 textAccent = { 0.60f, 0.65f, 0.95f, 1.0f };

            // Close
            static constexpr ImVec4 closeHov = { 0.85f, 0.18f, 0.22f, 0.9f };
            static constexpr ImVec4 closeAct = { 0.95f, 0.10f, 0.15f, 1.0f };

            // Status
            static constexpr ImVec4 statusOn = { 0.25f, 0.82f, 0.45f, 1.0f };
            static constexpr ImVec4 statusOff = { 0.35f, 0.35f, 0.45f, 1.0f };

            // Slider
            static constexpr ImVec4 trackBg = { 0.06f, 0.06f, 0.09f, 1.0f };
            static constexpr ImVec4 trackFill = { 0.28f, 0.33f, 0.80f, 1.0f };
            static constexpr ImVec4 grabColor = { 0.85f, 0.86f, 0.92f, 1.0f };
            static constexpr ImVec4 grabActive = { 1.0f,  1.0f,  1.0f,  1.0f };

            // Title bar
            static constexpr float titleH = 40.0f;
            static constexpr float titleIconX = 22.0f;
            static constexpr float titleGlowR = 12.0f;
            static constexpr float titleIconR = 6.0f;
            static constexpr float titleTextX = 38.0f;
            static constexpr float titleBtnSz = 24.0f;
            static constexpr float titleBtnMarg = 10.0f;
            static constexpr float titleXHalf = 4.5f;
            static constexpr float titleXStroke = 2.0f;
            static constexpr float titleShadowH = 4.0f;

            // Accent line
            static constexpr float accentLineH = 2.0f;
            static constexpr float accentGlowH = 6.0f;

            // Status bar
            static constexpr float statusH = 30.0f;
            static constexpr float statusDotX = 16.0f;
            static constexpr float statusDotR = 3.0f;
            static constexpr float statusGlowR1 = 7.0f;
            static constexpr float statusGlowR2 = 4.5f;
            static constexpr float statusTextOfs = 12.0f;

            // Section
            static constexpr float sectionPad = 10.0f;
            static constexpr float sectionDotOfs = 10.0f;
            static constexpr float sectionDotR = 3.5f;
            static constexpr float sectionGlowR = 5.5f;
            static constexpr float borderX = 6.0f;
            static constexpr float borderW = 2.0f;
            static constexpr float borderPad = 4.0f;

            // Slider
            static constexpr float sliderH = 6.0f;
            static constexpr float grabR = 7.0f;
            static constexpr float grabInnerR = 2.5f;
            static constexpr float grabShadow = 1.0f;

            // Shared
            static constexpr float rounding = 6.0f;
            static constexpr float framePad = 8.0f;
            static constexpr float contentPad = 14.0f;
            static constexpr float btnH = 34.0f;

            // Opacity
            static constexpr float alphaGlowSoft = 0.06f;
            static constexpr float alphaGlow = 0.08f;
            static constexpr float alphaGlowMed = 0.18f;
            static constexpr float alphaGlowHi = 0.20f;
            static constexpr float alphaDim = 0.15f;
            static constexpr float alphaSelect = 0.25f;
            static constexpr float alphaShadow = 0.25f;
            static constexpr float alphaHover = 0.30f;
            static constexpr float alphaShadowBg = 0.3f;

            // Computed
            static constexpr float headerH() {
                return titleH + accentLineH;
            }
            static constexpr float scrollH(float viewH) {
                return viewH - headerH() - statusH;
            }
        };

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // Lifecycle
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UIManager(ControllerCore*, Platform::WindowManager*);
        ~UIManager() noexcept;

        UIManager(const UIManager&) = delete;
        UIManager& operator=(const UIManager&) = delete;

        [[nodiscard]] bool Initialize();
        void Shutdown();

        void BeginFrame();
        void Render();
        void EndFrame();

        void OnResize(int, int);
        [[nodiscard]] bool HandleMessage(HWND, UINT, WPARAM, LPARAM);

    private:
        AudioManager* Audio()    const;
        RendererManager* Renderer() const;
        static const char* StyleName(RenderStyle);

        void DrawPanel();
        void DrawTitleBar(float w);
        void DrawAccentLine(float w);
        void DrawRendererSection();
        void DrawAudioSection();
        void DrawColorSection();
        void DrawDisplaySection();
        void DrawStatusBar(float w, float y);

        bool BeginSection(const char* label);
        void EndSection();
        bool AccentButton(const char* label);
        bool GlowButton(const char* label, const ImVec4& color);
        void DrawHoverGlow(const ImVec4& color);

        bool FancySliderFloat(
            const char* label, float* v,
            float mn, float mx, const char* fmt = "%.2f");
        bool FancySliderInt(
            const char* label, int* v, int mn, int mx);
        void DrawSliderTrack(ImVec2 mn, ImVec2 mx, float t);

        void PushComboStyle();
        void PopComboStyle();
        void DrawComboItems(
            const std::vector<std::string>& options,
            const std::string& current,
            const std::function<void(const std::string&)>& cb);
        void LabeledCombo(
            const char* label,
            std::string_view current,
            const std::vector<std::string>& options,
            const std::function<void(const std::string&)>& cb);

        static ImU32  U32(const ImVec4&);
        static ImU32  U32(float r, float g, float b, float a = 1.0f);
        static ImVec4 WithAlpha(const ImVec4&, float a);

        ControllerCore* m_ctrl;
        Platform::WindowManager* m_wm;
        std::unique_ptr<ImGuiContext> m_ctx;
        Color  m_color = Color::White();
        float  m_sectionY = 0.0f;
    };

} // namespace Spectrum

#endif