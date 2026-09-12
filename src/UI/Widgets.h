#ifndef SPECTRUM_CPP_WIDGETS_H
#define SPECTRUM_CPP_WIDGETS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Neon widgets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/ImGuiContext.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Spectrum::ui {

    inline const Palette& Pal() { return ImGuiContext::Theme(); }
    inline ImVec4 A(ImVec4 c, float a) { c.w = a; return c; }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Metrics
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    //
    // h      block height, 0 = auto
    // round  FrameRounding / GrabRounding
    // grab   GrabMinSize
    // padX Y WindowPadding / FramePadding
    // gapX Y ItemSpacing
    // aIdle  idle alpha
    // aHov   hover alpha
    // aAct   active / selected alpha
    // line   hairline inset
    // itemW  PushItemWidth / button width

    enum UiTok : std::size_t {
        UI_TITLE = 0,
        UI_CLOSE,
        UI_SEP,
        UI_STATUS,
        UI_SECTION,
        UI_COMBO,
        UI_SLIDER,
        UI_NEON,
        UI_BODY,
        UI_COUNT
    };

    struct Tok {
        float h, round, grab;
        float padX, padY;
        float gapX, gapY;
        float aIdle, aHov, aAct;
        float line, itemW;
    };

    inline constexpr Tok T[UI_COUNT] = {
        //            h   round grab  padX padY  gapX gapY  aIdle  aHov  aAct  line itemW
        /* Title   */ { 40,  0,   0,   14,  0,    8,   0,    0,     0,    0,    1,   0    },
        /* Close   */ {  0,  4,   0,    0,  0,    0,   0,    0,     0,    0,    0,   0    },
        /* Sep     */ {  0,  0,   0,    0,  0,    0,   0,    0.60f, 0,    0,    0,   0    },
        /* Status  */ { 30,  0,   0,   14,  6,    0,   0,    0,     0,    0,    0,   0    },
        /* Section */ {  0,  0,   0,    0,  0,    0,   0,    0,     0.12f,0.10f,0,   0    },
        /* Combo   */ {  0,  0,   0,    0,  0,    0,   0,    0,     0.35f,0.25f,0,   0    },
        /* Slider  */ {  0,  6,  14,   10,  4,    0,   0,    0,     0,    0,    0,   0    },
        /* Neon    */ {  0,  6,   0,    0,  0,    0,   0,    0.12f, 0.25f,0.40f,0,  -1    },
        /* Body    */ {  0,  0,   0,   14,  7,    0,   0,    0,     0,    0,    0,  -1    },
    };

    inline constexpr ImVec4 kClear = { 0, 0, 0, 0 };
    constexpr ImGuiWindowFlags kChromeFlags = ImGuiWindowFlags_NoScrollbar;

    constexpr ImVec2 Pad(std::size_t i) { return { T[i].padX, T[i].padY }; }
    constexpr ImVec2 Gap(std::size_t i) { return { T[i].gapX, T[i].gapY }; }

    constexpr float kTitleH = T[UI_TITLE].h;
    constexpr float kStatusH = T[UI_STATUS].h;

    constexpr float CenterY(float itemH, float parentH) {
        return (parentH - itemH) * 0.5f;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RAII
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct Fullscreen {
        float w{};
        float h{};

        explicit Fullscreen(const char* id) {
            const ImVec2 sz = ImGui::GetIO().DisplaySize;
            w = sz.x;
            h = sz.y;
            ImGui::SetNextWindowPos({ 0, 0 });
            ImGui::SetNextWindowSize(sz);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Pal().background);
            ImGui::Begin(id, nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        ~Fullscreen() { ImGui::End(); }
    };

    struct Body {
        explicit Body(float height) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Pad(UI_BODY));
            ImGui::BeginChild("##body", { 0, height }, ImGuiChildFlags_AlwaysUseWindowPadding);
            ImGui::PopStyleVar();
            ImGui::PushItemWidth(T[UI_BODY].itemW);
        }

        ~Body() {
            ImGui::PopItemWidth();
            ImGui::EndChild();
        }
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Chrome
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool CloseButton(ImVec2 pos, float size) {
        const auto& pal = Pal();
        ImGui::SetCursorPos(pos);
        ImGui::PushStyleColor(ImGuiCol_Button, kClear);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, pal.closeHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, pal.closeActive);
        ImGui::PushStyleColor(ImGuiCol_Text, pal.textSecondary);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, T[UI_CLOSE].round);
        const bool clicked = ImGui::Button("X", { size, size });
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        return clicked;
    }

    inline bool TitleBar(float width, const char* title) {
        const auto& pal = Pal();
        const Tok& t = T[UI_TITLE];
        const float btn = ImGui::GetFrameHeight();

        ImGui::PushID(title);
        ImGui::BeginChild("##title", { width, t.h }, 0, kChromeFlags);

        ImGui::SetCursorPos({ t.padX, CenterY(ImGui::GetTextLineHeight(), t.h) });
        ImGui::PushStyleColor(ImGuiCol_Text, pal.accent);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Gap(UI_TITLE));
        ImGui::Bullet();
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();

        const bool close = CloseButton({ width - t.padX - btn, CenterY(btn, t.h) }, btn);

        ImGui::SetCursorPosY(t.h - t.line);
        ImGui::PushStyleColor(ImGuiCol_Separator, pal.accent);
        ImGui::Separator();
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopID();
        return close;
    }

    inline void StatusBar(float y, float width, const char* text, const ImVec4& color) {
        const Tok& t = T[UI_STATUS];
        const auto& pal = Pal();

        ImGui::SetCursorScreenPos({ ImGui::GetWindowPos().x, y });
        ImGui::PushStyleColor(ImGuiCol_ChildBg, pal.background);
        ImGui::BeginChild("##status", { width, t.h }, 0, kChromeFlags);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Separator, A(pal.border, T[UI_SEP].aIdle));
        ImGui::Separator();
        ImGui::PopStyleColor();

        ImGui::SetCursorPos({ t.padX, CenterY(ImGui::GetTextLineHeight(), t.h) });
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextColored(pal.textSecondary, "%s", text);
        ImGui::EndChild();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Section
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class Section {
    public:
        explicit Section(const char* label) {
            const auto& accent = Pal().accent;
            const Tok& t = T[UI_SECTION];
            ImGui::PushStyleColor(ImGuiCol_Header, kClear);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, A(accent, t.aHov));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, A(accent, t.aAct));
            ImGui::PushStyleColor(ImGuiCol_Text, accent);
            m_open = ImGui::TreeNodeEx(label,
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_Bullet |
                ImGuiTreeNodeFlags_SpanAvailWidth);
            ImGui::PopStyleColor(4);

            if (m_open)
                ImGui::Spacing();
        }

        ~Section() {
            if (m_open)
                ImGui::TreePop();
        }

        Section(const Section&) = delete;
        Section& operator=(const Section&) = delete;

        explicit operator bool() const noexcept { return m_open; }

    private:
        bool m_open = false;
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Combo
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<typename Fn>
    inline void NamedCombo(
        const char* label, std::string_view current,
        const std::vector<std::string>& options, Fn&& onPick)
    {
        const auto& pal = Pal();
        const Tok& t = T[UI_COMBO];

        ImGui::TextColored(pal.textPrimary, "%s", label);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, pal.surface);
        ImGui::PushStyleColor(ImGuiCol_Header, A(pal.accent, t.aIdle));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, A(pal.accent, t.aHov));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, A(pal.accent, t.aAct));
        ImGui::PushID(label);
        if (ImGui::BeginCombo("##c", current.data())) {
            for (const auto& opt : options) {
                const bool sel = opt == current;
                if (ImGui::Selectable(opt.c_str(), sel))
                    onPick(opt);
                if (sel)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        ImGui::PopStyleColor(4);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Slider
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<class Ty>
    constexpr ImGuiDataType GuiType() {
        if constexpr (std::is_same_v<Ty, float>)
            return ImGuiDataType_Float;
        if constexpr (std::is_same_v<Ty, double>)
            return ImGuiDataType_Double;
        if constexpr (std::is_signed_v<Ty>) {
            if constexpr (sizeof(Ty) > 4)
                return ImGuiDataType_S64;
            return ImGuiDataType_S32;
        }
        if constexpr (sizeof(Ty) > 4)
            return ImGuiDataType_U64;
        return ImGuiDataType_U32;
    }

    template<class Ty>
    inline bool Slider(const char* label, Ty* v, Ty mn, Ty mx, const char* fmt = nullptr) {
        const auto& pal = Pal();
        const Tok& t = T[UI_SLIDER];

        ImGui::TextColored(pal.textPrimary, "%s", label);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, pal.surface);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, pal.surfaceHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, pal.surfaceActive);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, pal.accentDim);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, pal.accent);
        ImGui::PushStyleColor(ImGuiCol_Text, pal.accent);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, t.round);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, t.round);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, t.grab);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Pad(UI_SLIDER));
        ImGui::PushID(label);
        const bool changed = ImGui::SliderScalar("##sl", GuiType<Ty>(), v, &mn, &mx, fmt);
        ImGui::PopID();
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor(6);
        return changed;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Button / Color
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool NeonButton(const char* label, const ImVec4& col) {
        const Tok& t = T[UI_NEON];
        ImGui::PushStyleColor(ImGuiCol_Button, A(col, t.aIdle));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, A(col, t.aHov));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, A(col, t.aAct));
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, t.round);
        const bool clicked = ImGui::Button(label, { t.itemW, 0 });
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        return clicked;
    }

    inline bool ColorPicker(Color& color) {
        return ImGui::ColorPicker3(
            "##color",
            &color.r,
            ImGuiColorEditFlags_PickerHueWheel |
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview |
            ImGuiColorEditFlags_NoOptions);
    }

} // namespace Spectrum::ui

#endif