#ifndef SPECTRUM_CPP_WIDGETS_H
#define SPECTRUM_CPP_WIDGETS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Neon widgets
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "UI/ImGuiContext.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Spectrum::ui {

    inline const Palette& Pal() { return ImGuiContext::Theme(); }
    inline ImVec4 A(ImVec4 c, float a) { c.w = a; return c; }

    constexpr float kTitleH = 40.0f;
    constexpr float kStatusH = 30.0f;
    constexpr float kPad = 14.0f;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // RAII
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    struct Style {
        int nc = 0, nv = 0;

        Style() = default;
        ~Style() { pop(); }

        Style(const Style&) = delete;
        Style& operator=(const Style&) = delete;

        Style(Style&& o) noexcept : nc(o.nc), nv(o.nv) { o.nc = o.nv = 0; }
        Style& operator=(Style&& o) noexcept {
            if (this == &o) return *this;
            pop();
            nc = o.nc;
            nv = o.nv;
            o.nc = o.nv = 0;
            return *this;
        }

        Style& C(ImGuiCol i, const ImVec4& v) { ImGui::PushStyleColor(i, v); ++nc; return *this; }
        Style& V(ImGuiStyleVar i, float v) { ImGui::PushStyleVar(i, v);   ++nv; return *this; }
        Style& V(ImGuiStyleVar i, ImVec2 v) { ImGui::PushStyleVar(i, v);   ++nv; return *this; }

    private:
        void pop() {
            if (nc) ImGui::PopStyleColor(nc);
            if (nv) ImGui::PopStyleVar(nv);
            nc = nv = 0;
        }
    };

    struct Id {
        explicit Id(const char* s) { ImGui::PushID(s); }
        ~Id() { ImGui::PopID(); }
    };

    struct Child {
        Child(const char* id, ImVec2 size, ImGuiWindowFlags flags = 0) {
            ImGui::BeginChild(id, size, false, flags);
        }
        ~Child() { ImGui::EndChild(); }
    };

    struct Fullscreen {
        float w{};
        float h{};

        explicit Fullscreen(const char* id) {
            const ImVec2 sz = ImGui::GetIO().DisplaySize;
            w = sz.x;
            h = sz.y;
            ImGui::SetNextWindowPos({ 0, 0 });
            ImGui::SetNextWindowSize(sz);

            Style s;
            s.V(ImGuiStyleVar_WindowPadding, { 0, 0 })
                .C(ImGuiCol_WindowBg, Pal().background);

            ImGui::Begin(id, nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        }

        ~Fullscreen() { ImGui::End(); }
    };

    struct Body {
        explicit Body(float height) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { kPad, kPad * 0.5f });
            ImGui::BeginChild("##body", { 0, height }, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
            ImGui::PopStyleVar();
            ImGui::PushItemWidth(-1);
        }

        ~Body() {
            ImGui::PopItemWidth();
            ImGui::EndChild();
        }
    };

    inline Style HeaderTint(const ImVec4& c, float idle, float hov, float act = -1.0f) {
        Style s;
        s.C(ImGuiCol_Header, A(c, idle)).C(ImGuiCol_HeaderHovered, A(c, hov));
        if (act >= 0.0f) s.C(ImGuiCol_HeaderActive, A(c, act));
        return s;
    }

    inline void Label(const char* text) {
        ImGui::PushStyleColor(ImGuiCol_Text, Pal().textSecondary);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Chrome
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool TitleBar(float width, const char* title) {
        const auto& pal = Pal();
        Child bar("##title", { width, kTitleH }, ImGuiWindowFlags_NoScrollbar);

        const float textY = (kTitleH - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPos({ kPad, textY });

        ImGui::PushStyleColor(ImGuiCol_Text, pal.accent);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextColored(pal.accent, "%s", title);

        const float btn = ImGui::GetFrameHeight();
        ImGui::SameLine(width - kPad - btn);
        ImGui::SetCursorPosY((kTitleH - btn) * 0.5f);

        Style s;
        s.C(ImGuiCol_Button, { 0, 0, 0, 0 })
            .C(ImGuiCol_ButtonHovered, pal.closeHover)
            .C(ImGuiCol_ButtonActive, pal.closeActive)
            .C(ImGuiCol_Text, pal.textSecondary)
            .V(ImGuiStyleVar_FrameRounding, 4.0f);

        const bool close = ImGui::Button("X", { btn, btn });

        Style sep;
        sep.C(ImGuiCol_Separator, pal.accent)
            .V(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, 0.0f });
        ImGui::SetCursorPosY(kTitleH - 1.0f);
        ImGui::Separator();

        return close;
    }

    inline void StatusBar(float y, float width, const char* text, const ImVec4& color) {
        ImGui::SetCursorScreenPos({ ImGui::GetWindowPos().x, y });

        {
            Style sep;
            sep.C(ImGuiCol_Separator, A(Pal().border, 0.6f));
            ImGui::Separator();
        }

        Style s;
        s.V(ImGuiStyleVar_WindowPadding, { kPad, 6.0f })
            .C(ImGuiCol_ChildBg, { 0, 0, 0, 0 });

        ImGui::BeginChild("##status", { width, kStatusH }, false, ImGuiWindowFlags_NoScrollbar);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        Label(text);

        ImGui::EndChild();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Section
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    class Section {
    public:
        explicit Section(const char* label) {
            const auto& accent = Pal().accent;
            Style s;
            s.C(ImGuiCol_Header, { 0, 0, 0, 0 })
                .C(ImGuiCol_HeaderHovered, A(accent, 0.12f))
                .C(ImGuiCol_HeaderActive, A(accent, 0.10f))
                .C(ImGuiCol_Text, accent);

            m_open = ImGui::TreeNodeEx(label,
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_Bullet |
                ImGuiTreeNodeFlags_SpanAvailWidth);

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
        Label(label);

        char preview[64];
        const size_t n = current.size() < sizeof(preview) - 1 ? current.size() : sizeof(preview) - 1;
        memcpy(preview, current.data(), n);
        preview[n] = '\0';

        Id id(label);
        Style popup;
        popup.C(ImGuiCol_PopupBg, Pal().surface);

        if (!ImGui::BeginCombo("##c", preview))
            return;

        for (const auto& opt : options) {
            const bool sel = (current == opt);
            auto hs = HeaderTint(Pal().accent, sel ? 0.25f : 0.0f, 0.35f);
            if (ImGui::Selectable(opt.c_str(), sel))
                onPick(opt);
            if (sel)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Slider
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool Slider(const char* label, float* v, float mn, float mx, const char* fmt = "%.2f") {
        const auto& pal = Pal();

        ImGui::TextColored(pal.textPrimary, "%s", label);

        char val[32];
        snprintf(val, sizeof(val), fmt, static_cast<double>(*v));
        const float valW = ImGui::CalcTextSize(val).x;
        ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - valW);
        ImGui::TextColored(pal.accent, "%s", val);

        Style s;
        s.C(ImGuiCol_FrameBg, pal.surface)
            .C(ImGuiCol_FrameBgHovered, pal.surfaceHover)
            .C(ImGuiCol_FrameBgActive, pal.surfaceActive)
            .C(ImGuiCol_SliderGrab, pal.accentDim)
            .C(ImGuiCol_SliderGrabActive, pal.accent)
            .V(ImGuiStyleVar_FrameRounding, 6.0f)
            .V(ImGuiStyleVar_GrabRounding, 6.0f)
            .V(ImGuiStyleVar_GrabMinSize, 14.0f)
            .V(ImGuiStyleVar_FramePadding, ImVec2{ 10.0f, 4.0f });

        bool changed;
        {
            Id id(label);
            changed = ImGui::SliderFloat("##sl", v, mn, mx, "");
        }

        ImGui::Spacing();
        return changed;
    }

    inline bool Slider(const char* label, int* v, int mn, int mx) {
        float f = float(*v);
        const bool changed = Slider(label, &f, float(mn), float(mx), "%.0f");
        if (changed)
            *v = int(f);
        return changed;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Button / Color
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    inline bool NeonButton(const char* label, const ImVec4& col) {
        Style s;
        s.C(ImGuiCol_Button, A(col, 0.12f))
            .C(ImGuiCol_ButtonHovered, A(col, 0.25f))
            .C(ImGuiCol_ButtonActive, A(col, 0.40f))
            .C(ImGuiCol_Text, col)
            .V(ImGuiStyleVar_FrameRounding, 6.0f);
        return ImGui::Button(label, { -1, 0 });
    }

    inline bool ColorPicker(Color& color) {
        float rgb[3] = { color.r, color.g, color.b };

        ImGui::ColorButton("##preview", { rgb[0], rgb[1], rgb[2], 1.0f },
            ImGuiColorEditFlags_NoTooltip, { -1, 24 });
        ImGui::Spacing();

        if (!ImGui::ColorPicker3("##color", rgb,
            ImGuiColorEditFlags_PickerHueWheel |
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoAlpha |
            ImGuiColorEditFlags_NoInputs))
            return false;

        color = Color(rgb[0], rgb[1], rgb[2], 1.0f);
        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Bind  (GetX / SetX / GetXMin / GetXMax)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    template<class T, class Mn, class Mx>
    bool EditScalar(const char* label, T& v, Mn mn, Mx mx) {
        if constexpr (std::is_floating_point_v<T>) {
            return Slider(label, &v, float(mn), float(mx));
        }
        else {
            int i = int(v);
            if (!Slider(label, &i, int(mn), int(mx)))
                return false;
            v = T(i);
            return true;
        }
    }

    template<class Obj, class Get, class Set, class Mn, class Mx>
    void BindSlider(const char* label, Obj* o, Get get, Set set, Mn mn, Mx mx) {
        auto v = (o->*get)();
        if (EditScalar(label, v, (o->*mn)(), (o->*mx)()))
            (o->*set)(std::move(v));
    }

    template<class Obj, class GetName, class GetList, class SetName>
    void BindCombo(const char* label, Obj* o, GetName getName, GetList getList, SetName setName) {
        NamedCombo(label, (o->*getName)(), (o->*getList)(),
            [o, setName](const std::string& n) { (o->*setName)(n); });
    }

#define UI_SLIDER(obj, title, Prop) \
    ::Spectrum::ui::BindSlider(title, obj, \
        &std::remove_pointer_t<decltype(obj)>::Get##Prop, \
        &std::remove_pointer_t<decltype(obj)>::Set##Prop, \
        &std::remove_pointer_t<decltype(obj)>::Get##Prop##Min, \
        &std::remove_pointer_t<decltype(obj)>::Get##Prop##Max)

} // namespace Spectrum::ui

#endif