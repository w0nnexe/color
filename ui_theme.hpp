#pragma once
#include "imgui/imgui.h"
#include <windows.h>
#include <string>

namespace UITheme {
    inline float dpi_scale = 1.0f;
    inline ImFont* heading_font = nullptr;
    inline const ImVec4 kBackground = ImVec4(0.043f, 0.043f, 0.067f, 1.0f);
    inline const ImVec4 kAccent = ImVec4(0.43f, 0.33f, 0.94f, 1.0f);
    inline const ImVec4 kText = ImVec4(0.84f, 0.84f, 0.89f, 1.0f);
    inline const ImVec4 kMuted = ImVec4(0.43f, 0.43f, 0.51f, 1.0f);

    inline float Px(float value) { return value * dpi_scale; }

    inline void ApplyTheme(float scale = 1.0f) {
        dpi_scale = scale;
        ImGui::StyleColorsDark();
        auto& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(0, 0);
        style.WindowRounding = 6.0f;
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 5.0f;
        style.GrabRounding = 4.0f;
        style.GrabMinSize = 12.0f;
        style.FramePadding = ImVec2(12, 7);
        style.ItemSpacing = ImVec2(8, 8);
        style.PopupBorderSize = 1.0f;
        style.ScrollbarSize = 8.0f;
        style.ScrollbarRounding = 4.0f;

        auto& colors = style.Colors;
        colors[ImGuiCol_WindowBg] = kBackground;
        colors[ImGuiCol_PopupBg] = ImVec4(0.063f, 0.063f, 0.094f, 1.0f);
        colors[ImGuiCol_Text] = kText;
        colors[ImGuiCol_TextDisabled] = kMuted;
        colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.17f, 1.0f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg] = ImVec4(0.068f, 0.068f, 0.10f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.092f, 0.088f, 0.14f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.11f, 0.10f, 0.18f, 1.0f);
        colors[ImGuiCol_Button] = colors[ImGuiCol_FrameBg];
        colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgHovered];
        colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_FrameBgActive];
        colors[ImGuiCol_Header] = ImVec4(0.16f, 0.12f, 0.29f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.16f, 0.39f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.20f, 0.50f, 1.0f);
        colors[ImGuiCol_CheckMark] = kAccent;
        colors[ImGuiCol_SliderGrab] = kAccent;
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.57f, 0.47f, 1.0f, 1.0f);
        colors[ImGuiCol_Separator] = ImVec4(0.11f, 0.11f, 0.16f, 1.0f);
        colors[ImGuiCol_NavHighlight] = kAccent;
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.43f, 0.33f, 0.94f, 0.28f);
        style.ScaleAllSizes(scale);

        auto& io = ImGui::GetIO();
        char windowsDir[MAX_PATH] = {};
        GetWindowsDirectoryA(windowsDir, MAX_PATH);
        std::string fontDir = std::string(windowsDir) + "\\Fonts\\";
        std::string regular = fontDir + "segoeui.ttf";
        std::string semibold = fontDir + "seguisb.ttf";
        if (GetFileAttributesA(regular.c_str()) != INVALID_FILE_ATTRIBUTES)
            io.FontDefault = io.Fonts->AddFontFromFileTTF(regular.c_str(), Px(17.0f));
        if (!io.FontDefault) {
            ImFontConfig fontConfig;
            fontConfig.SizePixels = Px(17.0f);
            io.FontDefault = io.Fonts->AddFontDefault(&fontConfig);
        }
        heading_font = io.FontDefault;
        if (GetFileAttributesA(semibold.c_str()) != INVALID_FILE_ATTRIBUTES)
            heading_font = io.Fonts->AddFontFromFileTTF(semibold.c_str(), Px(17.0f));
    }
}
