#ifndef SPECTRUM_CPP_IMGUI_HELPERS_H
#define SPECTRUM_CPP_IMGUI_HELPERS_H

#include "Common/Common.h"
#include "imgui.h"

namespace Spectrum::ImGuiHelpers {

    inline ImVec4 ToImVec4(const Color& color)
    {
        return ImVec4(color.r, color.g, color.b, color.a);
    }

    inline Color FromImVec4(const ImVec4& color)
    {
        return Color(color.x, color.y, color.z, color.w);
    }

    inline void CenterNextWindow()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Appearing,
            ImVec2(0.5f, 0.5f)
        );
    }

    inline void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    inline void SetNextWindowTopRight(float offsetX = 10.0f, float offsetY = 10.0f)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x - offsetX, offsetY),
            ImGuiCond_FirstUseEver,
            ImVec2(1.0f, 0.0f)
        );
    }

    inline void SetNextWindowTopLeft(float offsetX = 10.0f, float offsetY = 10.0f)
    {
        ImGui::SetNextWindowPos(
            ImVec2(offsetX, offsetY),
            ImGuiCond_FirstUseEver,
            ImVec2(0.0f, 0.0f)
        );
    }

}

#endif